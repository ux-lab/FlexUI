/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * FlexUI error type. A small, allocation-free-on-success Result wrapper
 * used at API boundaries. Subsystem code should return Error from
 * fallible operations rather than throwing.
 */
#pragma once

#include <ostream>
#include <string>
#include <utility>

namespace flexui::common {

enum class ErrorCode : uint32_t {
  kOk = 0,
  kInvalidArgument,
  kNotFound,
  kAlreadyExists,
  kInternal,
  kJsException,
  kPluginConflict,
  kEngineNotInitialized,
  kScopeDestroyed,
};

inline const char* ErrorCodeName(ErrorCode code) {
  switch (code) {
    case ErrorCode::kOk:                   return "Ok";
    case ErrorCode::kInvalidArgument:      return "InvalidArgument";
    case ErrorCode::kNotFound:             return "NotFound";
    case ErrorCode::kAlreadyExists:        return "AlreadyExists";
    case ErrorCode::kInternal:             return "Internal";
    case ErrorCode::kJsException:          return "JsException";
    case ErrorCode::kPluginConflict:       return "PluginConflict";
    case ErrorCode::kEngineNotInitialized: return "EngineNotInitialized";
    case ErrorCode::kScopeDestroyed:       return "ScopeDestroyed";
  }
  return "Unknown";
}

class Error {
 public:
  static Error Ok() { return Error(ErrorCode::kOk, ""); }
  Error(ErrorCode code, std::string message)
      : code_(code), message_(std::move(message)) {}
  ErrorCode code() const { return code_; }
  const std::string& message() const { return message_; }
  bool ok() const { return code_ == ErrorCode::kOk; }
  explicit operator bool() const { return !ok(); }
  friend std::ostream& operator<<(std::ostream& os, const Error& e) {
    os << "Error(" << ErrorCodeName(e.code()) << ": " << e.message() << ")";
    return os;
  }
 private:
  ErrorCode code_;
  std::string message_;
};

}  // namespace flexui::common
