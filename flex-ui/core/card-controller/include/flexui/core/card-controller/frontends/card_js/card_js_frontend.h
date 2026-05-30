/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "flexui/core/plugin-host/frontend.h"
#include "flexui/core/js-engine/ijs_context.h"
#include "flexui/core/js-engine/ijs_value.h"

namespace flexui::core::card_controller::frontends::card_js {

class CardJsFrontend : public flexui::core::plugin_host::IFrontend {
 public:
  std::string Name() const override { return "card-js"; }
  std::string BundleType() const override { return "card-js"; }

  void Initialize(flexui::core::scope_manager::Scope& scope,
                  const flexui::core::plugin_host::BundleSource& bundle) override;
  std::shared_ptr<flexui::core::vdom::DomNode> Render(
      flexui::core::scope_manager::Scope& scope,
      const flexui::common::FlexUIValue& data) override;
  void HandleEvent(flexui::core::scope_manager::Scope& scope,
                   const std::string& event_name,
                   const flexui::common::FlexUIValue& payload) override;

 private:
  std::shared_ptr<js_engine::IJsValue> render_fn_;
  std::shared_ptr<js_engine::IJsValue> handlers_;
  std::atomic<uint32_t> next_node_id_{1};
};

flexui::core::plugin_host::FrontendRegistration MakeCardJsRegistration();

}  // namespace flexui::core::card_controller::frontends::card_js
