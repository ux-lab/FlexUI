/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/button/button_component.h"

#include "flexui/common/log_tag.h"
#include "flexui/core/plugin-host/component_factory.h"

namespace flexui::components::button::platform {
flexui::core::commit_pipeline::NodeHandle CreateNode();
void ApplyProps(flexui::core::commit_pipeline::NodeHandle, const ButtonProps&);
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle,
                 const flexui::core::commit_pipeline::LayoutRect&);
void Mount(flexui::core::commit_pipeline::NodeHandle,
           flexui::core::commit_pipeline::NodeHandle, uint32_t);
void Unmount(flexui::core::commit_pipeline::NodeHandle);
void RegisterClickListener(flexui::core::commit_pipeline::NodeHandle,
                           std::function<void()> cb);
}  // namespace flexui::components::button::platform

namespace flexui::components::button {

namespace {
void Apply(ButtonProps& p, const flexui::core::commit_pipeline::PropDelta& d) {
  for (auto& kv : d.updated) {
    const auto& k = kv.first;
    const auto& v = kv.second;
    if (k == "title")                p.title            = v.IsString() ? v.ToStringChecked() : "";
    else if (k == "onClick")         p.on_click         = v.IsString() ? v.ToStringChecked() : "";
    else if (k == "backgroundColor") p.background_color = v.IsString() ? v.ToStringChecked() : "";
  }
}
}  // namespace

ButtonComponent::ButtonComponent(flexui::core::commit_pipeline::ComponentContext&) {
  FLEXUI_TLOG(Component, ButtonCreate, DEBUG);
}
ButtonComponent::~ButtonComponent() = default;

flexui::core::commit_pipeline::NodeHandle ButtonComponent::OnCreate() {
  if (!handle_) {
    handle_ = platform::CreateNode();
    platform::RegisterClickListener(handle_, [this]() {
      FLEXUI_TLOG(Component, ButtonClick, DEBUG) << "handler=" << props_.on_click;
      OnEvent("onClick", flexui::common::FlexUIValue(props_.on_click));
    });
  }
  return handle_;
}
void ButtonComponent::OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& d) {
  Apply(props_, d);
  if (handle_) platform::ApplyProps(handle_, props_);
}
void ButtonComponent::OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& r) {
  if (handle_) platform::ApplyLayout(handle_, r);
}
void ButtonComponent::OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t idx) {
  platform::Mount(parent, handle_, idx);
}
void ButtonComponent::OnEvent(const std::string& name,
                              const flexui::common::FlexUIValue& payload) {
  FLEXUI_TLOG(Component, ButtonEvent, DEBUG) << "name=" << name;
  (void)payload;
}
void ButtonComponent::OnUnmount() {
  if (handle_) { platform::Unmount(handle_); handle_ = nullptr; }
}

static flexui::core::plugin_host::ComponentFactory MakeButtonFactoryImpl() {
  flexui::core::plugin_host::ComponentFactory f;
  f.name = "Button";
  f.implementation = flexui::core::plugin_host::ComponentImplementation::kCapi;
  f.create = [](flexui::core::commit_pipeline::ComponentContext& ctx) {
    return std::make_unique<ButtonComponent>(ctx);
  };
  return f;
}

}  // namespace flexui::components::button

namespace flexui::components {
flexui::core::plugin_host::ComponentFactory MakeButtonFactory() {
  return button::MakeButtonFactoryImpl();
}
}  // namespace flexui::components
