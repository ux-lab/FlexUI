#include "flexui/api/api_base_package.h"
#include "flexui/common/log_tag.h"
namespace flexui::api {
flexui::core::plugin_host::NativeApi MakeConsoleApi();
flexui::core::plugin_host::NativeApi MakeTimerApi();
flexui::core::plugin_host::NativeApi MakeLogApi();
flexui::core::plugin_host::FlexUIPlugin MakeApiBasePackage() {
  FLEXUI_TLOG(Plugin, MakeApiBase, INFO);
  flexui::core::plugin_host::FlexUIPlugin p;
  p.name = "flexui-api-base";
  p.version = "0.1.0";
  p.apis = { MakeConsoleApi(), MakeTimerApi(), MakeLogApi() };
  return p;
}
}
