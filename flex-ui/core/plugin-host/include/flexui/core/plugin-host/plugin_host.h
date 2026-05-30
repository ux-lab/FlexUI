#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "flexui/common/error.h"
#include "flexui/core/plugin-host/plugin.h"

namespace flexui::core::plugin_host {

class PluginHost {
 public:
  flexui::common::Error Install(FlexUIPlugin plugin);
  flexui::common::Error Uninstall(const std::string& name);

  // Lookups (read-only); return null if absent.
  const ComponentFactory*       FindComponent(const std::string& name) const;
  const NativeApi*              FindApi(const std::string& name) const;
  const BundleLoader*           FindLoader(const std::string& scheme) const;
  const FrontendRegistration*   FindFrontend(const std::string& bundle_type) const;

  size_t PluginCount() const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, FlexUIPlugin> plugins_;
  std::unordered_map<std::string, const ComponentFactory*> components_;
  std::unordered_map<std::string, const NativeApi*> apis_;
  std::unordered_map<std::string, const BundleLoader*> loaders_;
  std::unordered_map<std::string, const FrontendRegistration*> frontends_;
};

}  // namespace flexui::core::plugin_host
