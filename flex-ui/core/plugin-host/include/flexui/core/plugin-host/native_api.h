#pragma once

#include <functional>
#include <string>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::plugin_host {

enum class NativeApiMode { kSync, kAsync };

struct NativeApi {
  std::string name;                                       // "$location.getCurrent"
  NativeApiMode mode = NativeApiMode::kSync;
  std::function<flexui::common::FlexUIValue(
      const std::vector<flexui::common::FlexUIValue>& args)> invoke;
};

}  // namespace flexui::core::plugin_host
