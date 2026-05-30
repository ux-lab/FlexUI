/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "src/quickjs/quickjs_value.h"
#include "src/quickjs/quickjs_context.h"

#include <cstring>
#include <utility>

#include "flexui/common/log_tag.h"

namespace flexui::core::js_engine::quickjs {

QuickJSValue::QuickJSValue(QuickJSContext* ctx, JSValue v) : ctx_(ctx), value_(v) {}

QuickJSValue::~QuickJSValue() {
  JS_FreeValue(qjs(), value_);
}

JSContext* QuickJSValue::qjs() const { return ctx_->raw_ctx(); }

bool QuickJSValue::IsUndefined() const { return JS_IsUndefined(value_); }
bool QuickJSValue::IsNull() const      { return JS_IsNull(value_); }
bool QuickJSValue::IsBoolean() const   { return JS_IsBool(value_); }
bool QuickJSValue::IsNumber() const    { return JS_IsNumber(value_); }
bool QuickJSValue::IsString() const    { return JS_IsString(value_); }
bool QuickJSValue::IsObject() const    { return JS_IsObject(value_); }
bool QuickJSValue::IsArray() const     { return JS_IsArray(qjs(), value_) == 1; }
bool QuickJSValue::IsFunction() const  { return JS_IsFunction(qjs(), value_); }
bool QuickJSValue::IsException() const { return JS_IsException(value_); }

bool QuickJSValue::ToBoolean() const {
  return JS_ToBool(qjs(), value_) != 0;
}

double QuickJSValue::ToNumber() const {
  double d = 0;
  JS_ToFloat64(qjs(), &d, value_);
  return d;
}

std::string QuickJSValue::ToStdString() const {
  size_t len = 0;
  const char* p = JS_ToCStringLen(qjs(), &len, value_);
  if (!p) return "";
  std::string out(p, len);
  JS_FreeCString(qjs(), p);
  return out;
}

std::shared_ptr<IJsValue> QuickJSValue::GetProperty(const std::string& name) const {
  JSValue v = JS_GetPropertyStr(qjs(), value_, name.c_str());
  return std::make_shared<QuickJSValue>(ctx_, v);
}

void QuickJSValue::SetProperty(const std::string& name,
                               std::shared_ptr<IJsValue> value) {
  auto* qv = dynamic_cast<QuickJSValue*>(value.get());
  if (!qv) {
    FLEXUI_TLOG(JsEngine, SetProperty, ERROR) << "non-QuickJSValue passed";
    return;
  }
  JSValue dup = JS_DupValue(qjs(), qv->value_);
  JS_SetPropertyStr(qjs(), value_, name.c_str(), dup);
}

std::vector<std::string> QuickJSValue::GetPropertyNames() const {
  std::vector<std::string> names;
  JSPropertyEnum* tab = nullptr;
  uint32_t len = 0;
  if (JS_GetOwnPropertyNames(qjs(), &tab, &len, value_,
                             JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
    for (uint32_t i = 0; i < len; ++i) {
      const char* s = JS_AtomToCString(qjs(), tab[i].atom);
      if (s) {
        names.emplace_back(s);
        JS_FreeCString(qjs(), s);
      }
      JS_FreeAtom(qjs(), tab[i].atom);
    }
    js_free(qjs(), tab);
  }
  return names;
}

uint32_t QuickJSValue::GetArrayLength() const {
  JSValue len = JS_GetPropertyStr(qjs(), value_, "length");
  uint32_t out = 0;
  JS_ToUint32(qjs(), &out, len);
  JS_FreeValue(qjs(), len);
  return out;
}

std::shared_ptr<IJsValue> QuickJSValue::GetArrayItem(uint32_t index) const {
  JSValue v = JS_GetPropertyUint32(qjs(), value_, index);
  return std::make_shared<QuickJSValue>(ctx_, v);
}

flexui::common::FlexUIValue QuickJSValue::ToFlexUIValue() const {
  if (IsUndefined() || IsNull()) return flexui::common::FlexUIValue();
  if (IsBoolean()) return flexui::common::FlexUIValue(ToBoolean());
  if (IsNumber())  return flexui::common::FlexUIValue(ToNumber());
  if (IsString())  return flexui::common::FlexUIValue(ToStdString());

  if (IsArray()) {
    flexui::common::FlexUIValue::FlexUIValueArrayType arr;
    uint32_t n = GetArrayLength();
    for (uint32_t i = 0; i < n; ++i) {
      arr.push_back(GetArrayItem(i)->ToFlexUIValue());
    }
    return flexui::common::FlexUIValue(std::move(arr));
  }

  if (IsObject()) {
    flexui::common::FlexUIValue::FlexUIValueObjectType obj;
    for (const auto& key : GetPropertyNames()) {
      obj[key] = GetProperty(key)->ToFlexUIValue();
    }
    return flexui::common::FlexUIValue(std::move(obj));
  }

  return flexui::common::FlexUIValue();
}

}  // namespace flexui::core::js_engine::quickjs
