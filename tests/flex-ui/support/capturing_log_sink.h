#pragma once

#include <mutex>
#include <vector>

#include "flexui/common/log_sink.h"

namespace flexui::common::test {

class CapturingLogSink : public LogSink {
 public:
  void Write(const LogRecord& r) override;
  std::vector<LogRecord> Drain();

 private:
  std::mutex mu_;
  std::vector<LogRecord> records_;
};

// RAII helper: registers a fresh CapturingLogSink, removes on destruction.
class ScopedCapturingSink {
 public:
  ScopedCapturingSink();
  ~ScopedCapturingSink();
  CapturingLogSink& sink() { return *sink_ptr_; }

 private:
  uint64_t id_;
  CapturingLogSink* sink_ptr_;
};

}  // namespace flexui::common::test
