// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.
//
// NAPI bindings for FlexCardController.
// A static id-table maps uint32_t -> unique_ptr<FlexCardController>.
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <napi/native_api.h>

#include "flexui/core/card-controller/flex_card_controller.h"
#include "flexui/common/log_tag.h"

namespace flexui::platforms::harmony {

namespace cc = flexui::core::card_controller;

// ---------------------------------------------------------------------------
// Controller registry
// ---------------------------------------------------------------------------

namespace {
std::mutex g_mu;
std::unordered_map<uint32_t, std::unique_ptr<cc::FlexCardController>> g_controllers;
std::atomic<uint32_t> g_next_id{1};

cc::FlexCardController* FindController(uint32_t id) {
  auto it = g_controllers.find(id);
  return it == g_controllers.end() ? nullptr : it->second.get();
}
}  // namespace

// ---------------------------------------------------------------------------
// NAPI helpers
// ---------------------------------------------------------------------------

static uint32_t GetArgUint32(napi_env env, napi_value val) {
  uint32_t v = 0;
  napi_get_value_uint32(env, val, &v);
  return v;
}

static std::string GetArgString(napi_env env, napi_value val) {
  size_t len = 0;
  napi_get_value_string_utf8(env, val, nullptr, 0, &len);
  std::string s(len, '\0');
  napi_get_value_string_utf8(env, val, &s[0], len + 1, &len);
  return s;
}

// ---------------------------------------------------------------------------
// _create(optsJson: string) -> id: number
// ---------------------------------------------------------------------------

static napi_value Create(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  std::string opts_json = (argc > 0) ? GetArgString(env, args[0]) : "{}";
  // Parse minimal fields from opts_json: bundleUri, bundleType.
  // Full JSON parse lands in W7-W8; for PoC extract bundle_type heuristically.
  cc::FlexCardControllerOptions opts;
  // Simple scan for bundleType value.
  auto bt_pos = opts_json.find("\"card-js\"");
  if (bt_pos != std::string::npos) opts.bundle_type = "card-js";
  else opts.bundle_type = "a2ui-json";

  auto uri_pos = opts_json.find("\"bundleUri\"");
  if (uri_pos != std::string::npos) {
    auto colon = opts_json.find(':', uri_pos);
    auto q1 = opts_json.find('"', colon + 1);
    auto q2 = opts_json.find('"', q1 + 1);
    if (q1 != std::string::npos && q2 != std::string::npos)
      opts.bundle_uri = opts_json.substr(q1 + 1, q2 - q1 - 1);
  }

  uint32_t id = g_next_id.fetch_add(1);
  {
    std::lock_guard<std::mutex> lock(g_mu);
    g_controllers[id] = std::make_unique<cc::FlexCardController>(std::move(opts));
  }
  FLEXUI_TLOG(Bridge, NapiCreate, DEBUG) << "id=" << id;

  napi_value result;
  napi_create_uint32(env, id, &result);
  return result;
}

// ---------------------------------------------------------------------------
// _load(id: number) -> ok: boolean
// ---------------------------------------------------------------------------

static napi_value Load(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint32_t id = GetArgUint32(env, args[0]);

  std::lock_guard<std::mutex> lock(g_mu);
  auto* ctrl = FindController(id);
  bool ok = ctrl && ctrl->Load().ok();
  FLEXUI_TLOG(Bridge, NapiLoad, DEBUG) << "id=" << id << " ok=" << ok;

  napi_value result;
  napi_get_boolean(env, ok, &result);
  return result;
}

// ---------------------------------------------------------------------------
// _setData(id: number, dataJson: string) -> ok: boolean
// ---------------------------------------------------------------------------

static napi_value SetData(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint32_t id = GetArgUint32(env, args[0]);
  // dataJson parsing to FlexUIValue lands in W7-W8; pass empty value for now.
  (void)args[1];

  std::lock_guard<std::mutex> lock(g_mu);
  auto* ctrl = FindController(id);
  bool ok = ctrl && ctrl->SetData(flexui::common::FlexUIValue()).ok();
  FLEXUI_TLOG(Bridge, NapiSetData, DEBUG) << "id=" << id << " ok=" << ok;

  napi_value result;
  napi_get_boolean(env, ok, &result);
  return result;
}

// ---------------------------------------------------------------------------
// _destroy(id: number) -> ok: boolean
// ---------------------------------------------------------------------------

static napi_value Destroy(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint32_t id = GetArgUint32(env, args[0]);

  bool ok = false;
  {
    std::lock_guard<std::mutex> lock(g_mu);
    auto* ctrl = FindController(id);
    if (ctrl) {
      ok = ctrl->Destroy().ok();
      g_controllers.erase(id);
    }
  }
  FLEXUI_TLOG(Bridge, NapiDestroy, DEBUG) << "id=" << id << " ok=" << ok;

  napi_value result;
  napi_get_boolean(env, ok, &result);
  return result;
}

// ---------------------------------------------------------------------------
// _attach(id: number, nodeContent: NodeContent) -> void
// (NodeContent extraction delegated to napi_node_content.cc)
// ---------------------------------------------------------------------------

// Forward-declared; defined in napi_node_content.cc.
extern void AttachNodeContentById(napi_env env, uint32_t id, napi_value nc_val);

static napi_value Attach(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint32_t id = GetArgUint32(env, args[0]);
  FLEXUI_TLOG(Bridge, NapiAttach, DEBUG) << "id=" << id;
  if (argc > 1) AttachNodeContentById(env, id, args[1]);
  napi_value undef;
  napi_get_undefined(env, &undef);
  return undef;
}

// ---------------------------------------------------------------------------
// _detach(id: number) -> void
// ---------------------------------------------------------------------------

static napi_value Detach(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  uint32_t id = GetArgUint32(env, args[0]);

  {
    std::lock_guard<std::mutex> lock(g_mu);
    auto* ctrl = FindController(id);
    if (ctrl) ctrl->DetachNodeContent();
  }
  FLEXUI_TLOG(Bridge, NapiDetach, DEBUG) << "id=" << id;

  napi_value undef;
  napi_get_undefined(env, &undef);
  return undef;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

napi_value RegisterControllerMethods(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"flexCardControllerCreate",  nullptr, Create,  nullptr, nullptr, nullptr, napi_default, nullptr},
      {"flexCardControllerLoad",    nullptr, Load,    nullptr, nullptr, nullptr, napi_default, nullptr},
      {"flexCardControllerSetData", nullptr, SetData, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"flexCardControllerDestroy", nullptr, Destroy, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"flexCardControllerAttach",  nullptr, Attach,  nullptr, nullptr, nullptr, napi_default, nullptr},
      {"flexCardControllerDetach",  nullptr, Detach,  nullptr, nullptr, nullptr, napi_default, nullptr},
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(*desc), desc);
  return exports;
}

}  // namespace flexui::platforms::harmony
