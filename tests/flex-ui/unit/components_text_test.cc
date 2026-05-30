/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include "flexui/components/text/text_component.h"

namespace flexui::components::text {

class FakeCtx : public flexui::core::commit_pipeline::ComponentContext {
 public:
  uint32_t scope_id() const override { return 1; }
  uint32_t root_id()  const override { return 1; }
};

TEST(TextComponentTest, OnCreateReturnsNonNull) {
  FakeCtx ctx;
  TextComponent c(ctx);
  auto h = c.OnCreate();
  // On macOS, CreateNode returns &kStub (non-null).
  EXPECT_NE(h, nullptr);
  c.OnUnmount();
}

TEST(TextComponentTest, OnUpdatePropsCarriesText) {
  FakeCtx ctx;
  TextComponent c(ctx);
  c.OnCreate();

  flexui::core::commit_pipeline::PropDelta delta;
  delta.updated.push_back({"text", flexui::common::FlexUIValue(std::string("hello"))});
  delta.updated.push_back({"fontSize", flexui::common::FlexUIValue(18.0)});
  delta.updated.push_back({"fontWeight", flexui::common::FlexUIValue(std::string("bold"))});
  delta.updated.push_back({"textAlign", flexui::common::FlexUIValue(std::string("center"))});
  delta.updated.push_back({"maxLines", flexui::common::FlexUIValue(3.0)});
  c.OnUpdateProps(delta);

  EXPECT_EQ(c.props().text, "hello");
  EXPECT_DOUBLE_EQ(c.props().font_size, 18.0);
  EXPECT_EQ(c.props().font_weight, "bold");
  EXPECT_EQ(c.props().text_align, "center");
  EXPECT_EQ(c.props().max_lines, 3);

  c.OnUnmount();
}

TEST(TextComponentTest, OnUpdatePropsHandlesIntAsDouble) {
  FakeCtx ctx;
  TextComponent c(ctx);
  c.OnCreate();

  flexui::core::commit_pipeline::PropDelta delta;
  delta.updated.push_back({"fontSize", flexui::common::FlexUIValue(24.0)});
  c.OnUpdateProps(delta);

  EXPECT_DOUBLE_EQ(c.props().font_size, 24.0);

  c.OnUnmount();
}

TEST(TextComponentTest, OnUpdateLayoutRoundTrip) {
  FakeCtx ctx;
  TextComponent c(ctx);
  c.OnCreate();

  flexui::core::commit_pipeline::LayoutRect rect{10.0f, 20.0f, 100.0f, 50.0f};
  c.OnUpdateLayout(rect);
  // Layout is forwarded to platform; no crash = pass.

  c.OnUnmount();
}

TEST(TextComponentTest, OnMountAndUnmount) {
  FakeCtx ctx;
  TextComponent c(ctx);
  c.OnCreate();
  c.OnMount(nullptr, 0);
  c.OnUnmount();
  // No crash = pass.
}

TEST(TextComponentTest, DefaultProps) {
  FakeCtx ctx;
  TextComponent c(ctx);
  EXPECT_EQ(c.props().text, "");
  EXPECT_DOUBLE_EQ(c.props().font_size, 14.0);
  EXPECT_EQ(c.props().max_lines, 0);
}

}  // namespace flexui::components::text
