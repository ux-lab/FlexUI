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
 * modules/footstone/include/footstone/log_settings.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone::log` -> `flexui::common::log`.
 *   - Renamed include path `footstone/log_level.h` -> `flexui/common/log_level.h`.
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#include "flexui/common/log_level.h"

#include <string>

namespace flexui::common {
inline namespace log {

struct LogSettings {
  LogSeverity min_log_level = TDF_LOG_INFO;
};

void SetLogSettings(const LogSettings& settings);

LogSettings GetLogSettings();

int GetMinLogLevel();

}  // namespace log
}  // namespace flexui::common
