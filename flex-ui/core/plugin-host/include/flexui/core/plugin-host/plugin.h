#pragma once
#include <string>
#include <vector>

#include "flexui/core/plugin-host/component_factory.h"
#include "flexui/core/plugin-host/native_api.h"
#include "flexui/core/plugin-host/bundle_loader.h"
#include "flexui/core/plugin-host/frontend.h"

namespace flexui::core::plugin_host {

class FlexUIEngine;

struct FlexUIPlugin {
  std::string name;
  std::string version;
  std::vector<ComponentFactory> components;
  std::vector<NativeApi> apis;
  std::vector<BundleLoader> loaders;
  std::vector<FrontendRegistration> frontends;

  std::function<void(FlexUIEngine&)> on_install;
  std::function<void()> on_uninstall;
};

}  // namespace flexui::core::plugin_host
