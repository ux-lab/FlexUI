#include "flexui/core/bridge/decoder.h"

#include "flexui/common/deserializer.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::bridge {

flexui::common::Error Decoder::DecodeValue(const uint8_t* data, size_t len,
                                           flexui::common::FlexUIValue* out) {
  if (!data || !out) return flexui::common::Error(
      flexui::common::ErrorCode::kInvalidArgument, "null pointer");
  flexui::common::Deserializer de(data, len);
  if (!de.ReadHeader()) return flexui::common::Error(
      flexui::common::ErrorCode::kInternal, "bad header");
  if (!de.ReadValue(*out)) return flexui::common::Error(
      flexui::common::ErrorCode::kInternal, "value parse failed");
  FLEXUI_TLOG(Bridge, Decode, DEBUG) << "bytes=" << len;
  return flexui::common::Error::Ok();
}

}  // namespace flexui::core::bridge
