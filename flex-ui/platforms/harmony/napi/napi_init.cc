// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.
//
// Module entry point: register all NAPI methods exposed by FlexUI.
#include <napi/native_api.h>

namespace flexui::platforms::harmony {

extern napi_value RegisterEngineMethods(napi_env env, napi_value exports);
extern napi_value RegisterControllerMethods(napi_env env, napi_value exports);
extern napi_value RegisterNodeContentMethods(napi_env env, napi_value exports);

}  // namespace flexui::platforms::harmony

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  flexui::platforms::harmony::RegisterEngineMethods(env, exports);
  flexui::platforms::harmony::RegisterControllerMethods(env, exports);
  flexui::platforms::harmony::RegisterNodeContentMethods(env, exports);
  return exports;
}
EXTERN_C_END

static napi_module flexui_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "flexui",
    .nm_priv = (void*)0,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterFlexUIModule(void) {
  napi_module_register(&flexui_module);
}
