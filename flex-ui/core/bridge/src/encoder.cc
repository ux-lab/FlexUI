#include "flexui/core/bridge/encoder.h"

#include <cstdlib>

#include "flexui/common/serializer.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::bridge {

EncodedFrame Encoder::EncodeValue(const flexui::common::FlexUIValue& v) {
  flexui::common::Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(v);
  auto buf = ser.Release();  // returns std::pair<uint8_t*, size_t>; caller owns memory
  EncodedFrame frame;
  if (buf.first && buf.second > 0) {
    frame.bytes.assign(buf.first, buf.first + buf.second);
    // Release() transfers ownership; free per SerializerHelper contract.
    std::free(buf.first);
  }
  FLEXUI_TLOG(Bridge, Encode, DEBUG) << "bytes=" << frame.bytes.size();
  return frame;
}

}  // namespace flexui::core::bridge
