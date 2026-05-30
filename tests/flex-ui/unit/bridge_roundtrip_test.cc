#include <gtest/gtest.h>

#include "flexui/core/bridge/encoder.h"
#include "flexui/core/bridge/decoder.h"

namespace flexui::core::bridge {

TEST(BridgeRoundtripTest, StringRoundtrips) {
  using F = flexui::common::FlexUIValue;
  F input(std::string("flexui"));
  auto frame = Encoder::EncodeValue(input);
  F out;
  auto err = Decoder::DecodeValue(frame.bytes.data(), frame.bytes.size(), &out);
  EXPECT_TRUE(err.ok());
  EXPECT_TRUE(out.IsString());
  EXPECT_EQ(out.ToStringChecked(), "flexui");
}

TEST(BridgeRoundtripTest, NestedObjectRoundtrips) {
  using F = flexui::common::FlexUIValue;
  F input(F::FlexUIValueObjectType{
      {"name", F(std::string("card"))},
      {"size", F(7.5)},
      {"open", F(true)},
  });
  auto frame = Encoder::EncodeValue(input);
  F out;
  auto err = Decoder::DecodeValue(frame.bytes.data(), frame.bytes.size(), &out);
  EXPECT_TRUE(err.ok());
  EXPECT_EQ(out.ToObjectChecked()["name"].ToStringChecked(), "card");
  EXPECT_DOUBLE_EQ(out.ToObjectChecked()["size"].ToDoubleChecked(), 7.5);
  EXPECT_TRUE(out.ToObjectChecked()["open"].ToBooleanChecked());
}

TEST(BridgeRoundtripTest, MalformedReturnsError) {
  flexui::common::FlexUIValue out;
  uint8_t junk[] = {0xff, 0x00, 0x01};
  auto err = Decoder::DecodeValue(junk, sizeof(junk), &out);
  EXPECT_FALSE(err.ok());
}

}  // namespace flexui::core::bridge
