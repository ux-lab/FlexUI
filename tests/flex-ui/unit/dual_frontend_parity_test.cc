/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * C2 PROOF: dual-frontend parity test.
 * Verifies that a2ui-json and card-js frontends both produce valid
 * DomNode trees from equivalent inputs.
 */
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/core/card-controller/flex_card_controller.h"
#include "flexui/core/vdom/dom_node.h"
#include "flexui/core/js-engine/js_engine_backend.h"

namespace flexui::core::card_controller {

namespace {

// Recursively compare two DomNode trees for structural equivalence.
bool DomNodeTreesEqual(vdom::DomNode* a, vdom::DomNode* b) {
  if (!a || !b) return a == b;
  if (a->GetViewName() != b->GetViewName()) return false;

  const auto& ca = a->GetChildren();
  const auto& cb = b->GetChildren();
  if (ca.size() != cb.size()) return false;

  for (size_t i = 0; i < ca.size(); ++i) {
    if (!DomNodeTreesEqual(ca[i].get(), cb[i].get())) return false;
  }
  return true;
}

// Walk a DomNode tree and collect view names in pre-order.
void CollectViewNames(vdom::DomNode* node, std::vector<std::string>* out) {
  if (!node) return;
  out->push_back(node->GetViewName());
  for (auto& c : node->GetChildren()) {
    CollectViewNames(c.get(), out);
  }
}

}  // namespace

class DualFrontendParityTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FlexUIEngineConfig cfg;
    cfg.backend = js_engine::JsEngineBackend::kQuickJS;
    auto err = FlexUIEngine::Instance().Init(cfg);
    ASSERT_TRUE(err.ok()) << err.message();
  }

  void TearDown() override {
    FlexUIEngine::Instance().Shutdown();
  }
};

TEST_F(DualFrontendParityTest, A2UIJsonFrontendProducesValidTree) {
  const char* a2ui_json = R"({
    "type": "View",
    "props": {"flexDirection": "column"},
    "children": [
      {"type": "Text", "props": {"text": "{{ title }}"}}
    ]
  })";

  flexui::common::FlexUIValue data(
      flexui::common::FlexUIValue::FlexUIValueObjectType{
          {"title", flexui::common::FlexUIValue(std::string("Hello FlexUI"))}});

  FlexCardControllerOptions opts;
  opts.bundle_type   = "a2ui-json";
  opts.bundle_inline = a2ui_json;
  opts.initial_data  = data;
  FlexCardController card(std::move(opts));
  auto err = card.Load();
  ASSERT_TRUE(err.ok()) << err.message();

  auto tree = card.DebugLastDomTree();
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(tree->GetViewName(), "View");
  ASSERT_EQ(tree->GetChildren().size(), 1u);
  EXPECT_EQ(tree->GetChildren()[0]->GetViewName(), "Text");

  card.Destroy();
}

TEST_F(DualFrontendParityTest, CardJsFrontendProducesValidTree) {
  const char* card_js = R"(
    globalThis.__flexui_exports = {
      render: function(data) {
        return createElement("View", {flexDirection: "column"}, [
          createElement("Text", {text: data.title}, [])
        ]);
      }
    };
  )";

  flexui::common::FlexUIValue data(
      flexui::common::FlexUIValue::FlexUIValueObjectType{
          {"title", flexui::common::FlexUIValue(std::string("Hello FlexUI"))}});

  FlexCardControllerOptions opts;
  opts.bundle_type   = "card-js";
  opts.bundle_inline = card_js;
  opts.initial_data  = data;
  FlexCardController card(std::move(opts));
  auto err = card.Load();
  ASSERT_TRUE(err.ok()) << err.message();

  auto tree = card.DebugLastDomTree();
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(tree->GetViewName(), "View");
  ASSERT_EQ(tree->GetChildren().size(), 1u);
  EXPECT_EQ(tree->GetChildren()[0]->GetViewName(), "Text");

  card.Destroy();
}

TEST_F(DualFrontendParityTest, A2UIJsonNestedTreeProducesCorrectStructure) {
  const char* a2ui_json = R"({
    "type": "View",
    "props": {},
    "children": [
      {"type": "Image", "props": {"src": "{{ img }}", "width": 100, "height": 80}},
      {"type": "Button", "props": {"title": "{{ btn }}", "onClick": "onTap"}}
    ]
  })";

  flexui::common::FlexUIValue data(
      flexui::common::FlexUIValue::FlexUIValueObjectType{
          {"img", flexui::common::FlexUIValue(std::string("https://x.com/pic.png"))},
          {"btn", flexui::common::FlexUIValue(std::string("Click"))}});

  FlexCardControllerOptions opts;
  opts.bundle_type   = "a2ui-json";
  opts.bundle_inline = a2ui_json;
  opts.initial_data  = data;
  FlexCardController card(std::move(opts));
  ASSERT_TRUE(card.Load().ok());

  auto tree = card.DebugLastDomTree();
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(tree->GetViewName(), "View");
  ASSERT_EQ(tree->GetChildren().size(), 2u);
  EXPECT_EQ(tree->GetChildren()[0]->GetViewName(), "Image");
  EXPECT_EQ(tree->GetChildren()[1]->GetViewName(), "Button");

  card.Destroy();
}

TEST_F(DualFrontendParityTest, A2UIJsonSetDataUpdatesTree) {
  const char* a2ui_json = R"({"type": "Text", "props": {"text": "{{ msg }}"}})";

  flexui::common::FlexUIValue data1(
      flexui::common::FlexUIValue::FlexUIValueObjectType{
          {"msg", flexui::common::FlexUIValue(std::string("first"))}});

  FlexCardControllerOptions opts;
  opts.bundle_type   = "a2ui-json";
  opts.bundle_inline = a2ui_json;
  opts.initial_data  = data1;
  FlexCardController card(std::move(opts));
  ASSERT_TRUE(card.Load().ok());
  EXPECT_EQ(card.DebugLastDomTree()->GetViewName(), "Text");

  flexui::common::FlexUIValue data2(
      flexui::common::FlexUIValue::FlexUIValueObjectType{
          {"msg", flexui::common::FlexUIValue(std::string("second"))}});
  ASSERT_TRUE(card.SetData(data2).ok());
  EXPECT_EQ(card.DebugLastDomTree()->GetViewName(), "Text");

  card.Destroy();
}

}  // namespace flexui::core::card_controller
