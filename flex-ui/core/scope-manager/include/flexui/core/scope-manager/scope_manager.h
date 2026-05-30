/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/scope-manager/scope.h"

namespace flexui::core::scope_manager {

class ScopeManager {
 public:
  explicit ScopeManager(std::shared_ptr<js_engine::IJsEngine> engine);
  ~ScopeManager();

  // Allocates a Scope id, creates a JS context, returns the Scope handle.
  std::shared_ptr<Scope> CreateScope();

  // Find / destroy by id.
  std::shared_ptr<Scope> Find(uint32_t id) const;
  flexui::common::Error DestroyScope(uint32_t id);

  size_t ScopeCount() const;

 private:
  std::shared_ptr<js_engine::IJsEngine> engine_;
  mutable std::mutex mu_;
  std::unordered_map<uint32_t, std::shared_ptr<Scope>> scopes_;
  std::atomic<uint32_t> next_id_{1};
};

}  // namespace flexui::core::scope_manager
