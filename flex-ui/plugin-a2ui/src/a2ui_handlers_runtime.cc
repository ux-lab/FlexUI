/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/common/log_tag.h"
#include "flexui/core/scope-manager/scope.h"
#include "flexui/core/js-engine/ijs_context.h"
#include "flexui/common/flexui_value.h"

namespace flexui::plugin_a2ui {

void EvalHandlersJs(flexui::core::scope_manager::Scope& scope,
                    const std::string& source) {
  FLEXUI_TLOG(Frontend, A2UIHandlersEval, DEBUG);
  auto* ctx = scope.js_context();
  if (!ctx) return;
  auto err = flexui::common::Error::Ok();
  ctx->Eval(source, "<handlers.js>", &err);
  if (!err.ok()) {
    FLEXUI_TLOG(Frontend, A2UIHandlersError, ERROR) << err.message();
  }
}

void DispatchHandler(flexui::core::scope_manager::Scope& scope,
                     const std::string& name,
                     const flexui::common::FlexUIValue& payload) {
  FLEXUI_TLOG(Frontend, A2UIDispatchHandler, DEBUG) << "name=" << name;
  auto* ctx = scope.js_context();
  if (!ctx) return;
  auto err = flexui::common::Error::Ok();
  auto global = ctx->Eval("globalThis.__a2ui_handlers", "<dispatch>", &err);
  if (err || !global || !global->IsObject()) return;
  auto handler_val = global->GetProperty(name);
  if (!handler_val || !handler_val->IsFunction()) {
    FLEXUI_TLOG(Frontend, A2UIHandlerMissing, DEBUG) << "name=" << name;
    return;
  }
  auto arg = ctx->FromFlexUIValue(payload);
  ctx->Call(handler_val, ctx->NewUndefined(), {arg}, &err);
  if (!err.ok()) {
    FLEXUI_TLOG(Frontend, A2UIHandlerError, ERROR) << err.message();
  }
}

}  // namespace flexui::plugin_a2ui
