#include "timer_api.h"
#include "flexui/common/log_tag.h"
namespace flexui::api {
flexui::core::plugin_host::NativeApi MakeTimerApi() {
  flexui::core::plugin_host::NativeApi api;
  api.name = "setTimeout";
  api.mode = flexui::core::plugin_host::NativeApiMode::kSync;
  api.invoke = [](const std::vector<flexui::common::FlexUIValue>& args) {
    FLEXUI_TLOG(Api, SetTimeout, DEBUG);
    (void)args;
    return flexui::common::FlexUIValue(0);
  };
  return api;
}
}
