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
 * dom/include/dom/dom_argument.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Namespace: `hippy::dom` -> `flexui::core::vdom`.
 *   - Include path: `footstone/hippy_value.h` -> `flexui/common/flexui_value.h`.
 *   - Type rename: `HippyValue` -> `FlexUIValue` (all occurrences).
 */

#pragma once

#include <any>
#include <utility>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::vdom {

enum class ArgumentType {
  BSON,
  OBJECT,
};

class DomArgument {
 public:
  DomArgument() {}
  DomArgument(const DomArgument& source);

  DomArgument(const flexui::common::FlexUIValue& value)
      : data_(std::make_any<flexui::common::FlexUIValue>(value)),
        argument_type_(ArgumentType::OBJECT) {}
  DomArgument(const std::vector<uint8_t>& bson_value)
      : data_(std::make_any<std::vector<uint8_t>>(bson_value)),
        argument_type_(ArgumentType::BSON) {}
  DomArgument(const std::pair<uint8_t*, size_t>& bson_value)
      : data_(std::make_any<std::vector<uint8_t>>(bson_value.first,
                                                   bson_value.first + bson_value.second)),
        argument_type_(ArgumentType::BSON) {}

  ~DomArgument();

  DomArgument& operator=(const DomArgument& rhs) noexcept = default;

  bool ToBson(std::vector<uint8_t>& bson) const;
  bool ToObject(flexui::common::FlexUIValue& value) const;

 private:
  static bool ConvertObjectToBson(const flexui::common::FlexUIValue& value,
                                   std::vector<uint8_t>& bson);
  static bool ConvertBsonToObject(const std::vector<uint8_t>& bson,
                                   flexui::common::FlexUIValue& value);

  std::any data_;
  ArgumentType argument_type_;
};

}  // namespace flexui::core::vdom
