/*
 * FlexUI WorkerManager unit tests (Task 12).
 *
 * Tests:
 *  - WorkerManager(1) constructs and terminates cleanly.
 *  - CreateTaskRunner returns a non-null runner.
 *  - A task posted to the runner actually executes.
 *  - WorkerManager(2) with two workers: tasks on two runners both execute.
 *  - RemoveTaskRunner does not crash.
 *  - Resize(2) grows from 1 to 2 workers without crashing.
 */

#include "flexui/common/worker_manager.h"
#include "flexui/common/task_runner.h"
#include "flexui/common/task.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace flexui::common;

namespace {

bool WaitFor(const std::atomic<bool>& flag, int timeout_ms = 2000) {
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (!flag.load()) {
    if (std::chrono::steady_clock::now() >= deadline) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

}  // namespace

// ── Lifecycle ────────────────────────────────────────────────────────────────

TEST(WorkerManagerTest, ConstructAndTerminateOneWorker) {
  WorkerManager wm(1);
  EXPECT_NO_THROW(wm.Terminate());
}

TEST(WorkerManagerTest, ConstructAndTerminateTwoWorkers) {
  WorkerManager wm(2);
  EXPECT_NO_THROW(wm.Terminate());
}

// ── CreateTaskRunner ─────────────────────────────────────────────────────────

TEST(WorkerManagerTest, CreateTaskRunnerReturnsNonNull) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("test-runner");
  EXPECT_NE(runner, nullptr);
  wm.Terminate();
}

TEST(WorkerManagerTest, TaskRunnerHasExpectedName) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("my-runner");
  EXPECT_EQ(runner->GetName(), "my-runner");
  wm.Terminate();
}

// ── Task execution ───────────────────────────────────────────────────────────

TEST(WorkerManagerTest, TaskExecutesOnSingleWorker) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("runner");

  std::atomic<bool> ran{false};
  runner->PostTask(std::make_unique<Task>([&ran]() { ran.store(true); }));

  EXPECT_TRUE(WaitFor(ran)) << "Task did not execute within timeout";
  wm.Terminate();
}

TEST(WorkerManagerTest, TasksExecuteOnTwoWorkers) {
  WorkerManager wm(2);
  auto r1 = wm.CreateTaskRunner("r1");
  auto r2 = wm.CreateTaskRunner("r2");

  std::atomic<bool> ran1{false}, ran2{false};
  r1->PostTask(std::make_unique<Task>([&ran1]() { ran1.store(true); }));
  r2->PostTask(std::make_unique<Task>([&ran2]() { ran2.store(true); }));

  EXPECT_TRUE(WaitFor(ran1)) << "Task on r1 did not execute";
  EXPECT_TRUE(WaitFor(ran2)) << "Task on r2 did not execute";
  wm.Terminate();
}

// ── RemoveTaskRunner ─────────────────────────────────────────────────────────

TEST(WorkerManagerTest, RemoveTaskRunnerDoesNotCrash) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("removable");
  EXPECT_NO_THROW(wm.RemoveTaskRunner(runner));
  wm.Terminate();
}

// ── Resize ───────────────────────────────────────────────────────────────────

TEST(WorkerManagerTest, ResizeGrowsWorkers) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("runner");

  // Post a task, resize, then post another - both should work
  std::atomic<int> count{0};
  runner->PostTask(std::make_unique<Task>([&count]() { count.fetch_add(1); }));

  EXPECT_NO_THROW(wm.Resize(2));

  runner->PostTask(std::make_unique<Task>([&count]() { count.fetch_add(1); }));

  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
  while (count.load() < 2 && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  EXPECT_EQ(count.load(), 2);
  wm.Terminate();
}
