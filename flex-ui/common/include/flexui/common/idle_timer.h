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
 * modules/footstone/include/footstone/idle_timer.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone` -> `flexui::common`.
 *   - Renamed include path `footstone/...` -> `flexui/common/...`.
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#include "flexui/common/base_timer.h"
#include "flexui/common/idle_task.h"

namespace flexui::common {
inline namespace timer {

class IdleTimer : public BaseTimer {
 public:
  using Task = runner::Task;
  using TaskRunner = runner::TaskRunner;

  IdleTimer() = default;
  explicit IdleTimer(std::shared_ptr<TaskRunner> task_runner);
  virtual ~IdleTimer();

  IdleTimer(IdleTimer&) = delete;
  IdleTimer& operator=(IdleTimer&) = delete;

  virtual void Start(std::unique_ptr<IdleTask> idle_task, TimeDelta timeout);
  virtual void Start(std::unique_ptr<IdleTask> idle_task);

 private:
  void OnStop() final;
  void RunUserTask() final;

  std::shared_ptr<IdleTask> idle_task_;
};

}  // namespace timer
}  // namespace flexui::common
