#pragma once

#include <memory>
#include <string>

#include "flexui/common/error.h"
#include "flexui/core/js-engine/ijs_context.h"

namespace flexui::core::js_engine {

struct EngineConfig {
  size_t memory_limit_mb = 0;       // 0 = backend default
  bool enable_debug = false;
};

class IJsEngine {
 public:
  virtual ~IJsEngine() = default;

  virtual flexui::common::Error Initialize(const EngineConfig& cfg) = 0;
  virtual std::shared_ptr<IJsContext> CreateContext() = 0;
  virtual void Shutdown() = 0;
  virtual const char* BackendName() const = 0;
};

}  // namespace flexui::core::js_engine
