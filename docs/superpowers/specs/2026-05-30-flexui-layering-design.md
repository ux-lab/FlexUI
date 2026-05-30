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
│ Host Layer (flex-ui/host/)                                           │
│   • host/card/  : FlexCardController + FlexCard ETS component        │
│   • host/page/  : (reserved for future Page-level hosting)           │
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
│   │   └── plugin-host/             # 4 registries
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
│   ├── host/
│   │   ├── card/                    # PoC: FlexCardController + FlexCard @Component
│   │   │   ├── flex_card_controller.{h,cc}
│   │   │   └── platforms/
│   │   │       ├── harmony/ets/
│   │   │       ├── android/kotlin/  # PoC stub
│   │   │       └── ios/swift/       # Phase 3+
│   │   └── page/                    # reserved, README explaining future direction
│   └── platforms/                   # cross-language bridge implementations
│       ├── harmony/napi/
│       ├── android/jni/             # PoC stub
│       └── ios/objcxx/              # Phase 3+
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

HarmonyOS hosting uses `NodeContent` + `ContentSlot`, not `XComponent`. NodeContent is designed for direct C-API node tree mounting and natively supports mixing capi components and ets-builder components in one card. The PoC accepts API 12+ as the version floor.

Android hosting (interfaces only in PoC): `FlexCardView extends ViewGroup`, native node tree appended via JNI.

iOS hosting (Phase 3+): `FlexCardView : UIView`, node tree appended via Obj-C++ binding.

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
| Host | host/card abstract controller | ETS `FlexCard` + NodeContent | `FlexCardView` stub | (Phase 3+) |

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
| Phase 4+ | TBD | `host/page/` lands; FlexUI gains Page-level hosting parallel to FlexCard | n/a |

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

## 7. Risk Register

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

## 8. Glossary

| Term | Meaning |
|---|---|
| **FlexUI** | The framework (kernel + plugin host + component layer + APIs). |
| **FlexCard** | The card-hosting form of FlexUI (`host/card/`). |
| **FlexPage** | Future page-hosting form of FlexUI (`host/page/`), Phase 4+. |
| **FlexUIEngine** | Singleton entry point; holds runtime, bridge, plugin registries. |
| **FlexUIPlugin** | Bundle of component / api / loader / frontend registrations. |
| **FlexCardController** | Per-card instance; owns one Scope and one root DomNode tree. |
| **Scope** | Per-card JS execution context (QuickJS JSContext or JSVM analog). |
| **Vdom (DomNode)** | Shared tree contract produced by any frontend, consumed by reconciler. |
| **Mutation** | Atomic operation emitted by diff: Create / Update / Move / Delete / UpdateLayout. |
| **Frontend** | Plugin extension that turns a bundle into a DomNode tree. |
| **Component Layer** | The set of `ComponentInstance` factories registered to the engine. |
| **Host** | The platform-specific mounting surface (FlexCard / FlexPage). |
| **C2** | The chosen PoC validation target — "layering supports both FlexCard and AGenUI in one stack". |

---

*End of spec.*
