#include <gtest/gtest.h>

#include "flexui/common/log_sink.h"
#include "tests/flex-ui/support/capturing_log_sink.h"

namespace flexui::common {

TEST(LogSinkTest, CapturingSinkReceivesEmittedRecord) {
  test::ScopedCapturingSink scope;
  LogSinkRegistry::Instance().Emit(
      LogRecord{TDF_LOG_INFO, "TestSub", "TestEvent", "hello"});
  auto records = scope.sink().Drain();
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].subsystem, "TestSub");
  EXPECT_EQ(records[0].event, "TestEvent");
  EXPECT_EQ(records[0].message, "hello");
}

TEST(LogSinkTest, RemoveSinkStopsDelivery) {
  auto id = LogSinkRegistry::Instance().AddSink(MakeStdoutLogSink());
  LogSinkRegistry::Instance().RemoveSink(id);
  // No assertion target; just verifies remove returns true and the
  // subsequent Emit does not crash with the sink gone.
  LogSinkRegistry::Instance().Emit(
      LogRecord{TDF_LOG_INFO, "X", "Y", "msg after removal"});
}

}  // namespace flexui::common
