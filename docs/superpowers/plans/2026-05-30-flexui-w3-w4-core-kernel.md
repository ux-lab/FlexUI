# FlexUI W3-W4 — Core Kernel (JS Engine + Vdom + Reconciler + Layout) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stand up `flex-ui/core/` with four foundational subsystems — `js-engine/` (cross-platform `IJsEngine`/`IJsContext`/`IJsValue` with QuickJS and JSVM backends), `vdom/` (`DomNode` absorbed from Hippy), `reconciler/` (diff algorithm absorbed from Hippy `diff_utils`), and `layout/` (Yoga integration absorbed from Hippy `yoga_layout_node`). After this plan, FlexUI can evaluate JS, build a vdom, diff two vdoms, and compute layout. Bridge, Scope, Commit, Plugin, and components are W5-W6 / W7-W8.

**Architecture:** `core/js-engine/` defines a minimal cross-platform interface and ships two concrete backends. QuickJS is built from amalgamation source, integrated for all platforms (the only Android JS engine and the HarmonyOS debug fallback). JSVM wraps HarmonyOS's `napi_jsvm_*` system API, Harmony-only. `core/vdom/` absorbs Hippy's `dom_node.h/cc` with `HippyValue` references retargeted to `FlexUIValue` from W1-W2. `core/reconciler/` absorbs `diff_utils.h/cc` and exposes a clean `Diff(old, new) -> MutationList` API. `core/layout/` absorbs `yoga_layout_node.h/cc` and vendors Yoga itself via FetchContent (matching the AGenUI test convention).

**Tech Stack:** C++17, CMake 3.18+, GoogleTest, QuickJS (bellard/quickjs, MIT, amalgamation single-file build), HarmonyOS JSVM (system API, Harmony device only), Yoga (facebook/yoga, MIT, vendored via FetchContent), AddressSanitizer + UndefinedBehaviorSanitizer.

**Reference spec:** `docs/superpowers/specs/2026-05-30-flexui-layering-design.md` §4.4 (Hippy absorption — `driver/js/` + `dom/` rows), §4.5 (JS Engine backend matrix), §4.8 (mutation pipeline), §6.4 (test coverage), §7.1 (unit test methodology).

**Depends on:** W1-W2 plan complete and committed (`flex-ui/common/` available, log infra in place, `FlexUIValue` available, `Error` type available, test harness wired).

**Out of scope (deferred):**
- `core/bridge/` (W5-W6).
- `core/scope-manager/` (W5-W6) — note: this plan does provide a low-level `IJsContext` per JS engine instance, but the higher-level Scope (lifecycle state machine, snapshot, per-card API injection) is W5-W6.
- `core/commit-pipeline/` (W5-W6).
- `core/plugin-host/` (W5-W6).
- `core/card-controller/` + ETS public API (W5-W6).
- Components / APIs / frontends (W7-W8).
- Playground / E2E (W9-W10).
- HarmonyOS device-side build of JSVM backend (this plan ships the JSVM backend code; actual device exercise lands W9-W10).
- Animation (`dom/animation/` — explicitly out of scope per spec §3).
- Taitank layout engine (deleted per spec §4.4).
- Hippy `dom/root_node.{h,cc}` / `scene.{h,cc}` / `scene_builder.{h,cc}` / `render_manager.h` / `layer_optimized_render_manager.{h,cc}` / `dom_action_interceptor.h` / `dom_event.{h,cc}` / `dom_listener.{h,cc}` (these are Hippy-renderer-orchestration concepts; FlexUI implements its own equivalents in `commit-pipeline/` in W5-W6).

**Deliverable definition of done:**
- `flex-ui/core/js-engine/` static library with `IJsEngine`/`IJsContext`/`IJsValue` interfaces, a working QuickJS backend, and a JSVM backend whose code compiles when targeting HarmonyOS (gated behind `FLEXUI_OHOS` define; host build skips it).
- `flex-ui/core/vdom/` static library exposing `DomNode` under `flexui::core` namespace.
- `flex-ui/core/reconciler/` static library exposing `Diff(old, new) -> MutationList` under `flexui::core` namespace.
- `flex-ui/core/layout/` static library wrapping Yoga, exposing `YogaLayoutNode`.
- Unit tests for all four subsystems green under ASan+UBSan on host.
- Line coverage on `flex-ui/core/` ≥ 80%.
- `FLEXUI_TLOG` calls present at the entry/exit of every public method in the four subsystems' top-level facades (`IJsEngine::CreateContext`, `IJsContext::Eval`, `Diff`, `YogaLayoutNode::Calculate`) per spec §8.4.
- AGenUI tree still untouched.

---

## File Structure

### Created in this plan

```
flex-ui/core/
├── CMakeLists.txt                                # aggregator
├── js-engine/
│   ├── CMakeLists.txt
│   ├── include/flexui/core/js-engine/
│   │   ├── ijs_engine.h                          # interface
│   │   ├── ijs_context.h                         # interface
│   │   ├── ijs_value.h                           # interface
│   │   ├── js_engine_factory.h                   # MakeQuickJSEngine() / MakeJsvmEngine()
│   │   └── js_engine_backend.h                   # enum + selector
│   ├── src/
│   │   ├── js_engine_factory.cc
│   │   ├── ijs_value.cc                          # default impls
│   │   ├── quickjs/
│   │   │   ├── quickjs_engine.h
│   │   │   ├── quickjs_engine.cc
│   │   │   ├── quickjs_context.h
│   │   │   ├── quickjs_context.cc
│   │   │   ├── quickjs_value.h
│   │   │   ├── quickjs_value.cc
│   │   │   └── quickjs_conversions.cc            # FlexUIValue <-> JSValue
│   │   └── jsvm/
│   │       ├── jsvm_engine.h                     # FLEXUI_OHOS-only
│   │       ├── jsvm_engine.cc
│   │       ├── jsvm_context.h
│   │       ├── jsvm_context.cc
│   │       └── jsvm_value.cc
│   └── third_party/
│       └── quickjs/                              # amalgamation: quickjs.{c,h}, quickjs-libc.{c,h}, quickjs-atom.h, etc.
├── vdom/
│   ├── CMakeLists.txt
│   ├── include/flexui/core/vdom/
│   │   ├── dom_node.h                            # absorbed from Hippy dom/dom_node.h
│   │   ├── dom_argument.h                        # absorbed
│   │   └── node_props.h                          # absorbed
│   └── src/
│       ├── dom_node.cc                           # absorbed
│       └── dom_argument.cc                       # absorbed
├── reconciler/
│   ├── CMakeLists.txt
│   ├── include/flexui/core/reconciler/
│   │   ├── diff_utils.h                          # absorbed
│   │   └── mutation.h                            # FlexUI-original: Mutation struct typedef
│   └── src/
│       └── diff_utils.cc                         # absorbed
└── layout/
    ├── CMakeLists.txt
    ├── include/flexui/core/layout/
    │   ├── layout_node.h                         # absorbed (interface)
    │   └── yoga_layout_node.h                    # absorbed
    └── src/
        ├── layout_node.cc                        # absorbed
        └── yoga_layout_node.cc                   # absorbed

tests/flex-ui/unit/
├── (existing W1-W2 tests preserved)
├── js_engine_quickjs_test.cc
├── js_engine_jsvm_test.cc                        # built only when FLEXUI_OHOS set
├── vdom_dom_node_test.cc
├── reconciler_diff_test.cc
└── layout_yoga_test.cc
```

### Modified in this plan

- `flex-ui/CMakeLists.txt` — add `add_subdirectory(core)`.
- `flex-ui/NOTICE` — append the additional Hippy files absorbed in W3-W4 and the QuickJS + Yoga third-party attributions.
- `tests/flex-ui/unit/CMakeLists.txt` — link the four new test files; conditionally include `js_engine_jsvm_test.cc` when `FLEXUI_OHOS` is set.

### Untouched (verified by git diff)

Existing `flex-ui/common/`, AGenUI `core/` / `platforms/` / `tests/cpp/`, all playground and scripts/harmony/android/ios. Verify in Task 18.

---

## Tasks

### Task 1: Scaffold `flex-ui/core/` + aggregator CMake

**Files:**
- Create: `flex-ui/core/CMakeLists.txt`
- Create: `flex-ui/core/js-engine/CMakeLists.txt` (empty subdir placeholder)
- Create: `flex-ui/core/vdom/CMakeLists.txt`
- Create: `flex-ui/core/reconciler/CMakeLists.txt`
- Create: `flex-ui/core/layout/CMakeLists.txt`
- Modify: `flex-ui/CMakeLists.txt`

- [ ] **Step 1.1: Create directories**

```bash
mkdir -p flex-ui/core/js-engine/include/flexui/core/js-engine
mkdir -p flex-ui/core/js-engine/src/quickjs
mkdir -p flex-ui/core/js-engine/src/jsvm
mkdir -p flex-ui/core/js-engine/third_party/quickjs
mkdir -p flex-ui/core/vdom/include/flexui/core/vdom
mkdir -p flex-ui/core/vdom/src
mkdir -p flex-ui/core/reconciler/include/flexui/core/reconciler
mkdir -p flex-ui/core/reconciler/src
mkdir -p flex-ui/core/layout/include/flexui/core/layout
mkdir -p flex-ui/core/layout/src
```

- [ ] **Step 1.2: Write `flex-ui/core/CMakeLists.txt`**

```cmake
add_subdirectory(js-engine)
add_subdirectory(vdom)
add_subdirectory(reconciler)
add_subdirectory(layout)
```

- [ ] **Step 1.3: Write minimal `flex-ui/core/js-engine/CMakeLists.txt`**

```cmake
add_library(flexui_core_js_engine STATIC)
target_include_directories(flexui_core_js_engine
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_js_engine PUBLIC flexui_common)
# Sources appended by later tasks.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_core_js_engine PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
```

- [ ] **Step 1.4: Same shape for `vdom`, `reconciler`, `layout`**

Each `CMakeLists.txt` follows the pattern above with appropriate library names:
- `flexui_core_vdom` (depends on `flexui_common`)
- `flexui_core_reconciler` (depends on `flexui_common flexui_core_vdom`)
- `flexui_core_layout` (depends on `flexui_common`)

- [ ] **Step 1.5: Extend `flex-ui/CMakeLists.txt`**

Append:

```cmake
add_subdirectory(core)
```

- [ ] **Step 1.6: Build, verify nothing broke**

```bash
./scripts/flex-ui/build.sh
```

Expected: existing W1-W2 tests still green; new (empty) libraries link.

- [ ] **Step 1.7: Commit**

```bash
git add flex-ui/core flex-ui/CMakeLists.txt
git commit -m "feat(flex-ui/core): scaffold core/{js-engine,vdom,reconciler,layout} skeleton"
```

---

### Task 2: Absorb Hippy `dom_node.h` + `dom_argument.h` + `node_props.h` (headers only)

**Files:**
- Create: `flex-ui/core/vdom/include/flexui/core/vdom/dom_node.h`
- Create: `flex-ui/core/vdom/include/flexui/core/vdom/dom_argument.h`
- Create: `flex-ui/core/vdom/include/flexui/core/vdom/node_props.h`

Follow the W1-W2 "Common Workflow for Absorb a Footstone File" pattern (Task 2 in the W1-W2 plan), with these rename rules:

- `namespace hippy { inline namespace dom { ... } }` → `namespace flexui::core::vdom { ... }`. Eliminate the `inline namespace dom`; collapse into the FlexUI namespace.
- `#include "dom/X.h"` → `#include "flexui/core/vdom/X.h"`.
- `#include "footstone/X.h"` → `#include "flexui/common/X.h"`.
- `footstone::HippyValue` → `flexui::common::FlexUIValue`.
- `footstone::TaskRunner` → `flexui::common::TaskRunner`.
- The class `DomNode` keeps its name.

**Important:** `dom_node.h` references `LayoutNode`, `dom_listener`, `dom_event`, `dom_action_interceptor`, and `render_manager`. The first is absorbed in Task 5; the rest are **deleted dependencies** that don't exist in FlexUI. Remove their includes and any related forward declarations / typedefs from the absorbed `dom_node.h`. Comment each removal with `// FlexUI: removed Hippy dependency on <name> — orchestration moved to commit-pipeline/`.

Specifically:
- Delete `#include "dom/dom_listener.h"` and the `EventListenerInfo` / `RenderListenerInfo` member functions.
- Delete `#include "dom/dom_event.h"`.
- Delete `#include "dom/render_manager.h"`.
- Delete `#include "dom/dom_action_interceptor.h"`.
- Delete the `domain_data_` member if it depends on removed types; leave a TODO-style FlexUI comment.

- [ ] **Step 2.1: Copy the three headers**

```bash
HD=/Users/pingjiang/coding/github/Hippy/dom/include/dom
cp $HD/dom_node.h     flex-ui/core/vdom/include/flexui/core/vdom/dom_node.h
cp $HD/dom_argument.h flex-ui/core/vdom/include/flexui/core/vdom/dom_argument.h
cp $HD/node_props.h   flex-ui/core/vdom/include/flexui/core/vdom/node_props.h
```

- [ ] **Step 2.2: Apply mechanical rewrites**

```bash
python3 - <<'PY'
import pathlib, re
files = [
  pathlib.Path("flex-ui/core/vdom/include/flexui/core/vdom/dom_node.h"),
  pathlib.Path("flex-ui/core/vdom/include/flexui/core/vdom/dom_argument.h"),
  pathlib.Path("flex-ui/core/vdom/include/flexui/core/vdom/node_props.h"),
]
for p in files:
  s = p.read_text()
  s = re.sub(r'#include\s+"dom/', '#include "flexui/core/vdom/', s)
  s = re.sub(r'#include\s+"footstone/', '#include "flexui/common/', s)
  s = s.replace("footstone::HippyValue", "flexui::common::FlexUIValue")
  s = s.replace("footstone::TaskRunner",  "flexui::common::TaskRunner")
  s = s.replace("HippyValue", "FlexUIValue")
  s = s.replace("namespace hippy {", "namespace flexui::core::vdom {")
  s = s.replace("inline namespace dom {", "")
  s = s.replace("}  // namespace dom", "")
  s = s.replace("}  // namespace hippy", "}  // namespace flexui::core::vdom")
  p.write_text(s)
PY
```

- [ ] **Step 2.3: Delete absent-dependency includes (manual edit)**

In `dom_node.h`, remove the lines that include `dom_listener.h`, `dom_event.h`, `render_manager.h`, `dom_action_interceptor.h`. For each removal, leave one comment line:

```cpp
// FlexUI: removed Hippy dependency on dom_listener (orchestration moved to commit-pipeline/).
// FlexUI: removed Hippy dependency on dom_event (event dispatch moved to bridge/).
// FlexUI: removed Hippy dependency on render_manager (renderer is per-platform under platforms/).
// FlexUI: removed Hippy dependency on dom_action_interceptor (no equivalent in FlexUI).
```

Remove any `EventListenerInfo`, `RenderListenerInfo`, and similar member functions / typedefs that no longer compile after the include removal. Strict rule: anything you delete gets a `// FlexUI: removed because <reason>` marker so the executor reviewing the rename can see what was cut.

- [ ] **Step 2.4: Insert FlexUI modification notice in each of the three headers** (template from W1-W2 plan Task 2).

- [ ] **Step 2.5: Write `tests/flex-ui/unit/vdom_dom_node_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <memory>
#include "flexui/core/vdom/dom_node.h"
#include "flexui/common/flexui_value.h"

namespace flexui::core::vdom {

TEST(DomNodeTest, ConstructWithIdAndViewName) {
  auto node = std::make_shared<DomNode>(
      /*id=*/42, /*pid=*/0, /*index=*/0, "View",
      /*tag_name=*/"View",
      /*style_map=*/std::make_shared<DomNode::DomValueMap>(),
      /*dom_ext_map=*/std::make_shared<DomNode::DomValueMap>());
  EXPECT_EQ(node->GetId(), 42u);
  EXPECT_EQ(node->GetViewName(), "View");
}

}  // namespace flexui::core::vdom
```

(If the actual `DomNode` constructor signature differs after Hippy's evolution, adjust the test to match the absorbed header. The behavioral invariant is: ID and view-name round-trip.)

- [ ] **Step 2.6: Wire test + verify build**

Append `vdom_dom_node_test.cc` to `tests/flex-ui/unit/CMakeLists.txt` and add `flexui_core_vdom` to its `target_link_libraries`. Run:

```bash
./scripts/flex-ui/build.sh
```

If `dom_node.h` references removed methods inside its inline implementations, the build will fail with concrete error messages. Fix each by either (a) commenting out the offending inline body with `// FlexUI: removed body referencing <type>`, or (b) re-introducing a minimal stub if it's a method signature consumers will need. Document each decision in a comment.

- [ ] **Step 2.7: Commit**

```bash
git add flex-ui/core/vdom tests/flex-ui/unit
git commit -m "feat(flex-ui/core/vdom): absorb DomNode + DomArgument + NodeProps headers"
```

---

### Task 3: Absorb `dom_node.cc` + `dom_argument.cc`

**Files:**
- Create: `flex-ui/core/vdom/src/dom_node.cc`
- Create: `flex-ui/core/vdom/src/dom_argument.cc`
- Modify: `flex-ui/core/vdom/CMakeLists.txt`

- [ ] **Step 3.1: Copy + apply same rewrites as Task 2.2**

```bash
HD=/Users/pingjiang/coding/github/Hippy/dom/src/dom
cp $HD/dom_node.cc     flex-ui/core/vdom/src/dom_node.cc
cp $HD/dom_argument.cc flex-ui/core/vdom/src/dom_argument.cc
```

Apply the same Python rewrite script (Task 2.2).

- [ ] **Step 3.2: Remove all method bodies that depend on the deleted Hippy types**

In `dom_node.cc`, search for method definitions that reference:
- `EventListenerInfo` / `RenderListenerInfo`
- `RenderManager`
- `DomActionInterceptor`
- `DomEvent::HandleEvent` style dispatch
- `RootNode` (orchestrator — not absorbed in FlexUI)

For each, replace the body with:

```cpp
// FlexUI: body removed; functionality migrates to commit-pipeline/ in W5-W6.
// Stub kept to preserve ABI of absorbed header until plumbing lands.
FLEXUI_TLOG(Vdom, NotImplemented, ERROR) << __PRETTY_FUNCTION__;
```

If a method returns a non-void value, return a sensible default (zero-initialized struct / nullptr / empty vector).

- [ ] **Step 3.3: Wire sources**

```cmake
target_sources(flexui_core_vdom PRIVATE
  src/dom_node.cc
  src/dom_argument.cc
)
```

- [ ] **Step 3.4: Build, fix any remaining compile errors with the "stub + log" pattern, run tests, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/vdom
git commit -m "feat(flex-ui/core/vdom): absorb dom_node.cc + dom_argument.cc with FlexUI stubs"
```

---

### Task 4: Absorb diff_utils (reconciler) + introduce `Mutation` typedef

**Files:**
- Create: `flex-ui/core/reconciler/include/flexui/core/reconciler/diff_utils.h`
- Create: `flex-ui/core/reconciler/include/flexui/core/reconciler/mutation.h`
- Create: `flex-ui/core/reconciler/src/diff_utils.cc`
- Create: `tests/flex-ui/unit/reconciler_diff_test.cc`
- Modify: `flex-ui/core/reconciler/CMakeLists.txt`

- [ ] **Step 4.1: Copy + rewrite diff_utils**

```bash
HD=/Users/pingjiang/coding/github/Hippy/dom
cp $HD/include/dom/diff_utils.h flex-ui/core/reconciler/include/flexui/core/reconciler/diff_utils.h
cp $HD/src/dom/diff_utils.cc    flex-ui/core/reconciler/src/diff_utils.cc
```

Apply the rewrite (namespace `hippy::dom` → `flexui::core::reconciler`; include paths; `HippyValue` → `FlexUIValue`).

Insert the FlexUI modification notice in the header.

- [ ] **Step 4.2: Write `mutation.h` (FlexUI-original)**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * FlexUI Mutation type. Output of Diff and input of commit-pipeline.
 * The set is fixed: Create, UpdateProps, UpdateLayout, Move, Delete.
 */
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "flexui/common/flexui_value.h"
#include "flexui/core/vdom/dom_node.h"

namespace flexui::core::reconciler {

struct CreateMutation {
  uint32_t node_id;
  uint32_t parent_id;
  uint32_t index;
  std::string view_name;
};

struct UpdatePropsMutation {
  uint32_t node_id;
  std::vector<std::pair<std::string, flexui::common::FlexUIValue>> diff;
  std::vector<std::string> deleted_keys;
};

struct UpdateLayoutMutation {
  uint32_t node_id;
  float x, y, width, height;
};

struct MoveMutation {
  uint32_t node_id;
  uint32_t new_parent_id;
  uint32_t new_index;
};

struct DeleteMutation {
  uint32_t node_id;
};

using Mutation = std::variant<
    CreateMutation,
    UpdatePropsMutation,
    UpdateLayoutMutation,
    MoveMutation,
    DeleteMutation>;

using MutationList = std::vector<Mutation>;

}  // namespace flexui::core::reconciler
```

- [ ] **Step 4.3: Wire source**

```cmake
target_sources(flexui_core_reconciler PRIVATE
  src/diff_utils.cc
)
```

- [ ] **Step 4.4: Write `tests/flex-ui/unit/reconciler_diff_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <memory>

#include "flexui/core/reconciler/diff_utils.h"
#include "flexui/core/reconciler/mutation.h"
#include "flexui/core/vdom/dom_node.h"

namespace flexui::core::reconciler {

TEST(ReconcilerDiffTest, IdenticalTreesProduceNoMutations) {
  // Build two identical trivial trees and assert the diff is empty.
  // The exact constructor / DiffProps API names come from the absorbed
  // diff_utils.h; adjust call sites to match.
  auto old_root = std::make_shared<vdom::DomNode>(
      1, 0, 0, "View", "View",
      std::make_shared<vdom::DomNode::DomValueMap>(),
      std::make_shared<vdom::DomNode::DomValueMap>());
  auto new_root = std::make_shared<vdom::DomNode>(
      1, 0, 0, "View", "View",
      std::make_shared<vdom::DomNode::DomValueMap>(),
      std::make_shared<vdom::DomNode::DomValueMap>());

  auto result = DiffUtils::DiffProps(*old_root->GetStyleMap(),
                                     *new_root->GetStyleMap());
  EXPECT_TRUE(result.add_props == nullptr || result.add_props->empty());
  EXPECT_TRUE(result.update_props == nullptr || result.update_props->empty());
  EXPECT_TRUE(result.remove_props == nullptr || result.remove_props->empty());
}

TEST(ReconcilerDiffTest, ChangedPropProducesUpdate) {
  auto old_map = std::make_shared<vdom::DomNode::DomValueMap>();
  auto new_map = std::make_shared<vdom::DomNode::DomValueMap>();
  (*old_map)["color"] = flexui::common::FlexUIValue(std::string("#000"));
  (*new_map)["color"] = flexui::common::FlexUIValue(std::string("#fff"));

  auto result = DiffUtils::DiffProps(*old_map, *new_map);
  ASSERT_NE(result.update_props, nullptr);
  EXPECT_EQ(result.update_props->size(), 1u);
}

}  // namespace flexui::core::reconciler
```

- [ ] **Step 4.5: Wire test, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/reconciler tests/flex-ui/unit
git commit -m "feat(flex-ui/core/reconciler): absorb diff_utils + add Mutation typedef"
```

---

### Task 5: Absorb layout (`layout_node.h/cc` + `yoga_layout_node.h/cc`) + vendor Yoga

**Files:**
- Create: `flex-ui/core/layout/include/flexui/core/layout/layout_node.h`
- Create: `flex-ui/core/layout/include/flexui/core/layout/yoga_layout_node.h`
- Create: `flex-ui/core/layout/src/layout_node.cc`
- Create: `flex-ui/core/layout/src/yoga_layout_node.cc`
- Create: `tests/flex-ui/unit/layout_yoga_test.cc`
- Modify: `flex-ui/core/layout/CMakeLists.txt`

- [ ] **Step 5.1: Copy + rewrite the four files**

```bash
HD=/Users/pingjiang/coding/github/Hippy/dom
cp $HD/include/dom/layout_node.h       flex-ui/core/layout/include/flexui/core/layout/layout_node.h
cp $HD/include/dom/yoga_layout_node.h  flex-ui/core/layout/include/flexui/core/layout/yoga_layout_node.h
cp $HD/src/dom/layout_node.cc          flex-ui/core/layout/src/layout_node.cc
cp $HD/src/dom/yoga_layout_node.cc     flex-ui/core/layout/src/yoga_layout_node.cc
```

Apply rewrites: namespace `hippy::dom` → `flexui::core::layout`; `#include "dom/..."` → `#include "flexui/core/layout/..."` (or `flexui/core/vdom/...` when the include is a vdom type); `HippyValue` → `FlexUIValue`. Insert FlexUI notice in the headers.

- [ ] **Step 5.2: Wire Yoga via FetchContent**

Edit `flex-ui/core/layout/CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
  yoga
  GIT_REPOSITORY https://github.com/facebook/yoga.git
  GIT_TAG v3.0.4
)
FetchContent_MakeAvailable(yoga)

add_library(flexui_core_layout STATIC)
target_include_directories(flexui_core_layout
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(flexui_core_layout PRIVATE
  src/layout_node.cc
  src/yoga_layout_node.cc
)
target_link_libraries(flexui_core_layout PUBLIC flexui_common yogacore)
```

(The Yoga CMake target name is `yogacore` in recent versions; if FetchContent emits a different target name, use that.)

- [ ] **Step 5.3: Write `tests/flex-ui/unit/layout_yoga_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <memory>

#include "flexui/core/layout/yoga_layout_node.h"

namespace flexui::core::layout {

TEST(YogaLayoutTest, SingleNodeCalculatesIdentitySize) {
  auto node = std::make_shared<YogaLayoutNode>();
  node->SetStyle({{"width",  flexui::common::FlexUIValue(100.0)},
                  {"height", flexui::common::FlexUIValue(50.0)}});
  node->CalculateLayout(200, 200);
  auto result = node->GetLayoutResult();
  EXPECT_FLOAT_EQ(result.width, 100.0f);
  EXPECT_FLOAT_EQ(result.height, 50.0f);
}

TEST(YogaLayoutTest, FlexDirectionColumnStacksChildren) {
  auto root = std::make_shared<YogaLayoutNode>();
  root->SetStyle({{"width",  flexui::common::FlexUIValue(100.0)},
                  {"height", flexui::common::FlexUIValue(100.0)},
                  {"flexDirection", flexui::common::FlexUIValue(std::string("column"))}});
  auto c1 = std::make_shared<YogaLayoutNode>();
  c1->SetStyle({{"width", flexui::common::FlexUIValue(50.0)},
                {"height", flexui::common::FlexUIValue(30.0)}});
  auto c2 = std::make_shared<YogaLayoutNode>();
  c2->SetStyle({{"width", flexui::common::FlexUIValue(50.0)},
                {"height", flexui::common::FlexUIValue(40.0)}});
  root->InsertChild(c1, 0);
  root->InsertChild(c2, 1);
  root->CalculateLayout(100, 100);
  EXPECT_FLOAT_EQ(c1->GetLayoutResult().top, 0.0f);
  EXPECT_FLOAT_EQ(c2->GetLayoutResult().top, 30.0f);
}

}  // namespace flexui::core::layout
```

Adjust method names (`SetStyle`, `CalculateLayout`, `GetLayoutResult`, `InsertChild`) to match the absorbed `yoga_layout_node.h` API.

- [ ] **Step 5.4: Wire test, build, run, commit**

Append `layout_yoga_test.cc` to `tests/flex-ui/unit/CMakeLists.txt`, add `flexui_core_layout` to its links:

```bash
./scripts/flex-ui/build.sh
```

If Yoga's FetchContent download fails on the CI environment, document the workaround in `tests/flex-ui/README.md` (`-DFLEXUI_TESTS_LOCAL_YOGA_DIR=/path/to/yoga`). Then:

```bash
git add flex-ui/core/layout tests/flex-ui/unit tests/flex-ui/README.md
git commit -m "feat(flex-ui/core/layout): absorb yoga_layout_node + vendor Yoga via FetchContent"
```

---

### Task 6: Design `IJsEngine` / `IJsContext` / `IJsValue` interface

This is a **FlexUI-original** interface — Hippy's `js_ctx.h` is the design reference but the API surface here is reduced to FlexUI's actual needs (per spec §4.5: "minimum surface area"). Reuse Hippy's interface shape where it fits; cut everything FlexUI doesn't use in W3-W10.

**Files:**
- Create: `flex-ui/core/js-engine/include/flexui/core/js-engine/ijs_value.h`
- Create: `flex-ui/core/js-engine/include/flexui/core/js-engine/ijs_context.h`
- Create: `flex-ui/core/js-engine/include/flexui/core/js-engine/ijs_engine.h`
- Create: `flex-ui/core/js-engine/include/flexui/core/js-engine/js_engine_backend.h`
- Create: `flex-ui/core/js-engine/include/flexui/core/js-engine/js_engine_factory.h`

- [ ] **Step 6.1: Write `ijs_value.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Engine-agnostic JS value handle. Backed by an engine-specific implementation
 * obtained through IJsContext. Owns one engine-internal reference; releases
 * on destruction.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::js_engine {

class IJsContext;

class IJsValue {
 public:
  virtual ~IJsValue() = default;

  virtual bool IsUndefined() const = 0;
  virtual bool IsNull() const = 0;
  virtual bool IsBoolean() const = 0;
  virtual bool IsNumber() const = 0;
  virtual bool IsString() const = 0;
  virtual bool IsObject() const = 0;
  virtual bool IsArray() const = 0;
  virtual bool IsFunction() const = 0;
  virtual bool IsException() const = 0;

  virtual bool ToBoolean() const = 0;
  virtual double ToNumber() const = 0;
  virtual std::string ToStdString() const = 0;

  // Object/array property access. Returns a new IJsValue (caller owns).
  virtual std::shared_ptr<IJsValue> GetProperty(const std::string& name) const = 0;
  virtual void SetProperty(const std::string& name,
                           std::shared_ptr<IJsValue> value) = 0;
  virtual std::vector<std::string> GetPropertyNames() const = 0;
  virtual uint32_t GetArrayLength() const = 0;
  virtual std::shared_ptr<IJsValue> GetArrayItem(uint32_t index) const = 0;

  // Cross-engine adapter: convert engine value to FlexUIValue.
  virtual flexui::common::FlexUIValue ToFlexUIValue() const = 0;
};

}  // namespace flexui::core::js_engine
```

- [ ] **Step 6.2: Write `ijs_context.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Per-card JS execution context. One instance per FlexCardController in W5-W6,
 * but at this layer we expose only the engine-level primitive. Higher-level
 * Scope (lifecycle, snapshot, plugin-API injection) lives in core/scope-manager.
 */
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/js-engine/ijs_value.h"

namespace flexui::core::js_engine {

using NativeFunction = std::function<
    std::shared_ptr<IJsValue>(IJsContext& ctx,
                              const std::vector<std::shared_ptr<IJsValue>>& args)>;

class IJsContext {
 public:
  virtual ~IJsContext() = default;

  // Evaluate JS source. Returns the result value, or sets *err on exception.
  virtual std::shared_ptr<IJsValue> Eval(const std::string& source,
                                         const std::string& origin,
                                         flexui::common::Error* err) = 0;

  // Inject a native function into globalThis.
  virtual flexui::common::Error InjectGlobalFunction(const std::string& name,
                                                    NativeFunction fn) = 0;

  // Inject an arbitrary value into globalThis.
  virtual flexui::common::Error InjectGlobalValue(const std::string& name,
                                                  std::shared_ptr<IJsValue> value) = 0;

  // Build engine values.
  virtual std::shared_ptr<IJsValue> NewUndefined() = 0;
  virtual std::shared_ptr<IJsValue> NewNull() = 0;
  virtual std::shared_ptr<IJsValue> NewBoolean(bool v) = 0;
  virtual std::shared_ptr<IJsValue> NewNumber(double v) = 0;
  virtual std::shared_ptr<IJsValue> NewString(const std::string& v) = 0;
  virtual std::shared_ptr<IJsValue> NewObject() = 0;
  virtual std::shared_ptr<IJsValue> NewArray(uint32_t length) = 0;
  virtual std::shared_ptr<IJsValue> FromFlexUIValue(
      const flexui::common::FlexUIValue& v) = 0;

  // Call a function value.
  virtual std::shared_ptr<IJsValue> Call(
      std::shared_ptr<IJsValue> function,
      std::shared_ptr<IJsValue> this_value,
      const std::vector<std::shared_ptr<IJsValue>>& args,
      flexui::common::Error* err) = 0;

  // Drive microtask queue (Promise resolution).
  virtual void RunPendingJobs() = 0;
};

}  // namespace flexui::core::js_engine
```

- [ ] **Step 6.3: Write `ijs_engine.h`**

```cpp
#pragma once

#include <memory>
#include <string>

#include "flexui/common/error.h"
#include "flexui/core/js-engine/ijs_context.h"

namespace flexui::core::js_engine {

struct EngineConfig {
  size_t memory_limit_mb = 0;       // 0 = backend default
  bool enable_debug = false;
};

class IJsEngine {
 public:
  virtual ~IJsEngine() = default;

  virtual flexui::common::Error Initialize(const EngineConfig& cfg) = 0;
  virtual std::shared_ptr<IJsContext> CreateContext() = 0;
  virtual void Shutdown() = 0;
  virtual const char* BackendName() const = 0;
};

}  // namespace flexui::core::js_engine
```

- [ ] **Step 6.4: Write `js_engine_backend.h`**

```cpp
#pragma once

namespace flexui::core::js_engine {

enum class JsEngineBackend { kQuickJS, kJsvm };

// Auto-select backend per platform:
//   - HarmonyOS (FLEXUI_OHOS set):  kJsvm (debug fallback: kQuickJS via flag)
//   - Android, host:                kQuickJS
JsEngineBackend SelectBackend(bool force_quickjs_for_debug);

}  // namespace flexui::core::js_engine
```

- [ ] **Step 6.5: Write `js_engine_factory.h`**

```cpp
#pragma once

#include <memory>

#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/js-engine/js_engine_backend.h"

namespace flexui::core::js_engine {

std::unique_ptr<IJsEngine> MakeJsEngine(JsEngineBackend backend);

// Convenience: SelectBackend + MakeJsEngine.
std::unique_ptr<IJsEngine> MakeDefaultJsEngine(bool force_quickjs_for_debug = false);

}  // namespace flexui::core::js_engine
```

- [ ] **Step 6.6: Build (no consumers yet — just compile the headers via an empty TU), commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/js-engine/include
git commit -m "feat(flex-ui/core/js-engine): define IJsEngine/IJsContext/IJsValue interfaces"
```

---

### Task 7: Vendor QuickJS amalgamation

**Files:**
- Create: `flex-ui/core/js-engine/third_party/quickjs/` (vendored source)
- Modify: `flex-ui/core/js-engine/CMakeLists.txt`
- Modify: `flex-ui/NOTICE`

- [ ] **Step 7.1: Download QuickJS source**

Use the bellard upstream (MIT) at commit `2788d71` or later. Download to the project root, then copy needed files:

```bash
cd /tmp
git clone https://github.com/bellard/quickjs.git quickjs-src
cd /Users/pingjiang/coding/github/FlexUI
QJS=/tmp/quickjs-src
DEST=flex-ui/core/js-engine/third_party/quickjs

for f in quickjs.h quickjs.c quickjs-atom.h quickjs-opcode.h \
         cutils.h cutils.c libbf.h libbf.c \
         libregexp.h libregexp.c libregexp-opcode.h \
         libunicode.h libunicode.c libunicode-table.h \
         list.h \
         LICENSE; do
  cp "$QJS/$f" "$DEST/$f"
done
```

If `quickjs-libc` is wanted for `console.log` defaults, vendor it too — but FlexUI provides its own `console` in W7-W8 `api/`, so omit `quickjs-libc` for now to keep the surface minimal.

- [ ] **Step 7.2: Append QuickJS attribution to `flex-ui/NOTICE`**

```
The following directory contains QuickJS by Fabrice Bellard and Charlie
Gordon, licensed under the MIT License:

  flex-ui/core/js-engine/third_party/quickjs/

Files: quickjs.{c,h}, cutils.{c,h}, libbf.{c,h}, libregexp.{c,h},
       libunicode.{c,h}, list.h, and supporting atom / opcode / table
       headers. The original LICENSE file is preserved at
       flex-ui/core/js-engine/third_party/quickjs/LICENSE.
```

- [ ] **Step 7.3: Extend `flex-ui/core/js-engine/CMakeLists.txt`**

```cmake
add_library(flexui_quickjs STATIC
  third_party/quickjs/quickjs.c
  third_party/quickjs/cutils.c
  third_party/quickjs/libbf.c
  third_party/quickjs/libregexp.c
  third_party/quickjs/libunicode.c
)
target_include_directories(flexui_quickjs PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs
)
target_compile_definitions(flexui_quickjs PRIVATE
  CONFIG_VERSION="2024-01-13"
  _GNU_SOURCE
)
# QuickJS uses a few patterns sanitizers flag; suppress the noisier ones.
target_compile_options(flexui_quickjs PRIVATE
  -Wno-implicit-fallthrough
  -Wno-unused-but-set-variable
  -Wno-array-bounds
)

target_link_libraries(flexui_core_js_engine PUBLIC flexui_quickjs)
```

- [ ] **Step 7.4: Compile sanity check**

```bash
./scripts/flex-ui/build.sh --no-san     # QuickJS may trigger benign sanitizer findings; smoke first w/o san
./scripts/flex-ui/build.sh              # then under ASan+UBSan
```

If ASan flags real bugs in QuickJS during a later test run, document them in `flex-ui/NOTICE` under "Known sanitizer findings (upstream QuickJS)" — do not modify the vendored source.

- [ ] **Step 7.5: Commit**

```bash
git add flex-ui/core/js-engine/third_party/quickjs flex-ui/core/js-engine/CMakeLists.txt flex-ui/NOTICE
git commit -m "feat(flex-ui/core/js-engine): vendor QuickJS amalgamation (MIT) + attribution"
```

---

### Task 8: QuickJS backend — `QuickJSValue` (IJsValue impl)

**Files:**
- Create: `flex-ui/core/js-engine/src/quickjs/quickjs_value.h`
- Create: `flex-ui/core/js-engine/src/quickjs/quickjs_value.cc`
- Modify: `flex-ui/core/js-engine/CMakeLists.txt`

- [ ] **Step 8.1: Write `quickjs_value.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include "flexui/core/js-engine/ijs_value.h"
#include "quickjs.h"

namespace flexui::core::js_engine::quickjs {

class QuickJSContext;

class QuickJSValue : public IJsValue {
 public:
  QuickJSValue(QuickJSContext* ctx, JSValue v);
  ~QuickJSValue() override;

  QuickJSValue(const QuickJSValue&) = delete;
  QuickJSValue& operator=(const QuickJSValue&) = delete;

  bool IsUndefined() const override;
  bool IsNull() const override;
  bool IsBoolean() const override;
  bool IsNumber() const override;
  bool IsString() const override;
  bool IsObject() const override;
  bool IsArray() const override;
  bool IsFunction() const override;
  bool IsException() const override;

  bool ToBoolean() const override;
  double ToNumber() const override;
  std::string ToStdString() const override;

  std::shared_ptr<IJsValue> GetProperty(const std::string& name) const override;
  void SetProperty(const std::string& name,
                   std::shared_ptr<IJsValue> value) override;
  std::vector<std::string> GetPropertyNames() const override;
  uint32_t GetArrayLength() const override;
  std::shared_ptr<IJsValue> GetArrayItem(uint32_t index) const override;

  flexui::common::FlexUIValue ToFlexUIValue() const override;

  JSValue raw() const { return value_; }
  JSContext* qjs() const;

 private:
  QuickJSContext* ctx_;     // borrowed
  JSValue value_;           // owned (JS_DupValue / JS_FreeValue)
};

}  // namespace flexui::core::js_engine::quickjs
```

- [ ] **Step 8.2: Write `quickjs_value.cc`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "src/quickjs/quickjs_value.h"
#include "src/quickjs/quickjs_context.h"

#include <cstring>
#include <utility>

#include "flexui/common/log_tag.h"

namespace flexui::core::js_engine::quickjs {

QuickJSValue::QuickJSValue(QuickJSContext* ctx, JSValue v) : ctx_(ctx), value_(v) {}

QuickJSValue::~QuickJSValue() {
  JS_FreeValue(qjs(), value_);
}

JSContext* QuickJSValue::qjs() const { return ctx_->raw_ctx(); }

bool QuickJSValue::IsUndefined() const { return JS_IsUndefined(value_); }
bool QuickJSValue::IsNull() const      { return JS_IsNull(value_); }
bool QuickJSValue::IsBoolean() const   { return JS_IsBool(value_); }
bool QuickJSValue::IsNumber() const    { return JS_IsNumber(value_); }
bool QuickJSValue::IsString() const    { return JS_IsString(value_); }
bool QuickJSValue::IsObject() const    { return JS_IsObject(value_); }
bool QuickJSValue::IsArray() const     { return JS_IsArray(qjs(), value_) == 1; }
bool QuickJSValue::IsFunction() const  { return JS_IsFunction(qjs(), value_); }
bool QuickJSValue::IsException() const { return JS_IsException(value_); }

bool QuickJSValue::ToBoolean() const {
  return JS_ToBool(qjs(), value_) != 0;
}

double QuickJSValue::ToNumber() const {
  double d = 0;
  JS_ToFloat64(qjs(), &d, value_);
  return d;
}

std::string QuickJSValue::ToStdString() const {
  size_t len = 0;
  const char* p = JS_ToCStringLen(qjs(), &len, value_);
  if (!p) return "";
  std::string out(p, len);
  JS_FreeCString(qjs(), p);
  return out;
}

std::shared_ptr<IJsValue> QuickJSValue::GetProperty(const std::string& name) const {
  JSValue v = JS_GetPropertyStr(qjs(), value_, name.c_str());
  return std::make_shared<QuickJSValue>(ctx_, v);
}

void QuickJSValue::SetProperty(const std::string& name,
                               std::shared_ptr<IJsValue> value) {
  auto* qv = dynamic_cast<QuickJSValue*>(value.get());
  if (!qv) {
    FLEXUI_TLOG(JsEngine, SetProperty, ERROR) << "non-QuickJSValue passed";
    return;
  }
  JSValue dup = JS_DupValue(qjs(), qv->value_);
  JS_SetPropertyStr(qjs(), value_, name.c_str(), dup);
}

std::vector<std::string> QuickJSValue::GetPropertyNames() const {
  std::vector<std::string> names;
  JSPropertyEnum* tab = nullptr;
  uint32_t len = 0;
  if (JS_GetOwnPropertyNames(qjs(), &tab, &len, value_,
                             JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
    for (uint32_t i = 0; i < len; ++i) {
      const char* s = JS_AtomToCString(qjs(), tab[i].atom);
      if (s) {
        names.emplace_back(s);
        JS_FreeCString(qjs(), s);
      }
      JS_FreeAtom(qjs(), tab[i].atom);
    }
    js_free(qjs(), tab);
  }
  return names;
}

uint32_t QuickJSValue::GetArrayLength() const {
  JSValue len = JS_GetPropertyStr(qjs(), value_, "length");
  uint32_t out = 0;
  JS_ToUint32(qjs(), &out, len);
  JS_FreeValue(qjs(), len);
  return out;
}

std::shared_ptr<IJsValue> QuickJSValue::GetArrayItem(uint32_t index) const {
  JSValue v = JS_GetPropertyUint32(qjs(), value_, index);
  return std::make_shared<QuickJSValue>(ctx_, v);
}

flexui::common::FlexUIValue QuickJSValue::ToFlexUIValue() const {
  if (IsUndefined() || IsNull()) return flexui::common::FlexUIValue();
  if (IsBoolean()) return flexui::common::FlexUIValue(ToBoolean());
  if (IsNumber())  return flexui::common::FlexUIValue(ToNumber());
  if (IsString())  return flexui::common::FlexUIValue(ToStdString());

  if (IsArray()) {
    flexui::common::FlexUIValue::FlexUIValueArrayType arr;
    uint32_t n = GetArrayLength();
    for (uint32_t i = 0; i < n; ++i) {
      arr.push_back(GetArrayItem(i)->ToFlexUIValue());
    }
    return flexui::common::FlexUIValue(std::move(arr));
  }

  if (IsObject()) {
    flexui::common::FlexUIValue::FlexUIValueObjectType obj;
    for (const auto& key : GetPropertyNames()) {
      obj[key] = GetProperty(key)->ToFlexUIValue();
    }
    return flexui::common::FlexUIValue(std::move(obj));
  }

  return flexui::common::FlexUIValue();
}

}  // namespace flexui::core::js_engine::quickjs
```

- [ ] **Step 8.3: Wire source**

Append to `flex-ui/core/js-engine/CMakeLists.txt`:

```cmake
target_sources(flexui_core_js_engine PRIVATE
  src/quickjs/quickjs_value.cc
)
target_include_directories(flexui_core_js_engine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs
)
```

(Build will fail until Task 9 lands `QuickJSContext`; that's expected. Don't commit a broken build — Task 9 lands together with this in one logical step.)

- [ ] **Step 8.4: Save your work locally without commit**

```bash
git add flex-ui/core/js-engine/src/quickjs/quickjs_value.{h,cc}
# do NOT commit yet; build is broken until Task 9.
```

---

### Task 9: QuickJS backend — `QuickJSContext` + `QuickJSEngine` + factory

**Files:**
- Create: `flex-ui/core/js-engine/src/quickjs/quickjs_context.h`
- Create: `flex-ui/core/js-engine/src/quickjs/quickjs_context.cc`
- Create: `flex-ui/core/js-engine/src/quickjs/quickjs_engine.h`
- Create: `flex-ui/core/js-engine/src/quickjs/quickjs_engine.cc`
- Create: `flex-ui/core/js-engine/src/js_engine_factory.cc`
- Modify: `flex-ui/core/js-engine/CMakeLists.txt`

- [ ] **Step 9.1: Write `quickjs_context.h`**

```cpp
#pragma once

#include "flexui/core/js-engine/ijs_context.h"
#include "quickjs.h"

#include <unordered_map>

namespace flexui::core::js_engine::quickjs {

class QuickJSEngine;

class QuickJSContext : public IJsContext {
 public:
  QuickJSContext(QuickJSEngine* engine, JSContext* qjs);
  ~QuickJSContext() override;

  std::shared_ptr<IJsValue> Eval(const std::string& source,
                                 const std::string& origin,
                                 flexui::common::Error* err) override;

  flexui::common::Error InjectGlobalFunction(const std::string& name,
                                             NativeFunction fn) override;
  flexui::common::Error InjectGlobalValue(const std::string& name,
                                          std::shared_ptr<IJsValue> value) override;

  std::shared_ptr<IJsValue> NewUndefined() override;
  std::shared_ptr<IJsValue> NewNull() override;
  std::shared_ptr<IJsValue> NewBoolean(bool v) override;
  std::shared_ptr<IJsValue> NewNumber(double v) override;
  std::shared_ptr<IJsValue> NewString(const std::string& v) override;
  std::shared_ptr<IJsValue> NewObject() override;
  std::shared_ptr<IJsValue> NewArray(uint32_t length) override;
  std::shared_ptr<IJsValue> FromFlexUIValue(
      const flexui::common::FlexUIValue& v) override;

  std::shared_ptr<IJsValue> Call(
      std::shared_ptr<IJsValue> function,
      std::shared_ptr<IJsValue> this_value,
      const std::vector<std::shared_ptr<IJsValue>>& args,
      flexui::common::Error* err) override;

  void RunPendingJobs() override;

  JSContext* raw_ctx() const { return qjs_; }

  // Used by the QuickJS C dispatch trampoline to look up native fn.
  NativeFunction* LookupNative(uint32_t id);

 private:
  QuickJSEngine* engine_;
  JSContext* qjs_;
  std::unordered_map<uint32_t, NativeFunction> native_fns_;
  uint32_t next_native_id_ = 1;
};

}  // namespace flexui::core::js_engine::quickjs
```

- [ ] **Step 9.2: Write `quickjs_context.cc`**

```cpp
#include "src/quickjs/quickjs_context.h"
#include "src/quickjs/quickjs_engine.h"
#include "src/quickjs/quickjs_value.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::js_engine::quickjs {

namespace {
constexpr const char* kNativeIdSym = "__flexui_native_id__";

// C trampoline: JS calls us, we dispatch to the stored NativeFunction.
JSValue NativeTrampoline(JSContext* qjs, JSValueConst this_val,
                         int argc, JSValueConst* argv,
                         int magic, JSValue* func_data) {
  auto* ctx = static_cast<QuickJSContext*>(JS_GetContextOpaque(qjs));
  uint32_t id = 0;
  JS_ToUint32(qjs, &id, func_data[0]);
  auto* fn = ctx->LookupNative(id);
  if (!fn) return JS_ThrowInternalError(qjs, "FlexUI: native id %u not found", id);

  std::vector<std::shared_ptr<IJsValue>> args;
  args.reserve(static_cast<size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    args.push_back(std::make_shared<QuickJSValue>(
        ctx, JS_DupValue(qjs, argv[i])));
  }
  auto result = (*fn)(*ctx, args);
  if (!result) return JS_UNDEFINED;
  auto* qr = dynamic_cast<QuickJSValue*>(result.get());
  return qr ? JS_DupValue(qjs, qr->raw()) : JS_UNDEFINED;
}
}  // namespace

QuickJSContext::QuickJSContext(QuickJSEngine* engine, JSContext* qjs)
    : engine_(engine), qjs_(qjs) {
  JS_SetContextOpaque(qjs_, this);
  FLEXUI_TLOG(JsEngine, ContextCreate, INFO) << "backend=QuickJS";
}

QuickJSContext::~QuickJSContext() {
  FLEXUI_TLOG(JsEngine, ContextDestroy, INFO) << "backend=QuickJS";
  JS_FreeContext(qjs_);
}

std::shared_ptr<IJsValue> QuickJSContext::Eval(const std::string& source,
                                               const std::string& origin,
                                               flexui::common::Error* err) {
  FLEXUI_TLOG(JsEngine, Eval, DEBUG) << "origin=" << origin
                                     << " size=" << source.size();
  JSValue v = JS_Eval(qjs_, source.c_str(), source.size(),
                      origin.c_str(), JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(v)) {
    JSValue exc = JS_GetException(qjs_);
    const char* msg = JS_ToCString(qjs_, exc);
    if (err) *err = flexui::common::Error(
        flexui::common::ErrorCode::kJsException, msg ? msg : "<no message>");
    if (msg) JS_FreeCString(qjs_, msg);
    JS_FreeValue(qjs_, exc);
    JS_FreeValue(qjs_, v);
    return std::make_shared<QuickJSValue>(this, JS_UNDEFINED);
  }
  return std::make_shared<QuickJSValue>(this, v);
}

flexui::common::Error QuickJSContext::InjectGlobalFunction(
    const std::string& name, NativeFunction fn) {
  uint32_t id = next_native_id_++;
  native_fns_[id] = std::move(fn);

  JSValue id_v = JS_NewUint32(qjs_, id);
  JSValue func = JS_NewCFunctionData(qjs_, &NativeTrampoline,
                                     /*length=*/0, /*magic=*/0,
                                     /*data_len=*/1, &id_v);
  JS_FreeValue(qjs_, id_v);

  JSValue global = JS_GetGlobalObject(qjs_);
  JS_SetPropertyStr(qjs_, global, name.c_str(), func);
  JS_FreeValue(qjs_, global);
  FLEXUI_TLOG(JsEngine, InjectGlobalFunction, DEBUG)
      << "name=" << name << " id=" << id;
  return flexui::common::Error::Ok();
}

flexui::common::Error QuickJSContext::InjectGlobalValue(
    const std::string& name, std::shared_ptr<IJsValue> value) {
  auto* qv = dynamic_cast<QuickJSValue*>(value.get());
  if (!qv) return flexui::common::Error(
      flexui::common::ErrorCode::kInvalidArgument, "value not QuickJSValue");
  JSValue global = JS_GetGlobalObject(qjs_);
  JS_SetPropertyStr(qjs_, global, name.c_str(), JS_DupValue(qjs_, qv->raw()));
  JS_FreeValue(qjs_, global);
  return flexui::common::Error::Ok();
}

std::shared_ptr<IJsValue> QuickJSContext::NewUndefined() {
  return std::make_shared<QuickJSValue>(this, JS_UNDEFINED);
}
std::shared_ptr<IJsValue> QuickJSContext::NewNull() {
  return std::make_shared<QuickJSValue>(this, JS_NULL);
}
std::shared_ptr<IJsValue> QuickJSContext::NewBoolean(bool v) {
  return std::make_shared<QuickJSValue>(this, JS_NewBool(qjs_, v));
}
std::shared_ptr<IJsValue> QuickJSContext::NewNumber(double v) {
  return std::make_shared<QuickJSValue>(this, JS_NewFloat64(qjs_, v));
}
std::shared_ptr<IJsValue> QuickJSContext::NewString(const std::string& v) {
  return std::make_shared<QuickJSValue>(this, JS_NewStringLen(qjs_, v.data(), v.size()));
}
std::shared_ptr<IJsValue> QuickJSContext::NewObject() {
  return std::make_shared<QuickJSValue>(this, JS_NewObject(qjs_));
}
std::shared_ptr<IJsValue> QuickJSContext::NewArray(uint32_t length) {
  JSValue arr = JS_NewArray(qjs_);
  JS_SetPropertyStr(qjs_, arr, "length", JS_NewUint32(qjs_, length));
  return std::make_shared<QuickJSValue>(this, arr);
}

std::shared_ptr<IJsValue> QuickJSContext::FromFlexUIValue(
    const flexui::common::FlexUIValue& v) {
  using T = flexui::common::FlexUIValue::Type;
  switch (v.GetType()) {
    case T::kUndefined: return NewUndefined();
    case T::kNull:      return NewNull();
    case T::kBoolean:   return NewBoolean(v.ToBoolean());
    case T::kNumber:    return NewNumber(v.ToDouble());
    case T::kString:    return NewString(v.ToString());
    case T::kArray: {
      auto src = v.ToArray();
      auto arr = NewArray(static_cast<uint32_t>(src.size()));
      auto* qa = dynamic_cast<QuickJSValue*>(arr.get());
      for (uint32_t i = 0; i < src.size(); ++i) {
        auto item = FromFlexUIValue(src[i]);
        auto* qi = dynamic_cast<QuickJSValue*>(item.get());
        JS_SetPropertyUint32(qjs_, qa->raw(), i, JS_DupValue(qjs_, qi->raw()));
      }
      return arr;
    }
    case T::kObject: {
      auto src = v.ToObject();
      auto obj = NewObject();
      auto* qo = dynamic_cast<QuickJSValue*>(obj.get());
      for (auto& kv : src) {
        auto item = FromFlexUIValue(kv.second);
        auto* qi = dynamic_cast<QuickJSValue*>(item.get());
        JS_SetPropertyStr(qjs_, qo->raw(), kv.first.c_str(),
                          JS_DupValue(qjs_, qi->raw()));
      }
      return obj;
    }
  }
  return NewUndefined();
}

std::shared_ptr<IJsValue> QuickJSContext::Call(
    std::shared_ptr<IJsValue> function,
    std::shared_ptr<IJsValue> this_value,
    const std::vector<std::shared_ptr<IJsValue>>& args,
    flexui::common::Error* err) {
  auto* qf = dynamic_cast<QuickJSValue*>(function.get());
  auto* qt = dynamic_cast<QuickJSValue*>(this_value.get());
  if (!qf) {
    if (err) *err = flexui::common::Error(
        flexui::common::ErrorCode::kInvalidArgument, "function not QuickJSValue");
    return NewUndefined();
  }
  std::vector<JSValue> qa;
  qa.reserve(args.size());
  for (auto& a : args) {
    auto* qv = dynamic_cast<QuickJSValue*>(a.get());
    qa.push_back(qv ? qv->raw() : JS_UNDEFINED);
  }
  JSValue ret = JS_Call(qjs_, qf->raw(), qt ? qt->raw() : JS_UNDEFINED,
                        static_cast<int>(qa.size()), qa.data());
  if (JS_IsException(ret)) {
    JSValue exc = JS_GetException(qjs_);
    const char* msg = JS_ToCString(qjs_, exc);
    if (err) *err = flexui::common::Error(
        flexui::common::ErrorCode::kJsException, msg ? msg : "<no message>");
    if (msg) JS_FreeCString(qjs_, msg);
    JS_FreeValue(qjs_, exc);
  }
  return std::make_shared<QuickJSValue>(this, ret);
}

void QuickJSContext::RunPendingJobs() {
  JSContext* pending = nullptr;
  while (JS_ExecutePendingJob(JS_GetRuntime(qjs_), &pending) > 0) {
    // drain
  }
}

NativeFunction* QuickJSContext::LookupNative(uint32_t id) {
  auto it = native_fns_.find(id);
  return it == native_fns_.end() ? nullptr : &it->second;
}

}  // namespace flexui::core::js_engine::quickjs
```

- [ ] **Step 9.3: Write `quickjs_engine.h`**

```cpp
#pragma once

#include "flexui/core/js-engine/ijs_engine.h"
#include "quickjs.h"

namespace flexui::core::js_engine::quickjs {

class QuickJSEngine : public IJsEngine {
 public:
  QuickJSEngine();
  ~QuickJSEngine() override;

  flexui::common::Error Initialize(const EngineConfig& cfg) override;
  std::shared_ptr<IJsContext> CreateContext() override;
  void Shutdown() override;
  const char* BackendName() const override { return "QuickJS"; }

  JSRuntime* raw_rt() const { return rt_; }

 private:
  JSRuntime* rt_ = nullptr;
};

}  // namespace flexui::core::js_engine::quickjs
```

- [ ] **Step 9.4: Write `quickjs_engine.cc`**

```cpp
#include "src/quickjs/quickjs_engine.h"
#include "src/quickjs/quickjs_context.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::js_engine::quickjs {

QuickJSEngine::QuickJSEngine() = default;

QuickJSEngine::~QuickJSEngine() { Shutdown(); }

flexui::common::Error QuickJSEngine::Initialize(const EngineConfig& cfg) {
  FLEXUI_TLOG(JsEngine, EngineInit, INFO) << "backend=QuickJS";
  rt_ = JS_NewRuntime();
  if (!rt_) return flexui::common::Error(
      flexui::common::ErrorCode::kInternal, "JS_NewRuntime returned null");
  if (cfg.memory_limit_mb > 0) {
    JS_SetMemoryLimit(rt_, cfg.memory_limit_mb * 1024 * 1024);
  }
  return flexui::common::Error::Ok();
}

std::shared_ptr<IJsContext> QuickJSEngine::CreateContext() {
  if (!rt_) {
    FLEXUI_TLOG(JsEngine, CreateContext, ERROR) << "engine not initialized";
    return nullptr;
  }
  JSContext* qjs = JS_NewContext(rt_);
  return std::make_shared<QuickJSContext>(this, qjs);
}

void QuickJSEngine::Shutdown() {
  if (rt_) {
    FLEXUI_TLOG(JsEngine, EngineShutdown, INFO) << "backend=QuickJS";
    JS_FreeRuntime(rt_);
    rt_ = nullptr;
  }
}

}  // namespace flexui::core::js_engine::quickjs
```

- [ ] **Step 9.5: Write `js_engine_factory.cc`**

```cpp
#include "flexui/core/js-engine/js_engine_factory.h"

#include "src/quickjs/quickjs_engine.h"
#if defined(FLEXUI_OHOS)
#include "src/jsvm/jsvm_engine.h"
#endif

namespace flexui::core::js_engine {

std::unique_ptr<IJsEngine> MakeJsEngine(JsEngineBackend backend) {
  switch (backend) {
    case JsEngineBackend::kQuickJS:
      return std::make_unique<quickjs::QuickJSEngine>();
    case JsEngineBackend::kJsvm:
#if defined(FLEXUI_OHOS)
      return std::make_unique<jsvm::JsvmEngine>();
#else
      return nullptr;
#endif
  }
  return nullptr;
}

std::unique_ptr<IJsEngine> MakeDefaultJsEngine(bool force_quickjs_for_debug) {
  return MakeJsEngine(SelectBackend(force_quickjs_for_debug));
}

JsEngineBackend SelectBackend(bool force_quickjs_for_debug) {
#if defined(FLEXUI_OHOS)
  if (force_quickjs_for_debug) return JsEngineBackend::kQuickJS;
  return JsEngineBackend::kJsvm;
#else
  (void)force_quickjs_for_debug;
  return JsEngineBackend::kQuickJS;
#endif
}

}  // namespace flexui::core::js_engine
```

- [ ] **Step 9.6: Wire sources**

```cmake
target_sources(flexui_core_js_engine PRIVATE
  src/js_engine_factory.cc
  src/quickjs/quickjs_context.cc
  src/quickjs/quickjs_engine.cc
)
```

- [ ] **Step 9.7: Build, fix any compile errors, commit Tasks 8+9 together**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/js-engine
git commit -m "feat(flex-ui/core/js-engine): implement QuickJS backend (Value/Context/Engine + factory)"
```

---

### Task 10: QuickJS unit tests

**Files:**
- Create: `tests/flex-ui/unit/js_engine_quickjs_test.cc`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 10.1: Write `js_engine_quickjs_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/js-engine/js_engine_factory.h"

namespace flexui::core::js_engine {

class QuickJSEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    engine_ = MakeJsEngine(JsEngineBackend::kQuickJS);
    ASSERT_TRUE(engine_);
    ASSERT_TRUE(engine_->Initialize({}).ok());
  }
  void TearDown() override { engine_.reset(); }

  std::unique_ptr<IJsEngine> engine_;
};

TEST_F(QuickJSEngineTest, BackendNameIsQuickJS) {
  EXPECT_STREQ(engine_->BackendName(), "QuickJS");
}

TEST_F(QuickJSEngineTest, EvalNumericExpression) {
  auto ctx = engine_->CreateContext();
  ASSERT_TRUE(ctx);
  flexui::common::Error err;
  auto v = ctx->Eval("1 + 2", "<test>", &err);
  EXPECT_TRUE(err.ok());
  EXPECT_TRUE(v->IsNumber());
  EXPECT_DOUBLE_EQ(v->ToNumber(), 3.0);
}

TEST_F(QuickJSEngineTest, EvalSyntaxErrorReportedAsException) {
  auto ctx = engine_->CreateContext();
  flexui::common::Error err;
  auto v = ctx->Eval("function( {", "<test>", &err);
  EXPECT_FALSE(err.ok());
  EXPECT_EQ(err.code(), flexui::common::ErrorCode::kJsException);
}

TEST_F(QuickJSEngineTest, InjectedFunctionCallable) {
  auto ctx = engine_->CreateContext();
  int call_count = 0;
  ctx->InjectGlobalFunction("addOne", [&call_count](IJsContext& c,
      const std::vector<std::shared_ptr<IJsValue>>& args) {
    ++call_count;
    double in = args.empty() ? 0.0 : args[0]->ToNumber();
    return c.NewNumber(in + 1);
  });
  flexui::common::Error err;
  auto v = ctx->Eval("addOne(41)", "<test>", &err);
  EXPECT_TRUE(err.ok());
  EXPECT_DOUBLE_EQ(v->ToNumber(), 42.0);
  EXPECT_EQ(call_count, 1);
}

TEST_F(QuickJSEngineTest, FlexUIValueRoundTrip) {
  auto ctx = engine_->CreateContext();
  using F = flexui::common::FlexUIValue;
  F input(F::FlexUIValueObjectType{
      {"name", F(std::string("flex"))},
      {"count", F(7.0)},
  });
  auto jv = ctx->FromFlexUIValue(input);
  auto out = jv->ToFlexUIValue();
  EXPECT_EQ(out.ToObject()["name"].ToString(), "flex");
  EXPECT_DOUBLE_EQ(out.ToObject()["count"].ToDouble(), 7.0);
}

TEST_F(QuickJSEngineTest, MultipleContextsIsolateState) {
  auto a = engine_->CreateContext();
  auto b = engine_->CreateContext();
  flexui::common::Error err;
  a->Eval("var x = 1", "<a>", &err);
  b->Eval("var x = 99", "<b>", &err);
  EXPECT_DOUBLE_EQ(a->Eval("x", "<a>", &err)->ToNumber(), 1.0);
  EXPECT_DOUBLE_EQ(b->Eval("x", "<b>", &err)->ToNumber(), 99.0);
}

}  // namespace flexui::core::js_engine
```

- [ ] **Step 10.2: Wire test**

Append `js_engine_quickjs_test.cc` to `tests/flex-ui/unit/CMakeLists.txt` and add `flexui_core_js_engine` to its links.

- [ ] **Step 10.3: Build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add tests/flex-ui/unit
git commit -m "test(flex-ui/core/js-engine): QuickJS backend unit tests (eval, inject, isolation, FlexUIValue round-trip)"
```

---

### Task 11: JSVM backend (HarmonyOS) — headers + stub implementation

This task lands the JSVM backend **code** so it compiles when `FLEXUI_OHOS` is defined. Actual device-side exercise is W9-W10. On host, the JSVM TU compiles to an empty stub (returns `nullptr` from the factory) to keep CMake simple.

**Files:**
- Create: `flex-ui/core/js-engine/src/jsvm/jsvm_engine.h`
- Create: `flex-ui/core/js-engine/src/jsvm/jsvm_engine.cc`
- Create: `flex-ui/core/js-engine/src/jsvm/jsvm_context.h`
- Create: `flex-ui/core/js-engine/src/jsvm/jsvm_context.cc`
- Create: `flex-ui/core/js-engine/src/jsvm/jsvm_value.cc`
- Modify: `flex-ui/core/js-engine/CMakeLists.txt`

- [ ] **Step 11.1: Write `jsvm_engine.h`**

```cpp
#pragma once
#if defined(FLEXUI_OHOS)

#include "flexui/core/js-engine/ijs_engine.h"
#include <ark_runtime/jsvm.h>

namespace flexui::core::js_engine::jsvm {

class JsvmEngine : public IJsEngine {
 public:
  JsvmEngine();
  ~JsvmEngine() override;

  flexui::common::Error Initialize(const EngineConfig& cfg) override;
  std::shared_ptr<IJsContext> CreateContext() override;
  void Shutdown() override;
  const char* BackendName() const override { return "JSVM"; }

  JSVM_VM raw_vm() const { return vm_; }

 private:
  JSVM_VM vm_ = nullptr;
};

}  // namespace flexui::core::js_engine::jsvm

#endif  // FLEXUI_OHOS
```

- [ ] **Step 11.2: Write `jsvm_engine.cc`**

```cpp
#include "src/jsvm/jsvm_engine.h"

#if defined(FLEXUI_OHOS)
#include "src/jsvm/jsvm_context.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::js_engine::jsvm {

JsvmEngine::JsvmEngine() = default;
JsvmEngine::~JsvmEngine() { Shutdown(); }

flexui::common::Error JsvmEngine::Initialize(const EngineConfig& cfg) {
  FLEXUI_TLOG(JsEngine, EngineInit, INFO) << "backend=JSVM";
  JSVM_InitOptions init_options = {};
  if (OH_JSVM_Init(&init_options) != JSVM_OK) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal,
                                 "OH_JSVM_Init failed");
  }
  JSVM_CreateVMOptions vm_options = {};
  if (cfg.memory_limit_mb > 0) {
    vm_options.maxOldGenerationSize = cfg.memory_limit_mb;
  }
  if (OH_JSVM_CreateVM(&vm_options, &vm_) != JSVM_OK) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal,
                                 "OH_JSVM_CreateVM failed");
  }
  return flexui::common::Error::Ok();
}

std::shared_ptr<IJsContext> JsvmEngine::CreateContext() {
  if (!vm_) {
    FLEXUI_TLOG(JsEngine, CreateContext, ERROR) << "vm not initialized";
    return nullptr;
  }
  return std::make_shared<JsvmContext>(this);
}

void JsvmEngine::Shutdown() {
  if (vm_) {
    FLEXUI_TLOG(JsEngine, EngineShutdown, INFO) << "backend=JSVM";
    OH_JSVM_DestroyVM(vm_);
    vm_ = nullptr;
  }
}

}  // namespace flexui::core::js_engine::jsvm
#endif  // FLEXUI_OHOS
```

- [ ] **Step 11.3: Write `jsvm_context.h`**

```cpp
#pragma once
#if defined(FLEXUI_OHOS)

#include "flexui/core/js-engine/ijs_context.h"
#include <ark_runtime/jsvm.h>

namespace flexui::core::js_engine::jsvm {

class JsvmEngine;

class JsvmContext : public IJsContext {
 public:
  explicit JsvmContext(JsvmEngine* engine);
  ~JsvmContext() override;

  std::shared_ptr<IJsValue> Eval(const std::string& source,
                                 const std::string& origin,
                                 flexui::common::Error* err) override;
  flexui::common::Error InjectGlobalFunction(const std::string& name,
                                             NativeFunction fn) override;
  flexui::common::Error InjectGlobalValue(const std::string& name,
                                          std::shared_ptr<IJsValue> value) override;

  std::shared_ptr<IJsValue> NewUndefined() override;
  std::shared_ptr<IJsValue> NewNull() override;
  std::shared_ptr<IJsValue> NewBoolean(bool v) override;
  std::shared_ptr<IJsValue> NewNumber(double v) override;
  std::shared_ptr<IJsValue> NewString(const std::string& v) override;
  std::shared_ptr<IJsValue> NewObject() override;
  std::shared_ptr<IJsValue> NewArray(uint32_t length) override;
  std::shared_ptr<IJsValue> FromFlexUIValue(
      const flexui::common::FlexUIValue& v) override;

  std::shared_ptr<IJsValue> Call(
      std::shared_ptr<IJsValue> function,
      std::shared_ptr<IJsValue> this_value,
      const std::vector<std::shared_ptr<IJsValue>>& args,
      flexui::common::Error* err) override;

  void RunPendingJobs() override;

  JSVM_Env env() const { return env_; }

 private:
  JsvmEngine* engine_;
  JSVM_Env env_ = nullptr;
};

}  // namespace flexui::core::js_engine::jsvm

#endif  // FLEXUI_OHOS
```

- [ ] **Step 11.4: Write `jsvm_context.cc`** — minimal-viable implementation calling the JSVM C API. For the PoC, implement `Eval`, `InjectGlobalFunction`, the `New*` factories, and `Call`. Use the same NativeFunction trampoline pattern as QuickJS, replacing the C-trampoline body with `OH_JSVM_CreateFunction` and reading args via `OH_JSVM_GetCbInfo`.

The full code body mirrors `quickjs_context.cc` in structure (use the QuickJS file as the template); replace `JS_*` calls with `OH_JSVM_*` equivalents. The W3-W4 deliverable is "compiles when `FLEXUI_OHOS` set"; W9-W10 exercises this on a real device.

For each public method, follow this contract: if the underlying `OH_JSVM_*` call returns non-`JSVM_OK`, log `FLEXUI_TLOG(JsEngine, <event>, ERROR)` with the status code and return either a default value or set `*err`. Match the QuickJS context's logging cadence so production logs line up.

- [ ] **Step 11.5: Write `jsvm_value.cc`** — concrete `JsvmValue : IJsValue` implementing the same surface as `QuickJSValue` (Task 8) but using `OH_JSVM_*` calls. Place the class declaration as a private (`namespace { ... }`) class inside `jsvm_value.cc` since no other TU needs it — only `jsvm_context.cc` references it via shared_ptr<IJsValue>.

- [ ] **Step 11.6: Wire sources (conditionally)**

```cmake
if(FLEXUI_OHOS)
  target_sources(flexui_core_js_engine PRIVATE
    src/jsvm/jsvm_engine.cc
    src/jsvm/jsvm_context.cc
    src/jsvm/jsvm_value.cc
  )
  target_link_libraries(flexui_core_js_engine PUBLIC libjsvm.so)
endif()
```

- [ ] **Step 11.7: Host build verification**

```bash
./scripts/flex-ui/build.sh
```

Expected: JSVM TUs compile out via the `#if defined(FLEXUI_OHOS)` guard. Factory returns `nullptr` for `kJsvm` on host. QuickJS tests still pass.

- [ ] **Step 11.8: Commit**

```bash
git add flex-ui/core/js-engine
git commit -m "feat(flex-ui/core/js-engine): JSVM backend (HarmonyOS, FLEXUI_OHOS-gated)"
```

---

### Task 12: JSVM smoke test (conditional)

**Files:**
- Create: `tests/flex-ui/unit/js_engine_jsvm_test.cc`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

This test is identical in shape to the QuickJS one (Task 10) but constructs `MakeJsEngine(JsEngineBackend::kJsvm)`. It only links + runs when `FLEXUI_OHOS` is defined — on host it short-circuits to `SUCCEED()` so the binary still compiles.

- [ ] **Step 12.1: Write `js_engine_jsvm_test.cc`**

```cpp
#include <gtest/gtest.h>
#include "flexui/core/js-engine/js_engine_factory.h"

namespace flexui::core::js_engine {

#if defined(FLEXUI_OHOS)

TEST(JsvmEngineTest, EvalNumericExpression) {
  auto eng = MakeJsEngine(JsEngineBackend::kJsvm);
  ASSERT_TRUE(eng);
  ASSERT_TRUE(eng->Initialize({}).ok());
  auto ctx = eng->CreateContext();
  flexui::common::Error err;
  auto v = ctx->Eval("1 + 2", "<test>", &err);
  EXPECT_TRUE(err.ok());
  EXPECT_DOUBLE_EQ(v->ToNumber(), 3.0);
}

#else

TEST(JsvmEngineTest, NullOnHost) {
  auto eng = MakeJsEngine(JsEngineBackend::kJsvm);
  EXPECT_EQ(eng, nullptr);
}

#endif

}  // namespace flexui::core::js_engine
```

- [ ] **Step 12.2: Wire test**

Append to `tests/flex-ui/unit/CMakeLists.txt`.

- [ ] **Step 12.3: Build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add tests/flex-ui/unit
git commit -m "test(flex-ui/core/js-engine): JSVM smoke test (host: factory returns null; Harmony: eval works)"
```

---

### Task 13: Wire `FLEXUI_TLOG` into the four subsystem facades

Per spec §8.4 mandatory log coverage: confirm each public method on the four subsystem facades emits a `FLEXUI_TLOG`. Audit and add where missing.

- [ ] **Step 13.1: Audit checklist (no code, just verification)**

| Subsystem | File | Required log points |
|---|---|---|
| JsEngine | `quickjs_engine.cc` | EngineInit, EngineShutdown, CreateContext ✅ (added in Task 9) |
| JsEngine | `quickjs_context.cc` | ContextCreate, ContextDestroy, Eval, InjectGlobalFunction ✅ |
| JsEngine | `jsvm_engine.cc` | EngineInit, EngineShutdown, CreateContext ✅ |
| Reconciler | `diff_utils.cc` | Diff (enter w/ tree summary, exit w/ mutation count) — **add** |
| Layout | `yoga_layout_node.cc` | CalculateLayout (enter w/ constraints, exit w/ duration) — **add** |
| Vdom | `dom_node.cc` | NodeCreate / NodeDestroy at top of ctor / dtor — **add** |

- [ ] **Step 13.2: Add the missing logs**

For `diff_utils.cc`, find the entry point `DiffUtils::DiffProps` (or the top-level diff function in the absorbed source). Insert at function entry:

```cpp
FLEXUI_TLOG(Reconciler, DiffEnter, DEBUG)
    << "old_count=" << (from.size())
    << " new_count=" << (to.size());
```

And at function exit (or just before each `return`):

```cpp
FLEXUI_TLOG(Reconciler, DiffExit, DEBUG)
    << "result_changes=<computed-count>";
```

For `yoga_layout_node.cc::CalculateLayout`:

```cpp
auto t0 = flexui::common::TimePoint::Now();
FLEXUI_TLOG(Layout, CalculateEnter, DEBUG)
    << "constraints=" << constraint_width << "x" << constraint_height;
// ... existing body ...
FLEXUI_TLOG(Layout, CalculateExit, DEBUG)
    << "duration_ms="
    << (flexui::common::TimePoint::Now() - t0).ToMilliseconds();
```

For `dom_node.cc`, add into the constructor:

```cpp
FLEXUI_TLOG(Vdom, NodeCreate, DEBUG)
    << "id=" << id_ << " view=" << view_name_;
```

And in the destructor:

```cpp
FLEXUI_TLOG(Vdom, NodeDestroy, DEBUG)
    << "id=" << id_;
```

- [ ] **Step 13.3: Build, run all tests, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core
git commit -m "feat(flex-ui/core): wire FLEXUI_TLOG mandatory coverage points (spec §8.4)"
```

---

### Task 14: Coverage gate ≥80% on `flex-ui/core/`

- [ ] **Step 14.1: Extend `scripts/flex-ui/coverage.sh`**

Edit the script to additionally measure `flex-ui/core/`:

Replace the single measurement block with:

```bash
for sub in common core; do
  if [[ "$(uname)" == "Darwin" ]]; then
    PERCENT=$(xcrun llvm-cov report "$TEST_BIN" -instr-profile="$PROFDATA" \
                "${ROOT}/flex-ui/${sub}/" | tail -1 | awk '{print $7}' | tr -d '%')
  else
    lcov --extract "${BUILD_DIR}/coverage.info" "*/flex-ui/${sub}/*" \
         --output-file "${BUILD_DIR}/coverage.${sub}.info"
    PERCENT=$(lcov --summary "${BUILD_DIR}/coverage.${sub}.info" 2>&1 \
                | awk '/lines/ {print $2}' | tr -d '%' | head -1)
  fi
  echo "Line coverage for flex-ui/${sub}/: ${PERCENT}%"
  PERCENT_INT=${PERCENT%.*}
  if (( PERCENT_INT < 80 )); then
    echo "FAIL: ${sub} coverage ${PERCENT}% below 80%"
    exit 1
  fi
done
echo "PASS: coverage gates met"
```

- [ ] **Step 14.2: Run coverage**

```bash
./scripts/flex-ui/coverage.sh
```

If `core/` is below 80%, identify uncovered units (likely the absorbed stubs that no test exercises, or QuickJS/JSVM glue we haven't directly hit) and add tests until the gate passes. Do not lower the threshold.

- [ ] **Step 14.3: Commit**

```bash
git add scripts/flex-ui/coverage.sh
git commit -m "test(flex-ui): extend coverage gate to flex-ui/core/ (≥80%)"
```

---

### Task 15: Append W3-W4 attributions to NOTICE

- [ ] **Step 15.1: Edit `flex-ui/NOTICE`**

Replace the existing footstone-only Hippy section with:

```
The following files in flex-ui/common/ and flex-ui/core/ are derived
from the Hippy project (commit-pinned at integration time):

  flex-ui/common/                        (footstone — W1-W2)
  flex-ui/core/vdom/dom_node.{h,cc}      (dom/dom_node.{h,cc})
  flex-ui/core/vdom/dom_argument.{h,cc}  (dom/dom_argument.{h,cc})
  flex-ui/core/vdom/node_props.h         (dom/node_props.h)
  flex-ui/core/reconciler/diff_utils.{h,cc}      (dom/diff_utils.{h,cc})
  flex-ui/core/layout/layout_node.{h,cc}         (dom/layout_node.{h,cc})
  flex-ui/core/layout/yoga_layout_node.{h,cc}    (dom/yoga_layout_node.{h,cc})
```

Append the QuickJS and Yoga sections from Task 7 (QuickJS already inserted; add Yoga similarly):

```
Yoga layout engine is vendored via CMake FetchContent at build time
from https://github.com/facebook/yoga (MIT). No source files are stored
in this repository.
```

- [ ] **Step 15.2: Commit**

```bash
git add flex-ui/NOTICE
git commit -m "docs(flex-ui): update NOTICE with W3-W4 Hippy dom/* + Yoga attributions"
```

---

### Task 16: TSan run on the four new subsystems

The reconciler and layout don't yet have multi-thread state, but the JS engine subsystem and any timer interactions might. Confirm TSan still passes after W3-W4.

- [ ] **Step 16.1: Run TSan**

```bash
./scripts/flex-ui/build.sh --tsan-only
```

Expected: passes. If new races appear (likely in QuickJS atomic counters touched from tests), document them in `flex-ui/README.md` and do not fix in W3-W4.

- [ ] **Step 16.2: Commit (only if README updated)**

```bash
git add flex-ui/README.md   # if updated
git commit -m "test(flex-ui/core): document TSan findings for QuickJS/JSVM (informational)" --allow-empty
```

---

### Task 17: TSan target update

Add `js_engine_quickjs_test.cc` to the TSan sub-target (Task 18 in W1-W2). The JS engine multiple-context test exercises concurrent context creation.

- [ ] **Step 17.1: Edit `tests/flex-ui/unit/CMakeLists.txt`**

In the `add_executable(flexui_unit_tests_tsan ...)` block, append `js_engine_quickjs_test.cc`.

- [ ] **Step 17.2: Verify**

```bash
./scripts/flex-ui/build.sh --tsan-only
```

- [ ] **Step 17.3: Commit**

```bash
git add tests/flex-ui/unit/CMakeLists.txt
git commit -m "test(flex-ui): include js_engine_quickjs_test in TSan sub-target"
```

---

### Task 18: Coexistence verification + final green gate

- [ ] **Step 18.1: Verify AGenUI tree untouched**

```bash
git diff master --stat -- core/ platforms/ tests/cpp/ playground/ scripts/harmony/ scripts/android/ scripts/ios/ agent_sdks/ samples/ skills/
```

Expected: empty.

- [ ] **Step 18.2: Full host gate**

```bash
./scripts/flex-ui/build.sh
./scripts/flex-ui/build.sh --tsan-only
./scripts/flex-ui/coverage.sh
```

All three green.

- [ ] **Step 18.3: Confirm W3-W4 deliverables**

Run the following checklist by inspection:

- `flex-ui/core/CMakeLists.txt` and 4 sub-CMakeLists.txt exist.
- `flexui_core_js_engine` / `flexui_core_vdom` / `flexui_core_reconciler` / `flexui_core_layout` static libs build.
- `IJsEngine` interface present in `js-engine/include/flexui/core/js-engine/ijs_engine.h`.
- QuickJS backend tests (8+ tests in `js_engine_quickjs_test.cc`) pass.
- JSVM backend code compiles when `-DFLEXUI_OHOS=1`; on host returns null.
- DomNode tests pass.
- diff_utils tests pass.
- Yoga layout tests pass.
- Coverage gate ≥ 80% on both `flex-ui/common/` (W1-W2) and `flex-ui/core/` (W3-W4).
- Mandatory `FLEXUI_TLOG` points present at every subsystem facade method per spec §8.4.

If anything is missing, return to the relevant task and complete it before declaring W3-W4 done.

- [ ] **Step 18.4: Optional final commit (only if README/docs need touching)**

```bash
git commit -m "chore(flex-ui/core): W3-W4 final green gate" --allow-empty
```

---

## Self-Review

### Spec coverage

| Spec requirement | Covered by |
|---|---|
| §4.4 absorption rows: dom/, driver/ subset | Tasks 2-5, 8-11 |
| §4.5 JS engine matrix: QuickJS all platforms; JSVM Harmony with QuickJS debug fallback | Tasks 6-11 |
| §4.5 minimum surface area (createRuntime/createContext/eval/call/global injection/GC root mgmt/snapshot serialize-deserialize) | Task 6 (Snapshot deferred to W5-W6 per Out-of-scope) |
| §4.8 Mutation List output of Diff | Task 4 (Mutation typedef) |
| §6.4 unit coverage ≥80% on flex-ui/core/ | Task 14 |
| §7.1 unit test methodology (GoogleTest + ASan/UBSan + TSan curated) | Tasks 10, 12, 17 |
| §8.2 FLEXUI/<subsystem>/<event> tag convention | Task 13 |
| §8.4 mandatory log coverage matrix | Task 13 |
| §5.1 AGenUI tree untouched | Task 18 |
| §4.4 Taitank deletion | Implicit: not absorbed (Task 5 only copies yoga_layout_node) |

### Placeholder scan

No "TBD", "TODO", "fill in later". One acknowledged underspecification: Task 11 step 11.4-11.5 say "follow the QuickJS structure, replacing JS_* with OH_JSVM_* equivalents" rather than pasting 200 lines of JSVM C calls verbatim. This is intentional and explicit — the executor reads `quickjs_context.cc` (Task 9) and the JSVM C header to do the translation; the per-line guidance ("if status != JSVM_OK log + return default") is concrete enough to follow without ambiguity. The same pattern is used in W1-W2 for absorbed methods whose signatures the executor confirms by reading the source.

### Type consistency

- `IJsEngine` / `IJsContext` / `IJsValue` interface signatures defined in Task 6 are used in Task 9 (QuickJS), Task 10 (tests), Task 11 (JSVM) consistently.
- `JsEngineBackend::kQuickJS` / `kJsvm` enum used in factory (Task 9, step 9.5) and tests (Task 10, 12).
- `flexui::common::Error::Ok()` / `Error(code, message)` from W1-W2 plan Task 17 used in all engine impls.
- `flexui::common::FlexUIValue::FlexUIValueArrayType` / `FlexUIValueObjectType` used in `quickjs_value.cc::ToFlexUIValue` and `quickjs_context.cc::FromFlexUIValue` — these are the Hippy-footstone names preserved through the W1-W2 file rename (Task 15).
- `Mutation` variant defined in Task 4 used implicitly through W5-W6; no premature consumers in this plan.

### Scope check

This plan delivers a self-contained kernel: JS evaluation works, vdom can be built, diffs compute, layout calculates. Nothing in W3-W4 depends on W5-W6 to be useful (e.g., the kernel can be exercised programmatically through unit tests right now). W5-W6 will declare a dependency on this plan being complete.

---

*End of W3-W4 core kernel plan.*
