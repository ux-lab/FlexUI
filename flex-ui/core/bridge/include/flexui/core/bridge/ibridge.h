/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * IBridge: cross-thread / cross-VM channel for FlexUI. Carries opaque
 * EncodedFrame payloads between subsystems. The default InProcessBridge
 * marshals via a TaskRunner to the target thread; future RPC/IPC backends
 * may implement the same interface.
 */
#pragma once

#include <functional>
#include <memory>

#include "flexui/core/bridge/encoder.h"
#include "flexui/common/task_runner.h"

namespace flexui::core::bridge {

// A unique destination address for messages.
struct Endpoint {
  uint32_t scope_id;       // 0 = engine-wide
  std::string channel;     // e.g. "js2ui", "ui2js", "event", "callback"
};

inline bool operator==(const Endpoint& a, const Endpoint& b) {
  return a.scope_id == b.scope_id && a.channel == b.channel;
}

using MessageHandler =
    std::function<void(const Endpoint& source, EncodedFrame frame)>;

class IBridge {
 public:
  virtual ~IBridge() = default;

  // Post a frame to an endpoint. Returns immediately.
  virtual void Post(const Endpoint& dest, EncodedFrame frame) = 0;

  // Register a handler for messages arriving at `endpoint`. Returns a token
  // that can be used to unsubscribe.
  virtual uint64_t Subscribe(const Endpoint& endpoint, MessageHandler handler) = 0;
  virtual bool Unsubscribe(uint64_t token) = 0;
};

}  // namespace flexui::core::bridge
