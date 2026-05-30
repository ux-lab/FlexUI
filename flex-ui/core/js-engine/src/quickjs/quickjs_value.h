/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include "flexui/core/js-engine/ijs_value.h"
#include "quickjs.h"

namespace flexui::core::js_engine::quickjs {

class QuickJSContext;

class QuickJSValue : public IJsValue {
 public:
  QuickJSValue(QuickJSContext* ctx, JSValue v);
  ~QuickJSValue() override;

  QuickJSValue(const QuickJSValue&) = delete;
  QuickJSValue& operator=(const QuickJSValue&) = delete;

  bool IsUndefined() const override;
  bool IsNull() const override;
  bool IsBoolean() const override;
  bool IsNumber() const override;
  bool IsString() const override;
  bool IsObject() const override;
  bool IsArray() const override;
  bool IsFunction() const override;
  bool IsException() const override;

  bool ToBoolean() const override;
  double ToNumber() const override;
  std::string ToStdString() const override;

  std::shared_ptr<IJsValue> GetProperty(const std::string& name) const override;
  void SetProperty(const std::string& name,
                   std::shared_ptr<IJsValue> value) override;
  std::vector<std::string> GetPropertyNames() const override;
  uint32_t GetArrayLength() const override;
  std::shared_ptr<IJsValue> GetArrayItem(uint32_t index) const override;

  flexui::common::FlexUIValue ToFlexUIValue() const override;

  JSValue raw() const { return value_; }
  JSContext* qjs() const;

 private:
  QuickJSContext* ctx_;  // borrowed
  JSValue value_;        // owned (JS_DupValue / JS_FreeValue)
};

}  // namespace flexui::core::js_engine::quickjs
