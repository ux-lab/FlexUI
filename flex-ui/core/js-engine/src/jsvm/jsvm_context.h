/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * JSVM execution context. HarmonyOS only.
 */
#pragma once
#if defined(FLEXUI_OHOS)

#include "flexui/core/js-engine/ijs_context.h"
#include <ark_runtime/jsvm.h>

namespace flexui::core::js_engine::jsvm {

class JsvmEngine;

class JsvmContext : public IJsContext {
 public:
  explicit JsvmContext(JsvmEngine* engine);
  ~JsvmContext() override;

  std::shared_ptr<IJsValue> Eval(const std::string& source,
                                 const std::string& origin,
                                 flexui::common::Error* err) override;
  flexui::common::Error InjectGlobalFunction(const std::string& name,
                                             NativeFunction fn) override;
  flexui::common::Error InjectGlobalValue(const std::string& name,
                                          std::shared_ptr<IJsValue> value) override;

  std::shared_ptr<IJsValue> NewUndefined() override;
  std::shared_ptr<IJsValue> NewNull() override;
  std::shared_ptr<IJsValue> NewBoolean(bool v) override;
  std::shared_ptr<IJsValue> NewNumber(double v) override;
  std::shared_ptr<IJsValue> NewString(const std::string& v) override;
  std::shared_ptr<IJsValue> NewObject() override;
  std::shared_ptr<IJsValue> NewArray(uint32_t length) override;
  std::shared_ptr<IJsValue> FromFlexUIValue(
      const flexui::common::FlexUIValue& v) override;

  std::shared_ptr<IJsValue> Call(
      std::shared_ptr<IJsValue> function,
      std::shared_ptr<IJsValue> this_value,
      const std::vector<std::shared_ptr<IJsValue>>& args,
      flexui::common::Error* err) override;

  void RunPendingJobs() override;

  JSVM_Env env() const { return env_; }

 private:
  JsvmEngine* engine_;    // borrowed
  JSVM_Env env_ = nullptr;
  JSVM_EnvScope env_scope_{};
};

}  // namespace flexui::core::js_engine::jsvm

#endif  // FLEXUI_OHOS
