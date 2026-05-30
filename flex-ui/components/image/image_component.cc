/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/image/image_component.h"

#include "flexui/common/log_tag.h"
#include "flexui/core/plugin-host/component_factory.h"

namespace flexui::components::image::platform {
flexui::core::commit_pipeline::NodeHandle CreateNode();
void ApplyProps(flexui::core::commit_pipeline::NodeHandle, const ImageProps&);
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle,
                 const flexui::core::commit_pipeline::LayoutRect&);
void Mount(flexui::core::commit_pipeline::NodeHandle,
           flexui::core::commit_pipeline::NodeHandle, uint32_t);
void Unmount(flexui::core::commit_pipeline::NodeHandle);
}  // namespace flexui::components::image::platform

namespace flexui::components::image {

namespace {
void Apply(ImageProps& p, const flexui::core::commit_pipeline::PropDelta& d) {
  for (auto& kv : d.updated) {
    const auto& k = kv.first;
    const auto& v = kv.second;
    if (k == "src")            p.src        = v.IsString() ? v.ToStringChecked() : "";
    else if (k == "width")     p.width      = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
    else if (k == "height")    p.height     = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
    else if (k == "objectFit") p.object_fit = v.IsString() ? v.ToStringChecked() : "";
  }
}
}  // namespace

ImageComponent::ImageComponent(flexui::core::commit_pipeline::ComponentContext&) {
  FLEXUI_TLOG(Component, ImageCreate, DEBUG);
}
ImageComponent::~ImageComponent() = default;

flexui::core::commit_pipeline::NodeHandle ImageComponent::OnCreate() {
  if (!handle_) handle_ = platform::CreateNode();
  return handle_;
}
void ImageComponent::OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& d) {
  Apply(props_, d);
  if (handle_) platform::ApplyProps(handle_, props_);
}
void ImageComponent::OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& r) {
  if (handle_) platform::ApplyLayout(handle_, r);
}
void ImageComponent::OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t idx) {
  platform::Mount(parent, handle_, idx);
}
void ImageComponent::OnUnmount() {
  if (handle_) { platform::Unmount(handle_); handle_ = nullptr; }
}

static flexui::core::plugin_host::ComponentFactory MakeImageFactoryImpl() {
  flexui::core::plugin_host::ComponentFactory f;
  f.name = "Image";
  f.implementation = flexui::core::plugin_host::ComponentImplementation::kCapi;
  f.create = [](flexui::core::commit_pipeline::ComponentContext& ctx) {
    return std::make_unique<ImageComponent>(ctx);
  };
  return f;
}

}  // namespace flexui::components::image

namespace flexui::components {
flexui::core::plugin_host::ComponentFactory MakeImageFactory() {
  return image::MakeImageFactoryImpl();
}
}  // namespace flexui::components
