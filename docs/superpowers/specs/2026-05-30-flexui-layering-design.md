# FlexUI Layering Design — PoC Spec

**Date:** 2026-05-30
**Status:** Draft (pending user review)
**Scope:** PoC + architecture validation (C2). Not a product delivery.

---

## 1. Background and Motivation

The current `AGenUI` HarmonyOS implementation under `platforms/harmony/agenui/` is a vertically-integrated, surface-based renderer for A2UI JSON. It tightly couples streaming parsing, virtual DOM, Yoga layout, dispatcher, and ArkUI ETS component rendering. Two problems follow:

1. Performance in feed/waterfall scenarios (frame drops, layout jank) caused by surface-mode overhead.
2. Lack of layered architecture: there is no reusable "component layer" or "dynamic view layer" — anything new (FlexCard, future Page hosting, additional frontends) has to clone the surface stack.

The goal of this PoC is to **prove a layered, multi-frontend dynamic UI framework can replace the AGenUI monolith** while:

- Keeping existing AGenUI code untouched during PoC (parallel coexistence).
- Validating that the same A2UI JSON, rendered through two different frontend paths, produces pixel-identical results.
- Locking cross-platform abstractions on day one so HarmonyOS / Android / iOS share one architecture even if implementations land in phases.

## 2. Goals (PoC Validation Target)

PoC validation level is **C2 — Architecture supports both FlexCard and A2UI in one layered stack**.

The PoC succeeds when:

- Same A2UI JSON renders pixel-identical via `a2ui-json` frontend and `card-js` frontend (hand-written equivalent), proving the layered architecture supports both.
- HarmonyOS implementation runs end-to-end (engine init → bundle load → render → setData → diff → ArkUI C-API mount).
- Cross-platform interfaces (JS engine abstraction, bridge, component layer) compile on Android with stub implementations to prove portability.
- Existing AGenUI code is not touched; both engines coexist in `playground/harmony` for side-by-side demonstration.

## 3. Non-Goals

The following are explicitly out of scope for the PoC and must not creep in:

- Android runtime implementation (interfaces only; stubs allowed).
- iOS support (interfaces only; deferred to Phase 3).
- DevTools / Chrome Inspector / source map / breakpoint debugging.
- Hot reload.
- Animation module.
- A2UI streaming (incremental feed) mode.
- A2UI complex components (table, datetime, audioplayer, choice_picker, etc.).
- React-like / Vue-like upper framework (hooks, fiber, JSX compiler).
- WXML / mini-program compiler toolchain.
- Snapshot optimization (PoC uses naive per-Scope injection; accepts cold-start latency).
- Multi-threaded JS isolation (all Scopes serialize on one JS thread).
- `AGenUIEngine` facade (Phase 2+).
- Any work belonging to Phase 1/2/3/4+.

## 4. Architecture

### 4.1 Layered Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│ App (ETS / Kotlin / Swift)                                           │
│   Business code consuming FlexCard / (future) FlexPage host          │
└─────────────────────────────────┬────────────────────────────────────┘
                                  │ Public SDK API
┌─────────────────────────────────▼────────────────────────────────────┐
│ Host Layer (flex-ui/platforms/{platform}/card/)                      │
│   • platforms/{p}/card/ : FlexCardController + FlexCard ETS component│
│   • platforms/{p}/page/ : (reserved for future Page-level hosting)   │
└─────────────────────────────────┬────────────────────────────────────┘
                                  │ NAPI / JNI / Obj-C++ binding
┌─────────────────────────────────▼────────────────────────────────────┐
│ Dynamic View Layer (flex-ui/core/ + flex-ui/components/ + api/)      │
│   ┌──────────────────────────────┐                                   │
│   │ Frontend (plugin extension): │                                   │
│   │   • card-js   (built-in)     │ → createElement / render / setData│
│   │   • a2ui-json (plugin-a2ui)  │ → JSON template + data binding    │
│   └─────────────┬────────────────┘                                   │
│                 │ produces DomNode tree (Vdom)                       │
│                 ▼                                                    │
│   ┌──────────────────────────────────────────────────────────────┐   │
│   │ Reconciler (Diff) + Yoga Layout                              │   │
│   │ → Mutation List (Create/Update/Move/Delete/UpdateLayout)     │   │
│   └─────────────┬────────────────────────────────────────────────┘   │
│                 │ async post to UI thread                            │
│                 ▼                                                    │
│   ┌──────────────────────────────────────────────────────────────┐   │
│   │ Commit Pipeline → Component Layer (capi or ets-builder)      │   │
│   └─────────────┬────────────────────────────────────────────────┘   │
└─────────────────┼────────────────────────────────────────────────────┘
                  │
┌─────────────────▼────────────────────────────────────────────────────┐
│ OS Layer: HarmonyOS ArkUI C-API / Android View / iOS UIView          │
└──────────────────────────────────────────────────────────────────────┘
```

### 4.2 Public SDK API

Framework-level (singleton, shared across hosts):

```ts
class FlexUIEngine {
  static init(config: EngineConfig): void;
  static getInstance(): FlexUIEngine;
  install(plugin: FlexUIPlugin): void;
  uninstall(name: string): void;
}

interface FlexUIPlugin {
  name: string;
  version?: string;
  components?: ComponentFactory[];
  apis?: NativeApi[];
  loaders?: BundleLoader[];
  frontends?: Frontend[];
  onInstall?(engine: FlexUIEngine): void;
  onUninstall?(): void;
}
```

Card-host-level (per-card instance, plus ETS @Component for mounting):

```ts
class FlexCardController {
  constructor(options: {
    bundleUri?: string;
    bundle?: string | ArrayBuffer;
    bundleType: 'card-js' | 'a2ui-json';
    initialData?: object;
    contextOptions?: { memoryLimitMB?: number };
  });
  load(): Promise<void>;
  setData(data: object, cb?: () => void): void;
  callMethod(name: string, args?: any[]): Promise<any>;
  onEvent(event: string, handler: (payload: unknown) => void): void;
  onError(handler: (err: Error) => void): void;
  destroy(): void;
}

@Component
struct FlexCard {
  @ObjectLink controller: FlexCardController;
  build() { /* ContentSlot(NodeContent) — see §4.6 */ }
}
```

Future Page-host (Phase 4+): `FlexPageController` / `FlexPage`, same `FlexUIEngine` singleton, same plugin set, only host changes.

### 4.3 Module Layout

```
FlexUI/
├── core/                            # existing AGenUI core (untouched in PoC)
├── platforms/                       # existing AGenUI bindings (untouched in PoC)
│
├── flex-ui/                         # PoC main body
│   ├── CMakeLists.txt
│   ├── NOTICE                       # Hippy Apache-2.0 attribution
│   ├── common/                      # logger, error, types, TaskRunner, bytes
│   ├── core/                        # framework kernel, pure C++ cross-platform
│   │   ├── js-engine/
│   │   │   ├── ijs_engine.h         # cross-platform interface
│   │   │   ├── quickjs/             # all platforms
│   │   │   └── jsvm/                # Harmony only
│   │   ├── vdom/                    # DomNode
│   │   ├── reconciler/              # diff algorithm
│   │   ├── layout/                  # Yoga integration
│   │   ├── bridge/                  # binary serialization + IBridge
│   │   ├── scope-manager/           # per-card JSContext + Snapshot interface
│   │   ├── commit-pipeline/         # mutation dispatch
│   │   ├── plugin-host/             # 4 registries
│   │   └── card-controller/         # platform-neutral FlexCardController logic
│   ├── components/                  # 5 built-in components (capi)
│   │   ├── text/
│   │   ├── image/
│   │   ├── view/
│   │   ├── button/
│   │   └── scrollview/
│   │     └── (each: interface header, cross-platform .cc, platform/*.cc)
│   ├── api/                         # built-in native APIs
│   │   ├── console/ timer/ log/
│   ├── plugin-a2ui/                 # A2UI frontend plugin (PoC MVP)
│   │   ├── frontend/
│   │   └── components/              # empty in PoC
│   └── platforms/                   # all platform-specific code: bridge + card host
│       ├── harmony/
│       │   ├── napi/                # NAPI binding to core
│       │   └── card/                # FlexCard ETS @Component + NodeContent mount
│       │     └── (future) page/     # reserved, README explaining direction
│       ├── android/
│       │   ├── jni/                 # PoC stub
│       │   └── card/                # FlexCardView stub
│       └── ios/                     # Phase 3+
│           ├── objcxx/
│           └── card/
│
├── playground/harmony/entry/src/main/ets/pages/
│   ├── AGenUIDemoPage.ets           # unchanged
│   ├── WaterfallDemoPage.ets        # unchanged (existing surface)
│   └── FlexCardWaterfallDemoPage.ets# new (PoC demo)
├── scripts/flex-ui/build.sh         # new
├── tests/flex-ui/                   # new, independent gtest tree
└── docs/superpowers/specs/
    └── 2026-05-30-flexui-layering-design.md
```

### 4.4 Hippy Absorption (not vendoring, not dependency)

FlexUI absorbs Hippy source code into its own namespace, with attribution. Hippy is a **design and code reference**, not a runtime dependency. After absorption, FlexUI evolves independently and does not pull from Hippy upstream.

| Hippy Source | FlexUI Destination | Treatment |
|---|---|---|
| `footstone/` | `flex-ui/common/` | Selective copy: logger, task_runner, worker, byte_buffer, string_view. Rename namespace to `flexui::common::*`. |
| `driver/js/` | `flex-ui/core/js-engine/` + `flex-ui/core/scope-manager/` | Engine/Scope class shape preserved; interface refactored to FlexUI naming. |
| `driver/napi/quickjs/` | `flex-ui/core/js-engine/quickjs/` | Absorbed as-is, namespace renamed. |
| `dom/` (DomManager/DomNode/Diff) | `flex-ui/core/vdom/` + `flex-ui/core/reconciler/` | Add frontend injection points; rename namespace. |
| `dom/taitank_layout_node.*` | (deleted) | Yoga only. |
| `serializer/` | `flex-ui/core/bridge/serialization/` | Absorbed. |
| `framework/ohos/connector/` | (not absorbed) | Replaced by FlexUI Plugin mechanism. |
| `framework/ohos/renderer/` | (not absorbed) | Replaced by FlexUI Component Layer + capi/ets-builder modes. |
| `framework/android/`, `framework/ios/` | (not absorbed in PoC) | Reference for Phase 1+ and Phase 3+. |
| `framework/js/` (React/Vue) | (not absorbed) | FlexUI exposes only the createElement atom; React-like layer is left as future plugin. |

**License compliance:** every absorbed file retains its original Apache-2.0 header plus a FlexUI modification notice. `flex-ui/NOTICE` lists Hippy attribution at top.

Estimated code volume: absorbed ~30K LOC + FlexUI self-developed ~15K LOC = PoC body ~45K LOC.

### 4.5 JS Engine: Cross-Platform Backend Matrix

| Platform | Primary | Fallback | Trigger |
|---|---|---|---|
| Android | QuickJS | — | only option |
| HarmonyOS API 11+ | JSVM | QuickJS | when JSVM unavailable or `forceQuickJsForDebug` flag |
| iOS (Phase 3+) | JavaScriptCore | QuickJS | TBD |

`flex-ui/core/js-engine/ijs_engine.h` is the cross-platform contract. The minimum surface area is sized to FlexUI's actual needs (createRuntime / createContext / eval / call / global injection / GC root management / snapshot serialize/deserialize). Cross-engine value transfers go through binary byte buffers, never through engine-specific Value types.

### 4.6 Hosting: NodeContent (HarmonyOS API 12+)

HarmonyOS hosting (`flex-ui/platforms/harmony/card/`) uses `NodeContent` + `ContentSlot`, not `XComponent`. NodeContent is designed for direct C-API node tree mounting and natively supports mixing capi components and ets-builder components in one card. The PoC accepts API 12+ as the version floor.

Android hosting (interfaces only in PoC, under `flex-ui/platforms/android/card/`): `FlexCardView extends ViewGroup`, native node tree appended via JNI.

iOS hosting (Phase 3+, under `flex-ui/platforms/ios/card/`): `FlexCardView : UIView`, node tree appended via Obj-C++ binding.

Future `FlexPage` hosting will land as a sibling `page/` module under each platform directory, sharing the same `FlexUIEngine` singleton and plugin registries.

### 4.7 Component Layer — Dual Mode

**Mode A — `capi`:** C++ directly drives ArkUI C-API (or Android View / iOS UIView). Used for all built-in components (Text, Image, View, Button, ScrollView). High performance; implementation cost high.

**Mode B — `ets-builder`** (HarmonyOS-specific extension mechanism): a third-party component registers an `@Builder` callback; the engine creates a `FrameNode` via `OH_ArkUI_NodeUtils_NewFrameNodeFromBuilder` for it. Used for components depending on ArkUI ecosystem (Lottie, Map, Video, WebView). PoC exposes the interface and unit-tests creation/destroy lifecycle, but ships no example builder component.

Both modes share the same `ComponentInstance` contract:

```cpp
class ComponentInstance {
  virtual NodeHandle OnCreate() = 0;
  virtual void OnUpdateProps(const PropDelta&) = 0;
  virtual void OnUpdateLayout(const LayoutRect&) = 0;
  virtual void OnMount(NodeHandle parent, int index) = 0;
  virtual void OnEvent(std::string name, Payload payload);
  virtual void OnUnmount() = 0;
  virtual Size Measure(const Constraints&);  // text/image custom measure
};
```

### 4.8 Mutation Pipeline

Layout is computed on the JS thread; the UI thread only commits mutations.

```
JS thread (per-Engine, single):
  scope.render(data) → newVdom
  reconciler.diff(oldVdom, newVdom) → mutationList
  layout.calculate(newVdom)  ← Yoga
  fillLayoutResults(mutationList)
  scope.setLastVdom(newVdom)
  postUiTask(mutationList)

UI thread:
  commitPipeline.apply(rootId, mutationList) {
    for each mutation: dispatch to ComponentInstance
    OH_ArkUI_NodeContent_AddNode(...) or setAttribute(...)
  }
```

VSync alignment is via `OH_NativeVSync_RequestFrame`. Multiple `setData()` calls within one microtask are coalesced into one commit.

### 4.9 Scope Management

- Engine holds one global `JSRuntime` and one `Bridge`.
- Each card gets one `Scope` (one `JSContext` in QuickJS terms, one `napi_env` analog in JSVM).
- Snapshot of standard library + built-in API stubs accelerates Scope creation. PoC does **not** implement snapshot; uses naive per-Scope injection. Snapshot is a Phase 1+ optimization.
- JS thread is single, all Scopes serialize on it (Hippy default). Mitigations for backpressure are deferred to Phase 1+.

Scope lifecycle:

```
ENGINE_INIT'D
   │ controller.load()
   ▼
SCOPE_CREATING (acquireScope → applySnapshot → injectAPIs → eval(bundle) → render(initialData))
   │
   ▼
SCOPE_RUNNING (setData / event loop)
   │                          │ uncaught fatal
   │ controller.destroy()     ▼
   ▼                       SCOPE_ERRORED → wait for destroy()
SCOPE_DESTROYING (release Context, components, remove from ScopeManager)
   ▼
SCOPE_DESTROYED
```

### 4.10 Dual Frontend — Shared Vdom Contract

The Vdom is `DomNode` (absorbed from Hippy `dom/dom_node.h`):

```cpp
class DomNode {
  uint32_t id_;
  std::string view_name_;
  uint32_t parent_id_;
  std::vector<std::shared_ptr<DomNode>> children_;
  uint32_t index_;
  PropsMap props_;
  StyleMap style_;
  EventMap events_;
  LayoutResult layout_;
  YogaNode* yoga_node_;
};
```

Frontend extension contract:

```cpp
class IFrontend {
  virtual std::string Name() = 0;          // "card-js" / "a2ui-json"
  virtual std::string BundleType() = 0;
  virtual void Initialize(ScopeRef scope, BundleSource bundle) = 0;
  virtual DomNode Render(ScopeRef scope, IJsValue data) = 0;
  virtual void HandleEvent(ScopeRef scope, NativeEvent ev);
};
```

#### Frontend #1 — `card-js` (built-in)

A bundle is a JS module exporting `render(data) → vdom` and `handlers`. Engine injects `createElement / setData / registerHandler / console / timers / native APIs` into the Scope global. `createElement` produces plain JS `{type, props, children}` trees; on Render, C++ converts to DomNode. PoC uses full re-render on every `setData()`; no fiber, no hooks. React-like wrappers are future plugins, not kernel concerns.

#### Frontend #2 — `a2ui-json` (via plugin-a2ui)

A bundle is an A2UI JSON document plus an optional `handlers.js`. Frontend parses JSON once at Initialize, then on Render fills `{{ field }}` placeholders with current data to produce DomNode. PoC supports only pure interpolation (no expressions, conditionals, or loops). Event names map to handlers registered by `handlers.js`. Streaming mode is not supported in PoC; only complete JSON is accepted.

| Behavior | card-js | a2ui-json |
|---|---|---|
| Initialize | Scope eval(bundle) | Parse JSON, optional eval(handlers.js) |
| Render | call `render(data)` | apply data binding to JSON tree |
| Event model | handler-name → handlers map | handler-name → handlers map |
| Suitable scenario | complex business logic | LLM-generated or static premade card |

### 4.11 Plugin Mechanism — 4 Extension Points

```ts
interface FlexUIPlugin {
  name: string;
  version?: string;
  components?: ComponentFactory[];  // new component types
  apis?: NativeApi[];               // new native methods callable from JS
  loaders?: BundleLoader[];         // new bundle URI schemes
  frontends?: Frontend[];           // new bundle interpretation pipelines
  onInstall?(engine: FlexUIEngine): void;
  onUninstall?(): void;
}
```

Engine `init()` auto-registers built-in `components/` and `api/`. All other capability (including `plugin-a2ui`) requires explicit `engine.install(plugin)`. Conflict detection: registering two components of the same name throws. `common/` provides shared logging, error handling, and types for all plugins.

### 4.12 Bridge Protocol

| Direction | Pattern | Implementation |
|---|---|---|
| JS → native sync API (`console.log`, `getX`) | sync | JS thread calls registered function pointer directly |
| JS → native async API (`fetch`) | async | JS thread posts task to worker pool; promise resolves back |
| JS → native vdom construction (`createElement`) | sync, hot path | in-thread DomNode construction; no cross-thread |
| native → JS event (click/scroll/touch) | async | UI thread posts to JS thread Scope dispatcher |
| native → JS async callback | async | same |

All cross-thread payloads are encoded with the binary serializer absorbed from Hippy (`serializer/`), not JSON.stringify.

### 4.13 Cross-Platform Discipline

| Concern | Cross-platform interface | Harmony impl (PoC must) | Android impl (PoC) | iOS impl (PoC) |
|---|---|---|---|---|
| JS Engine | `IJsEngine`, `IJsContext`, `IJsValue` | QuickJS + JSVM | QuickJS | (Phase 3+) |
| Bridge | `IBridge` | NAPI | JNI stub | (Phase 3+) |
| Component instance | `ComponentInstance` + per-component header | ArkUI C-API | stub returning fixed size | (Phase 3+) |
| Host | abstract controller in `core/card-controller/` | ETS `FlexCard` + NodeContent under `platforms/harmony/card/` | `FlexCardView` stub under `platforms/android/card/` | (Phase 3+) |

Each PoC component ships interface + cross-platform logic + Harmony impl + Android stub. Android stub compiles and links but logs `NOT_IMPLEMENTED` at runtime. Estimated overhead vs Harmony-only: ~20% additional code volume.

## 5. AGenUI Coexistence and Phasing

### 5.1 PoC Coexistence Discipline

- New code lives only under `flex-ui/`. No file under `core/`, `platforms/agenui/`, or `playground/harmony/.../{AGenUIDemoPage,WaterfallDemoPage}.ets` is modified.
- `playground/harmony` gets a new `FlexCardWaterfallDemoPage.ets`; the old demos remain runnable side-by-side.
- Independent CMake at `tests/flex-ui/`; existing `tests/cpp/` is untouched.
- Public symbols are `FlexUI*` and `FlexCard*`; no name collisions with `AGenUI*`.
- Independent build script `scripts/flex-ui/build.sh`; existing `scripts/harmony/build.sh` is untouched.

### 5.2 Phasing Roadmap

| Phase | Duration | Scope | Existing AGenUI state |
|---|---|---|---|
| **PoC (0)** | 10 weeks / 2 engineers | Hippy absorption + FlexUI kernel + 5 capi components + card-js frontend + plugin-a2ui MVP + Harmony demo | untouched |
| Phase 1 | 4–6 weeks | a2ui-json frontend completeness; Android Bridge JNI + component porting | untouched |
| Phase 2 | 8–12 weeks | a2ui complex components migrated to plugin-a2ui in batches; `AGenUIEngine` facade routes to `FlexCardController` internally | facade preserves old API, internals replaced |
| Phase 3 | TBD | retire legacy `core/`; iOS implementation; streaming mode reborn as plugin-a2ui feature | retired |
| Phase 4+ | TBD | `platforms/{platform}/page/` lands; FlexUI gains Page-level hosting parallel to FlexCard | n/a |

### 5.3 Phase 2 Facade Sketch

```cpp
class AGenUIEngine {
  std::unique_ptr<FlexCardController> controller_;
public:
  AGenUIEngine(EngineConfig cfg) {
    FlexUIEngine::Instance().Install(A2UIPlugin);
    controller_ = std::make_unique<FlexCardController>(
      FlexCardControllerOptions{ .bundleType = "a2ui-json",
                                 .initialData = cfg.data });
  }
  void FeedData(string a2ui_json) { controller_->LoadInline(a2ui_json); }
};
```

Streaming semantics in the Phase 2 facade are downgraded to "complete-JSON-then-commit". Real streaming returns in Phase 3 as a plugin-a2ui feature.

## 6. PoC Acceptance Criteria

### 6.1 Functional (must pass)

- [ ] HarmonyOS `FlexCard({ controller })` ETS component renders inside a host page.
- [ ] `FlexUIEngine.init / install / destroy` full lifecycle.
- [ ] QuickJS + JSVM dual backend selectable; Android compiles QuickJS only; Harmony defaults JSVM with `forceQuickJsForDebug` fallback.
- [ ] Global JSRuntime + per-card JSContext: ≥2 cards running concurrently in one process with isolated data, handlers, and exceptions.
- [ ] `card-js` frontend end-to-end: `createElement / setData / handlers / native API call`.
- [ ] `plugin-a2ui` MVP: same A2UI JSON renders pixel-identical under `a2ui-json` frontend and a hand-written `card-js` equivalent. **This is the C2 layering proof.**
- [ ] 5 `@flexui/components-base` components implemented via capi with correct Yoga layout.
- [ ] `ets-builder` mode interfaces exposed; create/destroy unit-tested; no example builder component shipped.
- [ ] Plugin 4 extension points (component / api / loader / frontend) register and uninstall; collision detection works.
- [ ] Harmony Waterfall demo: 12 cards lazy-loaded, smooth scroll.
- [ ] Coexistence verified: AGenUI demos and FlexCard demo both run in `playground/harmony` without interference.

### 6.2 Performance Baseline (recorded, not gating)

- Single-card first paint (Scope create + bundle load + first render): < 50ms (JSVM) / < 80ms (QuickJS).
- `setData → on-screen`: < 16ms (single frame).
- Scroll FPS @ 12 visible cards: ≥ 55 fps.

### 6.3 Cross-Platform Compile Gate

- Android stub compiles, links, runs (logs `NOT_IMPLEMENTED` at runtime for unimplemented surfaces). Failure to compile on Android fails the PoC, even if HarmonyOS passes.

### 6.4 Test Coverage Gate

All three test tiers (unit, integration, E2E — see §7) must be green on HarmonyOS before PoC is accepted. Specifically:

- Unit tests: ≥ 80% line coverage on `flex-ui/core/`, ≥ 70% on `flex-ui/components/`.
- Integration tests: every public API on `FlexUIEngine` and `FlexCardController` has at least one happy-path and one error-path test.
- E2E tests: the eight scenarios listed in §7.3 all pass on a real HarmonyOS device via HDC.

## 7. Test Methodology

Testing is a first-class deliverable of this PoC, not an afterthought. The framework is the foundation for future business cards, so every layer must be exercised by automated tests before phase transitions are allowed.

### 7.1 Unit Tests (C++)

- Framework: GoogleTest, hosted under `tests/flex-ui/unit/`.
- Independent CMake target `flex-ui-unit-tests`, runnable on host (macOS / Linux), no device required.
- Coverage targets:
  - `core/vdom/` — DomNode construction, prop/style/event map invariants.
  - `core/reconciler/` — diff algorithm correctness (table-driven: old tree → new tree → expected mutation list).
  - `core/layout/` — Yoga integration: known input style → expected layout result.
  - `core/scope-manager/` — Scope lifecycle state machine; concurrent Scope create/destroy.
  - `core/plugin-host/` — registration, conflict detection, uninstall.
  - `core/bridge/serialization/` — round-trip encoding.
  - `core/js-engine/quickjs/` — eval, global injection, GC root pinning.
  - `core/js-engine/jsvm/` — same surface as QuickJS (Harmony only).
  - `components/*` — props parsing, measure correctness; ArkUI calls mocked via a `NodeApiTestDouble`.
- Sanitizer matrix: ASan + UBSan by default; TSan on a curated subset (Scope manager, commit pipeline) to catch threading regressions early.

### 7.2 Integration Tests (C++)

- Hosted under `tests/flex-ui/integration/`.
- Exercise the engine through public API (`FlexUIEngine::Init`, `FlexCardController::Load/SetData/Destroy`) end-to-end on the host, with platform impl replaced by a `TestComponentBackend` that records all ArkUI calls instead of executing them.
- Must cover:
  - Engine init + plugin install + uninstall.
  - Card lifecycle (load → render → setData → destroy) for both `card-js` and `a2ui-json` frontends.
  - **Dual-frontend golden test (the C2 proof):** load the same A2UI JSON via both frontends, assert resulting DomNode trees are byte-equal and the recorded ArkUI mutation streams are identical.
  - Plugin extension: register a custom component / api / loader / frontend, verify it is reachable from JS.
  - Error paths: malformed bundle, missing component type, JS throw inside `render`, plugin name collision.

### 7.3 E2E Automation Tests (on-device, HarmonyOS)

- Framework: **pytest + hmnextauto** (uiautomator2-compatible), the project's existing HarmonyOS automation stack.
- Hosted under `tests/flex-ui/e2e/`.
- The harness is built on a `TestBase` class adapted from the dview project's proven pattern at `/Users/pingjiang/coding/gitcode/dview/tests/integration/harmonyos/test_base.py`. The file is copied into `tests/flex-ui/e2e/test_base.py` and customized for FlexUI (bundle name, page wait conditions, log tag); we do not symlink or pip-install the dview module to avoid runtime coupling. The base provides:
  - `setup_class` — connects the `hmnextauto.Driver`, wakes/unlocks the device, ensures the playground HAP is installed.
  - `setup_method` — per-test app restart and `_wait_for_page` gate.
  - Element finders: `find_text(text, timeout)`, `wait_for_text(text, timeout)`, `dump()` (UI hierarchy, with graceful fallback).
  - Interactions: `click_text(text)` and `long_click_text(text)` with hierarchy-bounds coordinate fallback when the accessibility node isn't directly clickable — important for `ArkUI_Node` C-API rendered nodes whose accessibility surface is partial.
  - `screenshot(name)` writes to `tests/flex-ui/e2e/screenshots/` with a timestamp prefix; used by visual-parity assertions.
  - `grab_log(tag, lines)` — runs `hdc shell hilog -t <tag>` to pull recent logs. FlexUI tests call `grab_log("FLEXUI", lines=500)` after every failed assertion so the per-subsystem logs from §8 land in the test report automatically.
- FlexUI-specific extensions on top of `TestBase`:
  - Class attribute `BUNDLE_URI` (FlexUI analog of dview's `CARD_FILE`) names the bundle the test loads via `FlexCardController`. The playground page reads it from launch params, identical to the existing `WaterfallDemoPage` launch-param pattern.
  - `dump_diagnostics_on_failure` pytest fixture invokes `FlexUIEngine::DumpDiagnostics()` (see §8.5) via a debug-only NAPI method when any assertion fails, and attaches the dump to the pytest report.
  - Pixel-diff helper `assert_screenshot_matches(baseline, tolerance=1)` for visual-parity scenario #4 below.
- Run target: real HarmonyOS device or emulator connected via HDC. The pytest `device` marker (registered in `tests/flex-ui/e2e/conftest.py`, modeled after dview's `conftest.py`) gates these tests so they skip cleanly on host-only CI.
- **PoC must include these eight scenarios** (failure of any blocks PoC acceptance):
  1. `test_engine_init_and_destroy` — cold start, engine init, install plugin-a2ui, destroy engine; `grab_log` for `FLEXUI/Engine/*` shows matched Init/Destroy pairs; no `FLEXUI/.../Leak` lines.
  2. `test_card_js_first_paint` — launch `FlexCardWaterfallDemoPage` with a `card-js` bundle; assert visible content via `wait_for_text` ≤ 80 ms after `controller.load()` resolves.
  3. `test_a2ui_json_first_paint` — same scenario with an `a2ui-json` bundle.
  4. `test_dual_frontend_visual_parity` — render the same A2UI JSON via both frontends in adjacent cards on one page; `screenshot()` each and assert pixel diff per channel ≤ 1.
  5. `test_setdata_updates_view` — call `controller.setData(...)` from native side; `wait_for_text` confirms the new text appears within one frame.
  6. `test_event_round_trip` — `click_text("+1")` on a Button card; assert the bound JS handler executes and triggers a `setData` reflected in the visible count.
  7. `test_waterfall_scroll_smoothness` — scroll the 12-card waterfall through 5 viewports; sample frame timing via HiLog `FLEXUI/CommitPipeline/Apply` durations; assert no frame > 32 ms.
  8. `test_card_isolation` — two cards in different bundles/data; `setData` on card A must not change card B's visible DOM, and `grab_log` shows the two Scopes' events do not interleave incorrectly.
- All eight scenarios sub-class the FlexUI `TestBase` (no test directly instantiates `Driver`). Each test on failure auto-attaches: HiLog tail filtered by `FLEXUI`, the last `DumpDiagnostics()` snapshot, and a final-state screenshot.

### 7.4 Coexistence Smoke Test

- A dedicated playground page mounts both `AGenUIContainer` (legacy surface) and `FlexCard` (new) on screen simultaneously, with independent data. The E2E suite includes `test_coexistence_no_interference` asserting both render and both respond to taps.

## 8. Logging and Observability

The PoC is a foundation layer for all future business cards. When something is wrong, the answer must be in the log — not in a debugger. Logging is therefore a hard requirement, not optional.

### 8.1 Principles

- **Every architectural boundary crossing logs at DEBUG.** That includes: every `FlexUIEngine` / `FlexCardController` public API entry and exit, every plugin install/uninstall, every Scope state transition, every mutation batch commit, every JS↔native call, every NodeContent mount/unmount.
- **Every error logs at ERROR with full context.** Stack trace, Scope id, root id, last mutation id, last JS function name.
- **Every WARN explains what is degraded and why** (e.g. JSVM unavailable → falling back to QuickJS).
- **Logs must be greppable.** Use stable string prefixes per subsystem, never localized text.

### 8.2 Log Tag Convention

All logs go through `flex-ui/common/log/`, which wraps HiLog on HarmonyOS, `__android_log_print` on Android, `os_log` on iOS (Phase 3+).

Tag format: `FLEXUI/<subsystem>/<event>`. Examples:

- `FLEXUI/Engine/Init`
- `FLEXUI/Engine/InstallPlugin`
- `FLEXUI/Scope/Create`
- `FLEXUI/Scope/StateChange` (with from/to states)
- `FLEXUI/Bridge/JsToNative`
- `FLEXUI/Bridge/NativeToJs`
- `FLEXUI/Frontend/CardJs/Render`
- `FLEXUI/Frontend/A2uiJson/Bind`
- `FLEXUI/Reconciler/Diff` (with mutation count)
- `FLEXUI/Layout/Calculate` (with node count, duration)
- `FLEXUI/CommitPipeline/Apply` (with mutation count, duration)
- `FLEXUI/Component/<Name>/Create|Update|Destroy`
- `FLEXUI/NodeContent/AddNode|RemoveNode`

### 8.3 Log Levels

| Level | Used for | Default in build |
|---|---|---|
| ERROR | Unrecoverable failure, exception caught at boundary | always on |
| WARN | Degraded path, fallback taken | always on |
| INFO | Major lifecycle events (engine init, card load, card destroy) | always on |
| DEBUG | Every cross-layer call; mutation list summaries; layout durations | on by default in PoC; gated by `FLEXUI_DEBUG_LOG` build flag in release |
| VERBOSE | Per-mutation, per-prop, per-JS-call dumps | off by default; enabled via runtime flag for targeted debugging |

The PoC defaults to **DEBUG on** so on-device captures yield maximum diagnostic value. Release builds (Phase 1+) compile DEBUG out unless `FLEXUI_DEBUG_LOG=ON`.

### 8.4 Mandatory Log Coverage Points

These must be present in PoC code; missing any of them is a PR-blocker:

| Subsystem | Required log points |
|---|---|
| `FlexUIEngine` | Init enter/exit (with config), install/uninstall (with plugin name), destroy enter/exit |
| `FlexCardController` | constructor, load enter/exit/error, setData (with diff stats), callMethod, onEvent fired, destroy enter/exit |
| `ScopeManager` | Scope create (with id, snapshot used?), inject API, eval bundle (size, duration), state transitions, destroy |
| `Reconciler` | diff enter (root id, old/new node counts), exit (mutation count, duration) |
| `Layout` | calculate enter (root id, constraints), exit (duration, dirty count) |
| `CommitPipeline` | apply enter (root id, mutation count), per-mutation kind + node id at VERBOSE, exit |
| `Bridge` | every JS→native call (module, method, arg byte size), every native→JS event (scope id, node id, event type) |
| `Component` | every Create/Update/Destroy with node id + view name |
| `Plugin` | install (name, extension counts), uninstall, conflict detection |
| `Frontend` | initialize (bundle type, size), render (input data size, output node count, duration) |
| `JS engine` | uncaught exception, GC pressure events, snapshot serialize/deserialize duration |

### 8.5 Structured Diagnostic Dumps

`FlexUIEngine` exposes a `DumpDiagnostics()` method (debug build only) that synchronously emits:

- Engine config + installed plugin list.
- All active Scope ids, root ids, state, last commit timestamp.
- Last 16 mutation batches per Scope (ring buffer).
- Last 16 JS errors per Scope.

This is invoked at the start of every E2E test (§7.3) and on uncaught error. The dump is the single artifact a triager opens first.

### 8.6 Cross-Platform Logging Discipline

The `flex-ui/common/log/` abstraction takes a tag, level, and format string; platform impl chooses the sink. **No subsystem code uses HiLog / Android Log / NSLog directly.** This keeps the log surface uniform across platforms and prevents accidental log-level drift.

## 9. Risk Register

| # | Risk | Severity | Mitigation |
|---|---|---|---|
| 1 | Hippy absorption pulls in unintended dependencies | High | Pre-define absorption manifest (§4.4); reject anything not on the list. |
| 2 | JSVM API instability across HarmonyOS versions | Medium-High | Default QuickJS; JSVM activated only when API-level check passes. |
| 3 | NodeContent + FrameNode require API 12+ | Medium | PoC locks API 12+. XComponent fallback is a 2–3 week rework if business needs lower floor. |
| 4 | JS-thread serialization congests with many concurrent first-paints | Medium | Record baseline in PoC; Phase 1+ explores main-thread first-paint + async patch. |
| 5 | Dual-frontend rendering divergence | Medium | Golden tests: same JSON → both frontends → DomNode byte-equal. |
| 6 | Plugin install invalidates Snapshot, slows init | Medium | PoC skips Snapshot entirely; Phase 1+ optimizes. |
| 7 | AGenUI users may resist migration | Medium | Phase 2 facade preserves old API; PoC parallel coexistence demonstrates equivalence. |
| 8 | ets-builder ↔ NodeContent compatibility / perf | Low-Medium | PoC validates interface only; full validation in Phase 1+. |
| 9 | Cross-platform interface design ossifies prematurely | Medium | Write Android stub during PoC, not after — if stub can't compile, interface is wrong. |

## 10. Glossary

| Term | Meaning |
|---|---|
| **FlexUI** | The framework (kernel + plugin host + component layer + APIs). |
| **FlexCard** | The card-hosting form of FlexUI under `platforms/{platform}/card/`. |
| **FlexPage** | Future page-hosting form of FlexUI under `platforms/{platform}/page/`, Phase 4+. |
| **FlexUIEngine** | Singleton entry point; holds runtime, bridge, plugin registries. |
| **FlexUIPlugin** | Bundle of component / api / loader / frontend registrations. |
| **FlexCardController** | Per-card instance; owns one Scope and one root DomNode tree. |
| **Scope** | Per-card JS execution context (QuickJS JSContext or JSVM analog). |
| **Vdom (DomNode)** | Shared tree contract produced by any frontend, consumed by reconciler. |
| **Mutation** | Atomic operation emitted by diff: Create / Update / Move / Delete / UpdateLayout. |
| **Frontend** | Plugin extension that turns a bundle into a DomNode tree. |
| **Component Layer** | The set of `ComponentInstance` factories registered to the engine. |
| **Host** | The platform-specific mounting surface (FlexCard / FlexPage), located under `platforms/{platform}/`. |
| **C2** | The chosen PoC validation target — "layering supports both FlexCard and AGenUI in one stack". |

---

*End of spec.*
