#pragma once

#include "flexui/core/js-engine/ijs_context.h"
#include "quickjs.h"

#include <unordered_map>

namespace flexui::core::js_engine::quickjs {

class QuickJSEngine;

class QuickJSContext : public IJsContext {
 public:
  QuickJSContext(QuickJSEngine* engine, JSContext* qjs);
  ~QuickJSContext() override;

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

  JSContext* raw_ctx() const { return qjs_; }

  // Used by the QuickJS C dispatch trampoline to look up native fn.
  NativeFunction* LookupNative(uint32_t id);

 private:
  QuickJSEngine* engine_;
  JSContext* qjs_;
  std::unordered_map<uint32_t, NativeFunction> native_fns_;
  uint32_t next_native_id_ = 1;
};

}  // namespace flexui::core::js_engine::quickjs
