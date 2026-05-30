// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.
//
// Extended serializer/deserializer tests targeting uncovered paths (Task 19
// coverage gate).  The original serializer_roundtrip_test.cc covers string,
// int32, bool, and null.  This file covers: uint32, double, undefined, array,
// object, UInt32-value roundtrip, and Deserializer vector-constructor.

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <unordered_map>

#include "flexui/common/deserializer.h"
#include "flexui/common/flexui_value.h"
#include "flexui/common/serializer.h"

namespace flexui::common {

namespace {

// Helper: serialize a value and return the raw buffer (caller must free via
// SerializerHelper::DestroyBuffer).
std::pair<uint8_t*, size_t> Serialize(const FlexUIValue& v) {
  Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(v);
  return ser.Release();
}

// Helper: deserialize a single value from buffer.
FlexUIValue Deserialize(const uint8_t* data, size_t size) {
  Deserializer de(data, size);
  EXPECT_TRUE(de.ReadHeader());
  FlexUIValue out;
  EXPECT_TRUE(de.ReadValue(out));
  return out;
}

}  // namespace

// ── UInt32 roundtrip ──────────────────────────────────────────────────────────

TEST(SerializerExtendedTest, UInt32Roundtrips) {
  FlexUIValue input(static_cast<uint32_t>(0xDEADBEEFu));

  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);

  auto out = Deserialize(buf.first, buf.second);
  EXPECT_TRUE(out.IsUInt32());
  uint32_t v = 0;
  EXPECT_TRUE(out.ToUint32(v));
  EXPECT_EQ(v, 0xDEADBEEFu);

  SerializerHelper::DestroyBuffer(buf);
}

// ── Double roundtrip ──────────────────────────────────────────────────────────

TEST(SerializerExtendedTest, DoubleRoundtrips) {
  FlexUIValue input(3.14159);

  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);

  auto out = Deserialize(buf.first, buf.second);
  EXPECT_TRUE(out.IsDouble());
  double v = 0.0;
  EXPECT_TRUE(out.ToDouble(v));
  EXPECT_DOUBLE_EQ(v, 3.14159);

  SerializerHelper::DestroyBuffer(buf);
}

// ── Undefined roundtrip ───────────────────────────────────────────────────────

TEST(SerializerExtendedTest, UndefinedRoundtrips) {
  FlexUIValue input = FlexUIValue::Undefined();

  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);

  auto out = Deserialize(buf.first, buf.second);
  EXPECT_TRUE(out.IsUndefined());

  SerializerHelper::DestroyBuffer(buf);
}

// ── Array roundtrip ───────────────────────────────────────────────────────────

TEST(SerializerExtendedTest, ArrayRoundtrips) {
  FlexUIValue::FlexUIValueArrayType arr;
  arr.push_back(FlexUIValue(int32_t(1)));
  arr.push_back(FlexUIValue(std::string("two")));
  arr.push_back(FlexUIValue(true));
  FlexUIValue input(arr);

  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);

  auto out = Deserialize(buf.first, buf.second);
  EXPECT_TRUE(out.IsArray());
  FlexUIValue::FlexUIValueArrayType result;
  EXPECT_TRUE(out.ToArray(result));
  ASSERT_EQ(result.size(), 3u);
  EXPECT_EQ(result[0].ToInt32Checked(), 1);
  EXPECT_EQ(result[1].ToStringChecked(), "two");
  EXPECT_TRUE(result[2].ToBooleanChecked());

  SerializerHelper::DestroyBuffer(buf);
}

// ── Object roundtrip ──────────────────────────────────────────────────────────

TEST(SerializerExtendedTest, ObjectRoundtrips) {
  FlexUIValue::FlexUIValueObjectType obj;
  obj["name"] = FlexUIValue(std::string("flexui"));
  obj["count"] = FlexUIValue(int32_t(42));
  FlexUIValue input(obj);

  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);

  auto out = Deserialize(buf.first, buf.second);
  EXPECT_TRUE(out.IsObject());
  FlexUIValue::FlexUIValueObjectType result;
  EXPECT_TRUE(out.ToObject(result));
  EXPECT_EQ(result.count("name"), 1u);
  EXPECT_EQ(result.at("name").ToStringChecked(), "flexui");
  EXPECT_EQ(result.count("count"), 1u);
  EXPECT_EQ(result.at("count").ToInt32Checked(), 42);

  SerializerHelper::DestroyBuffer(buf);
}

// ── Deserializer vector constructor ──────────────────────────────────────────

TEST(SerializerExtendedTest, DeserializerFromVector) {
  FlexUIValue input(int32_t(99));

  Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(input);
  auto raw = ser.Release();

  // Copy into a vector<uint8_t>
  std::vector<uint8_t> vec(raw.first, raw.first + raw.second);
  SerializerHelper::DestroyBuffer(raw);

  Deserializer de(vec);
  ASSERT_TRUE(de.ReadHeader());
  FlexUIValue out;
  ASSERT_TRUE(de.ReadValue(out));
  EXPECT_EQ(out.ToInt32Checked(), 99);
}

// ── ReadHeaderChecked ─────────────────────────────────────────────────────────

TEST(SerializerExtendedTest, ReadHeaderCheckedDoesNotCrash) {
  FlexUIValue input(int32_t(7));
  auto buf = Serialize(input);

  Deserializer de(buf.first, buf.second);
  EXPECT_NO_THROW(de.ReadHeaderChecked());
  FlexUIValue out;
  EXPECT_TRUE(de.ReadValue(out));
  EXPECT_EQ(out.ToInt32Checked(), 7);

  SerializerHelper::DestroyBuffer(buf);
}

// ── Zero uint32 and max int32 edge cases ─────────────────────────────────────

TEST(SerializerExtendedTest, ZeroUInt32Roundtrips) {
  FlexUIValue input(static_cast<uint32_t>(0u));
  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);
  auto out = Deserialize(buf.first, buf.second);
  uint32_t v = 1;
  EXPECT_TRUE(out.ToUint32(v));
  EXPECT_EQ(v, 0u);
  SerializerHelper::DestroyBuffer(buf);
}

TEST(SerializerExtendedTest, MaxInt32Roundtrips) {
  FlexUIValue input(std::numeric_limits<int32_t>::max());
  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);
  auto out = Deserialize(buf.first, buf.second);
  EXPECT_EQ(out.ToInt32Checked(), std::numeric_limits<int32_t>::max());
  SerializerHelper::DestroyBuffer(buf);
}

TEST(SerializerExtendedTest, MinInt32Roundtrips) {
  FlexUIValue input(std::numeric_limits<int32_t>::min());
  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);
  auto out = Deserialize(buf.first, buf.second);
  EXPECT_EQ(out.ToInt32Checked(), std::numeric_limits<int32_t>::min());
  SerializerHelper::DestroyBuffer(buf);
}

// ── Empty string roundtrip ────────────────────────────────────────────────────

TEST(SerializerExtendedTest, EmptyStringRoundtrips) {
  FlexUIValue input(std::string(""));
  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);
  auto out = Deserialize(buf.first, buf.second);
  EXPECT_TRUE(out.IsString());
  EXPECT_EQ(out.ToStringChecked(), "");
  SerializerHelper::DestroyBuffer(buf);
}

// ── Nested array ─────────────────────────────────────────────────────────────

TEST(SerializerExtendedTest, NestedArrayRoundtrips) {
  FlexUIValue::FlexUIValueArrayType inner;
  inner.push_back(FlexUIValue(int32_t(1)));
  inner.push_back(FlexUIValue(int32_t(2)));

  FlexUIValue::FlexUIValueArrayType outer;
  outer.push_back(FlexUIValue(inner));
  outer.push_back(FlexUIValue(std::string("end")));
  FlexUIValue input(outer);

  auto buf = Serialize(input);
  ASSERT_NE(buf.first, nullptr);

  auto out = Deserialize(buf.first, buf.second);
  EXPECT_TRUE(out.IsArray());
  FlexUIValue::FlexUIValueArrayType result;
  EXPECT_TRUE(out.ToArray(result));
  ASSERT_EQ(result.size(), 2u);
  EXPECT_TRUE(result[0].IsArray());
  EXPECT_EQ(result[1].ToStringChecked(), "end");

  SerializerHelper::DestroyBuffer(buf);
}

}  // namespace flexui::common
