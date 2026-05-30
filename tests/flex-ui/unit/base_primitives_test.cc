#include <gtest/gtest.h>

#include "flexui/common/macros.h"
#include "flexui/common/check.h"
#include "flexui/common/log_level.h"
#include "flexui/common/time_delta.h"
#include "flexui/common/time_point.h"
#include "flexui/common/base_time.h"

namespace flexui::common {

TEST(BasePrimitivesTest, NamespaceResolves) {
  // Pure compile-time test: if any of the included headers still declares
  // `namespace footstone {`, this TU won't compile because the type lookup
  // inside `flexui::common` would fail.
  TimeDelta d = TimeDelta::FromMilliseconds(5);
  EXPECT_EQ(d.ToMilliseconds(), 5);
}

TEST(BasePrimitivesTest, TimePointAddDelta) {
  TimePoint t = TimePoint::Now();
  TimePoint t2 = t + TimeDelta::FromMilliseconds(10);
  EXPECT_GT(t2.ToEpochDelta().ToMilliseconds(), t.ToEpochDelta().ToMilliseconds());
}

}  // namespace flexui::common
