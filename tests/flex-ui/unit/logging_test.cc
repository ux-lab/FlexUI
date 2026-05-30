/*
 * FlexUI logging + log_settings unit tests.
 *
 * Covers:
 *  - LogSettings get/set round-trip
 *  - GetMinLogLevel clamped to TDF_LOG_FATAL
 *  - ShouldCreateLogMessage filtering by min level
 *  - FLEXUI_LOG / FLEXUI_DLOG / FLEXUI_CHECK macro compilation
 *  - Custom delegate is invoked by LogMessage
 */

#include "flexui/common/logging.h"
#include "flexui/common/log_settings.h"

#include <gtest/gtest.h>

#include <atomic>
#include <sstream>
#include <string>

using namespace flexui::common::log;

// ── LogSettings round-trip ──────────────────────────────────────────────────

TEST(LogSettingsTest, DefaultIsInfo) {
  // Reset to INFO (default).
  SetLogSettings({TDF_LOG_INFO});
  EXPECT_EQ(GetLogSettings().min_log_level, TDF_LOG_INFO);
  EXPECT_EQ(GetMinLogLevel(), TDF_LOG_INFO);
}

TEST(LogSettingsTest, SetAndGetWarning) {
  SetLogSettings({TDF_LOG_WARNING});
  EXPECT_EQ(GetLogSettings().min_log_level, TDF_LOG_WARNING);
  EXPECT_EQ(GetMinLogLevel(), TDF_LOG_WARNING);
  // Restore.
  SetLogSettings({TDF_LOG_INFO});
}

TEST(LogSettingsTest, ClampedToFatal) {
  // Anything above FATAL should be clamped to FATAL.
  SetLogSettings({TDF_LOG_NUM_SEVERITIES});
  EXPECT_EQ(GetMinLogLevel(), TDF_LOG_FATAL);
  // Restore.
  SetLogSettings({TDF_LOG_INFO});
}

// ── ShouldCreateLogMessage ──────────────────────────────────────────────────

TEST(LogFilterTest, InfoPassesWhenMinIsInfo) {
  SetLogSettings({TDF_LOG_INFO});
  EXPECT_TRUE(ShouldCreateLogMessage(TDF_LOG_INFO));
  EXPECT_TRUE(ShouldCreateLogMessage(TDF_LOG_WARNING));
  EXPECT_TRUE(ShouldCreateLogMessage(TDF_LOG_ERROR));
  SetLogSettings({TDF_LOG_INFO});
}

TEST(LogFilterTest, InfoSuppressedWhenMinIsWarning) {
  SetLogSettings({TDF_LOG_WARNING});
  EXPECT_FALSE(ShouldCreateLogMessage(TDF_LOG_INFO));
  EXPECT_TRUE(ShouldCreateLogMessage(TDF_LOG_WARNING));
  SetLogSettings({TDF_LOG_INFO});
}

// ── Custom delegate ──────────────────────────────────────────────────────────
// Note: InitializeDelegate can only be called once per process (it aborts on
// double-init). We test via a static guard so the test suite remains re-entrant.

static std::atomic<bool> s_delegate_installed{false};
static std::string s_last_log_message;

TEST(LogDelegateTest, DelegateReceivesMessage) {
  if (!s_delegate_installed.exchange(true)) {
    LogMessage::InitializeDelegate(
        [](const std::ostringstream& oss, LogSeverity) {
          s_last_log_message = oss.str();
        });
  }

  SetLogSettings({TDF_LOG_INFO});
  s_last_log_message.clear();
  FLEXUI_LOG(INFO) << "hello delegate";
  EXPECT_NE(s_last_log_message.find("hello delegate"), std::string::npos);
}

// ── Macro compilation check ──────────────────────────────────────────────────

TEST(LogMacroTest, MacrosCompile) {
  SetLogSettings({TDF_LOG_ERROR}); // suppress INFO/WARNING output
  FLEXUI_LOG(ERROR) << "error macro test";
  FLEXUI_DLOG(ERROR) << "dlog macro test";
  FLEXUI_CHECK(1 + 1 == 2);  // must not abort
  SetLogSettings({TDF_LOG_INFO});
}
