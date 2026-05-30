/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include "flexui/components/image/image_component.h"

namespace flexui::components::image {

class FakeCtx : public flexui::core::commit_pipeline::ComponentContext {
 public:
  uint32_t scope_id() const override { return 1; }
  uint32_t root_id()  const override { return 1; }
};

TEST(ImageComponentTest, OnCreateReturnsNonNull) {
  FakeCtx ctx;
  ImageComponent c(ctx);
  auto h = c.OnCreate();
  EXPECT_NE(h, nullptr);
  c.OnUnmount();
}

TEST(ImageComponentTest, OnUpdatePropsCarriesSrcAndSize) {
  FakeCtx ctx;
  ImageComponent c(ctx);
  c.OnCreate();

  flexui::core::commit_pipeline::PropDelta delta;
  delta.updated.push_back({"src", flexui::common::FlexUIValue(std::string("https://example.com/img.png"))});
  delta.updated.push_back({"width", flexui::common::FlexUIValue(100.0)});
  delta.updated.push_back({"height", flexui::common::FlexUIValue(80.0)});
  delta.updated.push_back({"objectFit", flexui::common::FlexUIValue(std::string("cover"))});
  c.OnUpdateProps(delta);

  EXPECT_EQ(c.props().src, "https://example.com/img.png");
  EXPECT_DOUBLE_EQ(c.props().width, 100.0);
  EXPECT_DOUBLE_EQ(c.props().height, 80.0);
  EXPECT_EQ(c.props().object_fit, "cover");

  c.OnUnmount();
}

TEST(ImageComponentTest, DefaultProps) {
  FakeCtx ctx;
  ImageComponent c(ctx);
  EXPECT_EQ(c.props().src, "");
  EXPECT_DOUBLE_EQ(c.props().width, 0.0);
}

}  // namespace flexui::components::image
