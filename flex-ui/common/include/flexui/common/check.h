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
 * modules/footstone/include/footstone/check.h in the Hippy project.
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

#include "flexui/common/logging.h"

namespace flexui::common {
inline namespace check {

template<typename SourceType, typename TargetType>
static constexpr bool numeric_cast(const SourceType& source, TargetType& target) {
  auto target_value = static_cast<TargetType>(source);
  if (static_cast<SourceType>(target_value) != source || (target_value < 0 && source > 0)
      || (target_value > 0 && source < 0)) {
    return false;
  }
  target = target_value;
  return true;
}

template<typename SourceType, typename TargetType>
static constexpr TargetType checked_numeric_cast(const SourceType& source) {
  TargetType target = 0;
  auto result = numeric_cast<SourceType, TargetType>(source, target);
  FLEXUI_CHECK(result);
  return target;
}

}
}
