#pragma once

#include "flexui/core/js-engine/ijs_engine.h"
#include "quickjs.h"

namespace flexui::core::js_engine::quickjs {

class QuickJSEngine : public IJsEngine {
 public:
  QuickJSEngine();
  ~QuickJSEngine() override;

  flexui::common::Error Initialize(const EngineConfig& cfg) override;
  std::shared_ptr<IJsContext> CreateContext() override;
  void Shutdown() override;
  const char* BackendName() const override { return "QuickJS"; }

  JSRuntime* raw_rt() const { return rt_; }

 private:
  JSRuntime* rt_ = nullptr;
};

}  // namespace flexui::core::js_engine::quickjs
