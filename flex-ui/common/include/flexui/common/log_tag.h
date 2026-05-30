/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * FLEXUI_TLOG: the canonical structured-log entry point for all FlexUI
 * subsystem code. See docs/superpowers/specs/2026-05-30-flexui-layering-design.md
 * §8 for the tag convention and mandatory coverage points.
 *
 * Usage:
 *   FLEXUI_TLOG(Engine, Init, INFO) << "config=" << cfg.summary();
 *   FLEXUI_TLOG(Scope, StateChange, DEBUG) << "from=" << from << " to=" << to;
 */
#pragma once

#include <sstream>
#include <string>

#include "flexui/common/log_level.h"
#include "flexui/common/log_sink.h"

namespace flexui::common {

class TaggedLogStream {
 public:
  TaggedLogStream(LogSeverity level, const char* subsystem, const char* event)
      : level_(level), subsystem_(subsystem), event_(event) {}

  ~TaggedLogStream() {
    LogSinkRegistry::Instance().Emit(
        LogRecord{level_, subsystem_, event_, oss_.str()});
  }

  std::ostringstream& stream() { return oss_; }

 private:
  LogSeverity level_;
  std::string subsystem_;
  std::string event_;
  std::ostringstream oss_;
};

}  // namespace flexui::common

#define FLEXUI_TLOG(subsystem, event, level)                                  \
  ::flexui::common::TaggedLogStream(                                          \
      ::flexui::common::TDF_LOG_##level, #subsystem, #event)                  \
      .stream()
