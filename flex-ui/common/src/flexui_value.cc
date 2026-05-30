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
 * modules/footstone/src/hippy_value.cc in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Renamed file: hippy_value.cc -> flexui_value.cc.
 *   - Renamed type: HippyValue -> FlexUIValue (all occurrences).
 *   - Renamed nested types: HippyValueObjectType -> FlexUIValueObjectType,
 *     HippyValueArrayType -> FlexUIValueArrayType.
 *   - Moved namespace `footstone::value` -> `flexui::common`.
 *   - Renamed include paths `footstone/...` -> `flexui/common/...`.
 *   - Renamed macros `FOOTSTONE_*` -> `FLEXUI_*`.
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#include "flexui/common/flexui_value.h"

#include "flexui/common/logging.h"
#include "flexui/common/hash.h"

using FlexUIValue = flexui::common::FlexUIValue;

std::size_t std::hash<FlexUIValue>::operator()(const FlexUIValue& value) const noexcept {
  switch (value.type_) {
    case FlexUIValue::Type::kUndefined:
      return UndefinedHashValue;
    case FlexUIValue::Type::kNull:
      return NullHashValue;
    case FlexUIValue::Type::kBoolean:
      return std::hash<bool>{}(value.b_);
    case FlexUIValue::Type::kNumber: {
      switch (value.number_type_) {
        case FlexUIValue::NumberType::kInt32:
          return std::hash<int32_t>{}(value.num_.i32_);
        case FlexUIValue::NumberType::kUInt32:
          return std::hash<uint32_t>{}(value.num_.u32_);
        case FlexUIValue::NumberType::kDouble:
          return std::hash<double>{}(value.num_.d_);
        case FlexUIValue::NumberType::kNaN:
          return 0;
        default:
          break;
      }
      return 0;
    }
    case FlexUIValue::Type::kString:
      return std::hash<std::string>{}(value.str_);
    case FlexUIValue::Type::kArray:
      return std::hash<FlexUIValue::FlexUIValueArrayType>{}(value.arr_);
    case FlexUIValue::Type::kObject:
      return std::hash<FlexUIValue::FlexUIValueObjectType>{}(value.obj_);
    default:
      break;
  }
  return 0;
}

namespace flexui {
namespace common {

static std::string global_empty_string;

const FlexUIValue FlexUIValue::Undefined() {
  FlexUIValue undefined;
  undefined.type_ = Type::kUndefined;
  return undefined;
}

const FlexUIValue FlexUIValue::Null() {
  FlexUIValue null;
  null.type_ = Type::kNull;
  return null;
}

FlexUIValue::FlexUIValue(const FlexUIValue& source) : type_(source.type_), number_type_(source.number_type_) {
  switch (type_) {
    case FlexUIValue::Type::kBoolean:
      b_ = source.b_;
      break;
    case FlexUIValue::Type::kNumber: {
      switch (source.number_type_) {
        case FlexUIValue::NumberType::kInt32:
          num_.i32_ = source.num_.i32_;
          break;
        case FlexUIValue::NumberType::kUInt32:
          num_.u32_ = source.num_.u32_;
          break;
        case FlexUIValue::NumberType::kDouble:
          num_.d_ = source.num_.d_;
          break;
        case FlexUIValue::NumberType::kNaN:
        default:
          break;
      }
      break;
    }
    case FlexUIValue::Type::kString:
      new (&str_) std::string(source.str_);
      break;
    case FlexUIValue::Type::kObject:
      new (&obj_) FlexUIValueObjectType(source.obj_);
      break;
    case FlexUIValue::Type::kArray:
      new (&arr_) FlexUIValueArrayType(source.arr_);
      break;
    default:
      break;
  }
}

FlexUIValue::~FlexUIValue() { Deallocate(); }

FlexUIValue& FlexUIValue::operator=(const FlexUIValue& rhs) noexcept {
  if (this == &rhs) {
    return *this;
  }

  switch (rhs.type_) {
    case FlexUIValue::Type::kNull:
    case FlexUIValue::Type::kUndefined:
      Deallocate();
      break;
    case FlexUIValue::Type::kNumber:
      Deallocate();
      switch (rhs.number_type_) {
        case FlexUIValue::NumberType::kInt32:
          num_.i32_ = rhs.num_.i32_;
          break;
        case FlexUIValue::NumberType::kUInt32:
          num_.u32_ = rhs.num_.u32_;
          break;
        case FlexUIValue::NumberType::kDouble:
          num_.d_ = rhs.num_.d_;
          break;
        case FlexUIValue::NumberType::kNaN:
          break;
        default:
          break;
      }
      break;
    case FlexUIValue::Type::kBoolean:
      Deallocate();
      b_ = rhs.b_;
      break;
    case FlexUIValue::Type::kString:
      if (type_ != FlexUIValue::Type::kString) {
        Deallocate();
        new (&str_) std::string(rhs.str_);
      } else {
        str_ = rhs.str_;
      }
      break;
    case FlexUIValue::Type::kObject:
      if (type_ != FlexUIValue::Type::kObject) {
        Deallocate();
        new (&obj_) FlexUIValueObjectType(rhs.obj_);
      } else {
        obj_ = rhs.obj_;
      }
      break;
    case FlexUIValue::Type::kArray:
      if (type_ != FlexUIValue::Type::kArray) {
        Deallocate();
        new (&arr_) FlexUIValueArrayType(rhs.arr_);
      } else {
        arr_ = rhs.arr_;
      }
      break;
    default:
      break;
  }

  type_ = rhs.type_;
  number_type_ = rhs.number_type_;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const int32_t rhs) noexcept {
  Deallocate();
  type_ = FlexUIValue::Type::kNumber;
  number_type_ = FlexUIValue::NumberType::kInt32;
  num_.i32_ = rhs;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const uint32_t rhs) noexcept {
  Deallocate();
  type_ = FlexUIValue::Type::kNumber;
  number_type_ = FlexUIValue::NumberType::kUInt32;
  num_.u32_ = rhs;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const double rhs) noexcept {
  Deallocate();
  type_ = FlexUIValue::Type::kNumber;
  number_type_ = FlexUIValue::NumberType::kDouble;
  num_.d_ = rhs;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const bool rhs) noexcept {
  Deallocate();
  type_ = FlexUIValue::Type::kBoolean;
  number_type_ = FlexUIValue::NumberType::kNaN;
  b_ = rhs;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const std::string& rhs) noexcept {
  if (type_ != FlexUIValue::Type::kString) {
    Deallocate();
    new (&str_) std::string(rhs);
  } else {
    str_ = rhs;
  }
  type_ = FlexUIValue::Type::kString;
  number_type_ = FlexUIValue::NumberType::kNaN;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const char* rhs) noexcept {
  if (type_ != FlexUIValue::Type::kString) {
    Deallocate();
    new (&str_) std::string(rhs);
  } else {
    str_ = rhs;
  }
  type_ = FlexUIValue::Type::kString;
  number_type_ = FlexUIValue::NumberType::kNaN;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const FlexUIValueObjectType& rhs) noexcept {
  if (type_ != FlexUIValue::Type::kObject) {
    Deallocate();
    new (&obj_) FlexUIValueObjectType(rhs);
  } else {
    obj_ = rhs;
  }
  type_ = FlexUIValue::Type::kObject;
  number_type_ = FlexUIValue::NumberType::kNaN;
  return *this;
}

FlexUIValue& FlexUIValue::operator=(const FlexUIValueArrayType& rhs) noexcept {
  if (type_ != FlexUIValue::Type::kArray) {
    Deallocate();
    new (&arr_) FlexUIValueArrayType(rhs);
  } else {
    arr_ = rhs;
  }
  type_ = FlexUIValue::Type::kArray;
  number_type_ = FlexUIValue::NumberType::kNaN;
  return *this;
}

bool FlexUIValue::operator==(const FlexUIValue& rhs) const noexcept {
  if (type_ != rhs.type_) {
    return false;
  }

  switch (type_) {
    case FlexUIValue::Type::kUndefined:
    case FlexUIValue::Type::kNull:
      return true;
    case FlexUIValue::Type::kBoolean:
      return b_ == rhs.b_;
    case FlexUIValue::Type::kNumber: {
      switch (number_type_) {
        case FlexUIValue::NumberType::kInt32:
          return num_.i32_ == rhs.num_.i32_;
        case FlexUIValue::NumberType::kUInt32:
          return num_.u32_ == rhs.num_.u32_;
        case FlexUIValue::NumberType::kDouble:
          return num_.d_ == rhs.num_.d_;
        default:
          break;
      }
      return false;
    }
    case FlexUIValue::Type::kString:
      return str_ == rhs.str_;
    case FlexUIValue::Type::kObject:
      return obj_ == rhs.obj_;
    case FlexUIValue::Type::kArray:
      return arr_ == rhs.arr_;
    default:
      break;
  }

  return false;
}

bool FlexUIValue::operator!=(const FlexUIValue& rhs) const noexcept { return !operator==(rhs); }

bool FlexUIValue::operator<(const FlexUIValue& rhs) const noexcept {
  if (type_ == FlexUIValue::Type::kNumber && rhs.type_ == FlexUIValue::Type::kNumber) {
    return number_type_ < rhs.number_type_;
  }
  return type_ < rhs.type_;
}

bool FlexUIValue::operator>(const FlexUIValue& rhs) const noexcept {
  if (type_ == FlexUIValue::Type::kNumber && rhs.type_ == FlexUIValue::Type::kNumber) {
    return number_type_ > rhs.number_type_;
  }
  return type_ > rhs.type_;
}

bool FlexUIValue::operator<=(const FlexUIValue& rhs) const noexcept { return !operator>(rhs); }

bool FlexUIValue::operator>=(const FlexUIValue& rhs) const noexcept { return !operator<(rhs); }

std::ostream& operator<<(std::ostream& os, const FlexUIValue& flexui_value) {
  if (flexui_value.type_ == FlexUIValue::Type::kUndefined) {
    os << "undefined";
  } else if (flexui_value.type_ == FlexUIValue::Type::kNull) {
    os << "null";
  } else if (flexui_value.type_ == FlexUIValue::Type::kNumber) {
    if (flexui_value.number_type_ == FlexUIValue::NumberType::kNaN) {
      os << "NaN";
    } else {
      os << flexui_value.ToDoubleChecked();
    }
  } else if (flexui_value.type_ == FlexUIValue::Type::kBoolean) {
    os << flexui_value.ToBooleanChecked();
  } else if (flexui_value.type_ == FlexUIValue::Type::kString) {
    os << "\"" << flexui_value.ToStringChecked() << "\"";
  } else if (flexui_value.type_ == FlexUIValue::Type::kObject) {
    os << "{";
    auto map = flexui_value.ToObjectChecked();
    size_t index = 0;
    for (const auto& kv : map) {
      os << "\"" << kv.first << "\": " << kv.second;
      if (index != map.size() - 1) os << ",";
      index++;
    }
    os << "}";
  } else if (flexui_value.type_ == FlexUIValue::Type::kArray) {
    os << "[ ";
    auto arr = flexui_value.ToArrayChecked();
    for (size_t i = 0; i < arr.size(); i++) {
      os << arr[i];
      if (i != arr.size() - 1) os << ",";
    }
    os << " ]";
  }
  return os;
}

bool FlexUIValue::IsUndefined() const noexcept { return type_ == Type::kUndefined; }

bool FlexUIValue::IsNull() const noexcept { return type_ == Type::kNull; }

bool FlexUIValue::IsBoolean() const noexcept { return type_ == Type::kBoolean; }

bool FlexUIValue::IsNumber() const noexcept { return type_ == Type::kNumber; }

bool FlexUIValue::IsString() const noexcept { return type_ == Type::kString; }

bool FlexUIValue::IsArray() const noexcept { return type_ == Type::kArray; }

bool FlexUIValue::IsObject() const noexcept { return type_ == Type::kObject; }

bool FlexUIValue::IsInt32() const noexcept { return type_ == Type::kNumber && number_type_ == NumberType::kInt32; }

bool FlexUIValue::IsUInt32() const noexcept { return type_ == Type::kNumber && number_type_ == NumberType::kUInt32; }

bool FlexUIValue::IsDouble() const noexcept { return type_ == Type::kNumber && number_type_ == NumberType::kDouble; }

bool FlexUIValue::ToInt32(int32_t& i32) const {
  bool is_int32 = IsInt32();
  if (is_int32) i32 = num_.i32_;
  return is_int32;
}

int32_t FlexUIValue::ToInt32Checked() const {
  FLEXUI_CHECK(IsInt32());
  return num_.i32_;
}

bool FlexUIValue::ToUint32(uint32_t& u32) const {
  bool is_uint32 = IsUInt32();
  if (is_uint32) u32 = num_.u32_;
  return is_uint32;
}

uint32_t FlexUIValue::ToUint32Checked() const {
  FLEXUI_CHECK(IsUInt32());
  return num_.u32_;
}

bool FlexUIValue::ToDouble(double& d) const {
  bool is_number = IsNumber();
  if (number_type_ == FlexUIValue::NumberType::kDouble) d = num_.d_;
  if (number_type_ == FlexUIValue::NumberType::kInt32) d = num_.i32_;
  if (number_type_ == FlexUIValue::NumberType::kUInt32) d = num_.u32_;
  return is_number;
}

double FlexUIValue::ToDoubleChecked() const {
  FLEXUI_CHECK(IsNumber());
  if (number_type_ == FlexUIValue::NumberType::kDouble) return num_.d_;
  if (number_type_ == FlexUIValue::NumberType::kInt32) return num_.i32_;
  if (number_type_ == FlexUIValue::NumberType::kUInt32) return num_.u32_;
  FLEXUI_UNREACHABLE();
}

bool FlexUIValue::ToBoolean(bool& b) const {
  bool is_bool = IsBoolean();
  b = b_;
  return is_bool;
}

bool FlexUIValue::ToBooleanChecked() const {
  FLEXUI_CHECK(IsBoolean());
  return b_;
}

bool FlexUIValue::ToString(std::string& str) const {
  bool is_string = IsString();
  if (is_string) {
    str = str_;
  }
  return is_string;
}

const std::string& FlexUIValue::ToStringChecked() const {
  FLEXUI_CHECK(IsString());
  return str_;
}

std::string& FlexUIValue::ToStringChecked() {
  FLEXUI_CHECK(IsString());
  return str_;
}

const std::string& FlexUIValue::ToStringSafe() const {
  if (IsString()) {
    return str_;
  }
  return global_empty_string;
}

std::string& FlexUIValue::ToStringSafe() {
  if (IsString()) {
    return str_;
  }
  return global_empty_string;
}

bool FlexUIValue::ToObject(FlexUIValue::FlexUIValueObjectType& obj) const {
  bool is_object = IsObject();
  obj = obj_;
  return is_object;
}

const FlexUIValue::FlexUIValueObjectType& FlexUIValue::ToObjectChecked() const {
  FLEXUI_CHECK(IsObject());
  return obj_;
}

FlexUIValue::FlexUIValueObjectType& FlexUIValue::ToObjectChecked() {
  FLEXUI_CHECK(IsObject());
  return obj_;
}

bool FlexUIValue::ToArray(FlexUIValue::FlexUIValueArrayType& arr) const {
  bool is_array = IsArray();
  arr = arr_;
  return is_array;
}

const FlexUIValue::FlexUIValueArrayType& FlexUIValue::ToArrayChecked() const {
  FLEXUI_CHECK(IsArray());
  return arr_;
}

FlexUIValue::FlexUIValueArrayType& FlexUIValue::ToArrayChecked() {
  FLEXUI_CHECK(IsArray());
  return arr_;
}

inline void FlexUIValue::Deallocate() {
  switch (type_) {
    case Type::kString:
      str_.~basic_string();
      break;
    case Type::kArray:
      arr_.~vector();
      break;
    case Type::kObject:
      obj_.~unordered_map();
      break;
    default:
      break;
  }
}

}  // namespace common
}  // namespace flexui
