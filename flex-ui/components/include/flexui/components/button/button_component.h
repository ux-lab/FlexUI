/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <string>
#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::components::button {

struct ButtonProps {
  std::string title;
  std::string on_click;         // handler name
  std::string background_color;
};

class ButtonComponent : public flexui::core::commit_pipeline::ComponentInstance {
 public:
  explicit ButtonComponent(flexui::core::commit_pipeline::ComponentContext& ctx);
  ~ButtonComponent() override;

  flexui::core::commit_pipeline::NodeHandle OnCreate() override;
  void OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& delta) override;
  void OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& rect) override;
  void OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t index) override;
  void OnEvent(const std::string& name,
               const flexui::common::FlexUIValue& payload) override;
  void OnUnmount() override;

  const ButtonProps& props() const { return props_; }

 private:
  ButtonProps props_;
  flexui::core::commit_pipeline::NodeHandle handle_ = nullptr;
};

}  // namespace flexui::components::button
