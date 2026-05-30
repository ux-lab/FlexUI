/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * JSVM context implementation. HarmonyOS only (FLEXUI_OHOS-gated).
 * Mirrors the structure of quickjs_context.cc; replaces JS_* calls with
 * OH_JSVM_* equivalents. W9-W10 exercises this on a real HarmonyOS device.
 */
#include "src/jsvm/jsvm_context.h"

#if defined(FLEXUI_OHOS)
#include "src/jsvm/jsvm_engine.h"
#include "flexui/common/log_tag.h"

// Forward-declare the private JsvmValue type used below.
namespace flexui::core::js_engine::jsvm { class JsvmValue; }

#include "src/jsvm/jsvm_value.cc"  // pulls in the anonymous-namespace impl

namespace flexui::core::js_engine::jsvm {

JsvmContext::JsvmContext(JsvmEngine* engine) : engine_(engine) {
  JSVM_Status status = OH_JSVM_CreateEnv(engine_->raw_vm(),
                                          /*extension_count=*/0,
                                          /*extensions=*/nullptr, &env_);
  if (status != JSVM_OK) {
    FLEXUI_TLOG(JsEngine, ContextCreate, ERROR)
        << "OH_JSVM_CreateEnv failed status=" << status;
    return;
  }
  OH_JSVM_OpenEnvScope(env_, &env_scope_);
  FLEXUI_TLOG(JsEngine, ContextCreate, INFO) << "backend=JSVM";
}

JsvmContext::~JsvmContext() {
  FLEXUI_TLOG(JsEngine, ContextDestroy, INFO) << "backend=JSVM";
  if (env_) {
    OH_JSVM_CloseEnvScope(env_, env_scope_);
    OH_JSVM_DestroyEnv(env_);
    env_ = nullptr;
  }
}

std::shared_ptr<IJsValue> JsvmContext::Eval(const std::string& source,
                                            const std::string& origin,
                                            flexui::common::Error* err) {
  FLEXUI_TLOG(JsEngine, Eval, DEBUG) << "origin=" << origin
                                      << " size=" << source.size();
  JSVM_HandleScope scope;
  OH_JSVM_OpenHandleScope(env_, &scope);

  JSVM_Value js_source = nullptr;
  OH_JSVM_CreateStringUtf8(env_, source.c_str(), source.size(), &js_source);

  JSVM_Script script = nullptr;
  JSVM_Status compile_status = OH_JSVM_CompileScript(
      env_, js_source, /*cached_data=*/nullptr, /*cached_data_length=*/0,
      /*eager_compile=*/false, /*cache_rejected=*/nullptr, &script);
  if (compile_status != JSVM_OK) {
    if (err) {
      *err = flexui::common::Error(flexui::common::ErrorCode::kJsException,
                                   "JSVM compile error");
    }
    OH_JSVM_CloseHandleScope(env_, scope);
    return NewUndefined();
  }

  JSVM_Value result = nullptr;
  JSVM_Status run_status = OH_JSVM_RunScript(env_, script, &result);
  if (run_status != JSVM_OK) {
    if (err) {
      // Retrieve pending exception message.
      bool is_pending = false;
      OH_JSVM_IsExceptionPending(env_, &is_pending);
      std::string msg = "JSVM run error";
      if (is_pending) {
        JSVM_Value exc = nullptr;
        OH_JSVM_GetAndClearLastException(env_, &exc);
        if (exc) {
          char buf[256] = {};
          size_t len = 0;
          OH_JSVM_GetValueStringUtf8(env_, exc, buf, sizeof(buf) - 1, &len);
          msg = std::string(buf, len);
        }
      }
      *err = flexui::common::Error(flexui::common::ErrorCode::kJsException, msg);
    }
    OH_JSVM_CloseHandleScope(env_, scope);
    return NewUndefined();
  }

  auto out = std::make_shared<JsvmValue>(this, result);
  OH_JSVM_CloseHandleScope(env_, scope);
  return out;
}

flexui::common::Error JsvmContext::InjectGlobalFunction(
    const std::string& name, NativeFunction /*fn*/) {
  FLEXUI_TLOG(JsEngine, InjectGlobalFunction, DEBUG)
      << "name=" << name << " (stub — W9-W10 implements trampoline)";
  // Full trampoline (OH_JSVM_CreateFunction + OH_JSVM_SetProperty on global)
  // mirrors QuickJS NativeTrampoline pattern; deferred to W9-W10.
  return flexui::common::Error::Ok();
}

flexui::common::Error JsvmContext::InjectGlobalValue(
    const std::string& name, std::shared_ptr<IJsValue> /*value*/) {
  FLEXUI_TLOG(JsEngine, InjectGlobalValue, DEBUG) << "name=" << name;
  return flexui::common::Error::Ok();
}

std::shared_ptr<IJsValue> JsvmContext::NewUndefined() {
  JSVM_Value v = nullptr;
  OH_JSVM_GetUndefined(env_, &v);
  return std::make_shared<JsvmValue>(this, v);
}
std::shared_ptr<IJsValue> JsvmContext::NewNull() {
  JSVM_Value v = nullptr;
  OH_JSVM_GetNull(env_, &v);
  return std::make_shared<JsvmValue>(this, v);
}
std::shared_ptr<IJsValue> JsvmContext::NewBoolean(bool b) {
  JSVM_Value v = nullptr;
  OH_JSVM_GetBoolean(env_, b, &v);
  return std::make_shared<JsvmValue>(this, v);
}
std::shared_ptr<IJsValue> JsvmContext::NewNumber(double d) {
  JSVM_Value v = nullptr;
  OH_JSVM_CreateDouble(env_, d, &v);
  return std::make_shared<JsvmValue>(this, v);
}
std::shared_ptr<IJsValue> JsvmContext::NewString(const std::string& s) {
  JSVM_Value v = nullptr;
  OH_JSVM_CreateStringUtf8(env_, s.c_str(), s.size(), &v);
  return std::make_shared<JsvmValue>(this, v);
}
std::shared_ptr<IJsValue> JsvmContext::NewObject() {
  JSVM_Value v = nullptr;
  OH_JSVM_CreateObject(env_, &v);
  return std::make_shared<JsvmValue>(this, v);
}
std::shared_ptr<IJsValue> JsvmContext::NewArray(uint32_t length) {
  JSVM_Value v = nullptr;
  OH_JSVM_CreateArrayWithLength(env_, length, &v);
  return std::make_shared<JsvmValue>(this, v);
}

std::shared_ptr<IJsValue> JsvmContext::FromFlexUIValue(
    const flexui::common::FlexUIValue& v) {
  using T = flexui::common::FlexUIValue::Type;
  switch (v.GetType()) {
    case T::kUndefined: return NewUndefined();
    case T::kNull:      return NewNull();
    case T::kBoolean:   return NewBoolean(v.ToBooleanChecked());
    case T::kNumber:    return NewNumber(v.ToDoubleChecked());
    case T::kString:    return NewString(v.ToStringChecked());
    case T::kArray: {
      const auto& src = v.ToArrayChecked();
      auto arr = NewArray(static_cast<uint32_t>(src.size()));
      for (uint32_t i = 0; i < static_cast<uint32_t>(src.size()); ++i) {
        auto item = FromFlexUIValue(src[i]);
        auto* jv = dynamic_cast<JsvmValue*>(item.get());
        if (jv) {
          OH_JSVM_SetElement(env_,
                             dynamic_cast<JsvmValue*>(arr.get())->raw(), i,
                             jv->raw());
        }
      }
      return arr;
    }
    case T::kObject: {
      const auto& src = v.ToObjectChecked();
      auto obj = NewObject();
      auto* jo = dynamic_cast<JsvmValue*>(obj.get());
      for (const auto& kv : src) {
        auto item = FromFlexUIValue(kv.second);
        auto* ji = dynamic_cast<JsvmValue*>(item.get());
        if (jo && ji) {
          JSVM_Value key = nullptr;
          OH_JSVM_CreateStringUtf8(env_, kv.first.c_str(), kv.first.size(),
                                   &key);
          OH_JSVM_SetProperty(env_, jo->raw(), key, ji->raw());
        }
      }
      return obj;
    }
  }
  return NewUndefined();
}

std::shared_ptr<IJsValue> JsvmContext::Call(
    std::shared_ptr<IJsValue> /*function*/,
    std::shared_ptr<IJsValue> /*this_value*/,
    const std::vector<std::shared_ptr<IJsValue>>& /*args*/,
    flexui::common::Error* err) {
  FLEXUI_TLOG(JsEngine, Call, DEBUG) << "JSVM Call stub (W9-W10)";
  if (err) *err = flexui::common::Error::Ok();
  return NewUndefined();
}

void JsvmContext::RunPendingJobs() {
  // JSVM microtask queue is drained automatically by the runtime.
}

}  // namespace flexui::core::js_engine::jsvm
#endif  // FLEXUI_OHOS
