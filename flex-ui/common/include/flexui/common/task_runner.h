/*
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2022 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Modified by the FlexUI authors. This file is derived from
 * modules/footstone/include/footstone/task_runner.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone` -> `flexui::common`.
 *   - Renamed include path `footstone/...` -> `flexui/common/...`.
 *   - Renamed macros `FOOTSTONE_*` -> `FLEXUI_*` (where present).
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <memory>
#include <cstdint>

#include "flexui/common/idle_task.h"
#include "flexui/common/task.h"
#include "flexui/common/time_delta.h"
#include "flexui/common/time_point.h"

// Worker is absorbed in Task 12; forward-declare here to avoid circular deps.
namespace flexui::common {
inline namespace runner {
class Worker;
}  // namespace runner
}  // namespace flexui::common

namespace flexui::common {
inline namespace runner {

constexpr uint32_t kDefaultGroupId = 0;  // 0 means no grouping
// The priority multiplied by the runtime determines the running priority of the task runner.
constexpr uint32_t kDefaultPriority = 10;

class TaskRunner {
 public:
  using TimePoint = time::TimePoint;
  using TimeDelta = time::TimeDelta;
  // group_id 0 means no grouping; callers can specify a non-zero group_id to
  // force different TaskRunners onto the same Worker.
  TaskRunner(uint32_t group_id, uint32_t priority, bool is_schedulable, std::string name);
  TaskRunner(std::string name = "");
  ~TaskRunner();

  TaskRunner(TaskRunner&) = delete;
  TaskRunner& operator=(TaskRunner&) = delete;

  void Clear();
  bool AddSubTaskRunner(const std::shared_ptr<TaskRunner> &sub_runner,
                        bool is_task_running = false);
  bool RemoveSubTaskRunner(const std::shared_ptr<TaskRunner> &sub_runner);
  void PostTask(std::unique_ptr<Task> task);
  template<typename F, typename... Args>
  void PostTask(F &&f, Args... args) {
    auto packaged_task = std::make_shared<std::packaged_task<std::invoke_result_t<F, Args...>()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto task = std::make_unique<Task>([packaged_task]() { (*packaged_task)(); });
    PostTask(std::move(task));
  }

  void PostDelayedTask(std::unique_ptr<Task> task, TimeDelta delay);

  template<typename F, typename... Args>
  void PostDelayedTask(F &&f, TimeDelta delay, Args... args) {
    auto packaged_task = std::make_shared<std::packaged_task<std::invoke_result_t<F, Args...>()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto task = std::make_unique<Task>([packaged_task]() { (*packaged_task)(); });
    PostDelayedTask(std::move(task), delay);
  }
  TimeDelta GetNextTimeDelta(TimePoint now);

  int32_t RunnerKeyCreate(const std::function<void(void *)> &destruct);
  bool RunnerKeyDelete(int32_t key);
  bool RunnerSetSpecific(int32_t key, void *p);
  void *RunnerGetSpecific(int32_t key);
  void RunnerDestroySpecifics();

  inline void SetWorker(std::weak_ptr<Worker> worker) {
    worker_ = worker;
  }
  inline uint32_t GetPriority() { return priority_; }
  inline uint32_t GetId() { return id_; }
  inline std::string GetName() {
    return name_;
  }
  inline uint32_t GetGroupId() {
    return group_id_;
  }
  inline TimeDelta GetTime() { return time_; }
  inline TimeDelta AddTime(TimeDelta time) {
    time_ = time_ + time;
    return time_;
  }
  inline void SetTime(TimeDelta time) { time_ = time; }
  inline bool IsSchedulable() { return is_schedulable_; }

  // Must be called while a task is running; aborts if called outside a task.
  static std::shared_ptr<TaskRunner> GetCurrentTaskRunner();

  void PostIdleTask(std::unique_ptr<IdleTask> task);

  // Returns the number of tasks currently in the immediate (non-delayed) queue.
  // Intended for unit tests that verify PostTask enqueues without executing.
  size_t GetQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
  }

 private:
  friend class Worker;
  friend class Scheduler;
  friend class WorkerManager;
  friend class IdleTimer;

  inline const std::weak_ptr<Worker>& GetWorker() {
    return worker_;
  }
  void NotifyWorker();
  std::unique_ptr<Task> popTaskFromDelayedQueueNoLock(TimePoint now);
  std::unique_ptr<Task> PopTask();
  // Called by friend IdleTimer
  std::unique_ptr<IdleTask> PopIdleTask();
  std::unique_ptr<Task> GetTopDelayTask();
  std::unique_ptr<Task> GetNext();
  bool HasTask();
  bool HasMoreUrgentTask(TimeDelta min_wait_time, TimePoint now);

  mutable std::mutex queue_mutex_;
  std::queue<std::unique_ptr<Task>> task_queue_;
  std::queue<std::unique_ptr<IdleTask>> idle_task_queue_;
  std::mutex idle_mutex_;
  using DelayedEntry = std::pair<TimePoint, std::unique_ptr<Task>>;
  struct DelayedEntryCompare {
    bool operator()(const DelayedEntry &left, const DelayedEntry &right) const {
      return left.first > right.first;
    }
  };
  std::priority_queue<DelayedEntry, std::vector<DelayedEntry>, DelayedEntryCompare>
      delayed_task_queue_;
  std::mutex delay_mutex_;
  std::weak_ptr<Worker> worker_;
  std::string name_;
  bool has_sub_runner_;
  uint32_t priority_;
  uint32_t id_;
  uint32_t group_id_;
  TimeDelta time_;
  bool is_schedulable_;
};

}  // namespace runner
}  // namespace flexui::common
