/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * PoC stub: implementations live in W7-W8+. The interface exists so that
 * Scope plumbing can call it; the default returns empty.
 */
#pragma once

#include <memory>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/core/js-engine/ijs_context.h"

namespace flexui::core::scope_manager {

class ISnapshot {
 public:
  virtual ~ISnapshot() = default;
  virtual std::vector<uint8_t> Serialize() const = 0;
  virtual flexui::common::Error ApplyTo(js_engine::IJsContext& ctx) const = 0;
};

std::unique_ptr<ISnapshot> MakeEmptySnapshot();

}  // namespace flexui::core::scope_manager
