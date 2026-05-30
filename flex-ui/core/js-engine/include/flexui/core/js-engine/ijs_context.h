/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Per-card JS execution context. One instance per FlexCardController in W5-W6,
 * but at this layer we expose only the engine-level primitive. Higher-level
 * Scope (lifecycle, snapshot, plugin-API injection) lives in core/scope-manager.
 */
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/js-engine/ijs_value.h"

namespace flexui::core::js_engine {

using NativeFunction = std::function<
    std::shared_ptr<IJsValue>(IJsContext& ctx,
                              const std::vector<std::shared_ptr<IJsValue>>& args)>;

class IJsContext {
 public:
  virtual ~IJsContext() = default;

  // Evaluate JS source. Returns the result value, or sets *err on exception.
  virtual std::shared_ptr<IJsValue> Eval(const std::string& source,
                                         const std::string& origin,
                                         flexui::common::Error* err) = 0;

  // Inject a native function into globalThis.
  virtual flexui::common::Error InjectGlobalFunction(const std::string& name,
                                                    NativeFunction fn) = 0;

  // Inject an arbitrary value into globalThis.
  virtual flexui::common::Error InjectGlobalValue(const std::string& name,
                                                  std::shared_ptr<IJsValue> value) = 0;

  // Build engine values.
  virtual std::shared_ptr<IJsValue> NewUndefined() = 0;
  virtual std::shared_ptr<IJsValue> NewNull() = 0;
  virtual std::shared_ptr<IJsValue> NewBoolean(bool v) = 0;
  virtual std::shared_ptr<IJsValue> NewNumber(double v) = 0;
  virtual std::shared_ptr<IJsValue> NewString(const std::string& v) = 0;
  virtual std::shared_ptr<IJsValue> NewObject() = 0;
  virtual std::shared_ptr<IJsValue> NewArray(uint32_t length) = 0;
  virtual std::shared_ptr<IJsValue> FromFlexUIValue(
      const flexui::common::FlexUIValue& v) = 0;

  // Call a function value.
  virtual std::shared_ptr<IJsValue> Call(
      std::shared_ptr<IJsValue> function,
      std::shared_ptr<IJsValue> this_value,
      const std::vector<std::shared_ptr<IJsValue>>& args,
      flexui::common::Error* err) = 0;

  // Drive microtask queue (Promise resolution).
  virtual void RunPendingJobs() = 0;
};

}  // namespace flexui::core::js_engine
