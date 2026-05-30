// Earliest gate: prove the flex-ui/common static library links and we can
// instantiate a TU that references no flex-ui symbols yet. Replaced/expanded
// by later tasks.

#include <gtest/gtest.h>

TEST(FlexUICommonSmoke, LinksAndRuns) {
  EXPECT_EQ(1 + 1, 2);
}
