/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "src/jsvm/jsvm_engine.h"

#if defined(FLEXUI_OHOS)
#include "src/jsvm/jsvm_context.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::js_engine::jsvm {

JsvmEngine::JsvmEngine() = default;
JsvmEngine::~JsvmEngine() { Shutdown(); }

flexui::common::Error JsvmEngine::Initialize(const EngineConfig& cfg) {
  FLEXUI_TLOG(JsEngine, EngineInit, INFO) << "backend=JSVM";
  JSVM_InitOptions init_options = {};
  if (OH_JSVM_Init(&init_options) != JSVM_OK) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal,
                                 "OH_JSVM_Init failed");
  }
  JSVM_CreateVMOptions vm_options = {};
  if (cfg.memory_limit_mb > 0) {
    vm_options.maxOldGenerationSize = cfg.memory_limit_mb;
  }
  if (OH_JSVM_CreateVM(&vm_options, &vm_) != JSVM_OK) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal,
                                 "OH_JSVM_CreateVM failed");
  }
  return flexui::common::Error::Ok();
}

std::shared_ptr<IJsContext> JsvmEngine::CreateContext() {
  if (!vm_) {
    FLEXUI_TLOG(JsEngine, CreateContext, ERROR) << "vm not initialized";
    return nullptr;
  }
  FLEXUI_TLOG(JsEngine, CreateContext, INFO) << "backend=JSVM";
  return std::make_shared<JsvmContext>(this);
}

void JsvmEngine::Shutdown() {
  if (vm_) {
    FLEXUI_TLOG(JsEngine, EngineShutdown, INFO) << "backend=JSVM";
    OH_JSVM_DestroyVM(vm_);
    vm_ = nullptr;
  }
}

}  // namespace flexui::core::js_engine::jsvm
#endif  // FLEXUI_OHOS
