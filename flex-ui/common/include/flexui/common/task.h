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
 * modules/footstone/include/footstone/task.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone` -> `flexui::common`.
 *   - Renamed include path `footstone/...` -> `flexui/common/...`.
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

namespace flexui::common {
inline namespace runner {

class Task {
 public:
  Task();
  explicit Task(std::function<void()> unit);
  ~Task() = default;

  inline uint32_t GetId() { return id_; }
  inline void SetExecUnit(std::function<void()> unit) { unit_ = unit; }
  inline void Run() {
    if (unit_) {
      unit_();
    }
  }

 private:
  static std::atomic<uint32_t> g_next_task_id;

  std::atomic<uint32_t> id_{};
  std::function<void()> unit_;  // A unit of work to be processed
};

}  // namespace runner
}  // namespace flexui::common
