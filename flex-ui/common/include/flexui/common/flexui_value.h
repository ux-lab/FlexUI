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
 * modules/footstone/include/footstone/hippy_value.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Renamed file: hippy_value.h -> flexui_value.h.
 *   - Renamed type: HippyValue -> FlexUIValue (all occurrences in header and .cc).
 *   - Renamed nested types: HippyValueObjectType -> FlexUIValueObjectType,
 *     HippyValueArrayType -> FlexUIValueArrayType.
 *   - Moved namespace `footstone::value` -> `flexui::common`.
 *   - Renamed include path `footstone/...` -> `flexui/common/...`.
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace flexui {
namespace common {

class FlexUIValue final {
 public:
  using FlexUIValueObjectType = typename std::unordered_map<std::string, FlexUIValue>;
  using FlexUIValueArrayType = typename std::vector<FlexUIValue>;
  enum class Type { kUndefined, kNull, kNumber, kBoolean, kString, kObject, kArray };
  enum class NumberType { kInt32, kUInt32, kDouble, kNaN };

  union Number {
    int32_t i32_;
    uint32_t u32_;
    double d_;
    Number(int32_t i32) : i32_(i32) {}
    Number(uint32_t u32) : u32_(u32) {}
    Number(float f) : d_(f) {}
    Number(double d) : d_(d) {}
  };

  static const FlexUIValue Undefined();
  static const FlexUIValue Null();

  FlexUIValue() {}
  FlexUIValue(const FlexUIValue& source);

  explicit FlexUIValue(int32_t i32) : type_(Type::kNumber), number_type_(NumberType::kInt32), num_(i32) {}
  explicit FlexUIValue(uint32_t u32) : type_(Type::kNumber), number_type_(NumberType::kUInt32), num_(u32) {}
  explicit FlexUIValue(float f) : type_(Type::kNumber), number_type_(NumberType::kDouble), num_(f) {}
  explicit FlexUIValue(double d) : type_(Type::kNumber), number_type_(NumberType::kDouble), num_(d) {}
  explicit FlexUIValue(bool b) : type_(Type::kBoolean), b_(b) {}
  explicit FlexUIValue(std::string&& str) : type_(Type::kString), str_(std::move(str)) {}
  explicit FlexUIValue(const std::string& str) : type_(Type::kString), str_(str) {}
  explicit FlexUIValue(const char* string_value) : type_(Type::kString), str_(std::string(string_value)) {}
  explicit FlexUIValue(const char* string_value, size_t length)
      : type_(Type::kString), str_(std::string(string_value, length)) {}
  explicit FlexUIValue(FlexUIValueObjectType&& object_value) : type_(Type::kObject), obj_(std::move(object_value)) {}
  explicit FlexUIValue(const FlexUIValueObjectType& object_value) : type_(Type::kObject), obj_(object_value) {}
  explicit FlexUIValue(FlexUIValueArrayType&& array_value) : type_(Type::kArray), arr_(array_value) {}
  explicit FlexUIValue(FlexUIValueArrayType& array_value) : type_(Type::kArray), arr_(array_value) {}
  ~FlexUIValue();

  FlexUIValue& operator=(const FlexUIValue& rhs) noexcept;
  FlexUIValue& operator=(const int32_t rhs) noexcept;
  FlexUIValue& operator=(const uint32_t rhs) noexcept;
  FlexUIValue& operator=(const double rhs) noexcept;
  FlexUIValue& operator=(const bool rhs) noexcept;
  FlexUIValue& operator=(const std::string& rhs) noexcept;
  FlexUIValue& operator=(const char* rhs) noexcept;
  FlexUIValue& operator=(const FlexUIValueObjectType& rhs) noexcept;
  FlexUIValue& operator=(const FlexUIValueArrayType& rhs) noexcept;

  bool operator==(const FlexUIValue& rhs) const noexcept;
  bool operator!=(const FlexUIValue& rhs) const noexcept;
  bool operator<(const FlexUIValue& rhs) const noexcept;
  bool operator<=(const FlexUIValue& rhs) const noexcept;
  bool operator>(const FlexUIValue& rhs) const noexcept;
  bool operator>=(const FlexUIValue& rhs) const noexcept;

  inline Type GetType() noexcept { return type_; }
  inline Type GetType() const noexcept { return type_; }
  inline NumberType GetNumberType() noexcept { return number_type_; }
  inline NumberType GetNumberType() const noexcept { return number_type_; }

  bool IsUndefined() const noexcept;
  bool IsNull() const noexcept;
  bool IsBoolean() const noexcept;
  bool IsNumber() const noexcept;
  bool IsString() const noexcept;
  bool IsArray() const noexcept;
  bool IsObject() const noexcept;
  bool IsInt32() const noexcept;
  bool IsUInt32() const noexcept;
  bool IsDouble() const noexcept;

  bool ToInt32(int32_t& i32) const;
  int32_t ToInt32Checked() const;
  bool ToUint32(uint32_t& u32) const;
  uint32_t ToUint32Checked() const;
  bool ToDouble(double& d) const;
  double ToDoubleChecked() const;
  bool ToBoolean(bool& b) const;
  bool ToBooleanChecked() const;
  bool ToString(std::string& str) const;
  const std::string& ToStringChecked() const;
  std::string& ToStringChecked();
  const std::string& ToStringSafe() const;
  std::string& ToStringSafe();
  bool ToObject(FlexUIValueObjectType& obj) const;
  const FlexUIValueObjectType& ToObjectChecked() const;
  FlexUIValueObjectType& ToObjectChecked();
  bool ToArray(FlexUIValueArrayType& arr) const;
  const FlexUIValueArrayType& ToArrayChecked() const;
  FlexUIValueArrayType& ToArrayChecked();

 private:
  inline void Deallocate();

  friend std::hash<FlexUIValue>;
  friend std::ostream& operator<<(std::ostream& os, const FlexUIValue& flexui_value);

  Type type_ = Type::kUndefined;
  NumberType number_type_ = NumberType::kNaN;
  union {
    bool b_{};
    FlexUIValueObjectType obj_;
    FlexUIValueArrayType arr_;
    std::string str_;
    Number num_;
  };
};

}  // namespace common
}  // namespace flexui

template <>
struct std::hash<flexui::common::FlexUIValue> {
  std::size_t operator()(const flexui::common::FlexUIValue& value) const noexcept;

 private:
  const static size_t UndefinedHashValue = 0x79476983;
  const static size_t NullHashValue = 0x7a695478;
};
