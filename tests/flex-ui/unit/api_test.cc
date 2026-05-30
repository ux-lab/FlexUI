/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include <vector>

#include "api/console/console_api.h"
#include "api/timer/timer_api.h"
#include "api/log/log_api.h"
#include "flexui/api/api_base_package.h"

namespace flexui::api {

TEST(ConsoleApiTest, NameIsConsoleLog) {
  auto api = MakeConsoleApi();
  EXPECT_EQ(api.name, "console.log");
  EXPECT_EQ(api.mode, flexui::core::plugin_host::NativeApiMode::kSync);
}

TEST(ConsoleApiTest, InvokeWithArgs) {
  auto api = MakeConsoleApi();
  std::vector<flexui::common::FlexUIValue> args;
  args.emplace_back(std::string("hello"));
  args.emplace_back(42.0);
  auto result = api.invoke(args);
  // Returns void FlexUIValue; no crash = pass.
  (void)result;
}

TEST(ConsoleApiTest, InvokeWithNoArgs) {
  auto api = MakeConsoleApi();
  auto result = api.invoke({});
  (void)result;
  // No crash = pass.
}

TEST(TimerApiTest, NameIsSetTimeout) {
  auto api = MakeTimerApi();
  EXPECT_EQ(api.name, "setTimeout");
}

TEST(TimerApiTest, InvokeReturnsZero) {
  auto api = MakeTimerApi();
  auto result = api.invoke({});
  EXPECT_TRUE(result.IsNumber());
  EXPECT_DOUBLE_EQ(result.ToDoubleChecked(), 0.0);
}

TEST(LogApiTest, NameIsFlexLog) {
  auto api = MakeLogApi();
  EXPECT_EQ(api.name, "flexLog");
}

TEST(LogApiTest, InvokeWithStringArg) {
  auto api = MakeLogApi();
  std::vector<flexui::common::FlexUIValue> args;
  args.emplace_back(std::string("test message"));
  auto result = api.invoke(args);
  (void)result;
  // No crash = pass.
}

TEST(ApiBasePackageTest, HasThreeApis) {
  auto pkg = MakeApiBasePackage();
  EXPECT_EQ(pkg.apis.size(), 3u);
  EXPECT_EQ(pkg.name, "flexui-api-base");
}

}  // namespace flexui::api
