/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

namespace flexui::core::scope_manager {

enum class ScopeState {
  kEngineInitd,
  kScopeCreating,
  kScopeRunning,
  kScopeDestroying,
  kScopeDestroyed,
  kScopeErrored,
};

const char* ScopeStateName(ScopeState s);

}  // namespace flexui::core::scope_manager
