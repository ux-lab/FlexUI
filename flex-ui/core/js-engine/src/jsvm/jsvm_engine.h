/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * JSVM backend for IJsEngine. HarmonyOS only — compiled when FLEXUI_OHOS is
 * defined. On all other platforms this header exposes nothing.
 */
#pragma once
#if defined(FLEXUI_OHOS)

#include "flexui/core/js-engine/ijs_engine.h"
#include <ark_runtime/jsvm.h>

namespace flexui::core::js_engine::jsvm {

class JsvmEngine : public IJsEngine {
 public:
  JsvmEngine();
  ~JsvmEngine() override;

  flexui::common::Error Initialize(const EngineConfig& cfg) override;
  std::shared_ptr<IJsContext> CreateContext() override;
  void Shutdown() override;
  const char* BackendName() const override { return "JSVM"; }

  JSVM_VM raw_vm() const { return vm_; }

 private:
  JSVM_VM vm_ = nullptr;
};

}  // namespace flexui::core::js_engine::jsvm

#endif  // FLEXUI_OHOS
