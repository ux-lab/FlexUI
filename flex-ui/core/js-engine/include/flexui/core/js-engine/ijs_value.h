/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Engine-agnostic JS value handle. Backed by an engine-specific implementation
 * obtained through IJsContext. Owns one engine-internal reference; releases
 * on destruction.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::js_engine {

class IJsContext;

class IJsValue {
 public:
  virtual ~IJsValue() = default;

  virtual bool IsUndefined() const = 0;
  virtual bool IsNull() const = 0;
  virtual bool IsBoolean() const = 0;
  virtual bool IsNumber() const = 0;
  virtual bool IsString() const = 0;
  virtual bool IsObject() const = 0;
  virtual bool IsArray() const = 0;
  virtual bool IsFunction() const = 0;
  virtual bool IsException() const = 0;

  virtual bool ToBoolean() const = 0;
  virtual double ToNumber() const = 0;
  virtual std::string ToStdString() const = 0;

  // Object/array property access. Returns a new IJsValue (caller owns).
  virtual std::shared_ptr<IJsValue> GetProperty(const std::string& name) const = 0;
  virtual void SetProperty(const std::string& name,
                           std::shared_ptr<IJsValue> value) = 0;
  virtual std::vector<std::string> GetPropertyNames() const = 0;
  virtual uint32_t GetArrayLength() const = 0;
  virtual std::shared_ptr<IJsValue> GetArrayItem(uint32_t index) const = 0;

  // Cross-engine adapter: convert engine value to FlexUIValue.
  virtual flexui::common::FlexUIValue ToFlexUIValue() const = 0;
};

}  // namespace flexui::core::js_engine
