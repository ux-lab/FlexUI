#include "tests/flex-ui/support/capturing_log_sink.h"

#include <memory>

namespace flexui::common::test {

void CapturingLogSink::Write(const LogRecord& r) {
  std::lock_guard<std::mutex> lock(mu_);
  records_.push_back(r);
}

std::vector<LogRecord> CapturingLogSink::Drain() {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<LogRecord> out;
  out.swap(records_);
  return out;
}

ScopedCapturingSink::ScopedCapturingSink() {
  auto sink = std::make_unique<CapturingLogSink>();
  sink_ptr_ = sink.get();
  id_ = LogSinkRegistry::Instance().AddSink(std::move(sink));
}

ScopedCapturingSink::~ScopedCapturingSink() {
  LogSinkRegistry::Instance().RemoveSink(id_);
}

}  // namespace flexui::common::test
