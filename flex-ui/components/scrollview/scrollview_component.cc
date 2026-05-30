/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/scrollview/scrollview_component.h"

#include "flexui/common/log_tag.h"
#include "flexui/core/plugin-host/component_factory.h"

namespace flexui::components::scrollview::platform {
flexui::core::commit_pipeline::NodeHandle CreateNode();
void ApplyProps(flexui::core::commit_pipeline::NodeHandle, const ScrollViewProps&);
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle,
                 const flexui::core::commit_pipeline::LayoutRect&);
void Mount(flexui::core::commit_pipeline::NodeHandle,
           flexui::core::commit_pipeline::NodeHandle, uint32_t);
void Unmount(flexui::core::commit_pipeline::NodeHandle);
}  // namespace flexui::components::scrollview::platform

namespace flexui::components::scrollview {

namespace {
void Apply(ScrollViewProps& p, const flexui::core::commit_pipeline::PropDelta& d) {
  for (auto& kv : d.updated) {
    const auto& k = kv.first;
    const auto& v = kv.second;
    if (k == "direction")    p.direction = v.IsString() ? v.ToStringChecked() : "vertical";
    else if (k == "bounces") p.bounces   = v.IsBoolean() ? v.ToBooleanChecked() : true;
    else if (k == "width")   p.width     = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
    else if (k == "height")  p.height    = v.IsNumber() ? v.ToDoubleChecked() : 0.0;
  }
}
}  // namespace

ScrollViewComponent::ScrollViewComponent(flexui::core::commit_pipeline::ComponentContext&) {
  FLEXUI_TLOG(Component, ScrollViewCreate, DEBUG);
}
ScrollViewComponent::~ScrollViewComponent() = default;

flexui::core::commit_pipeline::NodeHandle ScrollViewComponent::OnCreate() {
  if (!handle_) handle_ = platform::CreateNode();
  return handle_;
}
void ScrollViewComponent::OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& d) {
  Apply(props_, d);
  if (handle_) platform::ApplyProps(handle_, props_);
}
void ScrollViewComponent::OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& r) {
  if (handle_) platform::ApplyLayout(handle_, r);
}
void ScrollViewComponent::OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t idx) {
  platform::Mount(parent, handle_, idx);
}
void ScrollViewComponent::OnUnmount() {
  if (handle_) { platform::Unmount(handle_); handle_ = nullptr; }
}

static flexui::core::plugin_host::ComponentFactory MakeScrollViewFactoryImpl() {
  flexui::core::plugin_host::ComponentFactory f;
  f.name = "ScrollView";
  f.implementation = flexui::core::plugin_host::ComponentImplementation::kCapi;
  f.create = [](flexui::core::commit_pipeline::ComponentContext& ctx) {
    return std::make_unique<ScrollViewComponent>(ctx);
  };
  return f;
}

}  // namespace flexui::components::scrollview

namespace flexui::components {
flexui::core::plugin_host::ComponentFactory MakeScrollViewFactory() {
  return scrollview::MakeScrollViewFactoryImpl();
}
}  // namespace flexui::components
