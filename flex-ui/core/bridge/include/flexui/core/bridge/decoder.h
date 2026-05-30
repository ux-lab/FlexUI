#pragma once

#include <cstdint>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"

namespace flexui::core::bridge {

class Decoder {
 public:
  static flexui::common::Error DecodeValue(const uint8_t* data, size_t len,
                                           flexui::common::FlexUIValue* out);
};

}  // namespace flexui::core::bridge
