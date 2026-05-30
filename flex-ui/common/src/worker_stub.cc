/*
 * FlexUI - Worker static-method stub.
 *
 * Provides definitions for Worker::GetCurrentTaskRunner() and
 * Worker::IsTaskRunning() so that task_runner.cc can link before
 * Task 12 absorbs the full Worker implementation.
 *
 * Task 12 MUST replace or subsume this file with the real implementation.
 */

#include "flexui/common/worker.h"
#include "flexui/common/task_runner.h"

#include <memory>

namespace flexui::common {
inline namespace runner {

// Thread-local storage for the "current" runner while a task executes.
// Task 12's full Worker implementation will manage this properly.
thread_local std::shared_ptr<TaskRunner> g_current_task_runner{nullptr};
thread_local bool g_is_task_running{false};

std::shared_ptr<TaskRunner> Worker::GetCurrentTaskRunner() {
  return g_current_task_runner;
}

bool Worker::IsTaskRunning() {
  return g_is_task_running;
}

}  // namespace runner
}  // namespace flexui::common
