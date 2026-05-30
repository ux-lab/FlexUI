/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/button/button_component.h"

#if !defined(FLEXUI_OHOS)
#include "flexui/common/log_tag.h"

namespace flexui::components::button::platform {

static int kStub = 0;

flexui::core::commit_pipeline::NodeHandle CreateNode() {
  FLEXUI_TLOG(Component, ButtonStubCreate, WARNING) << "NOT_IMPLEMENTED";
  return &kStub;
}
void ApplyProps(flexui::core::commit_pipeline::NodeHandle, const ButtonProps&) {}
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle,
                 const flexui::core::commit_pipeline::LayoutRect&) {}
void Mount(flexui::core::commit_pipeline::NodeHandle,
           flexui::core::commit_pipeline::NodeHandle, uint32_t) {}
void Unmount(flexui::core::commit_pipeline::NodeHandle) {}
void RegisterClickListener(flexui::core::commit_pipeline::NodeHandle,
                           std::function<void()>) {}
}  // namespace flexui::components::button::platform
#endif  // !FLEXUI_OHOS
