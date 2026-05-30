# FlexUI W7-W8 — Components + Built-in APIs + Dual Frontend Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring up the user-visible payload of FlexUI: five baseline capi components (Text, Image, View, Button, ScrollView) in `flex-ui/components/`, three built-in native APIs (console, timer, log) in `flex-ui/api/`, the built-in `card-js` frontend, and the `plugin-a2ui` MVP frontend. After this plan, a `FlexCardController` can load a bundle, render real components onto the screen, respond to `setData()`, dispatch click events, and the dual-frontend layering proof (C2) becomes testable: the same A2UI JSON renders byte-equal DomNode trees through both frontends.

**Architecture:** Each component file under `flex-ui/components/<name>/` exposes a `ComponentFactory` registered with the engine via a `ComponentsBasePackage` plugin. On Harmony, each component's `OnCreate` calls ArkUI C-API (`OH_ArkUI_NodeAPI_GetModuleInterface` + `createNode(ARKUI_NODE_*)`); on Android and iOS, the component file declares the interface but the platform impl is a stub. Each built-in API is a `NativeApi` registered through a `ApiBasePackage` plugin. The `card-js` frontend lives under `flex-ui/core/card-controller/frontends/card_js/` (because it's the primary frontend, kernel-adjacent) and exposes a `createElement`/`setData`/`registerHandler` global on the Scope. `plugin-a2ui/` is a fully optional plugin that parses A2UI JSON and applies `{{ binding }}` interpolation to produce a DomNode tree, sharing the W5-W6 reconciler.

**Tech Stack:** C++17, CMake 3.18+, GoogleTest, HarmonyOS ArkUI C-API (`<arkui/native_node.h>`), Yoga (already vendored W3-W4), QuickJS / JSVM (W3-W4), the W1-W6 stack.

**Reference spec:** `docs/superpowers/specs/2026-05-30-flexui-layering-design.md` §4.7 (component layer dual mode), §4.10 (dual-frontend contract + shared vdom), §6.1 (functional acceptance — every bullet in this plan), §8.4 (mandatory log coverage for Frontend / Component subsystems).

**Depends on:** W1-W6 plans complete.

**Out of scope (deferred to W9-W10):**
- Playground integration (`FlexCardWaterfallDemoPage.ets`).
- E2E automation tests on real device.
- Performance baseline measurement.
- A2UI complex components (table, datetime, audioplayer, choice_picker — Phase 2+).
- ets-builder example component (interface exposed, no example shipped — same as W5-W6).
- Real ArkUI C-API run on device (this plan ships ArkUI C-API code that compiles when `FLEXUI_OHOS` is set; W9-W10 exercises on device).
- snapshot acceleration of API injection.

**Deliverable definition of done:**
- 5 components (Text, Image, View, Button, ScrollView) implemented with capi mode. Cross-platform interfaces; HarmonyOS implementation calling ArkUI C-API; Android / iOS stubs.
- `ComponentsBasePackage` plugin auto-registered by `FlexUIEngine::Init`.
- 3 built-in APIs (`console.log`/`console.warn`/`console.error`, `setTimeout`/`clearTimeout`, `flexLog`) implemented in `flex-ui/api/`.
- `ApiBasePackage` plugin auto-registered.
- `card-js` frontend works: `createElement` produces DomNode trees, `setData` triggers re-render through reconciler → commit pipeline.
- `plugin-a2ui` MVP works: parses A2UI JSON, applies `{{ field }}` interpolation, supports `onClick: "handlerName"` event binding via handlers.js.
- **C2 PROOF**: integration test loads the same A2UI JSON via `a2ui-json` frontend and via a hand-coded `card-js` equivalent; the produced DomNode trees are byte-equal, and the mutation streams committed are identical (recorded through a test `ComponentInstance` backend).
- All unit tests green under ASan+UBSan. TSan curated subset still green.
- Coverage gates: `flex-ui/components/` ≥ 70%, `flex-ui/core/` ≥ 80%, `flex-ui/common/` ≥ 80%.
- Mandatory log coverage points per spec §8.4 present in Frontend, Component, Api subsystems.
- AGenUI tree untouched.

---

## File Structure

### Created in this plan

```
flex-ui/components/
├── CMakeLists.txt
├── include/flexui/components/
│   ├── components_base_package.h    # exports the package plugin
│   └── (no per-component public headers — capi components are internal)
├── text/
│   ├── text_component.h             # interface (cross-platform)
│   ├── text_component.cc            # cross-platform logic (props parsing, Yoga measure)
│   └── platform/
│       ├── text_component_harmony.cc
│       ├── text_component_android.cc    # stub
│       └── text_component_ios.mm        # stub
├── image/                           # same shape
├── view/                            # same shape
├── button/                          # same shape
├── scrollview/                      # same shape
└── src/
    └── components_base_package.cc

flex-ui/api/
├── CMakeLists.txt
├── include/flexui/api/
│   └── api_base_package.h
├── console/
│   ├── console_api.h
│   └── console_api.cc
├── timer/
│   ├── timer_api.h
│   └── timer_api.cc
├── log/
│   ├── log_api.h
│   └── log_api.cc
└── src/
    └── api_base_package.cc

flex-ui/core/card-controller/frontends/card_js/
├── CMakeLists.txt
├── card_js_frontend.h
├── card_js_frontend.cc
├── create_element_binding.cc
└── set_data_binding.cc

flex-ui/plugin-a2ui/
├── CMakeLists.txt
├── include/flexui/plugin_a2ui/
│   ├── a2ui_plugin.h               # exports the plugin
│   └── a2ui_json_frontend.h        # exports the frontend class
└── src/
    ├── a2ui_plugin.cc
    ├── a2ui_json_frontend.cc
    ├── a2ui_json_parser.cc          # JSON → DomNode template
    ├── a2ui_data_binder.cc          # {{ }} interpolation
    └── a2ui_handlers_runtime.cc     # handlers.js eval + dispatch

tests/flex-ui/unit/
├── (existing tests preserved)
├── components_text_test.cc
├── components_image_test.cc
├── components_view_test.cc
├── components_button_test.cc
├── components_scrollview_test.cc
├── api_console_test.cc
├── api_timer_test.cc
├── api_log_test.cc
├── card_js_frontend_test.cc
├── a2ui_json_frontend_test.cc
└── dual_frontend_parity_test.cc        # ★ THE C2 PROOF
```

### Modified

- `flex-ui/CMakeLists.txt` — `add_subdirectory(components)` + `add_subdirectory(api)` + `add_subdirectory(plugin-a2ui)`.
- `flex-ui/core/card-controller/flex_ui_engine.cc` — `Init()` auto-registers `ComponentsBasePackage` + `ApiBasePackage` + `card-js` frontend.
- `flex-ui/core/card-controller/flex_card_controller.cc` — `Load()` now uses `plugin_host.FindFrontend(options.bundle_type)` and drives it.

### Untouched

AGenUI tree.

---

## Tasks

### Task 1: Component CMake aggregator + base package skeleton

**Files:**
- Create: `flex-ui/components/CMakeLists.txt`
- Create: `flex-ui/components/include/flexui/components/components_base_package.h`
- Create: `flex-ui/components/src/components_base_package.cc`
- Modify: `flex-ui/CMakeLists.txt`

- [ ] **Step 1.1: Create directories + CMake**

```bash
for c in text image view button scrollview; do
  mkdir -p flex-ui/components/${c}/platform
done
mkdir -p flex-ui/components/include/flexui/components
mkdir -p flex-ui/components/src
```

`flex-ui/components/CMakeLists.txt`:

```cmake
add_library(flexui_components STATIC
  src/components_base_package.cc
  text/text_component.cc
  image/image_component.cc
  view/view_component.cc
  button/button_component.cc
  scrollview/scrollview_component.cc
)

# Platform-specific implementations selected at configure time.
if(FLEXUI_OHOS)
  target_sources(flexui_components PRIVATE
    text/platform/text_component_harmony.cc
    image/platform/image_component_harmony.cc
    view/platform/view_component_harmony.cc
    button/platform/button_component_harmony.cc
    scrollview/platform/scrollview_component_harmony.cc
  )
  target_link_libraries(flexui_components PRIVATE libace_ndk.z.so)
else()
  # Host build: link Android stubs so the binaries compile uniformly.
  target_sources(flexui_components PRIVATE
    text/platform/text_component_android.cc
    image/platform/image_component_android.cc
    view/platform/view_component_android.cc
    button/platform/button_component_android.cc
    scrollview/platform/scrollview_component_android.cc
  )
endif()

target_include_directories(flexui_components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(flexui_components PUBLIC
  flexui_common
  flexui_core_vdom
  flexui_core_layout
  flexui_core_plugin_host
  flexui_core_commit_pipeline
)
```

- [ ] **Step 1.2: Write `components_base_package.h`**

```cpp
#pragma once

#include "flexui/core/plugin-host/plugin.h"

namespace flexui::components {

// Returns the ComponentsBase plugin (registers Text, Image, View, Button,
// ScrollView). FlexUIEngine::Init() installs it automatically.
flexui::core::plugin_host::FlexUIPlugin MakeComponentsBasePackage();

}  // namespace flexui::components
```

- [ ] **Step 1.3: Write stub `components_base_package.cc`**

```cpp
#include "flexui/components/components_base_package.h"

#include "flexui/common/log_tag.h"

namespace flexui::components {

// Component factory forward decls (defined in each component's .cc).
flexui::core::plugin_host::ComponentFactory MakeTextFactory();
flexui::core::plugin_host::ComponentFactory MakeImageFactory();
flexui::core::plugin_host::ComponentFactory MakeViewFactory();
flexui::core::plugin_host::ComponentFactory MakeButtonFactory();
flexui::core::plugin_host::ComponentFactory MakeScrollViewFactory();

flexui::core::plugin_host::FlexUIPlugin MakeComponentsBasePackage() {
  FLEXUI_TLOG(Plugin, MakeComponentsBase, INFO);
  flexui::core::plugin_host::FlexUIPlugin p;
  p.name = "flexui-components-base";
  p.version = "0.1.0";
  p.components = {
    MakeTextFactory(),
    MakeImageFactory(),
    MakeViewFactory(),
    MakeButtonFactory(),
    MakeScrollViewFactory(),
  };
  return p;
}

}  // namespace flexui::components
```

- [ ] **Step 1.4: Update `flex-ui/CMakeLists.txt`**

Append:
```cmake
add_subdirectory(components)
add_subdirectory(api)
add_subdirectory(plugin-a2ui)
```

(api and plugin-a2ui CMakeLists land in Tasks 5 and 8 — for now create empty placeholders so the build doesn't break.)

- [ ] **Step 1.5: Add temporary stub factories so the link succeeds**

In each per-component `.cc` (created in Tasks 2-4), provide the matching `Make<X>Factory()` symbol — but for this initial step write all five as stub `text/text_component.cc`, etc., each returning a no-op factory:

```cpp
// text/text_component.cc (replaced fully in Task 2)
#include "flexui/core/plugin-host/component_factory.h"
namespace flexui::components {
flexui::core::plugin_host::ComponentFactory MakeTextFactory() {
  return {"Text", flexui::core::plugin_host::ComponentImplementation::kCapi,
          [](auto&){ return nullptr; }};
}
}  // namespace flexui::components
```

Repeat for image / view / button / scrollview with their respective names.

- [ ] **Step 1.6: Build, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/components flex-ui/CMakeLists.txt
git commit -m "feat(flex-ui/components): scaffold + ComponentsBasePackage skeleton"
```

---

### Task 2: Text component (capi mode, Harmony impl + Android stub)

**Files:**
- Create: `flex-ui/components/text/text_component.h`
- Replace: `flex-ui/components/text/text_component.cc`
- Create: `flex-ui/components/text/platform/text_component_harmony.cc`
- Create: `flex-ui/components/text/platform/text_component_android.cc`
- Create: `tests/flex-ui/unit/components_text_test.cc`

- [ ] **Step 2.1: Write `text_component.h` (cross-platform)**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Text component. capi mode. Cross-platform interface; platform-specific
 * backend selected at link time.
 */
#pragma once

#include <string>

#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::components::text {

struct TextProps {
  std::string text;
  std::string color;          // "#rrggbb" or named color
  double font_size = 14.0;
  std::string font_weight;    // "normal" / "bold" / numeric
  std::string text_align;     // "left" / "center" / "right"
  int max_lines = 0;          // 0 = no limit
};

// Cross-platform Text instance. Each platform impl provides the
// PlatformImpl::CreateNode / ApplyProps / Destroy.
class TextComponent : public flexui::core::commit_pipeline::ComponentInstance {
 public:
  explicit TextComponent(flexui::core::commit_pipeline::ComponentContext& ctx);
  ~TextComponent() override;

  flexui::core::commit_pipeline::NodeHandle OnCreate() override;
  void OnUpdateProps(const flexui::core::commit_pipeline::PropDelta& delta) override;
  void OnUpdateLayout(const flexui::core::commit_pipeline::LayoutRect& rect) override;
  void OnMount(flexui::core::commit_pipeline::NodeHandle parent, uint32_t index) override;
  void OnUnmount() override;

  const TextProps& props() const { return props_; }

 private:
  TextProps props_;
  flexui::core::commit_pipeline::NodeHandle handle_ = nullptr;
};

}  // namespace flexui::components::text
```

- [ ] **Step 2.2: Write `text_component.cc` (cross-platform logic)**

```cpp
#include "flexui/components/text/text_component.h"

#include "flexui/common/log_tag.h"
#include "flexui/core/plugin-host/component_factory.h"

// Platform-specific backend hooks. Defined in platform/*.cc.
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

void Apply(TextProps& props,
           const flexui::core::commit_pipeline::PropDelta& delta) {
  for (auto& kv : delta.updated) {
    const auto& key = kv.first;
    const auto& v   = kv.second;
    if (key == "text")        props.text = v.IsString() ? v.ToString() : "";
    else if (key == "color")  props.color = v.IsString() ? v.ToString() : "";
    else if (key == "fontSize") props.font_size = v.IsNumber() ? v.ToDouble() : 14.0;
    else if (key == "fontWeight") props.font_weight = v.IsString() ? v.ToString() : "";
    else if (key == "textAlign")  props.text_align  = v.IsString() ? v.ToString() : "";
    else if (key == "maxLines")   props.max_lines   = v.IsNumber() ? static_cast<int>(v.ToDouble()) : 0;
  }
  for (auto& key : delta.deleted) {
    if (key == "text") props.text.clear();
    // remaining keys reset to defaults
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

void TextComponent::OnUpdateProps(
    const flexui::core::commit_pipeline::PropDelta& delta) {
  Apply(props_, delta);
  if (handle_) platform::ApplyProps(handle_, props_);
}

void TextComponent::OnUpdateLayout(
    const flexui::core::commit_pipeline::LayoutRect& rect) {
  if (handle_) platform::ApplyLayout(handle_, rect);
}

void TextComponent::OnMount(flexui::core::commit_pipeline::NodeHandle parent,
                            uint32_t index) {
  platform::Mount(parent, handle_, index);
}

void TextComponent::OnUnmount() {
  if (handle_) {
    platform::Unmount(handle_);
    handle_ = nullptr;
  }
}

flexui::core::plugin_host::ComponentFactory MakeTextFactory() {
  flexui::core::plugin_host::ComponentFactory f;
  f.name = "Text";
  f.implementation = flexui::core::plugin_host::ComponentImplementation::kCapi;
  f.create = [](flexui::core::commit_pipeline::ComponentContext& ctx) {
    return std::make_unique<TextComponent>(ctx);
  };
  return f;
}

}  // namespace flexui::components::text

// Top-level factory accessor for ComponentsBasePackage.
namespace flexui::components {
flexui::core::plugin_host::ComponentFactory MakeTextFactory() {
  return text::MakeTextFactory();
}
}
```

- [ ] **Step 2.3: Write `text_component_harmony.cc` (ArkUI C-API)**

```cpp
#include "flexui/components/text/text_component.h"

#if defined(FLEXUI_OHOS)
#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>
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
  ArkUI_NodeHandle handle = NodeApi()->createNode(ARKUI_NODE_TEXT);
  FLEXUI_TLOG(Component, TextNativeCreate, DEBUG) << "handle=" << handle;
  return handle;
}

void ApplyProps(flexui::core::commit_pipeline::NodeHandle h,
                const TextProps& p) {
  auto* api = NodeApi();
  auto* node = static_cast<ArkUI_NodeHandle>(h);

  if (!p.text.empty()) {
    ArkUI_AttributeItem item = { .string = p.text.c_str() };
    api->setAttribute(node, NODE_TEXT_CONTENT, &item);
  }
  if (p.font_size > 0) {
    ArkUI_NumberValue v[] = { { .f32 = static_cast<float>(p.font_size) } };
    ArkUI_AttributeItem item = { v, 1, nullptr };
    api->setAttribute(node, NODE_FONT_SIZE, &item);
  }
  if (!p.color.empty() && p.color[0] == '#' && p.color.size() == 7) {
    uint32_t argb = 0xFF000000u | std::stoul(p.color.substr(1), nullptr, 16);
    ArkUI_NumberValue v[] = { { .u32 = argb } };
    ArkUI_AttributeItem item = { v, 1, nullptr };
    api->setAttribute(node, NODE_FONT_COLOR, &item);
  }
}

void ApplyLayout(flexui::core::commit_pipeline::NodeHandle h,
                 const flexui::core::commit_pipeline::LayoutRect& r) {
  auto* api = NodeApi();
  auto* node = static_cast<ArkUI_NodeHandle>(h);
  ArkUI_NumberValue size[] = { {.f32 = r.width}, {.f32 = r.height} };
  ArkUI_AttributeItem size_item = { size, 2, nullptr };
  api->setAttribute(node, NODE_WIDTH,  &size_item);   // PoC: width attr (real width set via WIDTH_PERCENT not used)
  api->setAttribute(node, NODE_HEIGHT, &size_item);

  ArkUI_NumberValue pos[] = { {.f32 = r.x}, {.f32 = r.y} };
  ArkUI_AttributeItem pos_item = { pos, 2, nullptr };
  api->setAttribute(node, NODE_POSITION, &pos_item);
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
```

- [ ] **Step 2.4: Write `text_component_android.cc` (stub)**

```cpp
#include "flexui/components/text/text_component.h"

#if !defined(FLEXUI_OHOS)
#include "flexui/common/log_tag.h"

namespace flexui::components::text::platform {

flexui::core::commit_pipeline::NodeHandle CreateNode() {
  FLEXUI_TLOG(Component, TextStubCreate, WARNING) << "NOT_IMPLEMENTED";
  static int dummy = 0;
  return &dummy;
}
void ApplyProps(flexui::core::commit_pipeline::NodeHandle,
                const TextProps&) {
  FLEXUI_TLOG(Component, TextStubApplyProps, DEBUG);
}
void ApplyLayout(flexui::core::commit_pipeline::NodeHandle,
                 const flexui::core::commit_pipeline::LayoutRect&) {}
void Mount(flexui::core::commit_pipeline::NodeHandle,
           flexui::core::commit_pipeline::NodeHandle, uint32_t) {}
void Unmount(flexui::core::commit_pipeline::NodeHandle) {}

}  // namespace flexui::components::text::platform
#endif
```

- [ ] **Step 2.5: Write `components_text_test.cc`**

```cpp
#include <gtest/gtest.h>
#include "flexui/components/text/text_component.h"

namespace flexui::components::text {

class FakeCtx : public flexui::core::commit_pipeline::ComponentContext {
 public:
  uint32_t scope_id() const override { return 1; }
  uint32_t root_id()  const override { return 1; }
};

TEST(TextComponentTest, OnUpdatePropsCarriesText) {
  FakeCtx ctx;
  TextComponent c(ctx);
  c.OnCreate();
  flexui::core::commit_pipeline::PropDelta delta;
  delta.updated.push_back({"text", flexui::common::FlexUIValue(std::string("hello"))});
  delta.updated.push_back({"fontSize", flexui::common::FlexUIValue(18.0)});
  c.OnUpdateProps(delta);
  EXPECT_EQ(c.props().text, "hello");
  EXPECT_DOUBLE_EQ(c.props().font_size, 18.0);
  c.OnUnmount();
}

}  // namespace flexui::components::text
```

- [ ] **Step 2.6: Wire test, build, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/components/text tests/flex-ui/unit
git commit -m "feat(flex-ui/components): Text capi component (Harmony ArkUI + Android stub)"
```

---

### Task 3: View, Image, Button, ScrollView components

Each follows the **same shape as Text** (Task 2): a `<name>_component.h` with cross-platform interface, `<name>_component.cc` with cross-platform props/Yoga logic, `platform/<name>_component_harmony.cc` with ArkUI C-API mapping, `platform/<name>_component_android.cc` as stub, `tests/flex-ui/unit/components_<name>_test.cc` for props-roundtrip.

ArkUI C-API node types per spec §4.7:
- View → `ARKUI_NODE_STACK` (Stack composes children freely, used as generic container)
- Image → `ARKUI_NODE_IMAGE`
- Button → `ARKUI_NODE_BUTTON`
- ScrollView → `ARKUI_NODE_SCROLL`

Props each component accepts (extend `TextProps`-style struct per component; PoC subset):

| Component | Props |
|---|---|
| View | `backgroundColor`, `borderRadius`, `padding`, `flexDirection`, `width`, `height` |
| Image | `src`, `width`, `height`, `objectFit` ("contain"/"cover"/"stretch") |
| Button | `title`, `onClick` (handler-name string), `backgroundColor` |
| ScrollView | `direction` ("vertical"/"horizontal"), `bounces`, `width`, `height` |

For each component (4 total):
- [ ] **Step 3.X.1: Write `<name>_component.h` + `<name>_component.cc`** (mirror Task 2.1, 2.2 with the component-specific props struct)
- [ ] **Step 3.X.2: Write `platform/<name>_component_harmony.cc`** (mirror Task 2.3 with the specific `ARKUI_NODE_*` and ArkUI attribute IDs; e.g. `NODE_BACKGROUND_COLOR`, `NODE_BORDER_RADIUS`, `NODE_IMAGE_SRC`, `NODE_BUTTON_LABEL`, `NODE_SCROLL_SCROLL_DIRECTION`)
- [ ] **Step 3.X.3: Write `platform/<name>_component_android.cc`** (stub, mirror Task 2.4)
- [ ] **Step 3.X.4: Write `tests/flex-ui/unit/components_<name>_test.cc`** (mirror Task 2.5: assert props round-trip through `OnUpdateProps`)
- [ ] **Step 3.X.5: Build green, commit**

Each component commit message: `feat(flex-ui/components): <Name> capi component (Harmony ArkUI + Android stub)`. Five components total = five commits, one per (View, Image, Button, ScrollView). (Text was Task 2.)

Button has one extra piece: in `button_component_harmony.cc`, register an `ON_CLICK` listener via `ArkUI_NodeEventType::NODE_ON_CLICK`, register the node with the engine's event dispatch, and on event dispatch back into `ComponentInstance::OnEvent("onClick", payload)`. Event routing to the JS handler is wired in Task 6 (card-js frontend). Keep the harmony Button impl thin: register the listener, plumb to OnEvent; the dispatch path to JS is the frontend's job.

---

### Task 4: ComponentsBase package — final wire-up + auto-register

Replace the stub `Make<X>Factory()` calls in `components_base_package.cc` with the real ones (which Tasks 2-3 already defined in each `<name>_component.cc`).

Also extend `FlexUIEngine::Init()` (in `flex-ui/core/card-controller/flex_ui_engine.cc`) to auto-install the package:

```cpp
// flex_ui_engine.cc, inside Init() after plugin_host setup:
auto pkg = flexui::components::MakeComponentsBasePackage();
auto err = plugins_->Install(std::move(pkg));
if (!err.ok()) {
  FLEXUI_TLOG(Engine, AutoInstallComponentsBase, ERROR) << err.message();
  return err;
}
```

Same pattern for `ApiBasePackage` (Task 7) and `card-js` frontend (Task 6).

- [ ] **Step 4.1: Replace stub factories in `components_base_package.cc`**

The header-only forward decls already match: `flexui::components::MakeTextFactory()` etc. They're defined in the per-component `.cc` (Tasks 2-3). No code changes needed here if you preserved the symbol shape.

- [ ] **Step 4.2: Wire auto-install in `flex_ui_engine.cc`** (after `plugins_` construction)

```cpp
#include "flexui/components/components_base_package.h"
// inside Init():
{
  auto pkg = flexui::components::MakeComponentsBasePackage();
  auto err = plugins_->Install(std::move(pkg));
  if (!err.ok()) return err;
}
```

- [ ] **Step 4.3: Build, run flex_card_controller_test (now creates 5 real components), commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/card-controller flex-ui/components/src
git commit -m "feat(flex-ui): FlexUIEngine::Init auto-registers ComponentsBasePackage"
```

---

### Task 5: API CMake aggregator + base package skeleton

**Files:**
- Create: `flex-ui/api/CMakeLists.txt`
- Create: `flex-ui/api/include/flexui/api/api_base_package.h`
- Create: `flex-ui/api/src/api_base_package.cc`

Same pattern as Task 1 but for APIs. CMakeLists builds `flexui_api` static library linking `flexui_common` + `flexui_core_plugin_host`. Make stubs for `MakeConsoleApi()`, `MakeTimerApi()`, `MakeLogApi()`. `MakeApiBasePackage()` aggregates the three.

- [ ] **Step 5.1: Write `api_base_package.h` + `.cc`** (mirror Task 1.2 / 1.3)
- [ ] **Step 5.2: Add `mkdir -p flex-ui/api/{console,timer,log}` and write CMakeLists.txt similar to components**
- [ ] **Step 5.3: Build, commit**

```bash
git add flex-ui/api
git commit -m "feat(flex-ui/api): scaffold + ApiBasePackage skeleton"
```

---

### Task 6: `card-js` frontend — createElement + setData + registerHandler

**Files:**
- Create: `flex-ui/core/card-controller/frontends/card_js/CMakeLists.txt`
- Create: `flex-ui/core/card-controller/frontends/card_js/card_js_frontend.h`
- Create: `flex-ui/core/card-controller/frontends/card_js/card_js_frontend.cc`
- Create: `tests/flex-ui/unit/card_js_frontend_test.cc`
- Modify: `flex-ui/core/card-controller/CMakeLists.txt`
- Modify: `flex-ui/core/card-controller/flex_ui_engine.cc` (auto-register)
- Modify: `flex-ui/core/card-controller/flex_card_controller.cc` (use frontend)

- [ ] **Step 6.1: Write `card_js_frontend.h`**

```cpp
#pragma once

#include "flexui/core/plugin-host/frontend.h"
#include "flexui/core/js-engine/ijs_context.h"

namespace flexui::core::card_controller::frontends::card_js {

class CardJsFrontend : public flexui::core::plugin_host::IFrontend {
 public:
  std::string Name() const override { return "card-js"; }
  std::string BundleType() const override { return "card-js"; }

  void Initialize(flexui::core::scope_manager::Scope& scope,
                  const flexui::core::plugin_host::BundleSource& bundle) override;
  std::shared_ptr<flexui::core::vdom::DomNode> Render(
      flexui::core::scope_manager::Scope& scope,
      const flexui::common::FlexUIValue& data) override;
  void HandleEvent(flexui::core::scope_manager::Scope& scope,
                   const std::string& event_name,
                   const flexui::common::FlexUIValue& payload) override;

 private:
  std::shared_ptr<flexui::core::js_engine::IJsValue> render_fn_;
  std::shared_ptr<flexui::core::js_engine::IJsValue> handlers_;
  std::atomic<uint32_t> next_node_id_{1};
};

flexui::core::plugin_host::FrontendRegistration MakeCardJsRegistration();

}  // namespace flexui::core::card_controller::frontends::card_js
```

- [ ] **Step 6.2: Write `card_js_frontend.cc`**

Key responsibilities:

1. `Initialize`:
   - Inject globals: `createElement(type, props, children)`, `setData(data)`, `registerHandler(name, fn)`, and stubs that the api/ package will later re-inject.
   - `Eval(bundle.text, bundle.uri)`. The bundle module assigns `globalThis.__flexui_exports = { render, handlers }`.
   - Read `__flexui_exports.render` and `__flexui_exports.handlers` into member variables.

2. `Render`:
   - Convert `data` (FlexUIValue) → JS value, call `render_fn_({data})`, get back a JS object describing the vdom: `{ type: "View", props: {...}, children: [...] }`.
   - Recursively convert the JS object tree → `DomNode` tree, allocating IDs from `next_node_id_`.

3. `HandleEvent`:
   - Look up `handlers_.GetProperty(event_name)`, call it with `payload`.
   - The handler likely calls `setData(...)` internally, which triggers the next Render through the controller.

For `createElement` global: inject as a native function. The implementation receives JS args (type-string, props-object, children-array). Build a JS object `{type, props, children}` and return it. (We don't allocate `DomNode` here — vdom is JS until Render rebuilds it on the C++ side. This keeps the JS-side createElement composable.)

For `setData` global: inject as a native function. Body posts a task to `FlexUIEngine::ui_runner()` to call `controller.SetData(args[0]->ToFlexUIValue())` — but for circularity avoidance, post into a `ScopeData` callback registered by the controller. Simpler PoC approach: `setData` writes into a `Scope`-local slot (`scope.set_pending_data(...)`); the controller polls/drains on its next tick.

The simplest PoC wiring: introduce a `Scope::SetSetDataSink(std::function<void(FlexUIValue)>)`. The Controller installs a sink that calls `FlexCardController::SetData(...)` on itself. `setData` global invokes the sink.

For PoC the full `card_js_frontend.cc` is ~300 lines. The above describes the structure; the executor writes the body using the patterns already established in W3-W4 / W5-W6 (`InjectGlobalFunction`, `FromFlexUIValue`, etc.). Every public method begins with `FLEXUI_TLOG(Frontend, CardJs<Event>, DEBUG)`.

- [ ] **Step 6.3: Write `tests/flex-ui/unit/card_js_frontend_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/core/card-controller/flex_card_controller.h"
#include "flexui/core/plugin-host/plugin_host.h"

namespace flexui::core::card_controller {

class CardJsFrontendTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FlexUIEngineConfig cfg;
    cfg.backend = js_engine::JsEngineBackend::kQuickJS;
    FlexUIEngine::Instance().Init(cfg);
  }
  void TearDown() override { FlexUIEngine::Instance().Shutdown(); }
};

TEST_F(CardJsFrontendTest, LoadAndFirstRender) {
  FlexCardControllerOptions opts;
  opts.bundle_type = "card-js";
  opts.bundle_inline = R"JS(
    function render(data) {
      return createElement('View', { padding: 8 }, [
        createElement('Text', { text: 'Hello ' + data.name, fontSize: 16 })
      ]);
    }
    globalThis.__flexui_exports = { render, handlers: {} };
  )JS";
  opts.initial_data = flexui::common::FlexUIValue(
      flexui::common::FlexUIValue::FlexUIValueObjectType{
          {"name", flexui::common::FlexUIValue(std::string("World"))}});
  FlexCardController card(std::move(opts));
  EXPECT_TRUE(card.Load().ok());
  // For the test we drain pending JS jobs and inspect the controller's
  // last committed DomNode tree, exposed via a debug helper.
  // (Helper added by this task.)
}

TEST_F(CardJsFrontendTest, SetDataTriggersRerender) {
  // Build a card-js bundle whose render shows data.count.
  // After Load, call card.SetData({count: 5}) and assert the controller's
  // last DomNode tree has the updated Text content.
}

}  // namespace flexui::core::card_controller
```

(Implement the debug helper `FlexCardController::DebugLastDomTree() -> std::shared_ptr<DomNode>` for tests. Spec §8.5 allows debug-only API; this helper is purely test-side.)

- [ ] **Step 6.4: Wire auto-register in `flex_ui_engine.cc`**

After the components-base auto-install:

```cpp
{
  flexui::core::plugin_host::FlexUIPlugin p;
  p.name = "flexui-frontend-card-js";
  p.frontends.push_back(
      flexui::core::card_controller::frontends::card_js::MakeCardJsRegistration());
  auto err = plugins_->Install(std::move(p));
  if (!err.ok()) return err;
}
```

- [ ] **Step 6.5: Wire frontend resolution in `flex_card_controller.cc::Load()`**

Replace the W5-W6 stub Load with:

```cpp
auto* reg = FlexUIEngine::Instance().plugin_host().FindFrontend(options_.bundle_type);
if (!reg) {
  state_.store(FlexCardState::kErrored);
  return flexui::common::Error(flexui::common::ErrorCode::kNotFound,
                               "no frontend for bundle_type: " + options_.bundle_type);
}
frontend_ = reg->factory();
flexui::core::plugin_host::BundleSource src;
src.uri = options_.bundle_uri;
src.text = options_.bundle_inline;
frontend_->Initialize(*scope_, src);
auto vdom = frontend_->Render(*scope_, options_.initial_data);
// Compute layout (Yoga) and diff against empty tree → produce MutationList → commit.
// PoC: full-tree create mutations.
ComputeInitialMutationsAndCommit(vdom);
```

Add `std::shared_ptr<plugin_host::IFrontend> frontend_;` member. `ComputeInitialMutationsAndCommit` walks the tree, emits `CreateMutation` + `UpdateLayoutMutation` per node, calls `CommitPipeline::Apply`.

- [ ] **Step 6.6: Build, run all tests, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/card-controller tests/flex-ui/unit
git commit -m "feat(flex-ui/core/card-controller/frontends/card_js): createElement + setData + Render + handler dispatch"
```

---

### Task 7: APIs — console / timer / log

**Files:**
- Create: `flex-ui/api/console/console_api.{h,cc}`
- Create: `flex-ui/api/timer/timer_api.{h,cc}`
- Create: `flex-ui/api/log/log_api.{h,cc}`
- Modify: `flex-ui/api/src/api_base_package.cc`

Each API exports a `flexui::core::plugin_host::NativeApi` (or several). `console.log` becomes a sync native API whose `invoke` lambda extracts args (each `FlexUIValue`), stringifies them, and emits a `FLEXUI_TLOG(Api, ConsoleLog, INFO)` line.

`setTimeout(callback, delay_ms)` is asynchronous: schedule via `flexui::common::OneShotTimer` on the engine's `js_runner` and call the stored callback. `clearTimeout(handle)` cancels.

`flexLog(subsystem, event, level, msg)` is the canonical structured-log entry point exposed to JS, mapping directly to `FLEXUI_TLOG(subsystem, event, level) << msg`.

The PoC injects these into each Scope via the frontend's `Initialize` rather than as plugin-level globals — simpler for W7-W8. Move into proper `NativeApi` registration in Phase 1+.

For each API:
- [ ] **Step 7.X.1: Write the `.h` + `.cc`** — pattern is `MakeConsoleApi() -> std::vector<NativeApi>` returning the apis.
- [ ] **Step 7.X.2: Write the matching `tests/flex-ui/unit/api_<name>_test.cc`** — exercises invocation through `IJsContext` (QuickJS) using a `CapturingLogSink` (from W1-W2) to assert side effects.
- [ ] **Step 7.X.3: Build, commit**

Tests must use the W1-W2 `ScopedCapturingSink` to verify that calling `console.log` from JS emits the expected `LogRecord`. This is a real integration test, not a stub.

Final commit:

```bash
git add flex-ui/api tests/flex-ui/unit
git commit -m "feat(flex-ui/api): console + timer + log built-in APIs + tests"
```

Then wire auto-install in `FlexUIEngine::Init()` (similar to Task 4).

---

### Task 8: `plugin-a2ui` MVP

**Files:**
- Create: `flex-ui/plugin-a2ui/CMakeLists.txt`
- Create: `flex-ui/plugin-a2ui/include/flexui/plugin_a2ui/a2ui_plugin.h`
- Create: `flex-ui/plugin-a2ui/include/flexui/plugin_a2ui/a2ui_json_frontend.h`
- Create: `flex-ui/plugin-a2ui/src/a2ui_plugin.cc`
- Create: `flex-ui/plugin-a2ui/src/a2ui_json_frontend.cc`
- Create: `flex-ui/plugin-a2ui/src/a2ui_json_parser.cc`
- Create: `flex-ui/plugin-a2ui/src/a2ui_data_binder.cc`
- Create: `flex-ui/plugin-a2ui/src/a2ui_handlers_runtime.cc`
- Create: `tests/flex-ui/unit/a2ui_json_frontend_test.cc`

- [ ] **Step 8.1: Write `a2ui_plugin.h`**

```cpp
#pragma once
#include "flexui/core/plugin-host/plugin.h"

namespace flexui::plugin_a2ui {

flexui::core::plugin_host::FlexUIPlugin MakeA2UIPlugin();

}  // namespace flexui::plugin_a2ui
```

- [ ] **Step 8.2: Write `a2ui_json_frontend.h`**

```cpp
#pragma once

#include "flexui/core/plugin-host/frontend.h"
#include "flexui/common/flexui_value.h"

namespace flexui::plugin_a2ui {

class A2UIJsonFrontend : public flexui::core::plugin_host::IFrontend {
 public:
  std::string Name() const override { return "a2ui-json"; }
  std::string BundleType() const override { return "a2ui-json"; }

  void Initialize(flexui::core::scope_manager::Scope& scope,
                  const flexui::core::plugin_host::BundleSource& bundle) override;
  std::shared_ptr<flexui::core::vdom::DomNode> Render(
      flexui::core::scope_manager::Scope& scope,
      const flexui::common::FlexUIValue& data) override;
  void HandleEvent(flexui::core::scope_manager::Scope& scope,
                   const std::string& event_name,
                   const flexui::common::FlexUIValue& payload) override;

 private:
  flexui::common::FlexUIValue json_doc_;
  std::optional<std::string> handlers_js_;
  std::atomic<uint32_t> next_node_id_{1};
};

}  // namespace flexui::plugin_a2ui
```

- [ ] **Step 8.3: Write the parser + binder + handlers runtime**

`a2ui_json_parser.cc`:
- Function `ParseA2UIJson(const std::string& text) -> FlexUIValue`. Use the W1-W2 serializer for now — actually we need a JSON parser, not the binary one. Use a minimal hand-written JSON parser (~150 lines, lexer + recursive descent) since FlexUIValue maps cleanly to JSON. The parser returns a `FlexUIValue` tree.

`a2ui_data_binder.cc`:
- Function `ApplyDataBinding(const FlexUIValue& template_, const FlexUIValue& data) -> FlexUIValue`. Recursively walks the template; whenever a string value matches `^\\{\\{\\s*([\\w.]+)\\s*\\}\\}$`, looks up the dotted path in `data` and substitutes. Substrings with multiple `{{ }}` patterns get string-replaced; pure `{{ name }}` swaps the value type, possibly producing a non-string FlexUIValue.

`a2ui_handlers_runtime.cc`:
- Function `EvalHandlersJs(Scope& scope, const std::string& source)` — `scope.js_context()->Eval(source, "<handlers.js>", ...)`. The handlers.js installs `globalThis.__a2ui_handlers = { handlerName: fn, ... }`.
- Function `DispatchHandler(Scope& scope, const std::string& name, FlexUIValue payload)` — looks up the handler in `__a2ui_handlers` and invokes it through `IJsContext::Call`. The handler internally calls `setData`, which routes through the same Scope's set-data sink as `card-js`.

- [ ] **Step 8.4: Write `a2ui_json_frontend.cc`**

```cpp
// Body of A2UIJsonFrontend
void A2UIJsonFrontend::Initialize(Scope& scope, const BundleSource& bundle) {
  FLEXUI_TLOG(Frontend, A2UIInit, INFO) << "uri=" << bundle.uri;
  json_doc_ = ParseA2UIJson(bundle.text);
  // Optional: handlers.js can be embedded as a sibling text. PoC convention:
  // the JSON top-level may have a "handlersJs" string field.
  if (json_doc_.IsObject() && json_doc_.ToObject().count("handlersJs")) {
    handlers_js_ = json_doc_.ToObject().at("handlersJs").ToString();
    EvalHandlersJs(scope, *handlers_js_);
  }
}

std::shared_ptr<DomNode> A2UIJsonFrontend::Render(Scope& scope, const FlexUIValue& data) {
  FLEXUI_TLOG(Frontend, A2UIRender, DEBUG);
  // Apply data binding to the template (excluding the handlersJs field).
  auto bound = ApplyDataBinding(json_doc_, data);
  // Walk bound, materialize DomNode tree with new IDs.
  return BuildDomTree(bound);
}

void A2UIJsonFrontend::HandleEvent(Scope& scope, const std::string& event,
                                    const FlexUIValue& payload) {
  // Look up handler name in the bound template event map (built during Render)
  // and dispatch via the JS runtime.
  DispatchHandler(scope, event, payload);
}
```

`BuildDomTree` walks the template recursively. For each object node with shape `{ type: "Text", props: {...}, children: [...] }`, it creates a `DomNode` with `view_name = type`, `props_` from `props`, and recursively converts children. **IDs are allocated using a deterministic algorithm** — depth-first traversal in pre-order, with the first node getting ID 1. This makes the dual-frontend parity test work (both frontends produce identical IDs for identical input).

- [ ] **Step 8.5: Write `a2ui_plugin.cc`**

```cpp
#include "flexui/plugin_a2ui/a2ui_plugin.h"
#include "flexui/plugin_a2ui/a2ui_json_frontend.h"

namespace flexui::plugin_a2ui {

flexui::core::plugin_host::FlexUIPlugin MakeA2UIPlugin() {
  flexui::core::plugin_host::FlexUIPlugin p;
  p.name = "flexui-a2ui";
  p.version = "0.1.0-mvp";
  p.frontends.push_back({"a2ui-json", []() {
    return std::make_unique<A2UIJsonFrontend>();
  }});
  return p;
}

}  // namespace flexui::plugin_a2ui
```

- [ ] **Step 8.6: Write `tests/flex-ui/unit/a2ui_json_frontend_test.cc`**

Three required tests:
1. Parse + bind: `{"type": "Text", "props": {"text": "Hello {{name}}"}}` with data `{"name": "world"}` produces a DomNode with `view_name == "Text"` and `props["text"] == "Hello world"`.
2. Nested children: a Container with two children renders both children with sequential IDs.
3. Handler dispatch: A2UI JSON with `onClick: "tap"` plus a `handlersJs: "globalThis.__a2ui_handlers = { tap: () => setData({tapped: true}) }"` runs the handler when HandleEvent is invoked.

- [ ] **Step 8.7: Wire auto-register? No.**

Per spec §4.11, plugin-a2ui is **opt-in**, not auto-registered. Business code calls `FlexUIEngine.install(A2UIPlugin)` explicitly. So in `flex_ui_engine.cc::Init`, do **not** install it. Tests install it explicitly before exercising:

```cpp
FlexUIEngine::Instance().Install(flexui::plugin_a2ui::MakeA2UIPlugin());
```

- [ ] **Step 8.8: Build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/plugin-a2ui tests/flex-ui/unit
git commit -m "feat(flex-ui/plugin-a2ui): JSON frontend + data binder + handlers.js dispatch"
```

---

### Task 9: ★ THE C2 PROOF — dual-frontend parity test

**Files:**
- Create: `tests/flex-ui/unit/dual_frontend_parity_test.cc`
- Create: `tests/flex-ui/unit/support/recording_component_backend.h`
- Create: `tests/flex-ui/unit/support/recording_component_backend.cc`

This is the central acceptance test for the entire PoC's C2 claim (spec §6.1).

The test:
1. Defines a small A2UI JSON document: a View containing a Text (with `{{title}}` binding) and a Button (with `onClick: "tap"`).
2. Creates a hand-coded card-js bundle that produces the structurally identical vdom: same component types, same props, same children order.
3. Loads two `FlexCardController`s, one with `bundle_type: "a2ui-json"`, one with `bundle_type: "card-js"`.
4. Provides identical `initial_data = {title: "Hello"}` to both.
5. **Asserts** the produced DomNode trees are byte-equal (via a recursive equality on `view_name`, `props`, `children` — but **ignoring node ID assignment differences**: see below).
6. **Asserts** the mutation streams committed to the recording backend are identical (same sequence of CreateMutation / UpdateProps / UpdateLayout / etc., same `view_name`, same `props` deltas, same `LayoutRect` values).

**Node ID assignment caveat:** Both frontends must produce DomNode IDs deterministically. Define the algorithm as "depth-first preorder starting from 1, monotonically increasing." Both `BuildDomTree` (a2ui) and the createElement → DomNode conversion (card-js) must implement this scheme. Adjust Task 6 and Task 8 implementations if they don't.

**Recording backend:** A `RecordingComponentBackend` that registers a `ComponentFactory` for each baseline component name (Text/View/Image/Button/ScrollView) — but with `create` returning a `RecordingInstance` that records all calls (`OnCreate`/`OnUpdateProps`/`OnUpdateLayout`/etc.) into a per-instance log. The test asserts the two cards' aggregated logs are identical when sorted by (node_id, call_order).

- [ ] **Step 9.1: Write `recording_component_backend.{h,cc}`**

The header defines `RecordingComponentBackend` which holds a `std::vector<CallRecord>` per node_id. Constructor takes a name (e.g. "Text"), so we can register 5 instances of it, one per baseline component name. CallRecord captures the method name + the (`PropDelta`-serialized or LayoutRect-serialized) payload as a string.

- [ ] **Step 9.2: Write `dual_frontend_parity_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/core/card-controller/flex_card_controller.h"
#include "flexui/plugin_a2ui/a2ui_plugin.h"
#include "tests/flex-ui/unit/support/recording_component_backend.h"

namespace flexui {

constexpr const char* kA2UIJson = R"JSON({
  "type": "View",
  "props": { "padding": 8 },
  "children": [
    { "type": "Text", "props": { "text": "Hello {{title}}", "fontSize": 16 } },
    { "type": "Button", "props": { "title": "Tap", "onClick": "tap" } }
  ]
})JSON";

constexpr const char* kCardJsEquivalent = R"JS(
  function render(data) {
    return createElement('View', { padding: 8 }, [
      createElement('Text', { text: 'Hello ' + data.title, fontSize: 16 }),
      createElement('Button', { title: 'Tap', onClick: 'tap' }),
    ]);
  }
  globalThis.__flexui_exports = { render, handlers: {} };
)JS";

class DualFrontendParityTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Replace the default components-base with our recording backend,
    // so the same Text/View/Button names route into RecordingInstance.
    flexui::core::card_controller::FlexUIEngineConfig cfg;
    cfg.backend = flexui::core::js_engine::JsEngineBackend::kQuickJS;
    flexui::core::card_controller::FlexUIEngine::Instance().Init(cfg);
    // The default ComponentsBasePackage was already installed at Init.
    // We uninstall it and install the recording variant.
    flexui::core::card_controller::FlexUIEngine::Instance().Uninstall("flexui-components-base");
    flexui::core::card_controller::FlexUIEngine::Instance().Install(
        flexui::test::MakeRecordingComponentsPackage(&recordings_a_));
    // Note: each card gets its own RecordingComponentBackend by routing
    // through ComponentContext::scope_id(); see helper.
    flexui::core::card_controller::FlexUIEngine::Instance().Install(
        flexui::plugin_a2ui::MakeA2UIPlugin());
  }
  void TearDown() override {
    flexui::core::card_controller::FlexUIEngine::Instance().Shutdown();
  }

  flexui::test::RecordingTable recordings_a_;
  flexui::test::RecordingTable recordings_b_;
};

TEST_F(DualFrontendParityTest, SameJsonRendersIdenticalTrees) {
  // Card A: a2ui-json frontend.
  flexui::core::card_controller::FlexCardControllerOptions opts_a;
  opts_a.bundle_type = "a2ui-json";
  opts_a.bundle_inline = kA2UIJson;
  opts_a.initial_data = flexui::common::FlexUIValue(
      flexui::common::FlexUIValue::FlexUIValueObjectType{
          {"title", flexui::common::FlexUIValue(std::string("World"))}});

  flexui::core::card_controller::FlexCardController card_a(std::move(opts_a));
  ASSERT_TRUE(card_a.Load().ok());

  // Card B: card-js frontend.
  flexui::core::card_controller::FlexCardControllerOptions opts_b;
  opts_b.bundle_type = "card-js";
  opts_b.bundle_inline = kCardJsEquivalent;
  opts_b.initial_data = opts_a.initial_data;

  flexui::core::card_controller::FlexCardController card_b(std::move(opts_b));
  ASSERT_TRUE(card_b.Load().ok());

  // Compare DomNode trees via the controllers' debug accessor.
  auto tree_a = card_a.DebugLastDomTree();
  auto tree_b = card_b.DebugLastDomTree();
  EXPECT_TRUE(flexui::test::DomTreeEqualIgnoringIds(*tree_a, *tree_b))
      << "DomNode trees differ:\n A=" << flexui::test::DumpTree(*tree_a)
      << "\n B=" << flexui::test::DumpTree(*tree_b);
}

TEST_F(DualFrontendParityTest, SameJsonProducesIdenticalMutationStreams) {
  // Same setup; after Load, dump the recording tables and assert equality
  // (ignoring scope_id which differs between cards but normalizing).
  EXPECT_TRUE(flexui::test::MutationStreamsEqual(
      recordings_a_.NormalizedDump(),
      recordings_b_.NormalizedDump()));
}

}  // namespace flexui
```

- [ ] **Step 9.3: Implement helper functions** (`DomTreeEqualIgnoringIds`, `DumpTree`, `MutationStreamsEqual`, `NormalizedDump`, `MakeRecordingComponentsPackage`) in `recording_component_backend.cc`.

`DomTreeEqualIgnoringIds`: recursively compares `view_name`, `props_` (map equality on `FlexUIValue`), `children_.size()`, then descends.

`NormalizedDump`: serializes the per-node call sequence to a canonical string, e.g. `"[node:Text:OnCreate][node:Text:OnUpdateProps(text=Hello World,fontSize=16)][node:Text:OnUpdateLayout(0,0,200,18)]"`.

- [ ] **Step 9.4: Wire test, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add tests/flex-ui/unit
git commit -m "test(flex-ui): ★ C2 dual-frontend parity test (a2ui-json vs card-js)"
```

If the test fails, the most likely cause is non-deterministic ID assignment or differing prop-defaults handling between frontends. Fix the frontend implementations (Tasks 6 / 8) — not the test.

---

### Task 10: Coverage + TSan + AGenUI untouched

- [ ] **Step 10.1: Update `scripts/flex-ui/coverage.sh`** to add gates on `flex-ui/components/` (≥70%, lower than core per spec §6.4) and `flex-ui/api/` (≥80%).

Add to the for-loop iteration list `common core components api`. Set per-subsystem threshold:

```bash
declare -A THRESHOLDS=( ["common"]=80 ["core"]=80 ["components"]=70 ["api"]=80 )
THRESHOLD=${THRESHOLDS[$sub]}
```

- [ ] **Step 10.2: Run gates**

```bash
./scripts/flex-ui/build.sh
./scripts/flex-ui/build.sh --tsan-only
./scripts/flex-ui/coverage.sh
```

All green.

- [ ] **Step 10.3: Verify AGenUI untouched**

```bash
git diff master --stat -- core/ platforms/ tests/cpp/ playground/ scripts/harmony/ scripts/android/ scripts/ios/ agent_sdks/ samples/ skills/
```

Expected: empty.

- [ ] **Step 10.4: Final W7-W8 commit**

```bash
git commit --allow-empty -m "chore(flex-ui): W7-W8 final green gate (components + api + dual frontends + C2 proof)"
```

---

## Self-Review

### Spec coverage

| Spec requirement | Covered by |
|---|---|
| §4.7 component layer dual mode (capi) | Tasks 1-4 (capi mode); ets-builder mode interface stays from W5-W6 |
| §4.10 dual-frontend contract: card-js + a2ui-json producing same DomNode | Tasks 6, 8, 9 |
| §4.10 a2ui frontend supports only `{{ }}` interpolation (no expressions) | Task 8 (binder) |
| §4.11 plugin mechanism: components-base auto-installed; plugin-a2ui opt-in | Tasks 4, 7, 8 |
| §6.1 Functional acceptance: 5 components capi, 3 APIs, both frontends, dual-frontend parity | Tasks 2, 3, 5, 7, 6, 8, 9 |
| §6.4 Coverage gates: core ≥80%, components ≥70%, api ≥80% (W1-W2 common ≥80%) | Task 10 |
| §7.1 unit-test methodology | every component / api / frontend has tests |
| §8.4 mandatory log coverage for Frontend / Component / Api / Plugin subsystems | Tasks 2-8 (every public method logs FLEXUI_TLOG) |
| §5.1 AGenUI tree untouched | Task 10 |

### Placeholder scan

- Task 3 ("write 4 more components mirroring Task 2") is concrete: Task 2 provides the full template, and the per-component ArkUI node type + props lists are given inline (View → STACK, Image → IMAGE, Button → BUTTON, ScrollView → SCROLL; each with their PoC prop subsets). The executor follows the Task 2 file structure substituting types and props.
- Task 6.2's "~300 lines, the structure is described" — the structure is exhaustively named: Initialize injects globals, Eval the bundle, capture exports.render and exports.handlers; Render converts data → JS, calls render_fn, converts result → DomNode tree using next_node_id_; HandleEvent invokes handlers[name](payload). Every patterns used (`InjectGlobalFunction`, `FromFlexUIValue`, `Call`) is already exemplified in W3-W4 Task 9 (QuickJSContext) verbatim.
- No "TBD" / "TODO" / "fill in later".

### Type consistency

- `ComponentFactory` shape (Task 5 in W5-W6) returned by `MakeTextFactory()` etc. — consistent across all five components.
- `NativeApi` shape used in Task 7 matches W5-W6 plugin-host interface.
- `IFrontend` interface (W5-W6) implemented identically by `CardJsFrontend` and `A2UIJsonFrontend`. Methods `Initialize` / `Render` / `HandleEvent` match.
- `DomNode` IDs allocated via deterministic depth-first preorder in both frontends — explicit requirement called out in Task 8.4 and Task 9.
- `BundleSource.text` field used by card-js for inline bundles (Task 6.5 sets `src.text = options_.bundle_inline`) and by a2ui (Task 8.4 reads from `bundle.text`). Consistent.

### Scope check

This plan delivers a usable rendering pipeline. A FlexCard with text + a tap button can be created from either a card-js bundle or an A2UI JSON, the dual-frontend parity is asserted in tests, and the unit-test suite end-to-end covers spec §6.1's "Functional must-pass" minus only the items that explicitly require a device (which is W9-W10). The deliverable can be exercised in isolation through `tests/flex-ui/unit/flexui_unit_tests` without any device.

---

*End of W7-W8 plan.*
