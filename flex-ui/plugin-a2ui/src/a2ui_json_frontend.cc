/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/plugin_a2ui/a2ui_json_frontend.h"

#include <atomic>

#include "flexui/common/log_tag.h"
#include "flexui/core/vdom/dom_node.h"
#include "flexui/core/scope-manager/scope.h"

namespace flexui::plugin_a2ui {

// Forward decls from parser/binder/runtime translation units.
flexui::common::FlexUIValue ParseA2UIJson(const std::string& text);
flexui::common::FlexUIValue ApplyDataBinding(const flexui::common::FlexUIValue& tmpl,
                                              const flexui::common::FlexUIValue& data);
void EvalHandlersJs(flexui::core::scope_manager::Scope& scope,
                    const std::string& source);
void DispatchHandler(flexui::core::scope_manager::Scope& scope,
                     const std::string& name,
                     const flexui::common::FlexUIValue& payload);

namespace {

// Build DomNode tree from a bound FlexUIValue.
// IDs are allocated depth-first, pre-order, starting from *id_counter.
std::shared_ptr<flexui::core::vdom::DomNode> BuildDomTree(
    const flexui::common::FlexUIValue& node,
    std::atomic<uint32_t>& id_counter) {
  if (!node.IsObject()) return nullptr;
  const auto& obj = node.ToObjectChecked();

  auto type_it = obj.find("type");
  if (type_it == obj.end() || !type_it->second.IsString()) return nullptr;

  uint32_t id = id_counter.fetch_add(1);
  auto style_map = std::make_shared<
      std::unordered_map<std::string, std::shared_ptr<flexui::common::FlexUIValue>>>();

  // Collect props.
  auto props_it = obj.find("props");
  if (props_it != obj.end() && props_it->second.IsObject()) {
    for (auto& kv : props_it->second.ToObjectChecked()) {
      (*style_map)[kv.first] = std::make_shared<flexui::common::FlexUIValue>(kv.second);
    }
  }

  auto dom_ext = std::make_shared<
      std::unordered_map<std::string, std::shared_ptr<flexui::common::FlexUIValue>>>();

  const std::string& view_name = type_it->second.ToStringChecked();
  auto dom_node = std::make_shared<flexui::core::vdom::DomNode>(
      id, 0, 0, view_name, view_name, style_map, dom_ext);

  // Recurse children.
  auto children_it = obj.find("children");
  if (children_it != obj.end() && children_it->second.IsArray()) {
    for (auto& child_val : children_it->second.ToArrayChecked()) {
      auto child = BuildDomTree(child_val, id_counter);
      if (child) {
        child->SetPid(id);
        auto ref  = std::make_shared<flexui::core::vdom::RefInfo>(0, 0);
        auto diff = std::make_shared<flexui::core::vdom::DiffInfo>(false);
        auto info = std::make_shared<flexui::core::vdom::DomInfo>(child, ref, diff);
        dom_node->AddChildByRefInfo(info);
      }
    }
  }

  return dom_node;
}

}  // namespace

void A2UIJsonFrontend::Initialize(flexui::core::scope_manager::Scope& scope,
                                   const flexui::core::plugin_host::BundleSource& bundle) {
  FLEXUI_TLOG(Frontend, A2UIInit, INFO) << "uri=" << bundle.uri;
  next_node_id_.store(1);
  json_doc_ = ParseA2UIJson(bundle.text);
  // Check for embedded handlers.js
  if (json_doc_.IsObject()) {
    const auto& obj = json_doc_.ToObjectChecked();
    auto it = obj.find("handlersJs");
    if (it != obj.end() && it->second.IsString()) {
      handlers_js_ = it->second.ToStringChecked();
      EvalHandlersJs(scope, *handlers_js_);
    }
  }
}

std::shared_ptr<flexui::core::vdom::DomNode> A2UIJsonFrontend::Render(
    flexui::core::scope_manager::Scope& scope,
    const flexui::common::FlexUIValue& data) {
  FLEXUI_TLOG(Frontend, A2UIRender, DEBUG);
  (void)scope;
  next_node_id_.store(1);
  auto bound = ApplyDataBinding(json_doc_, data);
  return BuildDomTree(bound, next_node_id_);
}

void A2UIJsonFrontend::HandleEvent(flexui::core::scope_manager::Scope& scope,
                                    const std::string& event_name,
                                    const flexui::common::FlexUIValue& payload) {
  FLEXUI_TLOG(Frontend, A2UIHandleEvent, DEBUG) << "event=" << event_name;
  DispatchHandler(scope, event_name, payload);
}

}  // namespace flexui::plugin_a2ui
