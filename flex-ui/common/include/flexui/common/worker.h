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
 * modules/footstone/include/footstone/worker.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone` -> `flexui::common`.
 *   - Renamed include path `footstone/...` -> `flexui/common/...`.
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "flexui/common/driver.h"
#include "flexui/common/task.h"
#include "flexui/common/time_delta.h"
#include "flexui/common/time_point.h"

namespace flexui::common {
inline namespace runner {

class WorkerManager;
class TaskRunner;

class Worker {
 public:
  static const int32_t kWorkerKeysMax = 32;
  struct WorkerKey {
    bool is_used = false;
    std::function<void(void *)> destruct = nullptr;
  };

  Worker(std::string name, bool is_schedulable, std::unique_ptr<Driver> driver);
  virtual ~Worker();

  void Notify();
  void Terminate();
  void BindGroup(uint32_t father_id, const std::shared_ptr<TaskRunner>& child);
  void Bind(std::vector<std::shared_ptr<TaskRunner>> runner);
  void Bind(std::list<std::vector<std::shared_ptr<TaskRunner>>> list);
  void UnBind(const std::shared_ptr<TaskRunner>& runner);
  uint32_t GetRunningGroupSize();
  std::list<std::vector<std::shared_ptr<TaskRunner>>> UnBind();
  std::list<std::vector<std::shared_ptr<TaskRunner>>> ReleasePending();
  std::list<std::vector<std::shared_ptr<TaskRunner>>> RetainActiveAndUnschedulable();
  std::list<std::vector<std::shared_ptr<TaskRunner>>> Retain(const std::shared_ptr<TaskRunner>& runner);

  inline bool GetStackingMode() { return is_stacking_mode_; }
  inline void SetStackingMode(bool is_stacking_mode) { is_stacking_mode_ = is_stacking_mode; }
  inline TimeDelta GetTimeRemaining() { return TimePoint::Now() - next_task_time_; }
  inline uint32_t GetGroupId() {
    return group_id_;
  }
  inline void SetGroupId(uint32_t id) {
    group_id_ = id;
  }
  inline uint32_t FetchAndSubReuseCount() {
    return --reuse_count_;
  }
  inline uint32_t FetchAndAddReuseCount() {
    return ++reuse_count_;
  }
  static bool IsTaskRunning();
  bool RunTask();
  void BeforeStart(std::function<void()> before_start) { before_start_ = before_start; }
  void Start(bool in_new_thread = true);

  virtual void SetName(const std::string& name) = 0;
  virtual std::weak_ptr<Worker> GetSelf() = 0;

 private:
  friend class WorkerManager;
  friend class TaskRunner;

  static uint32_t GetCurrentWorkerId();
  static std::shared_ptr<TaskRunner> GetCurrentTaskRunner();

  std::unique_ptr<Task> GetNextTask();
  void AddImmediateTask(std::unique_ptr<Task> task);
  bool HasUnschedulableRunner();
  void BalanceNoLock();
  void SortNoLock();

  bool HasTask();
  bool HasMoreUrgentTask(TimeDelta min_wait_time, TimePoint now);

  int32_t WorkerKeyCreate(uint32_t task_runner_id, const std::function<void(void *)>& destruct);
  bool WorkerKeyDelete(uint32_t task_runner_id, int32_t key);
  bool WorkerSetSpecific(uint32_t task_runner_id, int32_t key, void *p);
  void *WorkerGetSpecific(uint32_t task_runner_id, int32_t key);
  void WorkerDestroySpecific(uint32_t task_runner_id);
  void WorkerDestroySpecificNoLock(uint32_t task_runner_id);
  void WorkerDestroySpecifics();
  std::array<WorkerKey, kWorkerKeysMax> GetMovedSpecificKeys(uint32_t task_runner_id);
  void UpdateSpecificKeys(uint32_t task_runner_id, std::array<WorkerKey, kWorkerKeysMax> array);
  std::array<void *, Worker::kWorkerKeysMax> GetMovedSpecific(uint32_t task_runner_id);
  void UpdateSpecific(uint32_t task_runner_id, std::array<void *, kWorkerKeysMax> array);

  std::thread thread_;
  std::function<void()> before_start_;
  std::string name_;
  std::list<std::vector<std::shared_ptr<TaskRunner>>> running_group_list_;
  std::list<std::vector<std::shared_ptr<TaskRunner>>> pending_group_list_;
  std::mutex running_mutex_;
  std::mutex pending_mutex_;
  std::map<uint32_t, std::array<Worker::WorkerKey, Worker::kWorkerKeysMax>> worker_key_map_;
  std::map<uint32_t, std::array<void *, Worker::kWorkerKeysMax>> specific_map_;
  std::queue<std::unique_ptr<Task>> immediate_task_queue_;
  TimeDelta min_wait_time_;
  TimePoint next_task_time_;
  bool need_balance_;
  bool is_stacking_mode_;
  bool has_migration_data_;
  bool is_schedulable_;
  uint32_t reuse_count_;
  uint32_t group_id_;
  std::unique_ptr<Driver> driver_;
};

}  // namespace runner
}  // namespace flexui::common
