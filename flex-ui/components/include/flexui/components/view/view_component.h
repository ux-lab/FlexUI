/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <string>
#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::components::view {

struct ViewProps {
  std::string background_color;
  double border_radius = 0.0;
  double padding = 0.0;
  std::string flex_direction;  // "row" / "column"
  double width = 0.0;
  double height = 0.0;
};

class ViewComponent : public flexui::core::commit_pipeline::ComponentInstance {
 public:
  explicit ViewComponent(flexui::core::commit_pipeline::ComponentContext& ctx);
  ~ViewComponent() override;

  flexui::core::commit_pipeline::NodeHandle OnCreate() override;
  void OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& delta) override;
  void OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& rect) override;
  void OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t index) override;
  void OnUnmount() override;

  const ViewProps& props() const { return props_; }

 private:
  ViewProps props_;
  flexui::core::commit_pipeline::NodeHandle handle_ = nullptr;
};

}  // namespace flexui::components::view
