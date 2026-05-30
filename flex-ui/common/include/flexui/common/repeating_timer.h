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
 * modules/footstone/include/footstone/repeating_timer.h in the Hippy project.
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

namespace flexui::common {
inline namespace timer {

class RepeatingTimer : public BaseTimer, public std::enable_shared_from_this<RepeatingTimer> {
 public:
  using TaskRunner = runner::TaskRunner;

  RepeatingTimer() = default;
  explicit RepeatingTimer(const std::shared_ptr<TaskRunner>& task_runner);
  virtual ~RepeatingTimer();

  RepeatingTimer(RepeatingTimer&) = delete;
  RepeatingTimer& operator=(RepeatingTimer&) = delete;

  void Start(std::unique_ptr<Task> user_task, TimeDelta delay);

  virtual std::weak_ptr<BaseTimer> GetWeakSelf() override;

 private:
  void OnStop() final;
  void RunUserTask() override;
};

}  // namespace timer
}  // namespace flexui::common
