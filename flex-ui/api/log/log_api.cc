#include "log_api.h"
#include "flexui/common/log_tag.h"
namespace flexui::api {
flexui::core::plugin_host::NativeApi MakeLogApi() {
  flexui::core::plugin_host::NativeApi api;
  api.name = "flexLog";
  api.mode = flexui::core::plugin_host::NativeApiMode::kSync;
  api.invoke = [](const std::vector<flexui::common::FlexUIValue>& args) {
    if (!args.empty() && args[0].IsString()) {
      FLEXUI_TLOG(Api, FlexLog, DEBUG) << args[0].ToStringChecked();
    }
    return flexui::common::FlexUIValue();
  };
  return api;
}
}
