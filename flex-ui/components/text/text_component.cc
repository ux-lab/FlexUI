/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/text/text_component.h"

#include "flexui/common/log_tag.h"
#include "flexui/core/plugin-host/component_factory.h"

namespace flexui::components::text::platform {
flexui::core::commit_pipeline::NodeHandle CreateNode();
void ApplyProps(flexui::core::commit_pipeline::NodeHandle handle,
                const TextProps& props);
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle handle,
                 const flexui::core::commit_pipeline::LayoutRect& rect);
void Mount(flexui::core::commit_pipeline::NodeHandle parent,
           flexui::core::commit_pipeline::NodeHandle child,
           uint32_t index);
void Unmount(flexui::core::commit_pipeline::NodeHandle handle);
}  // namespace flexui::components::text::platform

namespace flexui::components::text {

namespace {
void Apply(TextProps& props, const flexui::core::commit_pipeline::PropDelta& delta) {
  for (auto& kv : delta.updated) {
    const auto& key = kv.first;
    const auto& v   = kv.second;
    if (key == "text")         props.text        = v.IsString() ? v.ToStringChecked() : "";
    else if (key == "color")   props.color       = v.IsString() ? v.ToStringChecked() : "";
    else if (key == "fontSize") props.font_size  = v.IsNumber() ? v.ToDoubleChecked() : 14.0;
    else if (key == "fontWeight") props.font_weight = v.IsString() ? v.ToStringChecked() : "";
    else if (key == "textAlign")  props.text_align  = v.IsString() ? v.ToStringChecked() : "";
    else if (key == "maxLines")   props.max_lines   = v.IsNumber() ? static_cast<int>(v.ToDoubleChecked()) : 0;
  }
}
}  // namespace

TextComponent::TextComponent(flexui::core::commit_pipeline::ComponentContext&) {
  FLEXUI_TLOG(Component, TextCreate, DEBUG);
}

TextComponent::~TextComponent() = default;

flexui::core::commit_pipeline::NodeHandle TextComponent::OnCreate() {
  if (!handle_) handle_ = platform::CreateNode();
  return handle_;
}

void TextComponent::OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& delta) {
  Apply(props_, delta);
  if (handle_) platform::ApplyProps(handle_, props_);
}

void TextComponent::OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& rect) {
  if (handle_) platform::ApplyLayout(handle_, rect);
}

void TextComponent::OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t index) {
  platform::Mount(parent, handle_, index);
}

void TextComponent::OnUnmount() {
  if (handle_) {
    platform::Unmount(handle_);
    handle_ = nullptr;
  }
}

static flexui::core::plugin_host::ComponentFactory MakeTextFactoryImpl() {
  flexui::core::plugin_host::ComponentFactory f;
  f.name = "Text";
  f.implementation = flexui::core::plugin_host::ComponentImplementation::kCapi;
  f.create = [](flexui::core::commit_pipeline::ComponentContext& ctx) {
    return std::make_unique<TextComponent>(ctx);
  };
  return f;
}

}  // namespace flexui::components::text

namespace flexui::components {
flexui::core::plugin_host::ComponentFactory MakeTextFactory() {
  return text::MakeTextFactoryImpl();
}
}  // namespace flexui::components
