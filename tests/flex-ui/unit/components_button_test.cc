/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include "flexui/components/button/button_component.h"

namespace flexui::components::button {

class FakeCtx : public flexui::core::commit_pipeline::ComponentContext {
 public:
  uint32_t scope_id() const override { return 1; }
  uint32_t root_id()  const override { return 1; }
};

TEST(ButtonComponentTest, OnCreateReturnsNonNull) {
  FakeCtx ctx;
  ButtonComponent c(ctx);
  auto h = c.OnCreate();
  EXPECT_NE(h, nullptr);
  c.OnUnmount();
}

TEST(ButtonComponentTest, OnUpdatePropsCarriesTitle) {
  FakeCtx ctx;
  ButtonComponent c(ctx);
  c.OnCreate();

  flexui::core::commit_pipeline::PropDelta delta;
  delta.updated.push_back({"title", flexui::common::FlexUIValue(std::string("Click Me"))});
  delta.updated.push_back({"onClick", flexui::common::FlexUIValue(std::string("handleClick"))});
  delta.updated.push_back({"backgroundColor", flexui::common::FlexUIValue(std::string("#00FF00"))});
  c.OnUpdateProps(delta);

  EXPECT_EQ(c.props().title, "Click Me");
  EXPECT_EQ(c.props().on_click, "handleClick");
  EXPECT_EQ(c.props().background_color, "#00FF00");

  c.OnUnmount();
}

TEST(ButtonComponentTest, OnEventLogsAndReturns) {
  FakeCtx ctx;
  ButtonComponent c(ctx);
  c.OnCreate();

  flexui::common::FlexUIValue payload;
  c.OnEvent("click", payload);
  // No crash = pass.

  c.OnUnmount();
}

TEST(ButtonComponentTest, DefaultProps) {
  FakeCtx ctx;
  ButtonComponent c(ctx);
  EXPECT_EQ(c.props().title, "");
  EXPECT_EQ(c.props().on_click, "");
}

}  // namespace flexui::components::button
