// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.
//
// NAPI binding for NodeContent attachment.
// Uses OH_ArkUI_GetNodeContentFromNapiValue (HarmonyOS API 12+) to extract
// an ArkUI_NodeContentHandle from the ETS NodeContent napi_value, then
// forwards it to FlexCardController::AttachNodeContent.
#include <mutex>
#include <unordered_map>

#include <napi/native_api.h>

#include "flexui/core/card-controller/flex_card_controller.h"
#include "flexui/core/commit-pipeline/component_instance.h"
#include "flexui/common/log_tag.h"

// ArkUI C-API header (available when FLEXUI_OHOS=1).
#if defined(FLEXUI_OHOS)
#  include <arkui/native_node.h>
#endif

namespace flexui::platforms::harmony {

// g_controllers and FindController are defined in napi_controller.cc (same TU
// set). We access through a forward declaration of the helper used from Attach.
// The registry mutex and map are accessed via the helper below.

// Forward: declared in napi_controller.cc and used here.
namespace {
// Acquire the lock and look up the controller — defined there.
// We use a thin wrapper to avoid re-declaring the static map.
}  // namespace

// This function is called from napi_controller.cc via the forward declaration.
// It lives here so the ArkUI include is isolated.
extern std::mutex g_mu;  // NOLINT — defined in napi_controller.cc
extern std::unordered_map<uint32_t, std::unique_ptr<flexui::core::card_controller::FlexCardController>> g_controllers;

void AttachNodeContentById(napi_env env, uint32_t id, napi_value nc_val) {
  FLEXUI_TLOG(Bridge, NapiAttachNodeContent, DEBUG) << "id=" << id;

  flexui::core::commit_pipeline::NodeHandle handle = nullptr;

#if defined(FLEXUI_OHOS) && defined(OH_ArkUI_GetNodeContentFromNapiValue)
  // HarmonyOS API 12+: extract the native ArkUI_NodeContentHandle.
  ArkUI_NodeContentHandle node_content_handle = nullptr;
  int32_t ret = OH_ArkUI_GetNodeContentFromNapiValue(env, nc_val, &node_content_handle);
  if (ret == 0 && node_content_handle) {
    handle = static_cast<flexui::core::commit_pipeline::NodeHandle>(node_content_handle);
    FLEXUI_TLOG(Bridge, NapiAttachNodeContent, DEBUG)
        << "id=" << id << " handle=" << handle;
  } else {
    FLEXUI_TLOG(Bridge, NapiAttachNodeContent, WARNING)
        << "id=" << id << " OH_ArkUI_GetNodeContentFromNapiValue failed ret=" << ret;
  }
#else
  // TODO(W9-W10): OH_ArkUI_GetNodeContentFromNapiValue not available at build
  // time. When targeting a device with API 12+, include <arkui/native_node.h>
  // and ensure the toolchain version is >= 12.
  (void)env;
  (void)nc_val;
  FLEXUI_TLOG(Bridge, NapiAttachNodeContent, WARNING)
      << "id=" << id << " OH_ArkUI_GetNodeContentFromNapiValue unavailable (TODO W9-W10)";
#endif

  std::lock_guard<std::mutex> lock(g_mu);
  auto it = g_controllers.find(id);
  if (it != g_controllers.end()) {
    it->second->AttachNodeContent(handle);
  }
}

napi_value RegisterNodeContentMethods(napi_env env, napi_value exports) {
  // No standalone exports: NodeContent attachment is wired through
  // flexCardControllerAttach in napi_controller.cc which calls
  // AttachNodeContentById above. This registration point is reserved for
  // future standalone node-content utilities (W9-W10).
  (void)env;
  return exports;
}

}  // namespace flexui::platforms::harmony
