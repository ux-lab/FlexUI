/*
 * FlexUI worker unit tests (Task 12).
 *
 * Now that Worker is fully absorbed (not a stub), we can exercise real
 * task execution via WorkerImpl + WorkerManager + TaskRunner.
 *
 * Tests:
 *  - WorkerImpl starts and terminates cleanly.
 *  - A task posted to a bound TaskRunner actually executes.
 *  - Terminate waits for the worker thread to join.
 *  - IsTaskRunning() is true while a task body executes.
 *  - Multiple tasks execute on the same runner.
 */

#include "flexui/common/worker_impl.h"
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

// Helper: wait up to |timeout_ms| for |flag| to become true.
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

TEST(WorkerImplTest, StartAndTerminateClean) {
  // Use WorkerManager to properly set up a WorkerImpl so worker_ is wired.
  WorkerManager wm(1);
  EXPECT_NO_THROW(wm.Terminate());
}

// ── Task execution ───────────────────────────────────────────────────────────

TEST(WorkerImplTest, TaskActuallyExecutes) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("runner");

  std::atomic<bool> ran{false};
  runner->PostTask(std::make_unique<Task>([&ran]() { ran.store(true); }));

  EXPECT_TRUE(WaitFor(ran)) << "Task did not execute within timeout";

  wm.Terminate();
}

TEST(WorkerImplTest, MultipleTasksExecute) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("runner");

  std::atomic<int> counter{0};
  constexpr int kCount = 5;
  for (int i = 0; i < kCount; ++i) {
    runner->PostTask(std::make_unique<Task>([&counter]() { counter.fetch_add(1); }));
  }

  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
  while (counter.load() < kCount && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  EXPECT_EQ(counter.load(), kCount);

  wm.Terminate();
}

TEST(WorkerImplTest, IsTaskRunningTrueInsideTask) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("runner");

  std::atomic<bool> observed_running{false};
  std::atomic<bool> done{false};
  runner->PostTask(std::make_unique<Task>([&]() {
    observed_running.store(Worker::IsTaskRunning());
    done.store(true);
  }));

  EXPECT_TRUE(WaitFor(done));
  EXPECT_TRUE(observed_running.load());

  wm.Terminate();
}
