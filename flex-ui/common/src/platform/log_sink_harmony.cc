/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * HiLog backend for FlexUI's LogSink. Compiles only when FLEXUI_OHOS is set.
 */
#include "flexui/common/log_sink.h"

#include <memory>

#if defined(FLEXUI_OHOS)
#include <hilog/log.h>
#endif

namespace flexui::common {

#if defined(FLEXUI_OHOS)

namespace {

constexpr uint32_t kHiLogDomain = 0xFE00;  // FlexUI domain id, registered with platform team

class HiLogSink : public LogSink {
 public:
  void Write(const LogRecord& r) override {
    // Tag follows FLEXUI/<subsystem>/<event> convention.
    char tag[128];
    snprintf(tag, sizeof(tag), "FLEXUI/%s/%s", r.subsystem.c_str(), r.event.c_str());
    LogLevel level = LOG_INFO;
    switch (r.level) {
      case TDF_LOG_WARNING: level = LOG_WARN;  break;
      case TDF_LOG_ERROR:
      case TDF_LOG_FATAL:   level = LOG_ERROR; break;
      default:              level = LOG_INFO;  break;
    }
    OH_LOG_Print(LOG_APP, level, kHiLogDomain, tag, "%{public}s", r.message.c_str());
  }
};

}  // namespace

std::unique_ptr<LogSink> MakeHarmonyHiLogSink() {
  return std::make_unique<HiLogSink>();
}

#else  // !FLEXUI_OHOS

std::unique_ptr<LogSink> MakeHarmonyHiLogSink() { return nullptr; }

#endif  // FLEXUI_OHOS

}  // namespace flexui::common
