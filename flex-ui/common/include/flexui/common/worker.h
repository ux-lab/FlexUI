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
 *   - This is a STUB header providing forward declarations only so that
 *     task_runner.h/cc compile before Task 12 absorbs the full Worker
 *     implementation. Replace this file in Task 12.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "flexui/common/task.h"
#include "flexui/common/time_delta.h"
#include "flexui/common/time_point.h"

namespace flexui::common {
inline namespace runner {

class TaskRunner;

class Worker {
 public:
  static const int32_t kWorkerKeysMax = 32;

  virtual ~Worker() = default;

  // Called by TaskRunner to wake the worker when a new task is queued.
  virtual void Notify() = 0;

  // Sub-runner / group management (called from TaskRunner).
  virtual void BindGroup(uint32_t runner_id,
                         const std::shared_ptr<TaskRunner>& sub_runner) = 0;
  virtual void UnBind(const std::shared_ptr<TaskRunner>& sub_runner) = 0;

  // Stacking-mode support (sub-runner execution while parent runs).
  virtual void SetStackingMode(bool stacking) = 0;
  virtual void RunTask() = 0;

  // Thread-local storage keyed per TaskRunner id.
  virtual int32_t WorkerKeyCreate(uint32_t runner_id,
                                  const std::function<void(void*)>& destruct) = 0;
  virtual bool WorkerKeyDelete(uint32_t runner_id, int32_t key) = 0;
  virtual bool WorkerSetSpecific(uint32_t runner_id, int32_t key, void* p) = 0;
  virtual void* WorkerGetSpecific(uint32_t runner_id, int32_t key) = 0;
  virtual void WorkerDestroySpecific(uint32_t runner_id) = 0;

  // Static helpers used by TaskRunner::GetCurrentTaskRunner et al.
  static std::shared_ptr<TaskRunner> GetCurrentTaskRunner();
  static bool IsTaskRunning();
};

}  // namespace runner
}  // namespace flexui::common
