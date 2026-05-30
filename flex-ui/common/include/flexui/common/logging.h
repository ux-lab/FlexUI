/*
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2022 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Modified by the FlexUI authors. This file is derived from
 * modules/footstone/include/footstone/logging.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone::log` -> `flexui::common::log`.
 *   - Renamed include paths `footstone/...` -> `flexui/common/...`.
 *   - Renamed macros `FOOTSTONE_*` -> `FLEXUI_*`.
 *   - Removed Hippy-specific perf-log macros (TDF_PERF_LOG, etc.).
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */

#pragma once

#if defined(__cplusplus)

#include <cassert>
#include <codecvt>
#include <mutex>
#include <sstream>
#include <functional>

#include "flexui/common/log_level.h"
#include "flexui/common/macros.h"
#include "flexui/common/string_view.h"

namespace flexui::common {
inline namespace log {

constexpr char kCharConversionFailedPrompt[] = "<string conversion failed>";
constexpr char16_t kU16CharConversionFailedPrompt[] = u"<u16string conversion failed>";
constexpr char32_t kU32CharConversionFailedPrompt[] = U"<u32string conversion failed>";

inline std::ostream& operator<<(std::ostream& stream, const string_view& str_view) {
  string_view::Encoding encoding = str_view.encoding();
  switch (encoding) {
    case string_view::Encoding::Latin1: {
      std::string u8;
      for (const auto& ch: str_view.latin1_value()) {
        if (static_cast<uint8_t>(ch) < 0x80) {
          u8 += ch;
        } else {
          u8 += static_cast<char>((0xc0 | ch >> 6));
          u8 += static_cast<char>((0x80 | (ch & 0x3f)));
        }
      }
      stream << u8;
      break;
    }
    case string_view::Encoding::Utf16: {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      const std::u16string& str = str_view.utf16_value();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert(
          kCharConversionFailedPrompt, kU16CharConversionFailedPrompt);
#pragma clang diagnostic pop
      stream << convert.to_bytes(str);
#pragma clang diagnostic pop
      break;
    }
    case string_view::Encoding::Utf32: {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      const std::u32string& str = str_view.utf32_value();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
      std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert(
          kCharConversionFailedPrompt, kU32CharConversionFailedPrompt);
#pragma clang diagnostic pop
      stream << convert.to_bytes(str);
#pragma clang diagnostic pop
      break;
    }
    case string_view::Encoding::Utf8: {
      const string_view::u8string& str = str_view.utf8_value();
      stream << std::string(reinterpret_cast<const char*>(str.c_str()), str.length());
      break;
    }
    default: {
      assert(false);
      break;
    }
  }

  return stream;
}

class LogMessageVoidify {
 public:
  void operator&(std::ostream&) {}
};

class LogMessage {
 public:
  LogMessage(LogSeverity severity, const char* file, int line, const char* condition);
  ~LogMessage();

  LogMessage(LogMessage&) = delete;
  LogMessage& operator=(LogMessage&) = delete;

  inline static void InitializeDelegate(
      std::function<void(const std::ostringstream&, LogSeverity)> delegate) {
    if (!delegate) {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (delegate_) {
      abort(); // delegate can only be initialized once
    }
    delegate_ = delegate;
  }

  std::ostringstream& stream() { return stream_; }

 private:
  static std::function<void(const std::ostringstream&, LogSeverity)> delegate_;
  static std::function<void(const std::ostringstream&, LogSeverity)> default_delegate_;
  static std::mutex mutex_;

  std::ostringstream stream_;
  const LogSeverity severity_;
  const char* file_;
  const int line_;
};

int GetVlogVerbosity();

bool ShouldCreateLogMessage(LogSeverity severity);

}  // namespace log
}  // namespace flexui::common

#define FLEXUI_LOG_STREAM(severity)                                                                    \
  ::flexui::common::log::LogMessage(::flexui::common::log::LogSeverity::TDF_LOG_##severity, __FILE__, \
                                    __LINE__, nullptr)                                                 \
      .stream()

#define FLEXUI_LAZY_STREAM(stream, condition) \
  !(condition) ? (void)0 : ::flexui::common::log::LogMessageVoidify() & (stream)

#define FLEXUI_EAT_STREAM_PARAMETERS(ignored)                                                              \
  true || (ignored)                                                                                        \
      ? (void)0                                                                                            \
      : ::flexui::common::log::LogMessageVoidify() &                                                       \
            ::flexui::common::log::LogMessage(::flexui::common::log::LogSeverity::TDF_LOG_FATAL, 0, 0,    \
                                              nullptr)                                                     \
                .stream()

#define FLEXUI_LOG_IS_ON(severity) \
  (::flexui::common::log::ShouldCreateLogMessage(::flexui::common::log::LogSeverity::TDF_LOG_##severity))

#define FLEXUI_LOG(severity) \
  FLEXUI_LAZY_STREAM(FLEXUI_LOG_STREAM(severity), FLEXUI_LOG_IS_ON(severity))

#define FLEXUI_CHECK(condition)                                                                                \
  FLEXUI_LAZY_STREAM(::flexui::common::log::LogMessage(::flexui::common::log::LogSeverity::TDF_LOG_FATAL,    \
                                                       __FILE__, __LINE__, #condition)                        \
                         .stream(),                                                                           \
                     !(condition))

#define FLEXUI_VLOG_IS_ON(verbose_level) ((verbose_level) <= ::flexui::common::log::GetVlogVerbosity())

#define FLEXUI_VLOG_STREAM(verbose_level) \
  ::flexui::common::log::LogMessage(-verbose_level, __FILE__, __LINE__, nullptr).stream()

#define FLEXUI_VLOG(verbose_level) \
  FLEXUI_LAZY_STREAM(FLEXUI_VLOG_STREAM(verbose_level), FLEXUI_VLOG_IS_ON(verbose_level))

#ifndef NDEBUG
#define FLEXUI_DLOG(severity) FLEXUI_LOG(severity)
#define FLEXUI_DCHECK(condition) FLEXUI_CHECK(condition)
#else
#define FLEXUI_DLOG(severity) FLEXUI_EAT_STREAM_PARAMETERS(true)
#define FLEXUI_DCHECK(condition) FLEXUI_EAT_STREAM_PARAMETERS(condition)
#endif

#define FLEXUI_UNREACHABLE() \
  do {                        \
    FLEXUI_DCHECK(false);     \
    abort();                  \
  } while (0)

#define FLEXUI_UNIMPLEMENTED()                                                \
  do {                                                                        \
    FLEXUI_LOG(ERROR) << "Not implemented in: " << __PRETTY_FUNCTION__;      \
    abort();                                                                  \
  } while (0)

#define FLEXUI_USE(expr) \
  do {                    \
    (void)(expr);         \
  } while (0)

#endif
