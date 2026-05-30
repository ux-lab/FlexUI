/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * JsvmValue — concrete IJsValue backed by a JSVM_Value handle.
 * HarmonyOS only (FLEXUI_OHOS-gated).
 *
 * This file is #include-d directly by jsvm_context.cc so the class
 * definition is private to that translation unit; no other TU needs it.
 */
#if defined(FLEXUI_OHOS)

namespace flexui::core::js_engine::jsvm {

class JsvmValue : public IJsValue {
 public:
  JsvmValue(JsvmContext* ctx, JSVM_Value v) : ctx_(ctx), value_(v) {}
  ~JsvmValue() override = default;  // JSVM values are GC-managed

  JSVM_Value raw() const { return value_; }

  bool IsUndefined() const override {
    JSVM_ValueType t = JSVM_UNDEFINED;
    OH_JSVM_Typeof(ctx_->env(), value_, &t);
    return t == JSVM_UNDEFINED;
  }
  bool IsNull() const override {
    bool b = false; OH_JSVM_IsNull(ctx_->env(), value_, &b); return b;
  }
  bool IsBoolean() const override {
    JSVM_ValueType t = JSVM_UNDEFINED;
    OH_JSVM_Typeof(ctx_->env(), value_, &t);
    return t == JSVM_BOOLEAN;
  }
  bool IsNumber() const override {
    JSVM_ValueType t = JSVM_UNDEFINED;
    OH_JSVM_Typeof(ctx_->env(), value_, &t);
    return t == JSVM_NUMBER;
  }
  bool IsString() const override {
    JSVM_ValueType t = JSVM_UNDEFINED;
    OH_JSVM_Typeof(ctx_->env(), value_, &t);
    return t == JSVM_STRING;
  }
  bool IsObject() const override {
    JSVM_ValueType t = JSVM_UNDEFINED;
    OH_JSVM_Typeof(ctx_->env(), value_, &t);
    return t == JSVM_OBJECT;
  }
  bool IsArray() const override {
    bool b = false; OH_JSVM_IsArray(ctx_->env(), value_, &b); return b;
  }
  bool IsFunction() const override {
    JSVM_ValueType t = JSVM_UNDEFINED;
    OH_JSVM_Typeof(ctx_->env(), value_, &t);
    return t == JSVM_FUNCTION;
  }
  bool IsException() const override { return false; }  // JSVM uses status

  bool ToBoolean() const override {
    bool b = false;
    OH_JSVM_GetValueBool(ctx_->env(), value_, &b);
    return b;
  }
  double ToNumber() const override {
    double d = 0;
    OH_JSVM_GetValueDouble(ctx_->env(), value_, &d);
    return d;
  }
  std::string ToStdString() const override {
    char buf[4096] = {};
    size_t len = 0;
    OH_JSVM_GetValueStringUtf8(ctx_->env(), value_, buf, sizeof(buf) - 1, &len);
    return std::string(buf, len);
  }

  std::shared_ptr<IJsValue> GetProperty(const std::string& name) const override {
    JSVM_Value key = nullptr;
    OH_JSVM_CreateStringUtf8(ctx_->env(), name.c_str(), name.size(), &key);
    JSVM_Value out = nullptr;
    OH_JSVM_GetProperty(ctx_->env(), value_, key, &out);
    return std::make_shared<JsvmValue>(ctx_, out);
  }
  void SetProperty(const std::string& name,
                   std::shared_ptr<IJsValue> value) override {
    auto* jv = dynamic_cast<JsvmValue*>(value.get());
    if (!jv) return;
    JSVM_Value key = nullptr;
    OH_JSVM_CreateStringUtf8(ctx_->env(), name.c_str(), name.size(), &key);
    OH_JSVM_SetProperty(ctx_->env(), value_, key, jv->raw());
  }
  std::vector<std::string> GetPropertyNames() const override {
    return {};  // stub — full impl W9-W10
  }
  uint32_t GetArrayLength() const override {
    uint32_t len = 0;
    OH_JSVM_GetArrayLength(ctx_->env(), value_, &len);
    return len;
  }
  std::shared_ptr<IJsValue> GetArrayItem(uint32_t index) const override {
    JSVM_Value out = nullptr;
    OH_JSVM_GetElement(ctx_->env(), value_, index, &out);
    return std::make_shared<JsvmValue>(ctx_, out);
  }
  flexui::common::FlexUIValue ToFlexUIValue() const override {
    if (IsUndefined() || IsNull()) return flexui::common::FlexUIValue();
    if (IsBoolean()) return flexui::common::FlexUIValue(ToBoolean());
    if (IsNumber())  return flexui::common::FlexUIValue(ToNumber());
    if (IsString())  return flexui::common::FlexUIValue(ToStdString());
    // Array/Object conversion stub — full impl W9-W10
    return flexui::common::FlexUIValue();
  }

 private:
  JsvmContext* ctx_;    // borrowed
  JSVM_Value value_;    // GC-managed; no manual free needed
};

}  // namespace flexui::core::js_engine::jsvm

#endif  // FLEXUI_OHOS
