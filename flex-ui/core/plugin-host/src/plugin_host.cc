#include "flexui/core/plugin-host/plugin_host.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::plugin_host {

using namespace flexui::common;

Error PluginHost::Install(FlexUIPlugin plugin) {
  std::lock_guard<std::mutex> lock(mu_);
  if (plugins_.count(plugin.name)) {
    FLEXUI_TLOG(Plugin, InstallConflict, ERROR) << "name=" << plugin.name;
    return Error(ErrorCode::kPluginConflict,
                 "plugin already installed: " + plugin.name);
  }
  // Detect component/api/loader/frontend collisions upfront.
  for (auto& c : plugin.components) {
    if (components_.count(c.name))
      return Error(ErrorCode::kPluginConflict, "component name collision: " + c.name);
  }
  for (auto& a : plugin.apis) {
    if (apis_.count(a.name))
      return Error(ErrorCode::kPluginConflict, "api name collision: " + a.name);
  }
  for (auto& l : plugin.loaders) {
    if (loaders_.count(l.scheme))
      return Error(ErrorCode::kPluginConflict, "loader scheme collision: " + l.scheme);
  }
  for (auto& f : plugin.frontends) {
    if (frontends_.count(f.bundle_type))
      return Error(ErrorCode::kPluginConflict,
                   "frontend bundle_type collision: " + f.bundle_type);
  }
  // Insert.
  auto& stored = plugins_.emplace(plugin.name, std::move(plugin)).first->second;
  for (auto& c : stored.components) components_[c.name] = &c;
  for (auto& a : stored.apis)        apis_[a.name]      = &a;
  for (auto& l : stored.loaders)     loaders_[l.scheme] = &l;
  for (auto& f : stored.frontends)   frontends_[f.bundle_type] = &f;

  FLEXUI_TLOG(Plugin, Install, INFO)
      << "name=" << stored.name
      << " components=" << stored.components.size()
      << " apis=" << stored.apis.size()
      << " loaders=" << stored.loaders.size()
      << " frontends=" << stored.frontends.size();
  return Error::Ok();
}

Error PluginHost::Uninstall(const std::string& name) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = plugins_.find(name);
  if (it == plugins_.end()) {
    return Error(ErrorCode::kNotFound, "plugin not installed: " + name);
  }
  for (auto& c : it->second.components) components_.erase(c.name);
  for (auto& a : it->second.apis)        apis_.erase(a.name);
  for (auto& l : it->second.loaders)     loaders_.erase(l.scheme);
  for (auto& f : it->second.frontends)   frontends_.erase(f.bundle_type);
  if (it->second.on_uninstall) it->second.on_uninstall();
  plugins_.erase(it);
  FLEXUI_TLOG(Plugin, Uninstall, INFO) << "name=" << name;
  return Error::Ok();
}

const ComponentFactory* PluginHost::FindComponent(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = components_.find(name);
  return it == components_.end() ? nullptr : it->second;
}
const NativeApi* PluginHost::FindApi(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = apis_.find(name);
  return it == apis_.end() ? nullptr : it->second;
}
const BundleLoader* PluginHost::FindLoader(const std::string& scheme) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = loaders_.find(scheme);
  return it == loaders_.end() ? nullptr : it->second;
}
const FrontendRegistration* PluginHost::FindFrontend(
    const std::string& bundle_type) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = frontends_.find(bundle_type);
  return it == frontends_.end() ? nullptr : it->second;
}
size_t PluginHost::PluginCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return plugins_.size();
}

}  // namespace flexui::core::plugin_host
