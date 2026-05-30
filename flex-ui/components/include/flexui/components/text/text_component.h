/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <string>

#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::components::text {

struct TextProps {
  std::string text;
  std::string color;
  double font_size = 14.0;
  std::string font_weight;
  std::string text_align;
  int max_lines = 0;
};

class TextComponent : public flexui::core::commit_pipeline::ComponentInstance {
 public:
  explicit TextComponent(flexui::core::commit_pipeline::ComponentContext& ctx);
  ~TextComponent() override;

  flexui::core::commit_pipeline::NodeHandle OnCreate() override;
  void OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& delta) override;
  void OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& rect) override;
  void OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t index) override;
  void OnUnmount() override;

  const TextProps& props() const { return props_; }

 private:
  TextProps props_;
  flexui::core::commit_pipeline::NodeHandle handle_ = nullptr;
};

}  // namespace flexui::components::text
