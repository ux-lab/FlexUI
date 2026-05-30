/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Minimal IJsEngine / IJsContext / IJsValue fakes for unit tests.
 * Does NOT execute JavaScript; records Eval calls and returns stub values.
 */
#pragma once

#include <string>
#include <vector>

#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/js-engine/ijs_context.h"
#include "flexui/core/js-engine/ijs_value.h"

namespace flexui::core::js_engine::test {

class FakeJsValue : public IJsValue {
 public:
  bool IsUndefined() const override { return true; }
  bool IsNull() const override { return false; }
  bool IsBoolean() const override { return false; }
  bool IsNumber() const override { return false; }
  bool IsString() const override { return false; }
  bool IsObject() const override { return false; }
  bool IsArray() const override { return false; }
  bool IsFunction() const override { return false; }
  bool IsException() const override { return false; }

  bool ToBoolean() const override { return false; }
  double ToNumber() const override { return 0.0; }
  std::string ToStdString() const override { return ""; }

  std::shared_ptr<IJsValue> GetProperty(const std::string&) const override;
  void SetProperty(const std::string&, std::shared_ptr<IJsValue>) override {}
  std::vector<std::string> GetPropertyNames() const override { return {}; }
  uint32_t GetArrayLength() const override { return 0; }
  std::shared_ptr<IJsValue> GetArrayItem(uint32_t) const override;

  flexui::common::FlexUIValue ToFlexUIValue() const override {
    return flexui::common::FlexUIValue();
  }
};

class FakeJsContext : public IJsContext {
 public:
  int eval_call_count = 0;

  std::shared_ptr<IJsValue> Eval(const std::string& source,
                                  const std::string& origin,
                                  flexui::common::Error* err) override;

  flexui::common::Error InjectGlobalFunction(
      const std::string&, NativeFunction) override {
    return flexui::common::Error::Ok();
  }
  flexui::common::Error InjectGlobalValue(
      const std::string&, std::shared_ptr<IJsValue>) override {
    return flexui::common::Error::Ok();
  }

  std::shared_ptr<IJsValue> NewUndefined() override;
  std::shared_ptr<IJsValue> NewNull() override;
  std::shared_ptr<IJsValue> NewBoolean(bool) override;
  std::shared_ptr<IJsValue> NewNumber(double) override;
  std::shared_ptr<IJsValue> NewString(const std::string&) override;
  std::shared_ptr<IJsValue> NewObject() override;
  std::shared_ptr<IJsValue> NewArray(uint32_t) override;
  std::shared_ptr<IJsValue> FromFlexUIValue(
      const flexui::common::FlexUIValue&) override;

  std::shared_ptr<IJsValue> Call(
      std::shared_ptr<IJsValue>,
      std::shared_ptr<IJsValue>,
      const std::vector<std::shared_ptr<IJsValue>>&,
      flexui::common::Error*) override;

  void RunPendingJobs() override {}
};

class FakeJsEngine : public IJsEngine {
 public:
  flexui::common::Error Initialize(const EngineConfig&) override {
    return flexui::common::Error::Ok();
  }
  std::shared_ptr<IJsContext> CreateContext() override;
  void Shutdown() override {}
  const char* BackendName() const override { return "Fake"; }
};

}  // namespace flexui::core::js_engine::test
