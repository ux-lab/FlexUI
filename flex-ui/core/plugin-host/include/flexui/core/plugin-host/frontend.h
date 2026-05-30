#pragma once

#include <memory>
#include <string>

#include "flexui/common/flexui_value.h"
#include "flexui/core/vdom/dom_node.h"
#include "flexui/core/plugin-host/bundle_loader.h"

namespace flexui::core::scope_manager { class Scope; }

namespace flexui::core::plugin_host {

class IFrontend {
 public:
  virtual ~IFrontend() = default;
  virtual std::string Name() const = 0;
  virtual std::string BundleType() const = 0;
  virtual void Initialize(scope_manager::Scope& scope,
                          const BundleSource& bundle) = 0;
  virtual std::shared_ptr<flexui::core::vdom::DomNode> Render(
      scope_manager::Scope& scope,
      const flexui::common::FlexUIValue& data) = 0;
  virtual void HandleEvent(scope_manager::Scope& scope,
                           const std::string& event_name,
                           const flexui::common::FlexUIValue& payload) {}
};

using FrontendFactory = std::function<std::unique_ptr<IFrontend>()>;

struct FrontendRegistration {
  std::string bundle_type;   // "card-js" / "a2ui-json"
  FrontendFactory factory;
};

}  // namespace flexui::core::plugin_host
