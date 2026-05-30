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

#include "flexui/common/deserializer.h"

#include <cstring>

#include "flexui/common/flexui_value.h"
#include "flexui/common/logging.h"
#include "flexui/common/string_view_utils.h"
#include "flexui/common/serializer.h"
#include "flexui/common/string_view.h"

namespace flexui::common {
inline namespace value {

using string_view = flexui::common::stringview::string_view;
using StringViewUtils = flexui::common::stringview::StringViewUtils;
constexpr uint32_t kSupportedVersion = 15;

Deserializer::Deserializer(const std::vector<uint8_t>& data)
    : position_(&data[0]), end_(&data[0] + data.size()) {}

Deserializer::Deserializer(const uint8_t* data, size_t size)
    : position_(data), end_(data + size) {}

Deserializer::~Deserializer() = default;

bool Deserializer::ReadValue(FlexUIValue& value) {
  bool ret = ReadObject(value);
  return ret;
}

bool Deserializer::ReadHeader() {
  if (position_ < end_ && *position_ == static_cast<uint8_t>(SerializationTag::kVersion)) {
    SerializationTag tag;
    ReadTag(tag);
    version_ = ReadVarint<uint32_t>();
    if (version_ <= kSupportedVersion) return true;
  }
  return false;
}

void Deserializer::ReadHeaderChecked() {
  if (position_ < end_ && *position_ == static_cast<uint8_t>(SerializationTag::kVersion)) {
    SerializationTag tag;
    ReadTag(tag);
    version_ = ReadVarint<uint32_t>();
    FLEXUI_CHECK(version_ <= kSupportedVersion) << "deserializer version is " << version_;
  }
}

bool Deserializer::PeekTag(SerializationTag& tag) {
  const uint8_t* peek_position = position_;
  do {
    if (peek_position >= end_) return false;
    tag = static_cast<SerializationTag>(*peek_position);
    peek_position++;
  } while (tag == SerializationTag::kPadding);
  return true;
}

bool Deserializer::ReadTag(SerializationTag& tag) {
  do {
    if (position_ >= end_) return false;
    tag = static_cast<SerializationTag>(*position_);
    position_++;
  } while (tag == SerializationTag::kPadding);
  return true;
}

void Deserializer::ConsumeTag(SerializationTag peek_tag) {
  SerializationTag tag = (SerializationTag)0;
  ReadTag(tag);
  FLEXUI_DCHECK(tag == peek_tag);
}

bool Deserializer::ReadInt32(int32_t& value) {
  value = ReadZigZag<int32_t>();
  return true;
}

bool Deserializer::ReadInt32(FlexUIValue& value) {
  value = FlexUIValue(ReadZigZag<int32_t>());
  return true;
}

bool Deserializer::ReadUInt32(uint32_t& value) {
  value = ReadVarint<uint32_t>();
  return true;
}

bool Deserializer::ReadUInt32(FlexUIValue& value) {
  value = FlexUIValue(ReadVarint<uint32_t>());
  return true;
}

bool Deserializer::ReadDouble(double& value) {
  if (sizeof(double) > static_cast<unsigned>(end_ - position_)) return false;
  memcpy(&value, position_, sizeof(double));
  position_ += sizeof(double);
  if (std::isnan(value)) value = std::numeric_limits<double>::quiet_NaN();
  return true;
}

bool Deserializer::ReadDouble(FlexUIValue& value) {
  if (sizeof(double) > static_cast<unsigned>(end_ - position_)) return false;
  double d;
  memcpy(&d, position_, sizeof(double));
  position_ += sizeof(double);
  if (std::isnan(d)) d = std::numeric_limits<double>::quiet_NaN();
  value = FlexUIValue(d);
  return true;
}

bool Deserializer::ReadUtf8String(std::string& value) {
  uint32_t utf8_length;
  utf8_length = ReadVarint<uint32_t>();
  if (utf8_length > static_cast<uint32_t>(end_ - position_)) return false;

  const uint8_t* start = const_cast<uint8_t*>(position_);
  position_ += utf8_length;
  string_view sv(reinterpret_cast<const string_view::char8_t_*>(start), utf8_length);
  value = StringViewUtils::ToStdString(StringViewUtils::ConvertEncoding(
      sv, string_view::Encoding::Utf8).utf8_value());
  return true;
}

bool Deserializer::ReadUtf8String(FlexUIValue& value) {
  uint32_t utf8_length;
  utf8_length = ReadVarint<uint32_t>();
  if (utf8_length > static_cast<uint32_t>(end_ - position_)) return false;

  const uint8_t* start = position_;
  position_ += utf8_length;
  string_view sv(reinterpret_cast<const string_view::char8_t_*>(start), utf8_length);
  value = StringViewUtils::ToStdString(StringViewUtils::ConvertEncoding(
      sv, string_view::Encoding::Utf8).utf8_value());
  return true;
}

bool Deserializer::ReadOneByteString(std::string& value) {
  uint32_t one_byte_length;
  one_byte_length = ReadVarint<uint32_t>();
  if (one_byte_length > static_cast<uint32_t>(end_ - position_)) return false;

  const char* start = reinterpret_cast<char*>(const_cast<uint8_t*>(position_));
  position_ += one_byte_length;
  string_view sv(start, one_byte_length);
  value = StringViewUtils::ToStdString(StringViewUtils::ConvertEncoding(
      sv, string_view::Encoding::Utf8).utf8_value());
  return true;
}

bool Deserializer::ReadOneByteString(FlexUIValue& value) {
  uint32_t one_byte_length;
  one_byte_length = ReadVarint<uint32_t>();
  if (one_byte_length > static_cast<uint32_t>(end_ - position_)) return false;

  const char* start = reinterpret_cast<char*>(const_cast<uint8_t*>(position_));
  position_ += one_byte_length;
  string_view sv(start, one_byte_length);
  value = StringViewUtils::ToStdString(StringViewUtils::ConvertEncoding(
      sv, string_view::Encoding::Utf8).utf8_value());
  return true;
}

bool Deserializer::ReadTwoByteString(std::string& value) {
  uint32_t two_byte_length;
  two_byte_length = ReadVarint<uint32_t>();
  if (two_byte_length > static_cast<uint32_t>(end_ - position_)) return false;

  const char16_t* start = reinterpret_cast<char16_t*>(const_cast<uint8_t*>(position_));
  position_ += two_byte_length;
  string_view sv(start, two_byte_length / sizeof(char16_t));
  value = StringViewUtils::ToStdString(StringViewUtils::ConvertEncoding(
      sv, string_view::Encoding::Utf8).utf8_value());
  return true;
}

bool Deserializer::ReadTwoByteString(FlexUIValue& value) {
  uint32_t two_byte_length;
  two_byte_length = ReadVarint<uint32_t>();
  if (two_byte_length > static_cast<uint32_t>(end_ - position_)) return false;

  const char16_t* start = reinterpret_cast<char16_t*>(const_cast<uint8_t*>(position_));
  position_ += two_byte_length;
  string_view sv(start, two_byte_length / sizeof(char16_t));
  value = StringViewUtils::ToStdString(StringViewUtils::ConvertEncoding(
      sv, string_view::Encoding::Utf8).utf8_value());
  return true;
}

bool Deserializer::ReadDenseJSArray(FlexUIValue& value) {
  uint32_t length = ReadVarint<uint32_t>();
  FLEXUI_DCHECK(length <= static_cast<uint32_t>(end_ - position_));

  FlexUIValue::FlexUIValueArrayType array;
  array.resize(length);

  for (uint32_t i = 0; i < length; i++) {
    SerializationTag tag = (SerializationTag)0;
    PeekTag(tag);
    if (tag == SerializationTag::kTheHole) {
      ConsumeTag(SerializationTag::kTheHole);
      continue;
    }

    FlexUIValue elem;
    ReadObject(elem);
    array[i] = elem;
  }

  uint32_t num_properties;
  uint32_t expected_num_properties;
  uint32_t expected_length;
  bool ret = ReadObjectProperties(num_properties, SerializationTag::kEndDenseJSArray);
  if (!ret) return false;
  expected_num_properties = ReadVarint<uint32_t>();
  expected_length = ReadVarint<uint32_t>();
  if (num_properties != expected_num_properties) return false;
  if (length != expected_length) return false;

  value = array;
  return true;
}

bool Deserializer::ReadJSObject(FlexUIValue& value) {
  uint32_t num_properties;
  FlexUIValueObjectType object;
  if (!ReadObjectProperties(object, num_properties, SerializationTag::kEndJSObject)) {
    return false;
  }

  auto expected_num_properties = ReadVarint<uint32_t>();
  if (num_properties != expected_num_properties) {
    return false;
  }

  value = object;
  return true;
}

template <typename T>
T Deserializer::ReadVarint() {
  // Reads an unsigned integer as a base-128 varint.
  // The number is written, 7 bits at a time, from the least significant to the
  // most significant 7 bits. Each byte, except the last, has the MSB set.
  // If the varint is larger than T, any more significant bits are discarded.
  // See also https://developers.google.com/protocol-buffers/docs/encoding
  static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
                "Only unsigned integer types can be read as varints.");

  T val = 0;
  unsigned shift = 0;
  bool has_another_byte;
  do {
    FLEXUI_DCHECK(end_ > position_);
    uint8_t byte = *position_;
    if (shift < sizeof(T) * 8) {
      val |= static_cast<T>(byte & 0x7F) << shift;
      shift += 7;
    }
    has_another_byte = byte & 0x80;
    position_++;
  } while (has_another_byte);
  return val;
}

template <typename T>
T Deserializer::ReadZigZag() {
  // Reads a signed integer as a varint using ZigZag encoding.
  // See also https://developers.google.com/protocol-buffers/docs/encoding
  static_assert(std::is_integral<T>::value && std::is_signed<T>::value,
                "Only signed integer types can be read as zigzag.");
  using UnsignedT = typename std::make_unsigned<T>::type;
  UnsignedT unsigned_value;
  unsigned_value = ReadVarint<UnsignedT>();
  return static_cast<T>((unsigned_value >> 1) ^ static_cast<unsigned int>(-static_cast<T>(unsigned_value & 1)));
}

bool Deserializer::ReadObject(FlexUIValue& value) {
  bool ret = false;
  SerializationTag tag;
  ReadTag(tag);
  switch (tag) {
    case SerializationTag::kUndefined: {
      value = FlexUIValue::Undefined();
      return true;
    }
    case SerializationTag::kNull: {
      value = FlexUIValue::Null();
      return true;
    }
    case SerializationTag::kTrue: {
      value = FlexUIValue(true);
      return true;
    }
    case SerializationTag::kFalse: {
      value = FlexUIValue(false);
      return true;
    }
    case SerializationTag::kInt32: {
      int32_t i32 = ReadZigZag<int32_t>();
      value = FlexUIValue(i32);
      return true;
    }
    case SerializationTag::kUint32: {
      uint32_t u32 = ReadVarint<uint32_t>();
      value = FlexUIValue(u32);
      return true;
    }
    case SerializationTag::kDouble: {
      double d = 0;
      ReadDouble(d);
      value = FlexUIValue(d);
      return true;
    }
    case SerializationTag::kUtf8String: {
      ret = ReadUtf8String(value);
      return ret;
    }
    case SerializationTag::kOneByteString: {
      ret = ReadOneByteString(value);
      return ret;
    }
    case SerializationTag::kTwoByteString: {
      ret = ReadTwoByteString(value);
      return ret;
    }
    case SerializationTag::kBeginDenseJSArray: {
      ret = ReadDenseJSArray(value);
      return ret;
    }
    case SerializationTag::kBeginJSObject: {
      ret = ReadJSObject(value);
      return ret;
    }
    default: {
      ret = false;
    }
  }

  return ret;
}

bool Deserializer::ReadObjectProperties(FlexUIValueObjectType& property,
                                        uint32_t& number_properties,
                                        SerializationTag end_tag) {
  uint32_t number = 0;
  FlexUIValue::FlexUIValueObjectType object;
  bool ret = true;

  SerializationTag tag;
  while (PeekTag(tag)) {
    if (tag == end_tag) {
      ConsumeTag(end_tag);
      number_properties = number;
      return true;
    }

    if (end_tag == SerializationTag::kEndJSObject) {
      FlexUIValue key;
      ret = ReadObject(key);
      if (!ret) return false;
      FlexUIValue val;
      ret = ReadObject(val);
      if (!ret) return false;
      if (!key.IsString()) {
        FLEXUI_DLOG(WARNING) << "error key type:" + std::to_string(static_cast<int>(key.GetType()));
        return false;
      }
      object.insert(std::pair<std::string, FlexUIValue>(key.ToStringChecked(), val));
      property = object;
    }
    number++;
  }

  return false;
}

bool Deserializer::ReadObjectProperties(uint32_t& number_properties,
                                        SerializationTag end_tag) {
  uint32_t number = 0;

  SerializationTag tag;
  while (PeekTag(tag)) {
    if (tag == end_tag) {
      ConsumeTag(end_tag);
      number_properties = number;
      return true;
    }
    number++;
  }

  return false;
}

}  // namespace value
}  // namespace flexui::common
