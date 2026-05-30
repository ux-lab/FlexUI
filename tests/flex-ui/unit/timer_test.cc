/*
 * FlexUI timer family unit tests (Task 13).
 *
 * Tests:
 *  - OneShotTimer fires once and stops.
 *  - OneShotTimer with zero delay fires immediately.
 *  - OneShotTimer Stop() prevents firing.
 *  - OneShotTimer FireNow() triggers while running.
 *  - RepeatingTimer fires multiple times.
 *  - RepeatingTimer Stop() halts repetition.
 *  - BaseTimer::IsRunning() reflects state correctly.
 *
 * The API is:
 *   OneShotTimer::Start(std::unique_ptr<Task>, TimeDelta)
 *   RepeatingTimer::Start(std::unique_ptr<Task>, TimeDelta)
 *
 * A WorkerManager(1) provides the live TaskRunner needed for scheduling.
 */

#include "flexui/common/one_shot_timer.h"
#include "flexui/common/repeating_timer.h"
#include "flexui/common/task.h"
#include "flexui/common/time_delta.h"
#include "flexui/common/worker_manager.h"

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

bool WaitUntil(std::function<bool()> pred, int timeout_ms = 2000) {
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (!pred()) {
    if (std::chrono::steady_clock::now() >= deadline) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

}  // namespace

// ── OneShotTimer ─────────────────────────────────────────────────────────────

TEST(OneShotTimerTest, FiresOnceAfterDelay) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("timer-runner");

  std::atomic<int> count{0};
  auto timer = std::make_shared<OneShotTimer>(runner);
  timer->Start(
      std::make_unique<Task>([&count]() { count.fetch_add(1); }),
      TimeDelta::FromMilliseconds(20));

  EXPECT_TRUE(WaitUntil([&]() { return count.load() >= 1; }));
  // Wait a bit longer to verify it doesn't fire again.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(count.load(), 1) << "OneShotTimer fired more than once";

  wm.Terminate();
}

TEST(OneShotTimerTest, ZeroDelayFiresImmediately) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("timer-runner");

  std::atomic<bool> ran{false};
  auto timer = std::make_shared<OneShotTimer>(runner);
  timer->Start(
      std::make_unique<Task>([&ran]() { ran.store(true); }),
      TimeDelta::Zero());

  EXPECT_TRUE(WaitFor(ran)) << "OneShotTimer(zero delay) did not fire";
  wm.Terminate();
}

TEST(OneShotTimerTest, StopPreventsCallback) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("timer-runner");

  std::atomic<bool> ran{false};
  auto timer = std::make_shared<OneShotTimer>(runner);
  timer->Start(
      std::make_unique<Task>([&ran]() { ran.store(true); }),
      TimeDelta::FromMilliseconds(200));

  // Stop before the timer fires.
  timer->Stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_FALSE(ran.load()) << "Callback ran after Stop()";

  wm.Terminate();
}

TEST(OneShotTimerTest, IsRunningReflectsState) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("timer-runner");

  auto timer = std::make_shared<OneShotTimer>(runner);
  EXPECT_FALSE(timer->IsRunning());

  std::atomic<bool> ran{false};
  timer->Start(
      std::make_unique<Task>([&ran]() { ran.store(true); }),
      TimeDelta::FromMilliseconds(500));

  EXPECT_TRUE(timer->IsRunning());
  timer->Stop();
  EXPECT_FALSE(timer->IsRunning());

  wm.Terminate();
}

TEST(OneShotTimerTest, FireNowTriggersImmediately) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("timer-runner");

  std::atomic<bool> ran{false};
  auto timer = std::make_shared<OneShotTimer>(runner);
  timer->Start(
      std::make_unique<Task>([&ran]() { ran.store(true); }),
      TimeDelta::FromSeconds(10));  // Long delay — won't expire naturally.

  EXPECT_TRUE(timer->IsRunning());
  // FireNow must be called on the timer's runner thread.
  runner->PostTask(std::make_unique<Task>([&timer]() { timer->FireNow(); }));

  EXPECT_TRUE(WaitFor(ran)) << "FireNow() did not invoke the callback";
  wm.Terminate();
}

// ── RepeatingTimer ────────────────────────────────────────────────────────────

TEST(RepeatingTimerTest, FiresMultipleTimes) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("rep-runner");

  std::atomic<int> count{0};
  auto timer = std::make_shared<RepeatingTimer>(runner);
  timer->Start(
      std::make_unique<Task>([&count]() { count.fetch_add(1); }),
      TimeDelta::FromMilliseconds(30));

  EXPECT_TRUE(WaitUntil([&]() { return count.load() >= 3; }, 3000))
      << "RepeatingTimer fired fewer than 3 times";

  timer->Stop();
  wm.Terminate();
}

TEST(RepeatingTimerTest, StopHaltsRepetition) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("rep-runner");

  std::atomic<int> count{0};
  auto timer = std::make_shared<RepeatingTimer>(runner);
  timer->Start(
      std::make_unique<Task>([&count]() { count.fetch_add(1); }),
      TimeDelta::FromMilliseconds(20));

  // Let it fire at least once.
  EXPECT_TRUE(WaitUntil([&]() { return count.load() >= 1; }));
  timer->Stop();
  EXPECT_FALSE(timer->IsRunning());

  int snapshot = count.load();
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  EXPECT_EQ(count.load(), snapshot) << "Timer kept firing after Stop()";

  wm.Terminate();
}

TEST(RepeatingTimerTest, IsRunningAfterStart) {
  WorkerManager wm(1);
  auto runner = wm.CreateTaskRunner("rep-runner");

  auto timer = std::make_shared<RepeatingTimer>(runner);
  EXPECT_FALSE(timer->IsRunning());

  timer->Start(
      std::make_unique<Task>([]() {}),
      TimeDelta::FromMilliseconds(100));

  EXPECT_TRUE(timer->IsRunning());
  timer->Stop();
  EXPECT_FALSE(timer->IsRunning());

  wm.Terminate();
}
