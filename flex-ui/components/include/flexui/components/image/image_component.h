/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <string>
#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::components::image {

struct ImageProps {
  std::string src;
  double width = 0.0;
  double height = 0.0;
  std::string object_fit;  // "contain" / "cover" / "stretch"
};

class ImageComponent : public flexui::core::commit_pipeline::ComponentInstance {
 public:
  explicit ImageComponent(flexui::core::commit_pipeline::ComponentContext& ctx);
  ~ImageComponent() override;

  flexui::core::commit_pipeline::NodeHandle OnCreate() override;
  void OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& delta) override;
  void OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& rect) override;
  void OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t index) override;
  void OnUnmount() override;

  const ImageProps& props() const { return props_; }

 private:
  ImageProps props_;
  flexui::core::commit_pipeline::NodeHandle handle_ = nullptr;
};

}  // namespace flexui::components::image
