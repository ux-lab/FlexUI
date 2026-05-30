/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/js-engine/ijs_context.h"
#include "flexui/core/scope-manager/scope_state.h"

namespace flexui::core::scope_manager {

class ScopeManager;
class ISnapshot;

class Scope {
 public:
  Scope(ScopeManager* manager,
        uint32_t scope_id,
        std::shared_ptr<js_engine::IJsContext> ctx);
  ~Scope();

  uint32_t id() const { return id_; }
  js_engine::IJsContext* js_context() const { return ctx_.get(); }
  ScopeState state() const { return state_.load(); }

  // Lifecycle. Each transition logs FLEXUI_TLOG(Scope, StateChange, INFO).
  flexui::common::Error Begin(const ISnapshot* snapshot);
  flexui::common::Error MarkRunning();
  flexui::common::Error MarkErrored(const std::string& reason);
  flexui::common::Error Destroy();

 private:
  flexui::common::Error TransitionTo(ScopeState next);

  ScopeManager* manager_;
  uint32_t id_;
  std::shared_ptr<js_engine::IJsContext> ctx_;
  std::atomic<ScopeState> state_{ScopeState::kScopeCreating};
  mutable std::mutex mu_;
};

}  // namespace flexui::core::scope_manager
