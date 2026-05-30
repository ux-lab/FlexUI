/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Android backend STUB. Real implementation lands in W5-W6 (platform/android/jni)
 * with __android_log_print. Until then this returns nullptr so callers can
 * gracefully fall back to the stdout sink.
 */
#include "flexui/common/log_sink.h"

namespace flexui::common {

std::unique_ptr<LogSink> MakeAndroidLogSink() {
  // NOT_IMPLEMENTED: returning nullptr is the agreed contract for platform
  // sink factories whose backend is not yet wired. See plan W1-W2 Task 10.
  return nullptr;
}

}  // namespace flexui::common
