/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/text/text_component.h"

#if defined(FLEXUI_OHOS)
#include <arkui/native_node.h>
#include "flexui/common/log_tag.h"

namespace flexui::components::text::platform {

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
  ArkUI_NodeHandle h = NodeApi()->createNode(ARKUI_NODE_TEXT);
  FLEXUI_TLOG(Component, TextNativeCreate, DEBUG) << "handle=" << h;
  return h;
}

void ApplyProps(flexui::core::commit_pipeline::NodeHandle h, const TextProps& p) {
  auto* api  = NodeApi();
  auto* node = static_cast<ArkUI_NodeHandle>(h);
  if (!p.text.empty()) {
    ArkUI_AttributeItem item = {.string = p.text.c_str()};
    api->setAttribute(node, NODE_TEXT_CONTENT, &item);
  }
  if (p.font_size > 0) {
    ArkUI_NumberValue v[] = {{.f32 = static_cast<float>(p.font_size)}};
    ArkUI_AttributeItem item = {v, 1, nullptr};
    api->setAttribute(node, NODE_FONT_SIZE, &item);
  }
  if (!p.color.empty() && p.color[0] == '#' && p.color.size() == 7) {
    uint32_t argb = 0xFF000000u | std::stoul(p.color.substr(1), nullptr, 16);
    ArkUI_NumberValue v[] = {{.u32 = argb}};
    ArkUI_AttributeItem item = {v, 1, nullptr};
    api->setAttribute(node, NODE_FONT_COLOR, &item);
  }
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
  ArkUI_NumberValue pos[] = {{.f32 = r.x}, {.f32 = r.y}};
  ArkUI_AttributeItem pi = {pos, 2, nullptr};
  api->setAttribute(node, NODE_POSITION, &pi);
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

}  // namespace flexui::components::text::platform
#endif  // FLEXUI_OHOS
