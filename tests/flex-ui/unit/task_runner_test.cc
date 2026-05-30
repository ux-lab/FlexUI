/*
 * FlexUI task + task_runner unit tests.
 *
 * Scope (Task 11): Verify that Task and TaskRunner construct, destruct, and
 * enqueue correctly without a running Worker.  The "task actually executes"
 * assertion lives in Task 12's worker_test.cc once Worker is absorbed.
 *
 * Tests:
 *  - Task default-constructs and gets a unique non-zero id.
 *  - Task with an exec-unit runs the unit on Run().
 *  - Task::SetExecUnit replaces the unit.
 *  - TaskRunner default-constructs with a name.
 *  - TaskRunner two-arg constructor sets correct priority / group.
 *  - PostTask enqueues: GetQueueSize increments correctly.
 *  - PostTask with lambda overload enqueues without crashing.
 *  - Clear empties the queue.
 *  - Multiple runners have distinct IDs.
 *  - GetName returns the name given at construction.
 */

#include "flexui/common/task.h"
#include "flexui/common/task_runner.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace flexui::common;

// ── Task ─────────────────────────────────────────────────────────────────────

TEST(TaskTest, DefaultConstructorGetsNonZeroId) {
  Task t;
  EXPECT_GT(t.GetId(), 0u);
}

TEST(TaskTest, TwoTasksHaveDistinctIds) {
  Task a, b;
  EXPECT_NE(a.GetId(), b.GetId());
}

TEST(TaskTest, RunInvokesUnit) {
  bool ran = false;
  Task t([&ran]() { ran = true; });
  EXPECT_FALSE(ran);
  t.Run();
  EXPECT_TRUE(ran);
}

TEST(TaskTest, RunOnEmptyUnitDoesNotCrash) {
  Task t;
  EXPECT_NO_THROW(t.Run());
}

TEST(TaskTest, SetExecUnitReplacesUnit) {
  int counter = 0;
  Task t([&counter]() { counter += 1; });
  t.SetExecUnit([&counter]() { counter += 10; });
  t.Run();
  EXPECT_EQ(counter, 10);
}

// ── TaskRunner ────────────────────────────────────────────────────────────────

TEST(TaskRunnerTest, DefaultNameConstructor) {
  TaskRunner runner("test_runner");
  EXPECT_EQ(runner.GetName(), "test_runner");
}

TEST(TaskRunnerTest, EmptyNameConstructor) {
  TaskRunner runner;
  EXPECT_EQ(runner.GetName(), "");
}

TEST(TaskRunnerTest, FullConstructorSetsFields) {
  TaskRunner runner(/*group_id=*/42, /*priority=*/5, /*is_schedulable=*/false, "named");
  EXPECT_EQ(runner.GetGroupId(), 42u);
  EXPECT_EQ(runner.GetPriority(), 5u);
  EXPECT_FALSE(runner.IsSchedulable());
  EXPECT_EQ(runner.GetName(), "named");
}

TEST(TaskRunnerTest, TwoRunnersHaveDistinctIds) {
  TaskRunner a("a"), b("b");
  EXPECT_NE(a.GetId(), b.GetId());
}

TEST(TaskRunnerTest, PostTaskIncreasesQueueSize) {
  TaskRunner runner("q");
  EXPECT_EQ(runner.GetQueueSize(), 0u);
  runner.PostTask(std::make_unique<Task>([]() {}));
  EXPECT_EQ(runner.GetQueueSize(), 1u);
  runner.PostTask(std::make_unique<Task>([]() {}));
  EXPECT_EQ(runner.GetQueueSize(), 2u);
}

TEST(TaskRunnerTest, PostTaskLambdaOverloadEnqueues) {
  TaskRunner runner("lambda");
  runner.PostTask([]() {});
  EXPECT_EQ(runner.GetQueueSize(), 1u);
}

TEST(TaskRunnerTest, ClearEmptiesQueue) {
  TaskRunner runner("clear");
  runner.PostTask([]() {});
  runner.PostTask([]() {});
  EXPECT_EQ(runner.GetQueueSize(), 2u);
  runner.Clear();
  EXPECT_EQ(runner.GetQueueSize(), 0u);
}

TEST(TaskRunnerTest, HasTaskReturnsFalseWhenEmpty) {
  // HasTask is private; exercise via GetQueueSize proxy for now.
  TaskRunner runner("empty");
  EXPECT_EQ(runner.GetQueueSize(), 0u);
}

TEST(TaskRunnerTest, DefaultTimeDeltaIsZero) {
  using TimeDelta = flexui::common::time::TimeDelta;
  TaskRunner runner("time");
  EXPECT_EQ(runner.GetTime().ToNanoseconds(), 0);
}
