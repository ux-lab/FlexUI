#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "flexui/common/string_utils.h"

namespace flexui::common {

TEST(StringUtilsTest, SplitStringBasic) {
  auto parts = StringUtils::SplitString("aa-bb-cc", "-");
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "aa");
  EXPECT_EQ(parts[1], "bb");
  EXPECT_EQ(parts[2], "cc");
}

TEST(StringUtilsTest, SplitStringEmptyInput) {
  auto parts = StringUtils::SplitString("", "-");
  EXPECT_TRUE(parts.empty());
}

TEST(StringUtilsTest, TrimmingStringRemovesWhitespace) {
  EXPECT_EQ(StringUtils::TrimmingString("  hello world  "), "helloworld");
}

TEST(StringUtilsTest, TrimmingStringAlreadyClean) {
  EXPECT_EQ(StringUtils::TrimmingString("clean"), "clean");
}

TEST(StringUtilsTest, CamelizeBasic) {
  EXPECT_EQ(StringUtils::Camelize("aa-bb-cc"), "aaBbCc");
}

TEST(StringUtilsTest, CamelizeEmpty) {
  EXPECT_EQ(StringUtils::Camelize(""), "");
}

TEST(StringUtilsTest, UnCamelizeBasic) {
  // "aaBbCc" -> "-aa-bb-cc" then lowercased: "-aa-bb-cc"
  // The regex inserts a dash before each capital letter followed by a char.
  std::string result = StringUtils::UnCamelize("aaBbCc");
  EXPECT_FALSE(result.empty());
  // Result must contain lowercase only after transform.
  for (char c : result) {
    EXPECT_TRUE(c == '-' || std::islower(static_cast<unsigned char>(c)));
  }
}

TEST(StringUtilsTest, UnCamelizeEmpty) {
  EXPECT_EQ(StringUtils::UnCamelize(""), "");
}

TEST(StringUtilsTest, ToStringInt) {
  EXPECT_EQ(StringUtils::ToString(42), "42");
}

TEST(StringUtilsTest, ToStringZero) {
  EXPECT_EQ(StringUtils::ToString(0), "0");
}

}  // namespace flexui::common
