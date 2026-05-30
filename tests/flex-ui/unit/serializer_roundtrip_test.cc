// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.

#include <gtest/gtest.h>
#include <vector>

#include "flexui/common/deserializer.h"
#include "flexui/common/flexui_value.h"
#include "flexui/common/serializer.h"

namespace flexui::common {

TEST(SerializerRoundtripTest, StringRoundtrips) {
  FlexUIValue input(std::string("hello flexui"));

  Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(input);
  auto buffer = ser.Release();
  ASSERT_NE(buffer.first, nullptr);
  ASSERT_GT(buffer.second, 0u);

  Deserializer de(buffer.first, buffer.second);
  ASSERT_TRUE(de.ReadHeader());
  FlexUIValue output;
  ASSERT_TRUE(de.ReadValue(output));
  EXPECT_TRUE(output.IsString());
  EXPECT_EQ(output.ToStringChecked(), "hello flexui");

  // Release() transfers ownership — caller must free.
  SerializerHelper::DestroyBuffer(buffer);
}

TEST(SerializerRoundtripTest, Int32Roundtrips) {
  FlexUIValue input(int32_t(-42));

  Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(input);
  auto buffer = ser.Release();
  ASSERT_NE(buffer.first, nullptr);

  Deserializer de(buffer.first, buffer.second);
  ASSERT_TRUE(de.ReadHeader());
  FlexUIValue output;
  ASSERT_TRUE(de.ReadValue(output));
  EXPECT_TRUE(output.IsNumber());
  EXPECT_EQ(output.ToInt32Checked(), -42);

  SerializerHelper::DestroyBuffer(buffer);
}

TEST(SerializerRoundtripTest, BooleanRoundtrips) {
  for (bool b : {true, false}) {
    FlexUIValue input(b);

    Serializer ser;
    ser.WriteHeader();
    ser.WriteValue(input);
    auto buffer = ser.Release();
    ASSERT_NE(buffer.first, nullptr);

    Deserializer de(buffer.first, buffer.second);
    ASSERT_TRUE(de.ReadHeader());
    FlexUIValue output;
    ASSERT_TRUE(de.ReadValue(output));
    EXPECT_TRUE(output.IsBoolean());
    EXPECT_EQ(output.ToBooleanChecked(), b);

    SerializerHelper::DestroyBuffer(buffer);
  }
}

TEST(SerializerRoundtripTest, NullRoundtrips) {
  FlexUIValue input = FlexUIValue::Null();

  Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(input);
  auto buffer = ser.Release();
  ASSERT_NE(buffer.first, nullptr);

  Deserializer de(buffer.first, buffer.second);
  ASSERT_TRUE(de.ReadHeader());
  FlexUIValue output;
  ASSERT_TRUE(de.ReadValue(output));
  EXPECT_TRUE(output.IsNull());

  SerializerHelper::DestroyBuffer(buffer);
}

}  // namespace flexui::common
