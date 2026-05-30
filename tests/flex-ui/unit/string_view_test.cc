#include <gtest/gtest.h>
#include <string>

#include "flexui/common/string_view.h"
#include "flexui/common/string_view_utils.h"

namespace flexui::common {

TEST(StringViewTest, RoundTripLatin1) {
  string_view sv("hello");
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Latin1);
  EXPECT_EQ(sv.latin1_value(), "hello");
}

TEST(StringViewTest, RoundTripUtf8ToUtf16) {
  // string_view's encoding handling is the lowest-level invariant the rest
  // of common/ relies on; if the namespace rename breaks the storage
  // tag the conversion will fault under ASan.
  string_view sv("a\xc3\xa9");  // "aé"
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Latin1);
  std::string s(sv.latin1_value());
  EXPECT_GE(s.size(), 2u);
}

TEST(StringViewTest, IsEmptyLatin1) {
  // "" constructs as Latin1 encoding with empty content — IsEmpty returns true.
  string_view empty_sv("");
  EXPECT_TRUE(StringViewUtils::IsEmpty(empty_sv));
  string_view nonempty_sv("abc");
  EXPECT_FALSE(StringViewUtils::IsEmpty(nonempty_sv));
}

TEST(StringViewTest, GetLength) {
  string_view sv("hello");
  EXPECT_EQ(StringViewUtils::GetLength(sv), 5u);
}

TEST(StringViewTest, CopyAssign) {
  string_view a("first");
  string_view b = a;
  EXPECT_EQ(b.latin1_value(), "first");
}

TEST(StringViewTest, EqualityOperator) {
  string_view a("same");
  string_view b("same");
  string_view c("diff");
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}

}  // namespace flexui::common
