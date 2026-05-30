/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/view/view_component.h"

#include "flexui/common/log_tag.h"
#include "flexui/core/plugin-host/component_factory.h"

namespace flexui::components::view::platform {
flexui::core::commit_pipeline::NodeHandle CreateNode();
void ApplyProps(flexui::core::commit_pipeline::NodeHandle, const ViewProps&);
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle,
                 const flexui::core::commit_pipeline::LayoutRect&);
void Mount(flexui::core::commit_pipeline::NodeHandle,
           flexui::core::commit_pipeline::NodeHandle, uint32_t);
void Unmount(flexui::core::commit_pipeline::NodeHandle);
}  // namespace flexui::components::view::platform

namespace flexui::components::view {

namespace {
void Apply(ViewProps& p, const flexui::core::commit_pipeline::PropDelta& d) {
  for (auto& kv : d.updated) {
    const auto& k = kv.first;
    const auto& v = kv.second;
    if (k == "backgroundColor")  p.background_color = v.IsString() ? v.ToStringChecked() : "";
    else if (k == "borderRadius") p.border_radius    = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
    else if (k == "padding")      p.padding          = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
    else if (k == "flexDirection") p.flex_direction  = v.IsString() ? v.ToStringChecked() : "";
    else if (k == "width")        p.width            = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
    else if (k == "height")       p.height           = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
  }
}
}  // namespace

ViewComponent::ViewComponent(flexui::core::commit_pipeline::ComponentContext&) {
  FLEXUI_TLOG(Component, ViewCreate, DEBUG);
}
ViewComponent::~ViewComponent() = default;

flexui::core::commit_pipeline::NodeHandle ViewComponent::OnCreate() {
  if (!handle_) handle_ = platform::CreateNode();
  return handle_;
}
void ViewComponent::OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& d) {
  Apply(props_, d);
  if (handle_) platform::ApplyProps(handle_, props_);
}
void ViewComponent::OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& r) {
  if (handle_) platform::ApplyLayout(handle_, r);
}
void ViewComponent::OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t idx) {
  platform::Mount(parent, handle_, idx);
}
void ViewComponent::OnUnmount() {
  if (handle_) { platform::Unmount(handle_); handle_ = nullptr; }
}

static flexui::core::plugin_host::ComponentFactory MakeViewFactoryImpl() {
  flexui::core::plugin_host::ComponentFactory f;
  f.name = "View";
  f.implementation = flexui::core::plugin_host::ComponentImplementation::kCapi;
  f.create = [](flexui::core::commit_pipeline::ComponentContext& ctx) {
    return std::make_unique<ViewComponent>(ctx);
  };
  return f;
}

}  // namespace flexui::components::view

namespace flexui::components {
flexui::core::plugin_host::ComponentFactory MakeViewFactory() {
  return view::MakeViewFactoryImpl();
}
}  // namespace flexui::components
