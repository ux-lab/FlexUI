/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/common/log_sink.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace flexui::common {

namespace {

const char* SeverityName(LogSeverity s) {
  switch (s) {
    case TDF_LOG_DEBUG:   return "DEBUG";
    case TDF_LOG_INFO:    return "INFO";
    case TDF_LOG_WARNING: return "WARN";
    case TDF_LOG_ERROR:   return "ERROR";
    case TDF_LOG_FATAL:   return "FATAL";
    default:              return "UNKNOWN";
  }
}

class StdoutSink : public LogSink {
 public:
  void Write(const LogRecord& r) override {
    std::lock_guard<std::mutex> lock(mu_);
    std::cout << "[FLEXUI/" << r.subsystem << "/" << r.event << "]["
              << SeverityName(r.level) << "] " << r.message << std::endl;
  }
 private:
  std::mutex mu_;
};

struct State {
  std::mutex mu;
  std::unordered_map<uint64_t, std::unique_ptr<LogSink>> sinks;
  std::atomic<uint64_t> next_id{1};
};

State& S() {
  static State s;
  return s;
}

}  // namespace

LogSinkRegistry& LogSinkRegistry::Instance() {
  static LogSinkRegistry registry;
  return registry;
}

uint64_t LogSinkRegistry::AddSink(std::unique_ptr<LogSink> sink) {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  uint64_t id = s.next_id.fetch_add(1);
  s.sinks.emplace(id, std::move(sink));
  return id;
}

bool LogSinkRegistry::RemoveSink(uint64_t id) {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  return s.sinks.erase(id) > 0;
}

void LogSinkRegistry::Emit(const LogRecord& record) {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  for (auto& kv : s.sinks) {
    kv.second->Write(record);
  }
}

void LogSinkRegistry::Clear() {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  s.sinks.clear();
}

std::unique_ptr<LogSink> MakeStdoutLogSink() {
  return std::make_unique<StdoutSink>();
}

}  // namespace flexui::common
