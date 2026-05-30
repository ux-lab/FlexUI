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
 * modules/footstone/include/footstone/log_level.h in the Hippy project.
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

namespace flexui::common {
inline namespace log {

enum LogSeverity {
  TDF_LOG_DEBUG,
  TDF_LOG_INFO,
  TDF_LOG_WARNING,
  TDF_LOG_ERROR,
  TDF_LOG_FATAL,
  TDF_LOG_NUM_SEVERITIES
};

#ifdef _WIN32
#define LOG_0 LOG_ERROR
#endif

#ifdef NDEBUG
constexpr LogSeverity TDF_LOG_DFATAL = TDF_LOG_ERROR;
#else
constexpr LogSeverity TDF_LOG_DFATAL = TDF_LOG_FATAL;
#endif

}  // namespace log
}  // namespace flexui::common
