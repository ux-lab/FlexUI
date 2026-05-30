#include "src/quickjs/quickjs_context.h"
#include "src/quickjs/quickjs_engine.h"
#include "src/quickjs/quickjs_value.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::js_engine::quickjs {

namespace {
// C trampoline: JS calls us, we dispatch to the stored NativeFunction.
JSValue NativeTrampoline(JSContext* qjs, JSValueConst this_val,
                         int argc, JSValueConst* argv,
                         int magic, JSValue* func_data) {
  auto* ctx = static_cast<QuickJSContext*>(JS_GetContextOpaque(qjs));
  uint32_t id = 0;
  JS_ToUint32(qjs, &id, func_data[0]);
  auto* fn = ctx->LookupNative(id);
  if (!fn) return JS_ThrowInternalError(qjs, "FlexUI: native id %u not found", id);

  std::vector<std::shared_ptr<IJsValue>> args;
  args.reserve(static_cast<size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    args.push_back(std::make_shared<QuickJSValue>(
        ctx, JS_DupValue(qjs, argv[i])));
  }
  auto result = (*fn)(*ctx, args);
  if (!result) return JS_UNDEFINED;
  auto* qr = dynamic_cast<QuickJSValue*>(result.get());
  return qr ? JS_DupValue(qjs, qr->raw()) : JS_UNDEFINED;
}
}  // namespace

QuickJSContext::QuickJSContext(QuickJSEngine* engine, JSContext* qjs)
    : engine_(engine), qjs_(qjs) {
  JS_SetContextOpaque(qjs_, this);
  FLEXUI_TLOG(JsEngine, ContextCreate, INFO) << "backend=QuickJS";
}

QuickJSContext::~QuickJSContext() {
  FLEXUI_TLOG(JsEngine, ContextDestroy, INFO) << "backend=QuickJS";
  JS_FreeContext(qjs_);
}

std::shared_ptr<IJsValue> QuickJSContext::Eval(const std::string& source,
                                               const std::string& origin,
                                               flexui::common::Error* err) {
  FLEXUI_TLOG(JsEngine, Eval, DEBUG) << "origin=" << origin
                                     << " size=" << source.size();
  JSValue v = JS_Eval(qjs_, source.c_str(), source.size(),
                      origin.c_str(), JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(v)) {
    JSValue exc = JS_GetException(qjs_);
    const char* msg = JS_ToCString(qjs_, exc);
    if (err) *err = flexui::common::Error(
        flexui::common::ErrorCode::kJsException, msg ? msg : "<no message>");
    if (msg) JS_FreeCString(qjs_, msg);
    JS_FreeValue(qjs_, exc);
    JS_FreeValue(qjs_, v);
    return std::make_shared<QuickJSValue>(this, JS_UNDEFINED);
  }
  return std::make_shared<QuickJSValue>(this, v);
}

flexui::common::Error QuickJSContext::InjectGlobalFunction(
    const std::string& name, NativeFunction fn) {
  uint32_t id = next_native_id_++;
  native_fns_[id] = std::move(fn);

  JSValue id_v = JS_NewUint32(qjs_, id);
  JSValue func = JS_NewCFunctionData(qjs_, &NativeTrampoline,
                                     /*length=*/0, /*magic=*/0,
                                     /*data_len=*/1, &id_v);
  JS_FreeValue(qjs_, id_v);

  JSValue global = JS_GetGlobalObject(qjs_);
  JS_SetPropertyStr(qjs_, global, name.c_str(), func);
  JS_FreeValue(qjs_, global);
  FLEXUI_TLOG(JsEngine, InjectGlobalFunction, DEBUG)
      << "name=" << name << " id=" << id;
  return flexui::common::Error::Ok();
}

flexui::common::Error QuickJSContext::InjectGlobalValue(
    const std::string& name, std::shared_ptr<IJsValue> value) {
  auto* qv = dynamic_cast<QuickJSValue*>(value.get());
  if (!qv) return flexui::common::Error(
      flexui::common::ErrorCode::kInvalidArgument, "value not QuickJSValue");
  JSValue global = JS_GetGlobalObject(qjs_);
  JS_SetPropertyStr(qjs_, global, name.c_str(), JS_DupValue(qjs_, qv->raw()));
  JS_FreeValue(qjs_, global);
  return flexui::common::Error::Ok();
}

std::shared_ptr<IJsValue> QuickJSContext::NewUndefined() {
  return std::make_shared<QuickJSValue>(this, JS_UNDEFINED);
}
std::shared_ptr<IJsValue> QuickJSContext::NewNull() {
  return std::make_shared<QuickJSValue>(this, JS_NULL);
}
std::shared_ptr<IJsValue> QuickJSContext::NewBoolean(bool v) {
  return std::make_shared<QuickJSValue>(this, JS_NewBool(qjs_, v));
}
std::shared_ptr<IJsValue> QuickJSContext::NewNumber(double v) {
  return std::make_shared<QuickJSValue>(this, JS_NewFloat64(qjs_, v));
}
std::shared_ptr<IJsValue> QuickJSContext::NewString(const std::string& v) {
  return std::make_shared<QuickJSValue>(this,
                                        JS_NewStringLen(qjs_, v.data(), v.size()));
}
std::shared_ptr<IJsValue> QuickJSContext::NewObject() {
  return std::make_shared<QuickJSValue>(this, JS_NewObject(qjs_));
}
std::shared_ptr<IJsValue> QuickJSContext::NewArray(uint32_t length) {
  JSValue arr = JS_NewArray(qjs_);
  JS_SetPropertyStr(qjs_, arr, "length", JS_NewUint32(qjs_, length));
  return std::make_shared<QuickJSValue>(this, arr);
}

std::shared_ptr<IJsValue> QuickJSContext::FromFlexUIValue(
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
      auto* qa = dynamic_cast<QuickJSValue*>(arr.get());
      for (uint32_t i = 0; i < static_cast<uint32_t>(src.size()); ++i) {
        auto item = FromFlexUIValue(src[i]);
        auto* qi = dynamic_cast<QuickJSValue*>(item.get());
        JS_SetPropertyUint32(qjs_, qa->raw(), i,
                             JS_DupValue(qjs_, qi->raw()));
      }
      return arr;
    }
    case T::kObject: {
      const auto& src = v.ToObjectChecked();
      auto obj = NewObject();
      auto* qo = dynamic_cast<QuickJSValue*>(obj.get());
      for (const auto& kv : src) {
        auto item = FromFlexUIValue(kv.second);
        auto* qi = dynamic_cast<QuickJSValue*>(item.get());
        JS_SetPropertyStr(qjs_, qo->raw(), kv.first.c_str(),
                          JS_DupValue(qjs_, qi->raw()));
      }
      return obj;
    }
  }
  return NewUndefined();
}

std::shared_ptr<IJsValue> QuickJSContext::Call(
    std::shared_ptr<IJsValue> function,
    std::shared_ptr<IJsValue> this_value,
    const std::vector<std::shared_ptr<IJsValue>>& args,
    flexui::common::Error* err) {
  auto* qf = dynamic_cast<QuickJSValue*>(function.get());
  auto* qt = dynamic_cast<QuickJSValue*>(this_value.get());
  if (!qf) {
    if (err) *err = flexui::common::Error(
        flexui::common::ErrorCode::kInvalidArgument, "function not QuickJSValue");
    return NewUndefined();
  }
  std::vector<JSValue> qa;
  qa.reserve(args.size());
  for (auto& a : args) {
    auto* qv = dynamic_cast<QuickJSValue*>(a.get());
    qa.push_back(qv ? qv->raw() : JS_UNDEFINED);
  }
  JSValue ret = JS_Call(qjs_, qf->raw(), qt ? qt->raw() : JS_UNDEFINED,
                        static_cast<int>(qa.size()), qa.data());
  if (JS_IsException(ret)) {
    JSValue exc = JS_GetException(qjs_);
    const char* msg = JS_ToCString(qjs_, exc);
    if (err) *err = flexui::common::Error(
        flexui::common::ErrorCode::kJsException, msg ? msg : "<no message>");
    if (msg) JS_FreeCString(qjs_, msg);
    JS_FreeValue(qjs_, exc);
  }
  return std::make_shared<QuickJSValue>(this, ret);
}

void QuickJSContext::RunPendingJobs() {
  JSContext* pending = nullptr;
  while (JS_ExecutePendingJob(JS_GetRuntime(qjs_), &pending) > 0) {
    // drain microtask queue
  }
}

NativeFunction* QuickJSContext::LookupNative(uint32_t id) {
  auto it = native_fns_.find(id);
  return it == native_fns_.end() ? nullptr : &it->second;
}

}  // namespace flexui::core::js_engine::quickjs
