/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

namespace flexui::core::card_controller {

enum class FlexCardState {
  kIdle,
  kLoading,
  kRunning,
  kErrored,
  kDestroyed,
};

const char* FlexCardStateName(FlexCardState s);

}  // namespace flexui::core::card_controller
