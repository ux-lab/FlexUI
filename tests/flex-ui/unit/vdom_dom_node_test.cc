/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Unit tests for the absorbed DomNode, DomArgument headers in
 * flex-ui/core/vdom/. These tests verify namespace, type-rename, and
 * compile-correctness of the absorbed API.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "flexui/core/vdom/dom_argument.h"
#include "flexui/core/vdom/dom_node.h"
#include "flexui/core/vdom/node_props.h"

namespace flexui::core::vdom {

// ---------------------------------------------------------------------------
// DomArgument tests
// ---------------------------------------------------------------------------

TEST(VdomDomArgumentTest, DefaultConstruct) {
  DomArgument arg;
  // Default-constructed argument should not crash.
  (void)arg;
}

TEST(VdomDomArgumentTest, ConstructFromFlexUIValue) {
  flexui::common::FlexUIValue val(42.0);
  DomArgument arg(val);
  flexui::common::FlexUIValue out;
  EXPECT_TRUE(arg.ToObject(out));
}

TEST(VdomDomArgumentTest, RoundtripObjectToBsonToObject) {
  flexui::common::FlexUIValue val(std::string("hello"));
  DomArgument arg(val);
  std::vector<uint8_t> bson;
  EXPECT_TRUE(arg.ToBson(bson));
  EXPECT_FALSE(bson.empty());

  DomArgument from_bson(bson);
  flexui::common::FlexUIValue out;
  EXPECT_TRUE(from_bson.ToObject(out));
}

TEST(VdomDomArgumentTest, CopyConstruct) {
  flexui::common::FlexUIValue val(true);
  DomArgument original(val);
  DomArgument copy(original);
  flexui::common::FlexUIValue out;
  EXPECT_TRUE(copy.ToObject(out));
}

// ---------------------------------------------------------------------------
// DomNode tests
// ---------------------------------------------------------------------------

TEST(VdomDomNodeTest, DefaultConstruct) {
  DomNode node;
  EXPECT_EQ(node.GetId(), 0u);
  EXPECT_EQ(node.GetPid(), 0u);
}

TEST(VdomDomNodeTest, ConstructWithIdPid) {
  DomNode node(/*id=*/1, /*pid=*/0);
  EXPECT_EQ(node.GetId(), 1u);
  EXPECT_EQ(node.GetPid(), 0u);
}

TEST(VdomDomNodeTest, FullConstruct) {
  auto style_map =
      std::make_shared<std::unordered_map<std::string, std::shared_ptr<flexui::common::FlexUIValue>>>();
  (*style_map)["width"] = std::make_shared<flexui::common::FlexUIValue>(100.0);

  auto ext_map =
      std::make_shared<std::unordered_map<std::string, std::shared_ptr<flexui::common::FlexUIValue>>>();

  DomNode node(/*id=*/2, /*pid=*/1, /*index=*/0, "View", "view",
               style_map, ext_map);

  EXPECT_EQ(node.GetId(), 2u);
  EXPECT_EQ(node.GetPid(), 1u);
  EXPECT_EQ(node.GetTagName(), "View");
  EXPECT_EQ(node.GetViewName(), "view");
  EXPECT_NE(node.GetStyleMap(), nullptr);
}

TEST(VdomDomNodeTest, SetGetTagAndViewName) {
  DomNode node;
  node.SetTagName("Text");
  node.SetViewName("text_view");
  EXPECT_EQ(node.GetTagName(), "Text");
  EXPECT_EQ(node.GetViewName(), "text_view");
}

TEST(VdomDomNodeTest, AddRemoveChild) {
  auto parent = std::make_shared<DomNode>(/*id=*/1, /*pid=*/0);
  auto child = std::make_shared<DomNode>(/*id=*/2, /*pid=*/1);

  auto ref_info = std::make_shared<RefInfo>(0, RelativeType::kDefault);
  auto diff_info = std::make_shared<DiffInfo>(false);
  auto dom_info = std::make_shared<DomInfo>(child, nullptr, diff_info);

  parent->AddChildByRefInfo(dom_info);
  EXPECT_EQ(parent->GetChildCount(), 1u);

  auto removed = parent->RemoveChildById(2);
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(parent->GetChildCount(), 0u);
}

TEST(VdomDomNodeTest, GetChildAt) {
  auto parent = std::make_shared<DomNode>(/*id=*/1, /*pid=*/0);
  auto child = std::make_shared<DomNode>(/*id=*/2, /*pid=*/1);
  auto diff_info = std::make_shared<DiffInfo>(false);
  auto dom_info = std::make_shared<DomInfo>(child, nullptr, diff_info);
  parent->AddChildByRefInfo(dom_info);

  EXPECT_EQ(parent->GetChildAt(0), child);
  EXPECT_EQ(parent->GetChildAt(99), nullptr);
}

TEST(VdomDomNodeTest, LayoutOnlyFlag) {
  DomNode node;
  EXPECT_FALSE(node.IsLayoutOnly());
  node.SetLayoutOnly(true);
  EXPECT_TRUE(node.IsLayoutOnly());
}

TEST(VdomDomNodeTest, VirtualFlag) {
  DomNode node;
  EXPECT_FALSE(node.IsVirtual());
  node.SetIsVirtual(true);
  EXPECT_TRUE(node.IsVirtual());
}

TEST(VdomDomNodeTest, RenderInfoRoundtrip) {
  DomNode node;
  DomNode::RenderInfo ri;
  ri.id = 10;
  ri.pid = 5;
  ri.index = 3;
  ri.depth = 2;
  node.SetRenderInfo(ri);
  EXPECT_EQ(node.GetRenderInfo().id, 10u);
  EXPECT_EQ(node.GetRenderInfo().pid, 5u);
}

TEST(VdomDomNodeTest, SerializeDeserialize) {
  auto style_map =
      std::make_shared<std::unordered_map<std::string, std::shared_ptr<flexui::common::FlexUIValue>>>();
  (*style_map)["color"] = std::make_shared<flexui::common::FlexUIValue>(std::string("red"));

  DomNode node(2, 1, 0, "Text", "text", style_map, nullptr);
  auto serialized = node.Serialize();

  DomNode node2;
  EXPECT_TRUE(node2.Deserialize(serialized));
  EXPECT_EQ(node2.GetId(), 2u);
  EXPECT_EQ(node2.GetViewName(), "text");
}

// ---------------------------------------------------------------------------
// NodeProps tests (compile-time constant correctness)
// ---------------------------------------------------------------------------

TEST(VdomNodePropsTest, ConstantsResolvable) {
  EXPECT_STREQ(kWidth, "width");
  EXPECT_STREQ(kHeight, "height");
  EXPECT_STREQ(kFlex, "flex");
  EXPECT_STREQ(kTagNameView, "View");
}

}  // namespace flexui::core::vdom
