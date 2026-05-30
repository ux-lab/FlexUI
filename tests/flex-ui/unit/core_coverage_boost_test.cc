/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Coverage-boost tests for flex-ui/core subsystems (Task 14).
 * Targets: dom_node.cc, yoga_layout_node.cc, layout_node.cc, diff_utils.cc.
 */
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "flexui/common/flexui_value.h"
#include "flexui/core/layout/layout_node.h"
#include "flexui/core/layout/yoga_layout_node.h"
#include "flexui/core/reconciler/diff_utils.h"
#include "flexui/core/vdom/dom_node.h"

namespace flexui_core_coverage {

using FlexUIValue = flexui::common::FlexUIValue;
using StyleMap = std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>;
using DomValueMap = flexui::core::reconciler::DomValueMap;

static StyleMap MakeStyleStr(
    std::initializer_list<std::pair<std::string, std::string>> entries) {
  StyleMap m;
  for (auto& kv : entries) {
    m[kv.first] = std::make_shared<FlexUIValue>(kv.second);
  }
  return m;
}

static StyleMap MakeStyleNum(
    std::initializer_list<std::pair<std::string, double>> entries) {
  StyleMap m;
  for (auto& kv : entries) {
    m[kv.first] = std::make_shared<FlexUIValue>(kv.second);
  }
  return m;
}

// ─────────────────────────── layout_node.cc ──────────────────────────────────

TEST(LayoutNodeCov, CreateLayoutNodeYogaType) {
  auto node = flexui::core::layout::CreateLayoutNode(
      flexui::core::layout::LayoutEngineYoga, nullptr);
  EXPECT_NE(node, nullptr);
}

TEST(LayoutNodeCov, InitLayoutConsts) {
  // Just verify it doesn't crash.
  flexui::core::layout::InitLayoutConsts(
      flexui::core::layout::LayoutEngineYoga);
}

TEST(LayoutNodeCov, CreateAndDestroyLayoutConfig) {
  auto* cfg = flexui::core::layout::CreateLayoutConfig(
      flexui::core::layout::LayoutEngineYoga);
  EXPECT_EQ(cfg, nullptr);  // Yoga returns nullptr
  flexui::core::layout::DestroyLayoutConfig(
      flexui::core::layout::LayoutEngineYoga, cfg);
}

// ─────────────────────────── yoga_layout_node.cc ─────────────────────────────

TEST(YogaLayoutNodeCov, LtrRtlInheritDirections) {
  using flexui::core::layout::Direction;
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->SetLayoutStyles(MakeStyleNum({{"width", 100}, {"height", 100}}), {});
  // LTR
  node->CalculateLayout(200.f, 200.f, Direction::LTR);
  EXPECT_FLOAT_EQ(node->GetWidth(), 100.f);
  // RTL
  node->CalculateLayout(200.f, 200.f, Direction::RTL);
  EXPECT_FLOAT_EQ(node->GetWidth(), 100.f);
  // Inherit
  node->CalculateLayout(200.f, 200.f, Direction::Inherit);
  EXPECT_FLOAT_EQ(node->GetWidth(), 100.f);
}

TEST(YogaLayoutNodeCov, SetGetEdgeMethods) {
  using flexui::core::layout::Edge;
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  auto style = MakeStyleNum(
      {{"width", 100}, {"height", 100}, {"margin", 5}, {"padding", 3}});
  node->SetLayoutStyles(style, {});
  node->CalculateLayout(200.f, 200.f);
  // GetMargin, GetPadding, GetBorder (should not crash)
  (void)node->GetMargin(Edge::EdgeLeft);
  (void)node->GetMargin(Edge::EdgeTop);
  (void)node->GetMargin(Edge::EdgeRight);
  (void)node->GetMargin(Edge::EdgeBottom);
  (void)node->GetPadding(Edge::EdgeLeft);
  (void)node->GetBorder(Edge::EdgeTop);
}

TEST(YogaLayoutNodeCov, GetStyleWidthHeight) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->SetLayoutStyles(MakeStyleNum({{"width", 44}, {"height", 22}}), {});
  EXPECT_FLOAT_EQ(node->GetStyleWidth(), 44.f);
  EXPECT_FLOAT_EQ(node->GetStyleHeight(), 22.f);
}

TEST(YogaLayoutNodeCov, SetPosition) {
  using flexui::core::layout::Edge;
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->SetPosition(Edge::EdgeLeft, 10.f);
  node->SetPosition(Edge::EdgeTop, 5.f);
  // No crash expected
}

TEST(YogaLayoutNodeCov, SetWidthHeightMaxMin) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->SetWidth(50.f);
  node->SetHeight(80.f);
  node->SetMaxWidth(200.f);
  node->SetMaxHeight(300.f);
  node->CalculateLayout(200.f, 300.f);
  EXPECT_FLOAT_EQ(node->GetWidth(), 50.f);
  EXPECT_FLOAT_EQ(node->GetHeight(), 80.f);
}

TEST(YogaLayoutNodeCov, ScaleFactor) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->SetScaleFactor(2.0f);
  // No crash
}

TEST(YogaLayoutNodeCov, MeasureFunction) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  EXPECT_FALSE(node->HasMeasureFunction());
  node->SetMeasureFunction([](float, flexui::core::layout::LayoutMeasureMode,
                               float, flexui::core::layout::LayoutMeasureMode,
                               void*) {
    flexui::core::layout::LayoutSize s;
    s.width = 30.f;
    s.height = 20.f;
    return s;
  });
  EXPECT_TRUE(node->HasMeasureFunction());
}

TEST(YogaLayoutNodeCov, MarkDirtyIsDirtyReset) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->SetLayoutStyles(MakeStyleNum({{"width", 10}, {"height", 10}}), {});
  node->CalculateLayout(100.f, 100.f);
  node->SetHasNewLayout(false);
  EXPECT_FALSE(node->HasNewLayout());
  // Only leaf nodes with measure functions can be marked dirty.
  auto leaf = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  leaf->SetMeasureFunction([](float, flexui::core::layout::LayoutMeasureMode,
                               float, flexui::core::layout::LayoutMeasureMode,
                               void*) {
    return flexui::core::layout::LayoutSize{10.f, 10.f};
  });
  leaf->MarkDirty();
  EXPECT_TRUE(leaf->IsDirty());
  node->Reset();
  node->CalculateLayout(100.f, 100.f);
}

TEST(YogaLayoutNodeCov, ResetLayoutCache) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->ResetLayoutCache();  // no-op; should not crash
}

TEST(YogaLayoutNodeCov, HasParentEngineNode) {
  auto parent = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  auto child = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  parent->SetLayoutStyles(MakeStyleNum({{"width", 100}, {"height", 100}}), {});
  child->SetLayoutStyles(MakeStyleNum({{"width", 30}, {"height", 30}}), {});
  EXPECT_FALSE(child->HasParentEngineNode());
  parent->InsertChild(child, 0);
  EXPECT_TRUE(child->HasParentEngineNode());
  parent->RemoveChild(child);
  // parent_ weak_ptr remains set; parent still alive → still true
  EXPECT_TRUE(child->HasParentEngineNode());
}

TEST(YogaLayoutNodeCov, LayoutHadOverflow) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  node->SetLayoutStyles(MakeStyleNum({{"width", 100}, {"height", 100}}), {});
  node->CalculateLayout(100.f, 100.f);
  (void)node->LayoutHadOverflow();  // coverage; no crash
}

TEST(YogaLayoutNodeCov, GetLeftTopRightBottom) {
  auto root = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  auto child = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  root->SetLayoutStyles(MakeStyleNum({{"width", 200}, {"height", 200}}), {});
  child->SetLayoutStyles(MakeStyleNum({{"width", 50}, {"height", 50}}), {});
  root->InsertChild(child, 0);
  root->CalculateLayout(200.f, 200.f);
  (void)child->GetLeft();
  (void)child->GetTop();
  (void)child->GetRight();
  (void)child->GetBottom();
}

TEST(YogaLayoutNodeCov, StyleStringValues) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  // String-typed style values (percent, flex direction strings, etc.)
  auto style = StyleMap{};
  style["width"] = std::make_shared<FlexUIValue>(std::string("50%"));
  style["height"] = std::make_shared<FlexUIValue>(std::string("50%"));
  style["flexDirection"] = std::make_shared<FlexUIValue>(std::string("row"));
  style["flexWrap"] = std::make_shared<FlexUIValue>(std::string("wrap"));
  style["justifyContent"] = std::make_shared<FlexUIValue>(std::string("center"));
  style["alignItems"] = std::make_shared<FlexUIValue>(std::string("flex-end"));
  style["alignSelf"] = std::make_shared<FlexUIValue>(std::string("center"));
  style["overflow"] = std::make_shared<FlexUIValue>(std::string("hidden"));
  style["display"] = std::make_shared<FlexUIValue>(std::string("flex"));
  style["position"] = std::make_shared<FlexUIValue>(std::string("absolute"));
  style["direction"] = std::make_shared<FlexUIValue>(std::string("ltr"));
  node->SetLayoutStyles(style, {});
  node->CalculateLayout(400.f, 400.f);
  EXPECT_GT(node->GetWidth(), 0.f);
}

TEST(YogaLayoutNodeCov, StyleDeletePaths) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  auto style = MakeStyleNum({{"width", 100}, {"height", 100}, {"flex", 1},
                              {"margin", 5}, {"padding", 3}});
  node->SetLayoutStyles(style, {});
  // Now delete all the set props
  std::vector<std::string> deletes = {"width", "height", "flex", "margin",
                                      "padding", "marginLeft", "marginRight",
                                      "marginTop", "marginBottom",
                                      "paddingLeft", "paddingRight",
                                      "paddingTop", "paddingBottom",
                                      "borderWidth", "left", "top", "right",
                                      "bottom", "flexGrow", "flexShrink",
                                      "flexBasis", "flexDirection", "flexWrap",
                                      "alignSelf", "alignItems",
                                      "justifyContent", "overflow", "display",
                                      "direction", "minWidth", "maxWidth",
                                      "minHeight", "maxHeight", "aspectRatio",
                                      "borderLeftWidth", "borderTopWidth",
                                      "borderRightWidth", "borderBottomWidth",
                                      "marginVertical", "marginHorizontal",
                                      "paddingVertical", "paddingHorizontal"};
  node->SetLayoutStyles({}, deletes);
  node->CalculateLayout(100.f, 100.f);
}

TEST(YogaLayoutNodeCov, StyleAutoValues) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  auto style = StyleMap{};
  style["width"] = std::make_shared<FlexUIValue>(std::string("auto"));
  style["flexBasis"] = std::make_shared<FlexUIValue>(std::string("auto"));
  style["marginLeft"] = std::make_shared<FlexUIValue>(std::string("auto"));
  node->SetLayoutStyles(style, {});
  node->CalculateLayout(100.f, 100.f);
}

TEST(YogaLayoutNodeCov, AlignContent) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  auto style = StyleMap{};
  style["width"] = std::make_shared<FlexUIValue>(200.0);
  style["height"] = std::make_shared<FlexUIValue>(200.0);
  style["alignContent"] = std::make_shared<FlexUIValue>(std::string("center"));
  node->SetLayoutStyles(style, {});
  node->CalculateLayout(200.f, 200.f);
}

TEST(YogaLayoutNodeCov, MarginPaddingBorderAll) {
  auto node = std::make_shared<flexui::core::layout::YogaLayoutNode>();
  auto style = MakeStyleNum({
      {"width", 80}, {"height", 80},
      {"margin", 4}, {"marginLeft", 2}, {"marginRight", 2},
      {"marginTop", 3}, {"marginBottom", 3},
      {"marginVertical", 1}, {"marginHorizontal", 1},
      {"padding", 2}, {"paddingLeft", 1}, {"paddingRight", 1},
      {"paddingTop", 1}, {"paddingBottom", 1},
      {"paddingVertical", 1}, {"paddingHorizontal", 1},
      {"borderWidth", 1}, {"borderLeftWidth", 1}, {"borderTopWidth", 1},
      {"borderRightWidth", 1}, {"borderBottomWidth", 1},
      {"left", 0}, {"top", 0}, {"right", 0}, {"bottom", 0},
      {"flex", 1}, {"flexGrow", 1}, {"flexShrink", 1},
      {"aspectRatio", 1},
  });
  node->SetLayoutStyles(style, {});
  node->CalculateLayout(200.f, 200.f);
}

// ─────────────────────────── dom_node.cc ─────────────────────────────────────

TEST(DomNodeCov, UpdatePropertiesAndDomExt) {
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  std::unordered_map<std::string, std::shared_ptr<FlexUIValue>> style_upd;
  style_upd["color"] = std::make_shared<FlexUIValue>(std::string("blue"));
  std::unordered_map<std::string, std::shared_ptr<FlexUIValue>> ext_upd;
  ext_upd["key"] = std::make_shared<FlexUIValue>(42.0);
  node->UpdateProperties(style_upd, ext_upd);
  EXPECT_NE(node->GetStyleMap(), nullptr);
}

TEST(DomNodeCov, UpdateDomNodeStyleAndParseLayoutInfo) {
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  std::unordered_map<std::string, std::shared_ptr<FlexUIValue>> style;
  style["fontSize"] = std::make_shared<FlexUIValue>(14.0);
  node->UpdateDomNodeStyleAndParseLayoutInfo(style);
}

TEST(DomNodeCov, EmplaceStyleMap) {
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  FlexUIValue v(1.0);
  node->EmplaceStyleMap("opacity", v);
  node->EmplaceStyleMap("opacity", FlexUIValue(0.5));  // update existing
}

TEST(DomNodeCov, EmplaceStyleMapAndGetDiff) {
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  std::unordered_map<std::string, std::shared_ptr<FlexUIValue>> diff;
  node->EmplaceStyleMapAndGetDiff("width", FlexUIValue(100.0), diff);
  EXPECT_EQ(diff.count("width"), 1u);
  node->EmplaceStyleMapAndGetDiff("width", FlexUIValue(200.0), diff);
  EXPECT_EQ(diff.count("width"), 1u);
}

TEST(DomNodeCov, MarkWillChange) {
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  node->MarkWillChange(true);
  node->MarkWillChange(false);
}

TEST(DomNodeCov, IndexOf) {
  auto parent = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  auto child1 = std::make_shared<flexui::core::vdom::DomNode>(2, 1);
  auto child2 = std::make_shared<flexui::core::vdom::DomNode>(3, 1);
  auto diff_info = std::make_shared<flexui::core::vdom::DiffInfo>(false);
  auto dom_info1 = std::make_shared<flexui::core::vdom::DomInfo>(child1, nullptr, diff_info);
  auto dom_info2 = std::make_shared<flexui::core::vdom::DomInfo>(child2, nullptr, diff_info);
  parent->AddChildByRefInfo(dom_info1);
  parent->AddChildByRefInfo(dom_info2);
  EXPECT_EQ(parent->IndexOf(child1), 0);
  EXPECT_EQ(parent->IndexOf(child2), 1);
  auto unknown = std::make_shared<flexui::core::vdom::DomNode>(99, 1);
  EXPECT_EQ(parent->IndexOf(unknown), -1);
}

TEST(DomNodeCov, GetSelfDepth) {
  auto root = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  auto child = std::make_shared<flexui::core::vdom::DomNode>(2, 1);
  auto diff_info = std::make_shared<flexui::core::vdom::DiffInfo>(false);
  auto dom_info = std::make_shared<flexui::core::vdom::DomInfo>(child, nullptr, diff_info);
  root->AddChildByRefInfo(dom_info);
  EXPECT_EQ(root->GetSelfDepth(), 1);
  EXPECT_EQ(child->GetSelfDepth(), 2);
}

TEST(DomNodeCov, RemoveChildAt) {
  auto parent = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  auto child = std::make_shared<flexui::core::vdom::DomNode>(2, 1);
  auto diff_info = std::make_shared<flexui::core::vdom::DiffInfo>(false);
  auto dom_info = std::make_shared<flexui::core::vdom::DomInfo>(child, nullptr, diff_info);
  parent->AddChildByRefInfo(dom_info);
  EXPECT_EQ(parent->GetChildCount(), 1u);
  auto removed = parent->RemoveChildAt(0);
  EXPECT_EQ(removed, child);
  EXPECT_EQ(parent->GetChildCount(), 0u);
}

TEST(DomNodeCov, AddChildByRefInfoFront) {
  auto parent = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  auto c1 = std::make_shared<flexui::core::vdom::DomNode>(2, 1);
  auto c2 = std::make_shared<flexui::core::vdom::DomNode>(3, 1);
  auto c3 = std::make_shared<flexui::core::vdom::DomNode>(4, 1);
  auto diff_info = std::make_shared<flexui::core::vdom::DiffInfo>(false);
  // Add c1 first
  parent->AddChildByRefInfo(
      std::make_shared<flexui::core::vdom::DomInfo>(c1, nullptr, diff_info));
  // Add c2 before c1 (kFront)
  auto ref_front = std::make_shared<flexui::core::vdom::RefInfo>(
      2, flexui::core::vdom::RelativeType::kFront);
  parent->AddChildByRefInfo(
      std::make_shared<flexui::core::vdom::DomInfo>(c2, ref_front, diff_info));
  // Add c3 after c1 (kBack)
  auto ref_back = std::make_shared<flexui::core::vdom::RefInfo>(
      2, flexui::core::vdom::RelativeType::kBack);
  parent->AddChildByRefInfo(
      std::make_shared<flexui::core::vdom::DomInfo>(c3, ref_back, diff_info));
  EXPECT_EQ(parent->GetChildCount(), 3u);
}

TEST(DomNodeCov, HasEventListeners) {
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  EXPECT_FALSE(node->HasEventListeners());
}

TEST(DomNodeCov, OstreamOperators) {
  flexui::core::vdom::RefInfo ref_info(1, flexui::core::vdom::RelativeType::kDefault);
  flexui::core::vdom::DiffInfo diff_info(false);
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  auto dom_info = flexui::core::vdom::DomInfo(node, nullptr,
      std::make_shared<flexui::core::vdom::DiffInfo>(false));
  std::ostringstream os1, os2, os3, os4;
  os1 << ref_info;
  os2 << diff_info;
  os3 << *node;
  os4 << dom_info;
  EXPECT_FALSE(os1.str().empty());
  EXPECT_FALSE(os3.str().empty());
}

TEST(DomNodeCov, SerializeDeserializeWithExtMap) {
  auto style_map = std::make_shared<
      std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
  (*style_map)["color"] = std::make_shared<FlexUIValue>(std::string("red"));
  auto ext_map = std::make_shared<
      std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
  (*ext_map)["tag"] = std::make_shared<FlexUIValue>(std::string("card"));

  flexui::core::vdom::DomNode node(3, 1, 0, "View", "view", style_map, ext_map);
  auto serialized = node.Serialize();

  flexui::core::vdom::DomNode node2;
  EXPECT_TRUE(node2.Deserialize(serialized));
  EXPECT_EQ(node2.GetId(), 3u);
}

TEST(DomNodeCov, UpdateObjectStyleMerge) {
  auto node = std::make_shared<flexui::core::vdom::DomNode>(1, 0);
  // Build a nested-object style map
  FlexUIValue::FlexUIValueObjectType inner;
  inner["r"] = FlexUIValue(0.0);
  inner["g"] = FlexUIValue(0.0);
  auto style_map = std::make_shared<
      std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
  (*style_map)["color"] = std::make_shared<FlexUIValue>(std::move(inner));
  node->SetStyleMap(style_map);

  // Update with a new object value for "color" — goes through UpdateProperties->UpdateStyle
  FlexUIValue::FlexUIValueObjectType new_inner;
  new_inner["r"] = FlexUIValue(255.0);
  new_inner["b"] = FlexUIValue(128.0);
  std::unordered_map<std::string, std::shared_ptr<FlexUIValue>> upd;
  upd["color"] = std::make_shared<FlexUIValue>(std::move(new_inner));
  node->UpdateProperties(upd, {});
}

// ─────────────────────────── diff_utils.cc ───────────────────────────────────

TEST(DiffUtilsCov, MarginSpecialCaseUpdate) {
  // old has margin + marginBottom; new has only margin (marginBottom removed)
  // This triggers ShouldUpdateProperty for margin.
  DomValueMap old_map, new_map;
  old_map["margin"] = std::make_shared<FlexUIValue>(10.0);
  old_map["marginBottom"] = std::make_shared<FlexUIValue>(5.0);
  new_map["margin"] = std::make_shared<FlexUIValue>(10.0);
  // marginBottom not in new_map → ShouldUpdateProperty("margin",...) == true

  auto result = flexui::core::reconciler::DiffUtils::DiffProps(
      old_map, new_map, false);
  auto update_props = std::get<0>(result);
  // "margin" should be in update even though its value didn't change
  EXPECT_NE(update_props->find("margin"), update_props->end());
}

TEST(DiffUtilsCov, PaddingSpecialCaseUpdate) {
  DomValueMap old_map, new_map;
  old_map["padding"] = std::make_shared<FlexUIValue>(8.0);
  old_map["paddingLeft"] = std::make_shared<FlexUIValue>(4.0);
  new_map["padding"] = std::make_shared<FlexUIValue>(8.0);

  auto result = flexui::core::reconciler::DiffUtils::DiffProps(
      old_map, new_map, false);
  auto update_props = std::get<0>(result);
  EXPECT_NE(update_props->find("padding"), update_props->end());
}

TEST(DiffUtilsCov, BorderWidthSpecialCaseUpdate) {
  DomValueMap old_map, new_map;
  old_map["borderWidth"] = std::make_shared<FlexUIValue>(2.0);
  old_map["borderLeftWidth"] = std::make_shared<FlexUIValue>(1.0);
  new_map["borderWidth"] = std::make_shared<FlexUIValue>(2.0);

  auto result = flexui::core::reconciler::DiffUtils::DiffProps(
      old_map, new_map, false);
  auto update_props = std::get<0>(result);
  EXPECT_NE(update_props->find("borderWidth"), update_props->end());
}

TEST(DiffUtilsCov, NullOldValueTreatedAsChanged) {
  DomValueMap old_map, new_map;
  old_map["x"] = nullptr;  // explicitly null old value
  new_map["x"] = std::make_shared<FlexUIValue>(1.0);
  auto result = flexui::core::reconciler::DiffUtils::DiffProps(
      old_map, new_map, false);
  auto update_props = std::get<0>(result);
  EXPECT_NE(update_props->find("x"), update_props->end());
}

}  // namespace flexui_core_coverage
