// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.

#include <gtest/gtest.h>
#include <sstream>

#include "flexui/common/error.h"

namespace flexui::common {

TEST(ErrorTest, OkConstructionIsNotErrorish) {
  Error e = Error::Ok();
  EXPECT_TRUE(e.ok());
  EXPECT_FALSE(static_cast<bool>(e));
}

TEST(ErrorTest, NonOkErrorishness) {
  Error e(ErrorCode::kPluginConflict, "duplicate plugin name");
  EXPECT_FALSE(e.ok());
  EXPECT_TRUE(static_cast<bool>(e));
  EXPECT_EQ(e.code(), ErrorCode::kPluginConflict);
}

TEST(ErrorTest, FormatsCodeAndMessage) {
  Error e(ErrorCode::kJsException, "ReferenceError: foo");
  std::ostringstream oss;
  oss << e;
  EXPECT_EQ(oss.str(), "Error(JsException: ReferenceError: foo)");
}

}  // namespace flexui::common
