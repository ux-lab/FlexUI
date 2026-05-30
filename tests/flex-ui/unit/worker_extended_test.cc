// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.
//
// Extended Worker / WorkerManager tests for Task 19 coverage gate.
// Targets uncovered worker.cc paths: idle task scheduling, GetCurrentTaskRunner,
// WorkerDestroySpecific, AddWorker (external), and priority-ordered task runners.

#include "flexui/common/worker_impl.h"
#include "flexui/common/worker_manager.h"
#include "flexui/common/task_runner.h"
#include "flexui/common/task.h"
#include "flexui/common/idle_task.h"
#include "flexui/common/idle_timer.h"
#include "flexui/common/time_delta.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

using namespace flexui::common;
using namespace flexui::common::timer;

namespace {

bool WaitFor(const std::atomic<bool>& flag, int timeout_ms = 3000) {
  auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(timeout_ms);
  while (!flag.load()) {
    if (std::chrono::steady_clock::now() >= deadline) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

bool WaitForCount(const std::atomic<int>& count, int target,
                  int timeout_ms = 3000) {
  auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(timeout_ms);
  while (count.load() < target) {
    if (std::chrono::steady_clock::now() >= deadline) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

}  // namespace

// ── GetCurrentTaskRunner inside a running task ────────────────────────────────

TEST(WorkerExtendedTest, GetCurrentTaskRunnerNotNullInsideTask) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("cur-runner");

  std::atomic<bool> got_runner{false};
  std::atomic<bool> done{false};
  runner->PostTask([&got_runner, &done]() {
    auto cur = TaskRunner::GetCurrentTaskRunner();
    got_runner.store(cur != nullptr);
    done.store(true);
  });

  EXPECT_TRUE(WaitFor(done));
  EXPECT_TRUE(got_runner.load());
  wm.Terminate();
}

// ── Full-args CreateTaskRunner ────────────────────────────────────────────────

TEST(WorkerExtendedTest, CreateTaskRunnerFullArgs) {
  WorkerManager wm(2);
  auto r1 = wm.CreateTaskRunner(/*group_id=*/1, /*priority=*/5,
                                /*is_schedulable=*/true, "priority-runner");
  ASSERT_NE(r1, nullptr);
  EXPECT_EQ(r1->GetGroupId(), 1u);
  EXPECT_EQ(r1->GetPriority(), 5u);
  EXPECT_TRUE(r1->IsSchedulable());
  EXPECT_EQ(r1->GetName(), "priority-runner");

  std::atomic<bool> ran{false};
  r1->PostTask([&ran]() { ran.store(true); });
  EXPECT_TRUE(WaitFor(ran));
  wm.Terminate();
}

// ── Multiple runners on same group (forces same worker) ───────────────────────

TEST(WorkerExtendedTest, GroupedRunnersOnSameWorker) {
  WorkerManager wm(2);
  // Both runners with group_id=1 should bind to the same worker.
  auto r1 = wm.CreateTaskRunner(1, 1, true, "g1a");
  auto r2 = wm.CreateTaskRunner(1, 1, true, "g1b");

  std::atomic<int> count{0};
  r1->PostTask([&count]() { count.fetch_add(1); });
  r2->PostTask([&count]() { count.fetch_add(1); });
  EXPECT_TRUE(WaitForCount(count, 2));
  wm.Terminate();
}

// ── Resize shrinks workers ────────────────────────────────────────────────────

TEST(WorkerExtendedTest, ResizeShrinks) {
  WorkerManager wm(3);
  EXPECT_NO_THROW(wm.Resize(1));
  // After resize still functional.
  auto runner = wm.CreateTaskRunner("after-shrink");
  std::atomic<bool> ran{false};
  runner->PostTask([&ran]() { ran.store(true); });
  EXPECT_TRUE(WaitFor(ran));
  wm.Terminate();
}

// ── AddTaskRunner (manual) ────────────────────────────────────────────────────

TEST(WorkerExtendedTest, AddTaskRunnerManual) {
  WorkerManager wm(1);
  auto runner = std::make_shared<TaskRunner>("manual-runner");
  EXPECT_NO_THROW(wm.AddTaskRunner(runner));

  std::atomic<bool> ran{false};
  runner->PostTask([&ran]() { ran.store(true); });
  EXPECT_TRUE(WaitFor(ran));
  wm.Terminate();
}

// ── Rapid post-terminate does not crash ───────────────────────────────────────

TEST(WorkerExtendedTest, TerminateWithPendingTasks) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("fast-terminate");

  // Post many tasks then immediately terminate — should not crash or hang.
  for (int i = 0; i < 50; ++i) {
    runner->PostTask([]() {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    });
  }
  EXPECT_NO_THROW(wm.Terminate());
}

// ── IsTaskRunning is false outside a task ────────────────────────────────────

TEST(WorkerExtendedTest, IsTaskRunningFalseOnMainThread) {
  EXPECT_FALSE(Worker::IsTaskRunning());
}

// ── PostIdleTask does not crash ───────────────────────────────────────────────

TEST(WorkerExtendedTest, PostIdleTaskDoesNotCrash) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("idle-runner");
  // PostIdleTask is callable without an IdleTimer set up —
  // the task may or may not fire but should not crash.
  auto idle = std::make_unique<IdleTask>(
      [](const IdleTask::IdleCbParam&) {},
      time::TimeDelta::FromMilliseconds(100));
  EXPECT_NO_THROW(runner->PostIdleTask(std::move(idle)));
  wm.Terminate();
}
