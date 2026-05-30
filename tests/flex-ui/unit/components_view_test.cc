/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include "flexui/components/view/view_component.h"

namespace flexui::components::view {

class FakeCtx : public flexui::core::commit_pipeline::ComponentContext {
 public:
  uint32_t scope_id() const override { return 1; }
  uint32_t root_id()  const override { return 1; }
};

TEST(ViewComponentTest, OnCreateReturnsNonNull) {
  FakeCtx ctx;
  ViewComponent c(ctx);
  auto h = c.OnCreate();
  EXPECT_NE(h, nullptr);
  c.OnUnmount();
}

TEST(ViewComponentTest, OnUpdatePropsCarriesBackgroundColor) {
  FakeCtx ctx;
  ViewComponent c(ctx);
  c.OnCreate();

  flexui::core::commit_pipeline::PropDelta delta;
  delta.updated.push_back({"backgroundColor", flexui::common::FlexUIValue(std::string("#FF0000"))});
  delta.updated.push_back({"borderRadius", flexui::common::FlexUIValue(8.0)});
  delta.updated.push_back({"padding", flexui::common::FlexUIValue(12.0)});
  delta.updated.push_back({"flexDirection", flexui::common::FlexUIValue(std::string("column"))});
  delta.updated.push_back({"width", flexui::common::FlexUIValue(300.0)});
  delta.updated.push_back({"height", flexui::common::FlexUIValue(200.0)});
  c.OnUpdateProps(delta);

  EXPECT_EQ(c.props().background_color, "#FF0000");
  EXPECT_DOUBLE_EQ(c.props().border_radius, 8.0);
  EXPECT_DOUBLE_EQ(c.props().padding, 12.0);
  EXPECT_EQ(c.props().flex_direction, "column");
  EXPECT_DOUBLE_EQ(c.props().width, 300.0);
  EXPECT_DOUBLE_EQ(c.props().height, 200.0);

  c.OnUnmount();
}

TEST(ViewComponentTest, DefaultProps) {
  FakeCtx ctx;
  ViewComponent c(ctx);
  EXPECT_EQ(c.props().background_color, "");
  EXPECT_DOUBLE_EQ(c.props().border_radius, 0.0);
  EXPECT_DOUBLE_EQ(c.props().width, 0.0);
}

}  // namespace flexui::components::view
