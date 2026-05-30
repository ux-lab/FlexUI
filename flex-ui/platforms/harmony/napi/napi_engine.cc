// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.
//
// NAPI bindings for FlexUIEngine (Init / Shutdown).
#include <napi/native_api.h>

#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/common/log_tag.h"

namespace flexui::platforms::harmony {

namespace cc = flexui::core::card_controller;

static napi_value Init(napi_env env, napi_callback_info info) {
  FLEXUI_TLOG(Engine, NapiInit, INFO);
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int32_t backend_int = 0;
  bool force_debug = false;
  napi_get_value_int32(env, args[0], &backend_int);
  napi_get_value_bool(env, args[1], &force_debug);

  cc::FlexUIEngineConfig cfg;
  cfg.backend = static_cast<flexui::core::js_engine::JsEngineBackend>(backend_int);
  cfg.force_quickjs_for_debug = force_debug;
  auto err = cc::FlexUIEngine::Instance().Init(cfg);
  napi_value result;
  napi_get_boolean(env, err.ok(), &result);
  return result;
}

static napi_value Shutdown(napi_env env, napi_callback_info) {
  FLEXUI_TLOG(Engine, NapiShutdown, INFO);
  cc::FlexUIEngine::Instance().Shutdown();
  napi_value undef;
  napi_get_undefined(env, &undef);
  return undef;
}

napi_value RegisterEngineMethods(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"flexUiEngineInit",     nullptr, Init,     nullptr, nullptr, nullptr, napi_default, nullptr},
      {"flexUiEngineShutdown", nullptr, Shutdown, nullptr, nullptr, nullptr, napi_default, nullptr},
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(*desc), desc);
  return exports;
}

}  // namespace flexui::platforms::harmony
