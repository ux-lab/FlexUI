/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0. See LICENSE in the project root for full license information.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "flexui/common/log_level.h"

namespace flexui::common {

struct LogRecord {
  LogSeverity level;          // TDF_LOG_INFO / TDF_LOG_WARNING / TDF_LOG_ERROR / TDF_LOG_FATAL
  std::string subsystem;      // "Engine" / "Scope" / "Bridge" / ...
  std::string event;          // "Init" / "Create" / "Apply" / ...
  std::string message;        // formatted human-readable line
};

class LogSink {
 public:
  virtual ~LogSink() = default;
  virtual void Write(const LogRecord& record) = 0;
};

// Process-global list of sinks. AddSink returns an opaque id used to remove.
// Sinks are invoked in registration order.
class LogSinkRegistry {
 public:
  static LogSinkRegistry& Instance();
  uint64_t AddSink(std::unique_ptr<LogSink> sink);
  bool RemoveSink(uint64_t id);
  void Emit(const LogRecord& record);
  void Clear();   // test-only

 private:
  LogSinkRegistry() = default;
};

// Default stdout sink: prints "[FLEXUI/<subsystem>/<event>][<level>] <msg>".
std::unique_ptr<LogSink> MakeStdoutLogSink();

// Platform sink factories. Available only when the corresponding platform
// macro is set at compile time. Calling them otherwise returns nullptr.
std::unique_ptr<LogSink> MakeHarmonyHiLogSink();
std::unique_ptr<LogSink> MakeAndroidLogSink();

}  // namespace flexui::common
