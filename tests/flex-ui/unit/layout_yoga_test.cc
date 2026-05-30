/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Unit tests for YogaLayoutNode (absorbed from Hippy).
 * API is the actual absorbed API: SetLayoutStyles + CalculateLayout + GetLeft/GetTop/GetWidth/GetHeight.
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>

#include "flexui/core/layout/yoga_layout_node.h"
#include "flexui/common/flexui_value.h"

namespace flexui::core::layout {

using FlexUIValue = flexui::common::FlexUIValue;
using StyleMap = std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>;

// Helper: build a StyleMap from key/double pairs.
static StyleMap MakeStyle(
    std::initializer_list<std::pair<std::string, double>> entries) {
  StyleMap m;
  for (auto& kv : entries) {
    m[kv.first] = std::make_shared<FlexUIValue>(kv.second);
  }
  return m;
}

TEST(YogaLayoutTest, SingleNodeCalculatesSize) {
  auto node = std::make_shared<YogaLayoutNode>();
  auto style = MakeStyle({{"width", 100.0}, {"height", 50.0}});
  node->SetLayoutStyles(style, {});
  node->CalculateLayout(200.0f, 200.0f);
  EXPECT_FLOAT_EQ(node->GetWidth(), 100.0f);
  EXPECT_FLOAT_EQ(node->GetHeight(), 50.0f);
}

TEST(YogaLayoutTest, FlexDirectionColumnStacksChildren) {
  auto root = std::make_shared<YogaLayoutNode>();
  auto root_style = MakeStyle({{"width", 100.0}, {"height", 100.0}});
  // flexDirection column is the default; set width/height only.
  root->SetLayoutStyles(root_style, {});

  auto c1 = std::make_shared<YogaLayoutNode>();
  auto c1_style = MakeStyle({{"width", 50.0}, {"height", 30.0}});
  c1->SetLayoutStyles(c1_style, {});

  auto c2 = std::make_shared<YogaLayoutNode>();
  auto c2_style = MakeStyle({{"width", 50.0}, {"height", 40.0}});
  c2->SetLayoutStyles(c2_style, {});

  root->InsertChild(c1, 0);
  root->InsertChild(c2, 1);
  root->CalculateLayout(100.0f, 100.0f);

  EXPECT_FLOAT_EQ(c1->GetTop(), 0.0f);
  EXPECT_FLOAT_EQ(c2->GetTop(), 30.0f);
}

TEST(YogaLayoutTest, HasNewLayoutSetAfterCalculate) {
  auto node = std::make_shared<YogaLayoutNode>();
  auto style = MakeStyle({{"width", 80.0}, {"height", 40.0}});
  node->SetLayoutStyles(style, {});
  node->CalculateLayout(200.0f, 200.0f);
  EXPECT_TRUE(node->HasNewLayout());
}

}  // namespace flexui::core::layout
