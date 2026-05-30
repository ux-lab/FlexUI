/*
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2022 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// FlexUI: Taitank removed per spec §4.4; only Yoga is supported.

#include "flexui/core/layout/layout_node.h"
#include "flexui/core/layout/yoga_layout_node.h"

namespace flexui::core::layout {

LayoutNode::LayoutNode() = default;

LayoutNode::~LayoutNode() = default;

void InitLayoutConsts(LayoutEngineType /*type*/) {
  InitLayoutConstsYoga();
}

std::shared_ptr<LayoutNode> CreateLayoutNode(LayoutEngineType /*type*/, void* /*layout_config*/) {
  return CreateLayoutNodeYoga();
}

void* CreateLayoutConfig(LayoutEngineType /*type*/) {
  return nullptr;
}

void DestroyLayoutConfig(LayoutEngineType /*type*/, void* /*config*/) {
  // Yoga uses no heap config; nothing to free.
}

}  // namespace flexui::core::layout
