/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/text/text_component.h"

#if !defined(FLEXUI_OHOS)
#include "flexui/common/log_tag.h"

namespace flexui::components::text::platform {

static int kStub = 0;

flexui::core::commit_pipeline::NodeHandle CreateNode() {
  FLEXUI_TLOG(Component, TextStubCreate, WARNING) << "NOT_IMPLEMENTED";
  return &kStub;
}
void ApplyProps(flexui::core::commit_pipeline::NodeHandle, const TextProps&) {
  FLEXUI_TLOG(Component, TextStubApplyProps, DEBUG);
}
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle,
                 const flexui::core::commit_pipeline::LayoutRect&) {}
void Mount(flexui::core::commit_pipeline::NodeHandle,
           flexui::core::commit_pipeline::NodeHandle, uint32_t) {}
void Unmount(flexui::core::commit_pipeline::NodeHandle) {}

}  // namespace flexui::components::text::platform
#endif  // !FLEXUI_OHOS
