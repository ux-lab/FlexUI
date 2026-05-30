#pragma once
#include <optional>
#include <atomic>
#include "flexui/core/plugin-host/frontend.h"
#include "flexui/common/flexui_value.h"

namespace flexui::plugin_a2ui {

class A2UIJsonFrontend : public flexui::core::plugin_host::IFrontend {
 public:
  std::string Name() const override { return "a2ui-json"; }
  std::string BundleType() const override { return "a2ui-json"; }

  void Initialize(flexui::core::scope_manager::Scope& scope,
                  const flexui::core::plugin_host::BundleSource& bundle) override;
  std::shared_ptr<flexui::core::vdom::DomNode> Render(
      flexui::core::scope_manager::Scope& scope,
      const flexui::common::FlexUIValue& data) override;
  void HandleEvent(flexui::core::scope_manager::Scope& scope,
                   const std::string& event_name,
                   const flexui::common::FlexUIValue& payload) override;

 private:
  flexui::common::FlexUIValue json_doc_;
  std::optional<std::string> handlers_js_;
  std::atomic<uint32_t> next_node_id_{1};
};

}  // namespace flexui::plugin_a2ui
