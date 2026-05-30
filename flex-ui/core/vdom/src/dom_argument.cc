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
 * dom/src/dom/dom_argument.cc in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Namespace: `hippy::dom` -> `flexui::core::vdom`.
 *   - Include paths: `footstone/` -> `flexui/common/`.
 *   - Type rename: `HippyValue` -> `FlexUIValue`.
 */

#include "flexui/core/vdom/dom_argument.h"

#include <vector>

#include "flexui/common/deserializer.h"
#include "flexui/common/logging.h"
#include "flexui/common/serializer.h"

namespace flexui::core::vdom {

DomArgument::DomArgument(const DomArgument& source)
    : data_(source.data_), argument_type_(source.argument_type_) {}

DomArgument::~DomArgument() = default;

bool DomArgument::ToBson(std::vector<uint8_t>& bson) const {
  if (argument_type_ == ArgumentType::OBJECT) {
    auto value = std::any_cast<flexui::common::FlexUIValue>(&data_);
    return ConvertObjectToBson(*value, bson);
  } else if (argument_type_ == ArgumentType::BSON) {
    auto vec = std::any_cast<std::vector<uint8_t>>(&data_);
    bson = *vec;
    return true;
  }
  return false;
}

bool DomArgument::ToObject(flexui::common::FlexUIValue& value) const {
  if (argument_type_ == ArgumentType::OBJECT) {
    auto vec = std::any_cast<flexui::common::FlexUIValue>(&data_);
    value = *vec;
    return true;
  } else if (argument_type_ == ArgumentType::BSON) {
    auto vec = std::any_cast<std::vector<uint8_t>>(&data_);
    std::vector<uint8_t> bson(vec->begin(), vec->end());
    return ConvertBsonToObject(bson, value);
  }
  return false;
}

bool DomArgument::ConvertObjectToBson(const flexui::common::FlexUIValue& value,
                                       std::vector<uint8_t>& bson) {
  flexui::common::value::Serializer serializer;
  serializer.WriteHeader();
  serializer.WriteValue(value);
  std::pair<uint8_t*, size_t> pair = serializer.Release();
  bson.resize(pair.second);
  memcpy(&bson[0], pair.first, sizeof(uint8_t) * pair.second);
  flexui::common::value::SerializerHelper::DestroyBuffer(pair);
  return true;
}

bool DomArgument::ConvertBsonToObject(const std::vector<uint8_t>& bson,
                                       flexui::common::FlexUIValue& value) {
  flexui::common::value::Deserializer deserializer(bson);
  deserializer.ReadHeader();
  bool ret = deserializer.ReadValue(value);
  return ret;
}

}  // namespace flexui::core::vdom
