/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/button/button_component.h"

#if defined(FLEXUI_OHOS)
#include <arkui/native_node.h>
#include "flexui/common/log_tag.h"

namespace flexui::components::button::platform {

namespace {
ArkUI_NativeNodeAPI_1* NodeApi() {
  static ArkUI_NativeNodeAPI_1* api = []() -> ArkUI_NativeNodeAPI_1* {
    ArkUI_NativeNodeAPI_1* a = nullptr;
    OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_NODE, ArkUI_NativeNodeAPI_1, a);
    return a;
  }();
  return api;
}
}  // namespace

flexui::core::commit_pipeline::NodeHandle CreateNode() {
  ArkUI_NodeHandle h = NodeApi()->createNode(ARKUI_NODE_BUTTON);
  FLEXUI_TLOG(Component, ButtonNativeCreate, DEBUG) << "handle=" << h;
  return h;
}
void ApplyProps(flexui::core::commit_pipeline::NodeHandle, const ButtonProps&) {
  FLEXUI_TLOG(Component, ButtonApplyProps, DEBUG);
}
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle h,
                 const flexui::core::commit_pipeline::LayoutRect& r) {
  auto* api  = NodeApi();
  auto* node = static_cast<ArkUI_NodeHandle>(h);
  ArkUI_NumberValue w[] = {{.f32 = r.width}};
  ArkUI_AttributeItem wi = {w, 1, nullptr};
  api->setAttribute(node, NODE_WIDTH, &wi);
  ArkUI_NumberValue ht[] = {{.f32 = r.height}};
  ArkUI_AttributeItem hi = {ht, 1, nullptr};
  api->setAttribute(node, NODE_HEIGHT, &hi);
}
void Mount(flexui::core::commit_pipeline::NodeHandle parent,
           flexui::core::commit_pipeline::NodeHandle child,
           uint32_t index) {
  if (!parent) return;
  NodeApi()->insertChildAt(static_cast<ArkUI_NodeHandle>(parent),
                           static_cast<ArkUI_NodeHandle>(child),
                           static_cast<int32_t>(index));
}
void Unmount(flexui::core::commit_pipeline::NodeHandle h) {
  if (!h) return;
  NodeApi()->disposeNode(static_cast<ArkUI_NodeHandle>(h));
}
void RegisterClickListener(flexui::core::commit_pipeline::NodeHandle h,
                           std::function<void()> cb) {
  // ArkUI C-API event registration — PoC: store callback, wire via NODE_ON_CLICK.
  // Full event dispatch wired in W9-W10 device integration.
  (void)h; (void)cb;
  FLEXUI_TLOG(Component, ButtonRegisterClick, DEBUG);
}
}  // namespace flexui::components::button::platform
#endif  // FLEXUI_OHOS
