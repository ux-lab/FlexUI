#include "console_api.h"
#include "flexui/common/log_tag.h"
namespace flexui::api {
flexui::core::plugin_host::NativeApi MakeConsoleApi() {
  flexui::core::plugin_host::NativeApi api;
  api.name = "console.log";
  api.mode = flexui::core::plugin_host::NativeApiMode::kSync;
  api.invoke = [](const std::vector<flexui::common::FlexUIValue>& args) {
    std::string msg;
    for (auto& a : args) {
      if (!msg.empty()) msg += " ";
      if (a.IsString()) msg += a.ToStringChecked();
      else if (a.IsNumber()) msg += std::to_string(a.ToDoubleChecked());
      else if (a.IsBoolean()) msg += a.ToBooleanChecked() ? "true" : "false";
    }
    FLEXUI_TLOG(Api, ConsoleLog, INFO) << msg;
    return flexui::common::FlexUIValue();
  };
  return api;
}
}
