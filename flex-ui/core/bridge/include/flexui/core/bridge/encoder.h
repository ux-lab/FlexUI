/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Bridge Encoder: serializes a FlexUIValue (and small frame-header metadata)
 * to a byte buffer suitable for cross-thread or cross-process transport.
 * Wraps the W1-W2 binary serializer.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::bridge {

struct EncodedFrame {
  std::vector<uint8_t> bytes;
};

class Encoder {
 public:
  static EncodedFrame EncodeValue(const flexui::common::FlexUIValue& v);
};

}  // namespace flexui::core::bridge
