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
 * Modified by the FlexUI authors, 2026.
 * Changes:
 *   - namespace footstone -> namespace flexui::common
 *   - #include "footstone/..." -> #include "flexui/common/..."
 *   - FOOTSTONE_ macro prefix -> FLEXUI_
 *   - HippyValue -> FlexUIValue (type rename, Task 16)
 *   - HippyValueArrayType -> FlexUIValueArrayType
 *   - HippyValueObjectType -> FlexUIValueObjectType
 */

#pragma once

#include <vector>

#include "flexui/common/serializer.h"

namespace flexui::common {
inline namespace value {

class Deserializer {
  using FlexUIValueObjectType = flexui::common::FlexUIValue::FlexUIValueObjectType;
 public:
  Deserializer(const std::vector<uint8_t>& data);
  Deserializer(const uint8_t* data, size_t size);
  ~Deserializer();

  Deserializer(const Deserializer&) = delete;
  Deserializer& operator=(const Deserializer&) = delete;

  bool ReadHeader();

  void ReadHeaderChecked();

  bool ReadValue(FlexUIValue& value);

 private:
  bool ReadObject(FlexUIValue& value);

  bool PeekTag(SerializationTag& tag);

  bool ReadTag(SerializationTag& tag);

  void ConsumeTag(SerializationTag peek_tag);

  bool ReadInt32(int32_t& value);

  bool ReadInt32(FlexUIValue& value);

  bool ReadUInt32(uint32_t& value);

  bool ReadUInt32(FlexUIValue& value);

  bool ReadDouble(double& value);

  bool ReadDouble(FlexUIValue& value);

  bool ReadUtf8String(std::string& value);

  bool ReadUtf8String(FlexUIValue& value);

  bool ReadOneByteString(std::string& value);

  bool ReadOneByteString(FlexUIValue& value);

  bool ReadTwoByteString(std::string& value);

  bool ReadTwoByteString(FlexUIValue& value);

  bool ReadDenseJSArray(FlexUIValue& value);

  bool ReadJSObject(FlexUIValue& value);

 private:
  template <typename T>
  T ReadVarint();

  template <typename T>
  T ReadZigZag();

  bool ReadObjectProperties(FlexUIValueObjectType& value, uint32_t& number_properties, SerializationTag end_tag);

  bool ReadObjectProperties(uint32_t& number_properties, SerializationTag end_tag);

 private:
  const uint8_t* position_;
  const uint8_t* const end_;
  uint32_t version_ = 0;
};

}  // namespace value
}  // namespace flexui::common
