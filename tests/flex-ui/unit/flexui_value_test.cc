/*
 * Copyright (C) 2024 The FlexUI Authors.
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

#include "flexui/common/flexui_value.h"

#include <sstream>
#include <string>
#include <unordered_map>

#include "gtest/gtest.h"

using flexui::common::FlexUIValue;

// ---------------------------------------------------------------------------
// Static factories
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, UndefinedFactory) {
  auto v = FlexUIValue::Undefined();
  EXPECT_TRUE(v.IsUndefined());
  EXPECT_EQ(v.GetType(), FlexUIValue::Type::kUndefined);
}

TEST(FlexUIValueTest, NullFactory) {
  auto v = FlexUIValue::Null();
  EXPECT_TRUE(v.IsNull());
  EXPECT_EQ(v.GetType(), FlexUIValue::Type::kNull);
}

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, Int32Constructor) {
  FlexUIValue v(static_cast<int32_t>(-42));
  EXPECT_TRUE(v.IsInt32());
  EXPECT_TRUE(v.IsNumber());
  int32_t out = 0;
  EXPECT_TRUE(v.ToInt32(out));
  EXPECT_EQ(out, -42);
}

TEST(FlexUIValueTest, UInt32Constructor) {
  FlexUIValue v(static_cast<uint32_t>(100u));
  EXPECT_TRUE(v.IsUInt32());
  uint32_t out = 0;
  EXPECT_TRUE(v.ToUint32(out));
  EXPECT_EQ(out, 100u);
}

TEST(FlexUIValueTest, DoubleConstructor) {
  FlexUIValue v(3.14);
  EXPECT_TRUE(v.IsDouble());
  double out = 0.0;
  EXPECT_TRUE(v.ToDouble(out));
  EXPECT_DOUBLE_EQ(out, 3.14);
}

TEST(FlexUIValueTest, BoolConstructor) {
  FlexUIValue v(true);
  EXPECT_TRUE(v.IsBoolean());
  bool out = false;
  EXPECT_TRUE(v.ToBoolean(out));
  EXPECT_TRUE(out);
}

TEST(FlexUIValueTest, StringConstructor) {
  FlexUIValue v(std::string("hello"));
  EXPECT_TRUE(v.IsString());
  std::string out;
  EXPECT_TRUE(v.ToString(out));
  EXPECT_EQ(out, "hello");
}

TEST(FlexUIValueTest, CStringConstructor) {
  FlexUIValue v("world");
  EXPECT_TRUE(v.IsString());
  EXPECT_EQ(v.ToStringChecked(), "world");
}

TEST(FlexUIValueTest, ObjectConstructor) {
  FlexUIValue::FlexUIValueObjectType obj;
  obj["key"] = FlexUIValue(static_cast<int32_t>(1));
  FlexUIValue v(obj);
  EXPECT_TRUE(v.IsObject());
  FlexUIValue::FlexUIValueObjectType out;
  EXPECT_TRUE(v.ToObject(out));
  EXPECT_EQ(out.size(), 1u);
}

TEST(FlexUIValueTest, ArrayConstructor) {
  FlexUIValue::FlexUIValueArrayType arr;
  arr.emplace_back(static_cast<int32_t>(1));
  arr.emplace_back(static_cast<int32_t>(2));
  FlexUIValue v(arr);
  EXPECT_TRUE(v.IsArray());
  FlexUIValue::FlexUIValueArrayType out;
  EXPECT_TRUE(v.ToArray(out));
  EXPECT_EQ(out.size(), 2u);
}

// ---------------------------------------------------------------------------
// Copy constructor
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, CopyConstructor) {
  FlexUIValue orig(std::string("copy me"));
  FlexUIValue copy(orig);
  EXPECT_TRUE(copy.IsString());
  EXPECT_EQ(copy.ToStringChecked(), "copy me");
}

// ---------------------------------------------------------------------------
// Assignment operators
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, AssignInt32) {
  FlexUIValue v;
  v = static_cast<int32_t>(7);
  EXPECT_TRUE(v.IsInt32());
  EXPECT_EQ(v.ToInt32Checked(), 7);
}

TEST(FlexUIValueTest, AssignDouble) {
  FlexUIValue v;
  v = 2.71828;
  EXPECT_TRUE(v.IsDouble());
  EXPECT_DOUBLE_EQ(v.ToDoubleChecked(), 2.71828);
}

TEST(FlexUIValueTest, AssignBool) {
  FlexUIValue v;
  v = false;
  EXPECT_TRUE(v.IsBoolean());
  EXPECT_FALSE(v.ToBooleanChecked());
}

TEST(FlexUIValueTest, AssignString) {
  FlexUIValue v;
  v = std::string("assigned");
  EXPECT_TRUE(v.IsString());
  EXPECT_EQ(v.ToStringChecked(), "assigned");
}

TEST(FlexUIValueTest, AssignCString) {
  FlexUIValue v;
  v = "c_str";
  EXPECT_TRUE(v.IsString());
  EXPECT_EQ(v.ToStringChecked(), "c_str");
}

// ---------------------------------------------------------------------------
// Equality / comparison operators
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, EqualityUndefined) {
  EXPECT_EQ(FlexUIValue::Undefined(), FlexUIValue::Undefined());
}

TEST(FlexUIValueTest, EqualityNull) {
  EXPECT_EQ(FlexUIValue::Null(), FlexUIValue::Null());
}

TEST(FlexUIValueTest, EqualityInt32) {
  EXPECT_EQ(FlexUIValue(static_cast<int32_t>(5)), FlexUIValue(static_cast<int32_t>(5)));
  EXPECT_NE(FlexUIValue(static_cast<int32_t>(5)), FlexUIValue(static_cast<int32_t>(6)));
}

TEST(FlexUIValueTest, EqualityString) {
  EXPECT_EQ(FlexUIValue(std::string("a")), FlexUIValue(std::string("a")));
  EXPECT_NE(FlexUIValue(std::string("a")), FlexUIValue(std::string("b")));
}

// ---------------------------------------------------------------------------
// ToStringSafe (non-crashing fallback)
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, ToStringSafe_ReturnsEmptyForNonString) {
  FlexUIValue v(static_cast<int32_t>(1));
  EXPECT_EQ(v.ToStringSafe(), "");
}

TEST(FlexUIValueTest, ToStringSafe_ReturnsStringForString) {
  FlexUIValue v(std::string("safe"));
  EXPECT_EQ(v.ToStringSafe(), "safe");
}

// ---------------------------------------------------------------------------
// ToDouble widens int/uint
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, ToDoubleWidensInt32) {
  FlexUIValue v(static_cast<int32_t>(-3));
  double d = 0.0;
  EXPECT_TRUE(v.ToDouble(d));
  EXPECT_DOUBLE_EQ(d, -3.0);
}

// ---------------------------------------------------------------------------
// std::hash specialization
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, HashUndefined) {
  std::hash<FlexUIValue> h;
  EXPECT_NE(h(FlexUIValue::Undefined()), h(FlexUIValue::Null()));
}

TEST(FlexUIValueTest, HashConsistency) {
  std::hash<FlexUIValue> h;
  FlexUIValue v1(std::string("hello"));
  FlexUIValue v2(std::string("hello"));
  EXPECT_EQ(h(v1), h(v2));
}

// ---------------------------------------------------------------------------
// Stream operator
// ---------------------------------------------------------------------------

TEST(FlexUIValueTest, StreamUndefined) {
  std::ostringstream oss;
  oss << FlexUIValue::Undefined();
  EXPECT_EQ(oss.str(), "undefined");
}

TEST(FlexUIValueTest, StreamNull) {
  std::ostringstream oss;
  oss << FlexUIValue::Null();
  EXPECT_EQ(oss.str(), "null");
}

TEST(FlexUIValueTest, StreamString) {
  std::ostringstream oss;
  oss << FlexUIValue(std::string("hi"));
  EXPECT_EQ(oss.str(), "\"hi\"");
}
