/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include "flexui/components/scrollview/scrollview_component.h"

namespace flexui::components::scrollview {

class FakeCtx : public flexui::core::commit_pipeline::ComponentContext {
 public:
  uint32_t scope_id() const override { return 1; }
  uint32_t root_id()  const override { return 1; }
};

TEST(ScrollViewComponentTest, OnCreateReturnsNonNull) {
  FakeCtx ctx;
  ScrollViewComponent c(ctx);
  auto h = c.OnCreate();
  EXPECT_NE(h, nullptr);
  c.OnUnmount();
}

TEST(ScrollViewComponentTest, OnUpdatePropsCarriesDirection) {
  FakeCtx ctx;
  ScrollViewComponent c(ctx);
  c.OnCreate();

  flexui::core::commit_pipeline::PropDelta delta;
  delta.updated.push_back({"direction", flexui::common::FlexUIValue(std::string("horizontal"))});
  delta.updated.push_back({"bounces", flexui::common::FlexUIValue(false)});
  delta.updated.push_back({"width", flexui::common::FlexUIValue(400.0)});
  delta.updated.push_back({"height", flexui::common::FlexUIValue(300.0)});
  c.OnUpdateProps(delta);

  EXPECT_EQ(c.props().direction, "horizontal");
  EXPECT_EQ(c.props().bounces, false);
  EXPECT_DOUBLE_EQ(c.props().width, 400.0);
  EXPECT_DOUBLE_EQ(c.props().height, 300.0);

  c.OnUnmount();
}

TEST(ScrollViewComponentTest, DefaultProps) {
  FakeCtx ctx;
  ScrollViewComponent c(ctx);
  EXPECT_EQ(c.props().direction, "");
  EXPECT_EQ(c.props().bounces, true);
}

}  // namespace flexui::components::scrollview
