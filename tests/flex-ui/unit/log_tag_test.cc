#include <gtest/gtest.h>

#include "flexui/common/log_tag.h"
#include "tests/flex-ui/support/capturing_log_sink.h"

namespace flexui::common {

TEST(LogTagTest, EmitsRecordWithSubsystemAndEvent) {
  test::ScopedCapturingSink scope;
  FLEXUI_TLOG(Engine, Init, INFO) << "starting flexui";
  auto records = scope.sink().Drain();
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].subsystem, "Engine");
  EXPECT_EQ(records[0].event, "Init");
  EXPECT_EQ(records[0].level, TDF_LOG_INFO);
  EXPECT_EQ(records[0].message, "starting flexui");
}

TEST(LogTagTest, MultipleSinksReceiveSameRecord) {
  test::ScopedCapturingSink a, b;
  FLEXUI_TLOG(Bridge, Call, DEBUG) << "x=42";
  auto ra = a.sink().Drain();
  auto rb = b.sink().Drain();
  ASSERT_EQ(ra.size(), 1u);
  ASSERT_EQ(rb.size(), 1u);
  EXPECT_EQ(ra[0].subsystem, "Bridge");
  EXPECT_EQ(rb[0].subsystem, "Bridge");
}

}  // namespace flexui::common
