/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/core/card-controller/frontends/card_js/card_js_frontend.h"

#include <unordered_map>

#include "flexui/common/log_tag.h"
#include "flexui/core/vdom/dom_node.h"
#include "flexui/core/scope-manager/scope.h"

namespace flexui::core::card_controller::frontends::card_js {

namespace {

// Convert a JS value tree (from createElement) into a DomNode tree.
// IDs are allocated depth-first, pre-order, starting from *id_counter.
std::shared_ptr<flexui::core::vdom::DomNode> JsValueToDomNode(
    const js_engine::IJsValue& val,
    std::atomic<uint32_t>& id_counter) {
  if (!val.IsObject()) return nullptr;

  auto type_val = val.GetProperty("type");
  if (!type_val || !type_val->IsString()) return nullptr;

  uint32_t id = id_counter.fetch_add(1);
  const std::string view_name = type_val->ToStdString();

  auto style_map = std::make_shared<
      std::unordered_map<std::string, std::shared_ptr<flexui::common::FlexUIValue>>>();

  auto props_val = val.GetProperty("props");
  if (props_val && props_val->IsObject()) {
    for (auto& key : props_val->GetPropertyNames()) {
      auto pv = props_val->GetProperty(key);
      if (pv) {
        (*style_map)[key] = std::make_shared<flexui::common::FlexUIValue>(
            pv->ToFlexUIValue());
      }
    }
  }

  auto dom_ext = std::make_shared<
      std::unordered_map<std::string, std::shared_ptr<flexui::common::FlexUIValue>>>();

  auto node = std::make_shared<flexui::core::vdom::DomNode>(
      id, 0, 0, view_name, view_name, style_map, dom_ext);

  auto children_val = val.GetProperty("children");
  if (children_val && children_val->IsArray()) {
    uint32_t len = children_val->GetArrayLength();
    for (uint32_t i = 0; i < len; ++i) {
      auto child_js = children_val->GetArrayItem(i);
      if (child_js) {
        auto child = JsValueToDomNode(*child_js, id_counter);
        if (child) {
          child->SetPid(id);
          auto ref  = std::make_shared<flexui::core::vdom::RefInfo>(0, 0);
          auto diff = std::make_shared<flexui::core::vdom::DiffInfo>(false);
          auto info = std::make_shared<flexui::core::vdom::DomInfo>(child, ref, diff);
          node->AddChildByRefInfo(info);
        }
      }
    }
  }

  return node;
}

}  // namespace

void CardJsFrontend::Initialize(flexui::core::scope_manager::Scope& scope,
                                 const flexui::core::plugin_host::BundleSource& bundle) {
  FLEXUI_TLOG(Frontend, CardJsInit, INFO) << "uri=" << bundle.uri;
  next_node_id_.store(1);
  auto* ctx = scope.js_context();
  if (!ctx) return;

  // Inject createElement global.
  ctx->InjectGlobalFunction("createElement",
      [](js_engine::IJsContext& c,
         const std::vector<std::shared_ptr<js_engine::IJsValue>>& args)
          -> std::shared_ptr<js_engine::IJsValue> {
        auto obj = c.NewObject();
        if (args.size() > 0) obj->SetProperty("type", args[0]);
        if (args.size() > 1) obj->SetProperty("props", args[1]);
        if (args.size() > 2) obj->SetProperty("children", args[2]);
        else obj->SetProperty("children", c.NewArray(0));
        return obj;
      });

  // Inject setData stub (real dispatch wired via Scope sink in W9-W10).
  ctx->InjectGlobalFunction("setData",
      [](js_engine::IJsContext& c,
         const std::vector<std::shared_ptr<js_engine::IJsValue>>&)
          -> std::shared_ptr<js_engine::IJsValue> {
        return c.NewUndefined();
      });

  // Inject registerHandler stub.
  ctx->InjectGlobalFunction("registerHandler",
      [](js_engine::IJsContext& c,
         const std::vector<std::shared_ptr<js_engine::IJsValue>>&)
          -> std::shared_ptr<js_engine::IJsValue> {
        return c.NewUndefined();
      });

  // Eval the bundle.
  auto err = flexui::common::Error::Ok();
  ctx->Eval(bundle.text, bundle.uri.empty() ? "<card-js>" : bundle.uri, &err);
  if (!err.ok()) {
    FLEXUI_TLOG(Frontend, CardJsEvalError, ERROR) << err.message();
    return;
  }
  ctx->RunPendingJobs();

  // Read exports.
  auto exports = ctx->Eval("globalThis.__flexui_exports", "<exports>", &err);
  if (!err.ok() || !exports || !exports->IsObject()) {
    FLEXUI_TLOG(Frontend, CardJsNoExports, ERROR);
    return;
  }
  render_fn_ = exports->GetProperty("render");
  handlers_  = exports->GetProperty("handlers");
}

std::shared_ptr<flexui::core::vdom::DomNode> CardJsFrontend::Render(
    flexui::core::scope_manager::Scope& scope,
    const flexui::common::FlexUIValue& data) {
  FLEXUI_TLOG(Frontend, CardJsRender, DEBUG);
  next_node_id_.store(1);
  auto* ctx = scope.js_context();
  if (!ctx || !render_fn_ || !render_fn_->IsFunction()) return nullptr;

  auto js_data = ctx->FromFlexUIValue(data);
  auto err = flexui::common::Error::Ok();
  auto result = ctx->Call(render_fn_, ctx->NewUndefined(), {js_data}, &err);
  ctx->RunPendingJobs();
  if (!err.ok() || !result) {
    FLEXUI_TLOG(Frontend, CardJsRenderError, ERROR) << err.message();
    return nullptr;
  }
  return JsValueToDomNode(*result, next_node_id_);
}

void CardJsFrontend::HandleEvent(flexui::core::scope_manager::Scope& scope,
                                  const std::string& event_name,
                                  const flexui::common::FlexUIValue& payload) {
  FLEXUI_TLOG(Frontend, CardJsHandleEvent, DEBUG) << "event=" << event_name;
  auto* ctx = scope.js_context();
  if (!ctx || !handlers_ || !handlers_->IsObject()) return;
  auto handler = handlers_->GetProperty(event_name);
  if (!handler || !handler->IsFunction()) {
    FLEXUI_TLOG(Frontend, CardJsHandlerMissing, DEBUG) << "event=" << event_name;
    return;
  }
  auto arg = ctx->FromFlexUIValue(payload);
  auto err = flexui::common::Error::Ok();
  ctx->Call(handler, ctx->NewUndefined(), {arg}, &err);
  ctx->RunPendingJobs();
  if (!err.ok()) {
    FLEXUI_TLOG(Frontend, CardJsHandlerError, ERROR) << err.message();
  }
}

flexui::core::plugin_host::FrontendRegistration MakeCardJsRegistration() {
  return {"card-js", []() { return std::make_unique<CardJsFrontend>(); }};
}

}  // namespace flexui::core::card_controller::frontends::card_js
