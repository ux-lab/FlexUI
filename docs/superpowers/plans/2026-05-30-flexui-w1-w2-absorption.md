# FlexUI W1-W2 — Hippy Footstone Absorption Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stand up `flex-ui/common/` containing Hippy's `footstone` module absorbed into the FlexUI namespace, with FlexUI-specific log tag conventions, a working CMake build, and an independent gtest suite green on host (macOS) with ASan+UBSan.

**Architecture:** "Absorb, don't vendor" — copy each footstone source file into `flex-ui/common/`, rename `namespace footstone` → `namespace flexui::common`, rename includes `footstone/X.h` → `flexui/common/X.h`, preserve original Apache-2.0 + Tencent copyright headers, add a FlexUI modification notice and a `flex-ui/NOTICE` attribution file. On top of the absorbed log layer, add a FlexUI-specific `FLEXUI/<subsystem>/<event>` structured tag macro and a `LogSink` abstraction with HarmonyOS HiLog and Android stub backends.

**Tech Stack:** C++17, CMake 3.18+, GoogleTest 1.14+, AddressSanitizer + UndefinedBehaviorSanitizer (default), ThreadSanitizer (curated subset target), Hippy footstone sources at `/Users/pingjiang/coding/github/Hippy/modules/footstone/`.

**Reference spec:** `docs/superpowers/specs/2026-05-30-flexui-layering-design.md` §4.3, §4.4 (Hippy absorption row "footstone"), §8 (logging conventions), §6.4 (test coverage gate), §7.1 (unit test methodology).

**Out of scope (deferred to later plans):**
- JS engine (`flex-ui/core/js-engine/`) — W3-W4
- Vdom / reconciler / layout — W3-W4
- Bridge / scope-manager / commit-pipeline / plugin-host — W5-W6
- Components / APIs / frontends — W7-W8
- Playground / E2E / perf — W9-W10
- Android JNI implementation (HarmonyOS-only and host-only in W1-W2; Android log sink is a stub)
- iOS

**Deliverable definition of done:**
- `flex-ui/common/` library compiles as a static library on host (macOS) and emits `libflexui_common.a`.
- `tests/flex-ui/unit/` GoogleTest suite green under `ctest`, with ASan+UBSan enabled by default.
- Line coverage on `flex-ui/common/` ≥ 80% (measured via `llvm-cov` / `lcov`).
- TSan-only sub-target green on `task_runner`, `worker`, `worker_manager`, `idle_timer`.
- `flex-ui/NOTICE` lists Hippy footstone Apache-2.0 attribution and original copyright.
- `scripts/flex-ui/build.sh` runs configure → build → tests on macOS end-to-end with non-zero exit on any failure.
- `FLEXUI_LOG(subsystem, event, level)` structured macro is available, emits to a pluggable `LogSink`, and prints `[FLEXUI/<subsystem>/<event>][<level>] message` on the default stdout sink.
- Existing AGenUI tree (`core/`, `platforms/`, `tests/cpp/`, `playground/`, `scripts/harmony/`, `scripts/android/`, `scripts/ios/`) is untouched (verifiable via `git diff master -- core/ platforms/ tests/cpp/ playground/ scripts/harmony/ scripts/android/ scripts/ios/` showing no changes).

---

## File Structure

### Created in this plan

```
flex-ui/
├── NOTICE                                     # Hippy Apache-2.0 attribution
├── README.md                                  # one-paragraph + pointer to spec
├── CMakeLists.txt                             # top-level subdir aggregator
└── common/
    ├── CMakeLists.txt                         # builds libflexui_common.a
    ├── include/flexui/common/
    │   ├── macros.h                           # absorbed
    │   ├── check.h                            # absorbed
    │   ├── log_level.h                        # absorbed
    │   ├── time_delta.h                       # absorbed
    │   ├── time_point.h                       # absorbed
    │   ├── base_time.h                        # absorbed
    │   ├── string_view.h                      # absorbed (namespace flexui::common)
    │   ├── string_view_utils.h                # absorbed
    │   ├── string_utils.h                     # absorbed
    │   ├── logging.h                          # absorbed (footstone macros renamed FLEXUI_*)
    │   ├── log_settings.h                     # absorbed
    │   ├── task.h                             # absorbed
    │   ├── task_runner.h                      # absorbed
    │   ├── worker.h                           # absorbed
    │   ├── worker_impl.h                      # absorbed
    │   ├── worker_manager.h                   # absorbed
    │   ├── cv_driver.h                        # absorbed
    │   ├── driver.h                           # absorbed
    │   ├── idle_task.h                        # absorbed
    │   ├── idle_timer.h                       # absorbed
    │   ├── base_timer.h                       # absorbed
    │   ├── one_shot_timer.h                   # absorbed
    │   ├── repeating_timer.h                  # absorbed
    │   ├── persistent_object_map.h            # absorbed
    │   ├── hash.h                             # absorbed
    │   ├── flexui_value.h                     # absorbed (renamed from hippy_value.h)
    │   ├── serializer.h                       # absorbed
    │   ├── deserializer.h                     # absorbed
    │   ├── log_tag.h                          # FlexUI-original: FLEXUI_LOG macro layer
    │   ├── log_sink.h                         # FlexUI-original: pluggable sink interface
    │   └── error.h                            # FlexUI-original: unified error type
    └── src/
        ├── string_view.cc                     # absorbed
        ├── string_utils.cc                    # absorbed
        ├── log_settings.cc                    # absorbed
        ├── log_settings_state.cc              # absorbed
        ├── task.cc                            # absorbed
        ├── task_runner.cc                     # absorbed
        ├── worker.cc                          # absorbed
        ├── worker_manager.cc                  # absorbed
        ├── cv_driver.cc                       # absorbed
        ├── idle_task.cc                       # absorbed
        ├── idle_timer.cc                      # absorbed
        ├── base_timer.cc                      # absorbed
        ├── one_shot_timer.cc                  # absorbed
        ├── repeating_timer.cc                 # absorbed
        ├── flexui_value.cc                    # absorbed (renamed from hippy_value.cc)
        ├── serializer.cc                      # absorbed
        ├── deserializer.cc                    # absorbed
        ├── log_tag.cc                         # FlexUI-original
        ├── log_sink.cc                        # FlexUI-original (default stdout sink)
        └── platform/
            ├── log_sink_harmony.cc            # FlexUI-original (HiLog backend)
            └── log_sink_android.cc            # FlexUI-original (stub)

tests/flex-ui/
├── CMakeLists.txt                             # top-level test driver, mirrors tests/cpp/CMakeLists.txt
├── README.md
├── support/
│   ├── capturing_log_sink.h                   # test helper: collects log entries in memory
│   └── capturing_log_sink.cc
└── unit/
    ├── CMakeLists.txt
    ├── common_smoke_test.cc                   # earliest gate
    ├── string_view_test.cc
    ├── string_utils_test.cc
    ├── logging_test.cc                        # exercises footstone-level macro
    ├── log_tag_test.cc                        # exercises FLEXUI_LOG
    ├── log_sink_test.cc
    ├── task_runner_test.cc
    ├── worker_test.cc
    ├── worker_manager_test.cc
    ├── timer_test.cc                          # covers all four timers
    ├── persistent_object_map_test.cc
    ├── flexui_value_test.cc
    ├── serializer_roundtrip_test.cc
    └── error_test.cc

scripts/flex-ui/
└── build.sh                                   # configure + build + ctest, host-only
```

### Untouched (verified by grep + git diff)

- `core/`, `platforms/`, `tests/cpp/`, `playground/`, `scripts/harmony/`, `scripts/android/`, `scripts/ios/`, `agent_sdks/`, `samples/`, `skills/`, root `CMakeLists.txt` if any, root `.gitignore` (we add a top-level `flex-ui/build/` entry but only if absent — checked in Task 1).

---

## Common Workflow for "Absorb a Footstone File"

Most tasks below follow this loop. Variations are called out explicitly per-task.

1. Copy source from Hippy → FlexUI.
2. Edit copied file:
   - Keep original Apache-2.0 + Tencent copyright header intact.
   - Append a FlexUI modification notice block immediately below the original header (see Task 1 template).
   - Replace `namespace footstone {` → `namespace flexui::common {` and matching `}  // namespace footstone` → `}  // namespace flexui::common`. (`inline namespace log` and other nested namespaces stay.)
   - Replace every `#include "footstone/X.h"` → `#include "flexui/common/X.h"`.
   - Replace `FOOTSTONE_` macro prefix → `FLEXUI_` (this affects logging.h, macros.h, check.h consumers).
3. Add new test in `tests/flex-ui/unit/` that exercises one observable behavior using the new namespace + include path.
4. Run the test (fails — file not yet wired into CMake or namespace renames incomplete).
5. Add file to `flex-ui/common/CMakeLists.txt` source list if it has a `.cc`.
6. Run the test (passes).
7. Commit.

---

## Tasks

### Task 1: Worktree, skeleton, NOTICE, attribution template

**Files:**
- Create: `flex-ui/NOTICE`
- Create: `flex-ui/README.md`
- Create: `flex-ui/CMakeLists.txt`
- Create: `flex-ui/common/CMakeLists.txt`
- Create: `flex-ui/common/include/flexui/common/.gitkeep`
- Create: `flex-ui/common/src/.gitkeep`
- Modify: root `.gitignore` (add `flex-ui/build/` if absent)

- [ ] **Step 1.1: Create worktree (optional, if executor wants isolation)**

Run from the FlexUI repo root:

```bash
git worktree add ../FlexUI-w1w2-absorption -b feat/flexui-w1-w2-absorption master
cd ../FlexUI-w1w2-absorption
```

If the executor prefers to work directly on `master` or a topic branch in-place, skip this step. The plan does not depend on a worktree.

- [ ] **Step 1.2: Create the directory skeleton**

```bash
mkdir -p flex-ui/common/include/flexui/common
mkdir -p flex-ui/common/src/platform
mkdir -p tests/flex-ui/unit
mkdir -p tests/flex-ui/support
mkdir -p scripts/flex-ui
touch flex-ui/common/include/flexui/common/.gitkeep
touch flex-ui/common/src/.gitkeep
```

- [ ] **Step 1.3: Write `flex-ui/NOTICE`**

```
FlexUI
Copyright (c) 2026 the FlexUI authors.

This product includes software developed by Tencent (Hippy framework,
https://github.com/Tencent/Hippy) and made available under the
Apache License, Version 2.0.

The following files in flex-ui/common/ are derived from Hippy's
modules/footstone/ subtree (commit-pinned at integration time; see
docs/superpowers/specs/2026-05-30-flexui-layering-design.md §4.4):

  macros.h, check.h, log_level.h, time_delta.h, time_point.h, base_time.h,
  string_view.h, string_view_utils.h, string_utils.h,
  logging.h, log_settings.h, log_settings.cc, log_settings_state.cc,
  task.h, task.cc, task_runner.h, task_runner.cc,
  worker.h, worker.cc, worker_impl.h, worker_manager.h, worker_manager.cc,
  cv_driver.h, cv_driver.cc, driver.h,
  idle_task.h, idle_task.cc, idle_timer.h, idle_timer.cc,
  base_timer.h, base_timer.cc, one_shot_timer.h, one_shot_timer.cc,
  repeating_timer.h, repeating_timer.cc,
  persistent_object_map.h, hash.h,
  flexui_value.h, flexui_value.cc (renamed from hippy_value.{h,cc}),
  serializer.h, serializer.cc, deserializer.h, deserializer.cc,
  string_view.cc, string_utils.cc.

Each derived file retains its original Apache-2.0 license header and
THL A29 Limited copyright. Modifications by the FlexUI authors are
indicated by a "Modified by the FlexUI authors" block immediately below
the original header.
```

- [ ] **Step 1.4: Write `flex-ui/README.md`**

```markdown
# flex-ui

FlexUI framework body. See `docs/superpowers/specs/2026-05-30-flexui-layering-design.md`
for the design.

This directory is independent of the legacy AGenUI tree at `core/` +
`platforms/agenui/`; see §5.1 of the spec for the coexistence discipline.

Sub-modules (added incrementally over W1-W10):

| Module      | Status     | Plan |
|-------------|------------|------|
| `common/`   | W1-W2      | docs/superpowers/plans/2026-05-30-flexui-w1-w2-absorption.md |
| `core/`     | W3-W6      | (TBD) |
| `components/` | W7-W8    | (TBD) |
| `api/`      | W7-W8      | (TBD) |
| `plugin-a2ui/` | W7-W8   | (TBD) |
| `platforms/` | W5-W10    | (TBD) |
```

- [ ] **Step 1.5: Write `flex-ui/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.18)
project(flex_ui CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(common)
```

- [ ] **Step 1.6: Write minimal `flex-ui/common/CMakeLists.txt`** (sources added incrementally per later tasks)

```cmake
add_library(flexui_common STATIC)
target_include_directories(flexui_common
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_sources(flexui_common PRIVATE
  # Sources appended incrementally by later tasks in the W1-W2 absorption plan.
)
# Force at least one TU so the static lib produces an archive even when
# every source so far is header-only.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_common PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
```

- [ ] **Step 1.7: Update root `.gitignore`** (only if `flex-ui/build/` is not already covered)

Inspect first:

```bash
grep -nE "^flex-ui/build|^build/" .gitignore || echo "MISSING"
```

If `MISSING`, append to `.gitignore`:

```
flex-ui/build/
tests/flex-ui/build/
```

- [ ] **Step 1.8: Commit**

```bash
git add flex-ui/ .gitignore
git commit -m "feat(flex-ui): scaffold flex-ui/ with NOTICE, README, CMake skeleton"
```

---

### Task 2: Footstone-modification header template

The same 14-line attribution block appears at the top of every absorbed file. Codify it as a constant in the plan so engineers don't re-invent or drift.

**Template (paste below the original Apache-2.0 + Tencent copyright header, blank line in between):**

```cpp
/*
 * Modified by the FlexUI authors. This file is derived from
 * modules/footstone/<original-path> in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone` -> `flexui::common`.
 *   - Renamed include path `footstone/...` -> `flexui/common/...`.
 *   - Renamed macros `FOOTSTONE_*` -> `FLEXUI_*` (where present).
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */
```

For files with FlexUI-specific behavioral changes beyond the rename (none in W1-W2), the bullet list above is replaced with the actual change list.

This task has no file edits — it is the contract the executor follows in every Task 4+ below.

- [ ] **Step 2.1: Mark this task complete in the checklist** (no further action; just acknowledge the contract).

---

### Task 3: Test harness root + smoke test gate

**Files:**
- Create: `tests/flex-ui/CMakeLists.txt`
- Create: `tests/flex-ui/README.md`
- Create: `tests/flex-ui/unit/CMakeLists.txt`
- Create: `tests/flex-ui/unit/common_smoke_test.cc`
- Create: `scripts/flex-ui/build.sh`

- [ ] **Step 3.1: Write `tests/flex-ui/CMakeLists.txt`**

```cmake
# =============================================================================
# FlexUI C++ Test Suite (independent of tests/cpp/ for AGenUI)
# =============================================================================
cmake_minimum_required(VERSION 3.18)
project(flexui_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FLEXUI_TESTS_ENABLE_ASAN "Enable AddressSanitizer + UBSan" ON)
option(FLEXUI_TESTS_ENABLE_TSAN "Enable ThreadSanitizer (mutex with ASan)" OFF)
option(FLEXUI_TESTS_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if(FLEXUI_TESTS_ENABLE_ASAN AND FLEXUI_TESTS_ENABLE_TSAN)
  message(FATAL_ERROR "ASan and TSan are mutually exclusive; pick one.")
endif()

set(FLEXUI_TESTS_SAN_FLAGS "")
set(FLEXUI_TESTS_SAN_LINK_FLAGS "")
if(FLEXUI_TESTS_ENABLE_ASAN)
  list(APPEND FLEXUI_TESTS_SAN_FLAGS -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
  list(APPEND FLEXUI_TESTS_SAN_LINK_FLAGS -fsanitize=address -fsanitize=undefined)
endif()
if(FLEXUI_TESTS_ENABLE_TSAN)
  list(APPEND FLEXUI_TESTS_SAN_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
  list(APPEND FLEXUI_TESTS_SAN_LINK_FLAGS -fsanitize=thread)
endif()
if(FLEXUI_TESTS_ENABLE_COVERAGE)
  list(APPEND FLEXUI_TESTS_SAN_FLAGS --coverage -O0 -g)
  list(APPEND FLEXUI_TESTS_SAN_LINK_FLAGS --coverage)
endif()

add_compile_options(${FLEXUI_TESTS_SAN_FLAGS})
add_link_options(${FLEXUI_TESTS_SAN_LINK_FLAGS})

# Pull in flex-ui as a subdirectory build.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../flex-ui ${CMAKE_CURRENT_BINARY_DIR}/flex-ui-build)

# GoogleTest via FetchContent.
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(unit)
```

- [ ] **Step 3.2: Write `tests/flex-ui/unit/CMakeLists.txt`**

```cmake
add_executable(flexui_unit_tests
  common_smoke_test.cc
)
target_link_libraries(flexui_unit_tests PRIVATE
  flexui_common
  GTest::gtest
  GTest::gtest_main
)
add_test(NAME flexui_unit_tests COMMAND flexui_unit_tests)
```

- [ ] **Step 3.3: Write `tests/flex-ui/unit/common_smoke_test.cc`**

```cpp
// Earliest gate: prove the flex-ui/common static library links and we can
// instantiate a TU that references no flex-ui symbols yet. Replaced/expanded
// by later tasks.

#include <gtest/gtest.h>

TEST(FlexUICommonSmoke, LinksAndRuns) {
  EXPECT_EQ(1 + 1, 2);
}
```

- [ ] **Step 3.4: Write `scripts/flex-ui/build.sh`**

```bash
#!/usr/bin/env bash
# FlexUI host build + test runner.
# Default: ASan + UBSan. Pass --tsan-only to flip to TSan.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT}/tests/flex-ui/build"
MODE="asan"

for arg in "$@"; do
  case "$arg" in
    --tsan-only) MODE="tsan" ;;
    --no-san)    MODE="none" ;;
    --coverage)  MODE="coverage" ;;
    *)           echo "Unknown arg: $arg"; exit 2 ;;
  esac
done

CMAKE_FLAGS=()
case "$MODE" in
  asan)     CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=ON  -DFLEXUI_TESTS_ENABLE_TSAN=OFF) ;;
  tsan)     CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=OFF -DFLEXUI_TESTS_ENABLE_TSAN=ON ) ;;
  none)     CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=OFF -DFLEXUI_TESTS_ENABLE_TSAN=OFF) ;;
  coverage) CMAKE_FLAGS+=(-DFLEXUI_TESTS_ENABLE_ASAN=OFF -DFLEXUI_TESTS_ENABLE_TSAN=OFF -DFLEXUI_TESTS_ENABLE_COVERAGE=ON) ;;
esac

mkdir -p "$BUILD_DIR"
cmake -S "${ROOT}/tests/flex-ui" -B "$BUILD_DIR" "${CMAKE_FLAGS[@]}"
cmake --build "$BUILD_DIR" -j
ctest --test-dir "$BUILD_DIR" --output-on-failure
```

```bash
chmod +x scripts/flex-ui/build.sh
```

- [ ] **Step 3.5: Write `tests/flex-ui/README.md`**

```markdown
# tests/flex-ui

Test tree for the FlexUI framework body (`flex-ui/`). Independent of
`tests/cpp/` (the AGenUI tree).

Run:
  ./scripts/flex-ui/build.sh                # ASan + UBSan
  ./scripts/flex-ui/build.sh --tsan-only    # TSan
  ./scripts/flex-ui/build.sh --no-san       # No sanitizers
  ./scripts/flex-ui/build.sh --coverage     # gcov / llvm-cov instrumentation
```

- [ ] **Step 3.6: Verify the smoke test passes**

```bash
./scripts/flex-ui/build.sh
```

Expected last lines:
```
Test #1: flexui_unit_tests
1/1 Test #1: flexui_unit_tests .................   Passed
100% tests passed, 0 tests failed out of 1
```

- [ ] **Step 3.7: Commit**

```bash
git add tests/flex-ui scripts/flex-ui
git commit -m "feat(flex-ui): add gtest harness + scripts/flex-ui/build.sh, smoke green"
```

---

### Task 4: Absorb header-only base primitives (macros, check, log_level, time)

These have no `.cc` files and few dependencies. Absorb them in a single task.

**Files:**
- Create (copy + rewrite): `flex-ui/common/include/flexui/common/macros.h`
- Create (copy + rewrite): `flex-ui/common/include/flexui/common/check.h`
- Create (copy + rewrite): `flex-ui/common/include/flexui/common/log_level.h`
- Create (copy + rewrite): `flex-ui/common/include/flexui/common/time_delta.h`
- Create (copy + rewrite): `flex-ui/common/include/flexui/common/time_point.h`
- Create (copy + rewrite): `flex-ui/common/include/flexui/common/base_time.h`
- Create: `tests/flex-ui/unit/base_primitives_test.cc`
- Modify: `tests/flex-ui/unit/CMakeLists.txt` (append `base_primitives_test.cc`)

- [ ] **Step 4.1: Copy headers**

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone/include/footstone
HDEST=flex-ui/common/include/flexui/common
for f in macros.h check.h log_level.h time_delta.h time_point.h base_time.h; do
  cp "$HSRC/$f" "$HDEST/$f"
done
```

- [ ] **Step 4.2: Apply mechanical renames to each header**

For each of the six headers:

```bash
for f in macros.h check.h log_level.h time_delta.h time_point.h base_time.h; do
  python3 - <<PY
import re, sys, pathlib
p = pathlib.Path("flex-ui/common/include/flexui/common/$f")
s = p.read_text()
s = s.replace('namespace footstone {', 'namespace flexui::common {')
s = s.replace('}  // namespace footstone', '}  // namespace flexui::common')
s = re.sub(r'#include\s+"footstone/', '#include "flexui/common/', s)
s = re.sub(r'\bFOOTSTONE_', 'FLEXUI_', s)
p.write_text(s)
PY
done
```

- [ ] **Step 4.3: Insert the FlexUI modification notice into each header**

For each file, immediately after the original `*/` that closes the Tencent copyright (before the blank line that precedes `#pragma once`), insert the modification block from Task 2.

A scripted insertion (find the first `*/` line and insert below):

```bash
NOTICE='/*
 * Modified by the FlexUI authors. This file is derived from
 * modules/footstone/include/footstone/'$f' in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Moved namespace `footstone` -> `flexui::common`.
 *   - Renamed include path `footstone/...` -> `flexui/common/...`.
 *   - Renamed macros `FOOTSTONE_*` -> `FLEXUI_*` (where present).
 *   - No behavioral change.
 *
 * The original Apache-2.0 license terms above continue to apply.
 */
'
```

Use `awk` or hand-edit per file. Verify each file has both the original copyright and the FlexUI notice intact via:

```bash
grep -l "Tencent is pleased" flex-ui/common/include/flexui/common/*.h | wc -l   # expect: 6
grep -l "Modified by the FlexUI authors" flex-ui/common/include/flexui/common/*.h | wc -l   # expect: 6
```

- [ ] **Step 4.4: Write `tests/flex-ui/unit/base_primitives_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/common/macros.h"
#include "flexui/common/check.h"
#include "flexui/common/log_level.h"
#include "flexui/common/time_delta.h"
#include "flexui/common/time_point.h"
#include "flexui/common/base_time.h"

namespace flexui::common {

TEST(BasePrimitivesTest, NamespaceResolves) {
  // Pure compile-time test: if any of the included headers still declares
  // `namespace footstone {`, this TU won't compile because the type lookup
  // inside `flexui::common` would fail.
  TimeDelta d = TimeDelta::FromMilliseconds(5);
  EXPECT_EQ(d.ToMilliseconds(), 5);
}

TEST(BasePrimitivesTest, TimePointAddDelta) {
  TimePoint t = TimePoint::Now();
  TimePoint t2 = t + TimeDelta::FromMilliseconds(10);
  EXPECT_GT(t2.ToEpochDelta().ToMilliseconds(), t.ToEpochDelta().ToMilliseconds());
}

}  // namespace flexui::common
```

- [ ] **Step 4.5: Wire the test into `tests/flex-ui/unit/CMakeLists.txt`**

```cmake
add_executable(flexui_unit_tests
  common_smoke_test.cc
  base_primitives_test.cc
)
```

- [ ] **Step 4.6: Run tests**

```bash
./scripts/flex-ui/build.sh
```

Expected: both `FlexUICommonSmoke` and `BasePrimitivesTest` pass under ASan+UBSan.

If a TimePoint/TimeDelta API in the absorbed sources differs from what the test assumes (e.g. method renamed in Hippy), patch the test to use the actual API — do **not** rename APIs in the absorbed files.

- [ ] **Step 4.7: Commit**

```bash
git add flex-ui/common/include/flexui/common/{macros,check,log_level,time_delta,time_point,base_time}.h \
        tests/flex-ui/unit/{CMakeLists.txt,base_primitives_test.cc}
git commit -m "feat(flex-ui/common): absorb macros/check/log_level/time primitives"
```

---

### Task 5: Absorb string_view + string_view_utils

**Files:**
- Create: `flex-ui/common/include/flexui/common/string_view.h`
- Create: `flex-ui/common/include/flexui/common/string_view_utils.h`
- Create: `flex-ui/common/src/string_view.cc`
- Create: `tests/flex-ui/unit/string_view_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt` (append `src/string_view.cc`)
- Modify: `tests/flex-ui/unit/CMakeLists.txt` (append `string_view_test.cc`)

- [ ] **Step 5.1: Copy + rewrite per the common workflow**

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
cp $HSRC/include/footstone/string_view.h        flex-ui/common/include/flexui/common/string_view.h
cp $HSRC/include/footstone/string_view_utils.h  flex-ui/common/include/flexui/common/string_view_utils.h
cp $HSRC/src/string_view.cc                     flex-ui/common/src/string_view.cc
```

Apply the same rename script (namespace, include path, macro prefix) as Task 4.2 to each of the three files. Insert the FlexUI modification notice block (Task 2) into each header (the `.cc` file does not need a notice block per the spec convention, but its original copyright remains).

- [ ] **Step 5.2: Append source to `flex-ui/common/CMakeLists.txt`**

In the `target_sources(flexui_common PRIVATE ...)` block, add:

```cmake
target_sources(flexui_common PRIVATE
  src/string_view.cc
)
```

- [ ] **Step 5.3: Write `tests/flex-ui/unit/string_view_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <string>

#include "flexui/common/string_view.h"
#include "flexui/common/string_view_utils.h"

namespace flexui::common {

TEST(StringViewTest, RoundTripLatin1) {
  string_view sv("hello");
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Latin1);
  EXPECT_EQ(sv.latin1_value(), "hello");
}

TEST(StringViewTest, RoundTripUtf8ToUtf16) {
  // string_view's encoding handling is the lowest-level invariant the rest
  // of common/ relies on; if the namespace rename breaks the storage
  // tag the conversion will fault under ASan.
  string_view sv("a\xc3\xa9");  // "aé"
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Latin1);
  std::string s(sv.latin1_value());
  EXPECT_GE(s.size(), 2u);
}

}  // namespace flexui::common
```

- [ ] **Step 5.4: Wire test, rebuild, run**

Append `string_view_test.cc` to `tests/flex-ui/unit/CMakeLists.txt`. Then:

```bash
./scripts/flex-ui/build.sh
```

Expected: all three test binaries pass.

- [ ] **Step 5.5: Commit**

```bash
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb string_view + string_view_utils"
```

---

### Task 6: Absorb string_utils

**Files:**
- Create: `flex-ui/common/include/flexui/common/string_utils.h`
- Create: `flex-ui/common/src/string_utils.cc`
- Create: `tests/flex-ui/unit/string_utils_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 6.1: Copy + rewrite** (per Task 4.2 / 5.1)

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
cp $HSRC/include/footstone/string_utils.h flex-ui/common/include/flexui/common/string_utils.h
cp $HSRC/src/string_utils.cc              flex-ui/common/src/string_utils.cc
```

Apply the standard renames + insert the FlexUI modification notice in the header.

- [ ] **Step 6.2: Wire source**

Add `src/string_utils.cc` to `flex-ui/common/CMakeLists.txt`.

- [ ] **Step 6.3: Write `tests/flex-ui/unit/string_utils_test.cc`**

Look at the actual public API in the absorbed header. Pick **one** function and write a test for it. Example placeholder if Hippy's `string_utils` exposes `ToUpper`:

```cpp
#include <gtest/gtest.h>
#include "flexui/common/string_utils.h"

namespace flexui::common {

TEST(StringUtilsTest, BoundaryEmpty) {
  // Replace ToUpper with whatever the actual Hippy footstone string_utils
  // public function is (the executor inspects the header to confirm).
  string_view sv("");
  EXPECT_EQ(sv.length(), 0u);
}

}  // namespace flexui::common
```

(If the actual public API surface differs, expand the test to cover at least one happy path and one boundary case using the real names.)

- [ ] **Step 6.4: Wire test, rebuild, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb string_utils"
```

---

### Task 7: Absorb logging + log_settings (footstone-level)

This task brings over Hippy's `logging.h` / `log_settings.{h,cc}` / `log_settings_state.cc` essentially unchanged. The FlexUI-specific `FLEXUI/<subsystem>/<event>` tag macro layered on top of this is **Task 8**, not here.

**Files:**
- Create: `flex-ui/common/include/flexui/common/logging.h`
- Create: `flex-ui/common/include/flexui/common/log_settings.h`
- Create: `flex-ui/common/src/log_settings.cc`
- Create: `flex-ui/common/src/log_settings_state.cc`
- Create: `tests/flex-ui/unit/logging_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 7.1: Copy + rewrite four files** per Task 4.2.

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
cp $HSRC/include/footstone/logging.h          flex-ui/common/include/flexui/common/logging.h
cp $HSRC/include/footstone/log_settings.h     flex-ui/common/include/flexui/common/log_settings.h
cp $HSRC/src/log_settings.cc                  flex-ui/common/src/log_settings.cc
cp $HSRC/src/log_settings_state.cc            flex-ui/common/src/log_settings_state.cc
```

Apply renames. Note: `FOOTSTONE_LOG`, `FOOTSTONE_DLOG`, `FOOTSTONE_CHECK`, etc., become `FLEXUI_LOG`, `FLEXUI_DLOG`, `FLEXUI_CHECK`.

- [ ] **Step 7.2: Append sources**

```cmake
target_sources(flexui_common PRIVATE
  src/log_settings.cc
  src/log_settings_state.cc
)
```

- [ ] **Step 7.3: Write `tests/flex-ui/unit/logging_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <sstream>

#include "flexui/common/logging.h"

namespace flexui::common {

TEST(LoggingTest, MacroCompiles) {
  // Sanity: the macro expansion should not error and should not abort.
  // We don't yet have a pluggable sink (Task 9 adds one), so we only
  // verify compilation + non-fatal execution.
  FLEXUI_LOG(INFO) << "hello flexui logging";
  EXPECT_TRUE(true);
}

}  // namespace flexui::common
```

- [ ] **Step 7.4: Wire test, rebuild, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb footstone logging + log_settings, rename to FLEXUI_*"
```

---

### Task 8: FlexUI log sink abstraction (FlexUI-original)

This is the first **FlexUI-original** module (no Hippy source). Defines `LogSink` interface, registry, and a default stdout sink. Required by Task 9 (FLEXUI_LOG tag macro).

**Files:**
- Create: `flex-ui/common/include/flexui/common/log_sink.h`
- Create: `flex-ui/common/src/log_sink.cc`
- Create: `tests/flex-ui/support/capturing_log_sink.h`
- Create: `tests/flex-ui/support/capturing_log_sink.cc`
- Create: `tests/flex-ui/unit/log_sink_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/CMakeLists.txt` (add support/ to the lib)
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 8.1: Write `flex-ui/common/include/flexui/common/log_sink.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0. See LICENSE in the project root for full license information.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "flexui/common/log_level.h"

namespace flexui::common {

struct LogRecord {
  LogSeverity level;          // LOG_INFO / LOG_WARNING / LOG_ERROR / LOG_FATAL
  std::string subsystem;      // "Engine" / "Scope" / "Bridge" / ...
  std::string event;          // "Init" / "Create" / "Apply" / ...
  std::string message;        // formatted human-readable line
};

class LogSink {
 public:
  virtual ~LogSink() = default;
  virtual void Write(const LogRecord& record) = 0;
};

// Process-global list of sinks. AddSink returns an opaque id used to remove.
// Sinks are invoked in registration order.
class LogSinkRegistry {
 public:
  static LogSinkRegistry& Instance();
  uint64_t AddSink(std::unique_ptr<LogSink> sink);
  bool RemoveSink(uint64_t id);
  void Emit(const LogRecord& record);
  void Clear();   // test-only

 private:
  LogSinkRegistry() = default;
};

// Default stdout sink: prints "[FLEXUI/<subsystem>/<event>][<level>] <msg>".
std::unique_ptr<LogSink> MakeStdoutLogSink();

}  // namespace flexui::common
```

- [ ] **Step 8.2: Write `flex-ui/common/src/log_sink.cc`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/common/log_sink.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace flexui::common {

namespace {

const char* SeverityName(LogSeverity s) {
  switch (s) {
    case LOG_INFO:    return "INFO";
    case LOG_WARNING: return "WARN";
    case LOG_ERROR:   return "ERROR";
    case LOG_FATAL:   return "FATAL";
    default:          return "DEBUG";
  }
}

class StdoutSink : public LogSink {
 public:
  void Write(const LogRecord& r) override {
    std::lock_guard<std::mutex> lock(mu_);
    std::cout << "[FLEXUI/" << r.subsystem << "/" << r.event << "]["
              << SeverityName(r.level) << "] " << r.message << std::endl;
  }
 private:
  std::mutex mu_;
};

struct State {
  std::mutex mu;
  std::unordered_map<uint64_t, std::unique_ptr<LogSink>> sinks;
  std::atomic<uint64_t> next_id{1};
};

State& S() {
  static State s;
  return s;
}

}  // namespace

LogSinkRegistry& LogSinkRegistry::Instance() {
  static LogSinkRegistry registry;
  return registry;
}

uint64_t LogSinkRegistry::AddSink(std::unique_ptr<LogSink> sink) {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  uint64_t id = s.next_id.fetch_add(1);
  s.sinks.emplace(id, std::move(sink));
  return id;
}

bool LogSinkRegistry::RemoveSink(uint64_t id) {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  return s.sinks.erase(id) > 0;
}

void LogSinkRegistry::Emit(const LogRecord& record) {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  for (auto& kv : s.sinks) {
    kv.second->Write(record);
  }
}

void LogSinkRegistry::Clear() {
  auto& s = S();
  std::lock_guard<std::mutex> lock(s.mu);
  s.sinks.clear();
}

std::unique_ptr<LogSink> MakeStdoutLogSink() {
  return std::make_unique<StdoutSink>();
}

}  // namespace flexui::common
```

- [ ] **Step 8.3: Write `tests/flex-ui/support/capturing_log_sink.h`**

```cpp
#pragma once

#include <mutex>
#include <vector>

#include "flexui/common/log_sink.h"

namespace flexui::common::test {

class CapturingLogSink : public LogSink {
 public:
  void Write(const LogRecord& r) override;
  std::vector<LogRecord> Drain();

 private:
  std::mutex mu_;
  std::vector<LogRecord> records_;
};

// RAII helper: registers a fresh CapturingLogSink, removes on destruction.
class ScopedCapturingSink {
 public:
  ScopedCapturingSink();
  ~ScopedCapturingSink();
  CapturingLogSink& sink() { return *sink_ptr_; }

 private:
  uint64_t id_;
  CapturingLogSink* sink_ptr_;
};

}  // namespace flexui::common::test
```

- [ ] **Step 8.4: Write `tests/flex-ui/support/capturing_log_sink.cc`**

```cpp
#include "tests/flex-ui/support/capturing_log_sink.h"

#include <memory>

namespace flexui::common::test {

void CapturingLogSink::Write(const LogRecord& r) {
  std::lock_guard<std::mutex> lock(mu_);
  records_.push_back(r);
}

std::vector<LogRecord> CapturingLogSink::Drain() {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<LogRecord> out;
  out.swap(records_);
  return out;
}

ScopedCapturingSink::ScopedCapturingSink() {
  auto sink = std::make_unique<CapturingLogSink>();
  sink_ptr_ = sink.get();
  id_ = LogSinkRegistry::Instance().AddSink(std::move(sink));
}

ScopedCapturingSink::~ScopedCapturingSink() {
  LogSinkRegistry::Instance().RemoveSink(id_);
}

}  // namespace flexui::common::test
```

- [ ] **Step 8.5: Wire support library**

Append to `tests/flex-ui/CMakeLists.txt` (before `add_subdirectory(unit)`):

```cmake
add_library(flexui_test_support STATIC
  support/capturing_log_sink.cc
)
target_include_directories(flexui_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(flexui_test_support PUBLIC flexui_common)
```

- [ ] **Step 8.6: Write `tests/flex-ui/unit/log_sink_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/common/log_sink.h"
#include "tests/flex-ui/support/capturing_log_sink.h"

namespace flexui::common {

TEST(LogSinkTest, CapturingSinkReceivesEmittedRecord) {
  test::ScopedCapturingSink scope;
  LogSinkRegistry::Instance().Emit(
      LogRecord{LOG_INFO, "TestSub", "TestEvent", "hello"});
  auto records = scope.sink().Drain();
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].subsystem, "TestSub");
  EXPECT_EQ(records[0].event, "TestEvent");
  EXPECT_EQ(records[0].message, "hello");
}

TEST(LogSinkTest, RemoveSinkStopsDelivery) {
  auto id = LogSinkRegistry::Instance().AddSink(MakeStdoutLogSink());
  LogSinkRegistry::Instance().RemoveSink(id);
  // No assertion target; just verifies remove returns true and the
  // subsequent Emit does not crash with the sink gone.
  LogSinkRegistry::Instance().Emit(
      LogRecord{LOG_INFO, "X", "Y", "msg after removal"});
}

}  // namespace flexui::common
```

- [ ] **Step 8.7: Wire source + test, rebuild, run, commit**

Append `src/log_sink.cc` to `flex-ui/common/CMakeLists.txt`. Append `log_sink_test.cc` to `tests/flex-ui/unit/CMakeLists.txt` and add `flexui_test_support` to its `target_link_libraries`. Then:

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui
git commit -m "feat(flex-ui/common): add LogSink + LogSinkRegistry + stdout sink + test helper"
```

---

### Task 9: FLEXUI_LOG structured tag macro (FlexUI-original)

The `FLEXUI_LOG(subsystem, event, level)` macro emits a `LogRecord` to the sink registry. It is the canonical entry point that all FlexUI subsystems use (per spec §8.2). Subsystems must use this macro, **not** the bare footstone `FLEXUI_LOG(INFO)` form — but the footstone macro continues to exist for low-level diagnostics (and for backwards-compatible plumbing inside absorbed code).

To avoid collision with the footstone-style `FLEXUI_LOG(level)` macro brought in by Task 7, name the structured macro `FLEXUI_TLOG` (T for "tagged"). The spec text in §8.4 reads naturally either way.

**Files:**
- Create: `flex-ui/common/include/flexui/common/log_tag.h`
- Create: `flex-ui/common/src/log_tag.cc`
- Create: `tests/flex-ui/unit/log_tag_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 9.1: Write `flex-ui/common/include/flexui/common/log_tag.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * FLEXUI_TLOG: the canonical structured-log entry point for all FlexUI
 * subsystem code. See docs/superpowers/specs/2026-05-30-flexui-layering-design.md
 * §8 for the tag convention and mandatory coverage points.
 *
 * Usage:
 *   FLEXUI_TLOG(Engine, Init, INFO) << "config=" << cfg.summary();
 *   FLEXUI_TLOG(Scope, StateChange, DEBUG) << "from=" << from << " to=" << to;
 */
#pragma once

#include <sstream>
#include <string>

#include "flexui/common/log_level.h"
#include "flexui/common/log_sink.h"

namespace flexui::common {

class TaggedLogStream {
 public:
  TaggedLogStream(LogSeverity level, const char* subsystem, const char* event)
      : level_(level), subsystem_(subsystem), event_(event) {}

  ~TaggedLogStream() {
    LogSinkRegistry::Instance().Emit(
        LogRecord{level_, subsystem_, event_, oss_.str()});
  }

  std::ostringstream& stream() { return oss_; }

 private:
  LogSeverity level_;
  std::string subsystem_;
  std::string event_;
  std::ostringstream oss_;
};

}  // namespace flexui::common

#define FLEXUI_TLOG(subsystem, event, level)                                  \
  ::flexui::common::TaggedLogStream(                                          \
      ::flexui::common::LOG_##level, #subsystem, #event)                      \
      .stream()
```

- [ ] **Step 9.2: Write `flex-ui/common/src/log_tag.cc`** (empty TU; the macro is header-only but a `.cc` makes future ABI changes easier)

```cpp
#include "flexui/common/log_tag.h"
namespace flexui::common {
// Reserved for future non-inline helpers.
}
```

- [ ] **Step 9.3: Wire source**

```cmake
target_sources(flexui_common PRIVATE
  src/log_tag.cc
)
```

- [ ] **Step 9.4: Write `tests/flex-ui/unit/log_tag_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/common/log_tag.h"
#include "tests/flex-ui/support/capturing_log_sink.h"

namespace flexui::common {

TEST(LogTagTest, EmitsRecordWithSubsystemAndEvent) {
  test::ScopedCapturingSink scope;
  FLEXUI_TLOG(Engine, Init, INFO) << "starting flexui";
  auto records = scope.sink().Drain();
  ASSERT_EQ(records.size(), 1u);
  EXPECT_EQ(records[0].subsystem, "Engine");
  EXPECT_EQ(records[0].event, "Init");
  EXPECT_EQ(records[0].level, LOG_INFO);
  EXPECT_EQ(records[0].message, "starting flexui");
}

TEST(LogTagTest, MultipleSinksReceiveSameRecord) {
  test::ScopedCapturingSink a, b;
  FLEXUI_TLOG(Bridge, Call, DEBUG) << "x=42";
  auto ra = a.sink().Drain();
  auto rb = b.sink().Drain();
  ASSERT_EQ(ra.size(), 1u);
  ASSERT_EQ(rb.size(), 1u);
  EXPECT_EQ(ra[0].subsystem, "Bridge");
  EXPECT_EQ(rb[0].subsystem, "Bridge");
}

}  // namespace flexui::common
```

- [ ] **Step 9.5: Wire test, rebuild, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): add FLEXUI_TLOG structured-tag macro"
```

---

### Task 10: HiLog sink (HarmonyOS) + Android stub

**Files:**
- Create: `flex-ui/common/src/platform/log_sink_harmony.cc`
- Create: `flex-ui/common/src/platform/log_sink_android.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `flex-ui/common/include/flexui/common/log_sink.h` (add factory declarations)

W1-W2 builds + tests on host only, so the HiLog sink TU compiles but is not exercised in the host gate. It must compile under a `FLEXUI_OHOS` define (added to CMake when targeting Harmony in W3+). The Android stub is a no-op `LogSink` impl that always reports `[NOT_IMPLEMENTED]`.

- [ ] **Step 10.1: Append factory declarations to `log_sink.h`**

Add immediately before the closing `}  // namespace flexui::common`:

```cpp
// Platform sink factories. Available only when the corresponding platform
// macro is set at compile time. Calling them otherwise returns nullptr.
std::unique_ptr<LogSink> MakeHarmonyHiLogSink();
std::unique_ptr<LogSink> MakeAndroidLogSink();
```

- [ ] **Step 10.2: Write `flex-ui/common/src/platform/log_sink_harmony.cc`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * HiLog backend for FlexUI's LogSink. Compiles only when FLEXUI_OHOS is set.
 */
#include "flexui/common/log_sink.h"

#include <memory>

#if defined(FLEXUI_OHOS)
#include <hilog/log.h>
#endif

namespace flexui::common {

#if defined(FLEXUI_OHOS)

namespace {

constexpr uint32_t kHiLogDomain = 0xFE00;  // FlexUI domain id, registered with platform team

class HiLogSink : public LogSink {
 public:
  void Write(const LogRecord& r) override {
    // Tag follows FLEXUI/<subsystem>/<event> convention.
    char tag[128];
    snprintf(tag, sizeof(tag), "FLEXUI/%s/%s", r.subsystem.c_str(), r.event.c_str());
    LogLevel level = LOG_INFO;
    switch (r.level) {
      case LOG_WARNING: level = LOG_WARN;  break;
      case LOG_ERROR:
      case LOG_FATAL:   level = LOG_ERROR; break;
      default:          level = LOG_INFO;  break;
    }
    OH_LOG_Print(LOG_APP, level, kHiLogDomain, tag, "%{public}s", r.message.c_str());
  }
};

}  // namespace

std::unique_ptr<LogSink> MakeHarmonyHiLogSink() {
  return std::make_unique<HiLogSink>();
}

#else  // !FLEXUI_OHOS

std::unique_ptr<LogSink> MakeHarmonyHiLogSink() { return nullptr; }

#endif  // FLEXUI_OHOS

}  // namespace flexui::common
```

- [ ] **Step 10.3: Write `flex-ui/common/src/platform/log_sink_android.cc`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Android backend STUB. Real implementation lands in W5-W6 (platform/android/jni)
 * with __android_log_print. Until then this returns nullptr so callers can
 * gracefully fall back to the stdout sink.
 */
#include "flexui/common/log_sink.h"

namespace flexui::common {

std::unique_ptr<LogSink> MakeAndroidLogSink() {
  // NOT_IMPLEMENTED: returning nullptr is the agreed contract for platform
  // sink factories whose backend is not yet wired. See plan W1-W2 Task 10.
  return nullptr;
}

}  // namespace flexui::common
```

- [ ] **Step 10.4: Wire sources**

Append to `flex-ui/common/CMakeLists.txt`:

```cmake
target_sources(flexui_common PRIVATE
  src/platform/log_sink_harmony.cc
  src/platform/log_sink_android.cc
)
```

- [ ] **Step 10.5: Verify host build still green**

```bash
./scripts/flex-ui/build.sh
```

Expected: HiLog TU compiles as the stub path (no `FLEXUI_OHOS` define), Android TU returns nullptr, all tests still pass.

- [ ] **Step 10.6: Commit**

```bash
git add flex-ui/common
git commit -m "feat(flex-ui/common): add HiLog sink (Harmony) + Android stub factory"
```

---

### Task 11: Absorb task + task_runner

These two are inseparable — `task_runner.h` declares the runner type that consumes `task.h`.

**Files:**
- Create: `flex-ui/common/include/flexui/common/task.h`
- Create: `flex-ui/common/include/flexui/common/task_runner.h`
- Create: `flex-ui/common/src/task.cc`
- Create: `flex-ui/common/src/task_runner.cc`
- Create: `tests/flex-ui/unit/task_runner_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 11.1: Copy + rewrite four files** per Task 4.2.

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
cp $HSRC/include/footstone/task.h         flex-ui/common/include/flexui/common/task.h
cp $HSRC/include/footstone/task_runner.h  flex-ui/common/include/flexui/common/task_runner.h
cp $HSRC/src/task.cc                      flex-ui/common/src/task.cc
cp $HSRC/src/task_runner.cc               flex-ui/common/src/task_runner.cc
```

Apply the rename script. Insert the FlexUI modification notice into the two headers.

- [ ] **Step 11.2: Wire sources**

```cmake
target_sources(flexui_common PRIVATE
  src/task.cc
  src/task_runner.cc
)
```

- [ ] **Step 11.3: Write `tests/flex-ui/unit/task_runner_test.cc`**

```cpp
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "flexui/common/task.h"
#include "flexui/common/task_runner.h"

namespace flexui::common {

// Inspect task_runner.h to confirm the exact public API. The skeleton below
// assumes Hippy's footstone API (TaskRunner with PostTask(std::function<void()>)
// and a Run/Stop pair). Adjust signatures if the absorbed source differs.

TEST(TaskRunnerTest, PostedTaskExecutes) {
  TaskRunner runner("flexui-test-runner", 1);
  std::atomic<int> counter{0};
  runner.PostTask([&counter] { counter.fetch_add(1); });
  // Give the runner a tick to drain.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(counter.load(), 1);
  runner.Terminate();
}

}  // namespace flexui::common
```

If the actual `TaskRunner` constructor or `PostTask` signature differs, replace this test body with one that exercises the actual API surface — the executor reads `flex-ui/common/include/flexui/common/task_runner.h` to confirm.

- [ ] **Step 11.4: Wire test, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb task + task_runner"
```

---

### Task 12: Absorb worker + worker_impl + worker_manager + cv_driver

These four are inseparable — `worker.h` references `cv_driver` and the runner; `worker_manager` orchestrates pools of workers.

**Files:**
- Create headers under `flex-ui/common/include/flexui/common/`: `worker.h`, `worker_impl.h`, `worker_manager.h`, `cv_driver.h`
- Create sources under `flex-ui/common/src/`: `worker.cc`, `worker_manager.cc`, `cv_driver.cc`
- Create: `tests/flex-ui/unit/worker_test.cc`
- Create: `tests/flex-ui/unit/worker_manager_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 12.1: Copy + rewrite**

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
for f in worker.h worker_impl.h worker_manager.h cv_driver.h; do
  cp $HSRC/include/footstone/$f flex-ui/common/include/flexui/common/$f
done
for f in worker.cc worker_manager.cc cv_driver.cc; do
  cp $HSRC/src/$f flex-ui/common/src/$f
done
```

Apply rename script. Insert modification notice into the four headers.

- [ ] **Step 12.2: Wire sources**

```cmake
target_sources(flexui_common PRIVATE
  src/worker.cc
  src/worker_manager.cc
  src/cv_driver.cc
)
```

- [ ] **Step 12.3: Write `tests/flex-ui/unit/worker_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

#include "flexui/common/task.h"
#include "flexui/common/task_runner.h"
#include "flexui/common/worker.h"
#include "flexui/common/worker_impl.h"

namespace flexui::common {

TEST(WorkerTest, WorkerExecutesPostedTask) {
  // Use the absorbed footstone WorkerImpl. If the absorbed API differs,
  // adjust the construction call to match.
  auto worker = std::make_shared<WorkerImpl>("flexui-test-worker", false);
  worker->Start();

  auto runner = std::make_shared<TaskRunner>("inner-runner", 1);
  worker->Bind({runner});

  std::atomic<int> counter{0};
  runner->PostTask([&counter] { counter.fetch_add(1); });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(counter.load(), 1);

  worker->Terminate();
}

}  // namespace flexui::common
```

- [ ] **Step 12.4: Write `tests/flex-ui/unit/worker_manager_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

#include "flexui/common/worker_manager.h"

namespace flexui::common {

TEST(WorkerManagerTest, CreatesAndJoinsWorker) {
  // Adjust constructor args to match the absorbed WorkerManager API.
  WorkerManager manager(2 /*pool size*/);
  auto runner = manager.CreateTaskRunner(false, 1, "flexui-test-mgr");
  std::atomic<int> counter{0};
  runner->PostTask([&counter] { counter.fetch_add(1); });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(counter.load(), 1);
  manager.Terminate();
}

}  // namespace flexui::common
```

- [ ] **Step 12.5: Wire tests, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb worker + worker_manager + cv_driver"
```

---

### Task 13: Absorb timer family + idle_task + driver

**Files:**
- Create headers: `base_timer.h`, `one_shot_timer.h`, `repeating_timer.h`, `idle_timer.h`, `idle_task.h`, `driver.h`
- Create sources: `base_timer.cc`, `one_shot_timer.cc`, `repeating_timer.cc`, `idle_timer.cc`, `idle_task.cc`
- Create: `tests/flex-ui/unit/timer_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 13.1: Copy + rewrite**

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
for f in base_timer.h one_shot_timer.h repeating_timer.h idle_timer.h idle_task.h driver.h; do
  cp $HSRC/include/footstone/$f flex-ui/common/include/flexui/common/$f
done
for f in base_timer.cc one_shot_timer.cc repeating_timer.cc idle_timer.cc idle_task.cc; do
  cp $HSRC/src/$f flex-ui/common/src/$f
done
```

Apply rename + modification notice.

- [ ] **Step 13.2: Wire sources**

```cmake
target_sources(flexui_common PRIVATE
  src/base_timer.cc
  src/one_shot_timer.cc
  src/repeating_timer.cc
  src/idle_timer.cc
  src/idle_task.cc
)
```

- [ ] **Step 13.3: Write `tests/flex-ui/unit/timer_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

#include "flexui/common/one_shot_timer.h"
#include "flexui/common/repeating_timer.h"
#include "flexui/common/task_runner.h"
#include "flexui/common/time_delta.h"

namespace flexui::common {

TEST(TimerTest, OneShotTimerFiresOnce) {
  auto runner = std::make_shared<TaskRunner>("flexui-timer-rnr", 1);
  std::atomic<int> fired{0};
  OneShotTimer timer;
  timer.Start(runner, TimeDelta::FromMilliseconds(10),
              [&fired] { fired.fetch_add(1); });
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  EXPECT_EQ(fired.load(), 1);
  runner->Terminate();
}

TEST(TimerTest, RepeatingTimerFiresMultipleTimes) {
  auto runner = std::make_shared<TaskRunner>("flexui-timer-rnr", 1);
  std::atomic<int> fired{0};
  RepeatingTimer timer;
  timer.Start(runner, TimeDelta::FromMilliseconds(10),
              [&fired] { fired.fetch_add(1); });
  std::this_thread::sleep_for(std::chrono::milliseconds(55));
  timer.Stop();
  EXPECT_GE(fired.load(), 3);
  runner->Terminate();
}

}  // namespace flexui::common
```

Adjust constructor / `Start` arguments to match the absorbed API; the test asserts the timer's observable behavior, not its exact signature.

- [ ] **Step 13.4: Wire test, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb timer family + idle_task + driver"
```

---

### Task 14: Absorb persistent_object_map + hash

**Files:**
- Create: `flex-ui/common/include/flexui/common/persistent_object_map.h`
- Create: `flex-ui/common/include/flexui/common/hash.h`
- Create: `tests/flex-ui/unit/persistent_object_map_test.cc`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 14.1: Copy + rewrite** the two headers (both header-only). Apply notice.

- [ ] **Step 14.2: Write `tests/flex-ui/unit/persistent_object_map_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "flexui/common/persistent_object_map.h"

namespace flexui::common {

TEST(PersistentObjectMapTest, InsertAndFind) {
  PersistentObjectMap<uint32_t, std::shared_ptr<std::string>> map;
  auto s = std::make_shared<std::string>("hello");
  map.Insert(42, s);
  std::shared_ptr<std::string> found;
  ASSERT_TRUE(map.Find(42, found));
  EXPECT_EQ(*found, "hello");
}

TEST(PersistentObjectMapTest, EraseRemoves) {
  PersistentObjectMap<uint32_t, int> map;
  map.Insert(1, 100);
  EXPECT_TRUE(map.Erase(1));
  int v = 0;
  EXPECT_FALSE(map.Find(1, v));
}

}  // namespace flexui::common
```

- [ ] **Step 14.3: Build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb persistent_object_map + hash"
```

---

### Task 15: Absorb hippy_value (rename to flexui_value)

This module gets a **structural rename** (file + type), not just namespace, because it carries the framework's name.

**Files:**
- Create: `flex-ui/common/include/flexui/common/flexui_value.h` (from `hippy_value.h`)
- Create: `flex-ui/common/src/flexui_value.cc` (from `hippy_value.cc`)
- Create: `tests/flex-ui/unit/flexui_value_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 15.1: Copy + rewrite**

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
cp $HSRC/include/footstone/hippy_value.h flex-ui/common/include/flexui/common/flexui_value.h
cp $HSRC/src/hippy_value.cc              flex-ui/common/src/flexui_value.cc
```

Apply standard renames. Additionally:

```bash
python3 - <<'PY'
import pathlib, re
for p in [pathlib.Path("flex-ui/common/include/flexui/common/flexui_value.h"),
          pathlib.Path("flex-ui/common/src/flexui_value.cc")]:
  s = p.read_text()
  s = s.replace("HippyValue", "FlexUIValue")
  s = re.sub(r'#include\s+"footstone/hippy_value\.h"',
             '#include "flexui/common/flexui_value.h"', s)
  p.write_text(s)
PY
```

Insert FlexUI notice in the header. **Additionally** mention the rename in the notice block:

```
 *   - File renamed: hippy_value.{h,cc} -> flexui_value.{h,cc}.
 *   - Type renamed: HippyValue -> FlexUIValue.
```

- [ ] **Step 15.2: Wire source**

```cmake
target_sources(flexui_common PRIVATE
  src/flexui_value.cc
)
```

- [ ] **Step 15.3: Write `tests/flex-ui/unit/flexui_value_test.cc`**

```cpp
#include <gtest/gtest.h>
#include "flexui/common/flexui_value.h"

namespace flexui::common {

TEST(FlexUIValueTest, ConstructString) {
  FlexUIValue v(std::string("hello"));
  EXPECT_TRUE(v.IsString());
  EXPECT_EQ(v.ToString(), "hello");
}

TEST(FlexUIValueTest, ConstructInt) {
  FlexUIValue v(42);
  EXPECT_TRUE(v.IsInt32() || v.IsNumber());  // depending on absorbed API
}

}  // namespace flexui::common
```

If the absorbed `FlexUIValue` exposes different inspection methods (e.g., `Is<T>()` rather than `IsString()`), adjust the test to call the actual API.

- [ ] **Step 15.4: Wire test, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb hippy_value as flexui_value (HippyValue -> FlexUIValue)"
```

---

### Task 16: Absorb serializer + deserializer

**Files:**
- Create: `flex-ui/common/include/flexui/common/serializer.h`
- Create: `flex-ui/common/include/flexui/common/deserializer.h`
- Create: `flex-ui/common/src/serializer.cc`
- Create: `flex-ui/common/src/deserializer.cc`
- Create: `tests/flex-ui/unit/serializer_roundtrip_test.cc`
- Modify: `flex-ui/common/CMakeLists.txt`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 16.1: Copy + rewrite**

```bash
HSRC=/Users/pingjiang/coding/github/Hippy/modules/footstone
cp $HSRC/include/footstone/serializer.h    flex-ui/common/include/flexui/common/serializer.h
cp $HSRC/include/footstone/deserializer.h  flex-ui/common/include/flexui/common/deserializer.h
cp $HSRC/src/serializer.cc                 flex-ui/common/src/serializer.cc
cp $HSRC/src/deserializer.cc               flex-ui/common/src/deserializer.cc
```

Apply renames. The absorbed code likely references `HippyValue` — replace with `FlexUIValue` to match Task 15.

```bash
python3 - <<'PY'
import pathlib
for p in pathlib.Path("flex-ui/common").rglob("*"):
  if p.suffix in {".h", ".cc"} and "serializer" in p.name or "deserializer" in p.name:
    s = p.read_text()
    s = s.replace("HippyValue", "FlexUIValue")
    s = s.replace('"footstone/hippy_value.h"', '"flexui/common/flexui_value.h"')
    p.write_text(s)
PY
```

Insert FlexUI notice (mentioning the `HippyValue → FlexUIValue` rename) in both headers.

- [ ] **Step 16.2: Wire sources**

```cmake
target_sources(flexui_common PRIVATE
  src/serializer.cc
  src/deserializer.cc
)
```

- [ ] **Step 16.3: Write `tests/flex-ui/unit/serializer_roundtrip_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <vector>

#include "flexui/common/serializer.h"
#include "flexui/common/deserializer.h"
#include "flexui/common/flexui_value.h"

namespace flexui::common {

TEST(SerializerRoundtripTest, StringRoundtrips) {
  FlexUIValue input(std::string("hello flexui"));

  Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(input);
  auto buffer = ser.Release();
  ASSERT_FALSE(buffer.first == nullptr);

  Deserializer de(buffer.first, buffer.second);
  ASSERT_TRUE(de.ReadHeader());
  FlexUIValue output;
  ASSERT_TRUE(de.ReadValue(output));
  EXPECT_TRUE(output.IsString());
  EXPECT_EQ(output.ToString(), "hello flexui");
}

}  // namespace flexui::common
```

(Adapt `Release` / `ReadValue` to the actual Hippy footstone API.)

- [ ] **Step 16.4: Wire test, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): absorb serializer + deserializer (uses FlexUIValue)"
```

---

### Task 17: FlexUI error module (FlexUI-original)

**Files:**
- Create: `flex-ui/common/include/flexui/common/error.h`
- Create: `tests/flex-ui/unit/error_test.cc`
- Modify: `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 17.1: Write `flex-ui/common/include/flexui/common/error.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * FlexUI error type. A small, allocation-free-on-success Result wrapper
 * used at API boundaries. Subsystem code should return Error from
 * fallible operations rather than throwing.
 */
#pragma once

#include <ostream>
#include <string>
#include <utility>

namespace flexui::common {

enum class ErrorCode : uint32_t {
  kOk = 0,
  kInvalidArgument,
  kNotFound,
  kAlreadyExists,
  kInternal,
  kJsException,
  kPluginConflict,
  kEngineNotInitialized,
  kScopeDestroyed,
};

const char* ErrorCodeName(ErrorCode code);

class Error {
 public:
  static Error Ok() { return Error(ErrorCode::kOk, ""); }
  Error(ErrorCode code, std::string message)
      : code_(code), message_(std::move(message)) {}
  ErrorCode code() const { return code_; }
  const std::string& message() const { return message_; }
  bool ok() const { return code_ == ErrorCode::kOk; }
  explicit operator bool() const { return !ok(); }
  friend std::ostream& operator<<(std::ostream& os, const Error& e) {
    os << "Error(" << ErrorCodeName(e.code()) << ": " << e.message() << ")";
    return os;
  }
 private:
  ErrorCode code_;
  std::string message_;
};

}  // namespace flexui::common
```

- [ ] **Step 17.2: Add `ErrorCodeName` inline in the header (header-only is fine for W1-W2)**

Add immediately above the closing namespace brace:

```cpp
inline const char* ErrorCodeName(ErrorCode code) {
  switch (code) {
    case ErrorCode::kOk:                   return "Ok";
    case ErrorCode::kInvalidArgument:      return "InvalidArgument";
    case ErrorCode::kNotFound:             return "NotFound";
    case ErrorCode::kAlreadyExists:        return "AlreadyExists";
    case ErrorCode::kInternal:             return "Internal";
    case ErrorCode::kJsException:          return "JsException";
    case ErrorCode::kPluginConflict:       return "PluginConflict";
    case ErrorCode::kEngineNotInitialized: return "EngineNotInitialized";
    case ErrorCode::kScopeDestroyed:       return "ScopeDestroyed";
  }
  return "Unknown";
}
```

- [ ] **Step 17.3: Write `tests/flex-ui/unit/error_test.cc`**

```cpp
#include <gtest/gtest.h>
#include <sstream>

#include "flexui/common/error.h"

namespace flexui::common {

TEST(ErrorTest, OkConstructionIsNotErrorish) {
  Error e = Error::Ok();
  EXPECT_TRUE(e.ok());
  EXPECT_FALSE(static_cast<bool>(e));
}

TEST(ErrorTest, NonOkErrorishness) {
  Error e(ErrorCode::kPluginConflict, "duplicate plugin name");
  EXPECT_FALSE(e.ok());
  EXPECT_TRUE(static_cast<bool>(e));
  EXPECT_EQ(e.code(), ErrorCode::kPluginConflict);
}

TEST(ErrorTest, FormatsCodeAndMessage) {
  Error e(ErrorCode::kJsException, "ReferenceError: foo");
  std::ostringstream oss;
  oss << e;
  EXPECT_EQ(oss.str(), "Error(JsException: ReferenceError: foo)");
}

}  // namespace flexui::common
```

- [ ] **Step 17.4: Wire test, build, run, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/common tests/flex-ui/unit
git commit -m "feat(flex-ui/common): add Error + ErrorCode (FlexUI-original)"
```

---

### Task 18: TSan-only sub-target for concurrency-sensitive modules

**Files:**
- Modify: `tests/flex-ui/CMakeLists.txt` (add `flexui_unit_tests_tsan` target)
- Modify: `scripts/flex-ui/build.sh` (already supports `--tsan-only`, no change)

Add a separate executable that links only the concurrency-relevant test sources. This isolates TSan failures from the rest of the suite.

- [ ] **Step 18.1: Add TSan target to `tests/flex-ui/unit/CMakeLists.txt`**

```cmake
add_executable(flexui_unit_tests_tsan
  task_runner_test.cc
  worker_test.cc
  worker_manager_test.cc
  timer_test.cc
)
target_link_libraries(flexui_unit_tests_tsan PRIVATE
  flexui_common
  flexui_test_support
  GTest::gtest
  GTest::gtest_main
)
add_test(NAME flexui_unit_tests_tsan COMMAND flexui_unit_tests_tsan)
```

- [ ] **Step 18.2: Verify TSan run**

```bash
./scripts/flex-ui/build.sh --tsan-only
```

Expected: both targets compile and the concurrency tests pass under TSan. If TSan reports races on absorbed footstone code, **document them**: append a section "Known TSan races (absorbed)" to `flex-ui/README.md` listing each race the same way `tests/cpp/DESIGN.md §5` documents the AGenUI ones — **do not fix** within W1-W2 (absorbed-code policy: changes require an explicit ticket).

- [ ] **Step 18.3: Commit**

```bash
git add tests/flex-ui flex-ui/README.md
git commit -m "test(flex-ui): add TSan sub-target for task_runner/worker/timer"
```

---

### Task 19: Coverage measurement + ≥80% gate

**Files:**
- Modify: `scripts/flex-ui/build.sh` (extend `--coverage` flow with lcov)
- Create: `scripts/flex-ui/coverage.sh` (post-process)

- [ ] **Step 19.1: Write `scripts/flex-ui/coverage.sh`**

```bash
#!/usr/bin/env bash
# Run tests under coverage instrumentation and emit a line-coverage % for
# flex-ui/common/. Fails (exit 1) if coverage drops below 80%.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT}/tests/flex-ui/build"

./scripts/flex-ui/build.sh --coverage

# Use llvm-cov on macOS (default), gcov elsewhere.
if [[ "$(uname)" == "Darwin" ]]; then
  TEST_BIN="${BUILD_DIR}/unit/flexui_unit_tests"
  PROFRAW="${BUILD_DIR}/flexui.profraw"
  PROFDATA="${BUILD_DIR}/flexui.profdata"

  LLVM_PROFILE_FILE="$PROFRAW" "$TEST_BIN"
  xcrun llvm-profdata merge -sparse "$PROFRAW" -o "$PROFDATA"
  PERCENT=$(xcrun llvm-cov report "$TEST_BIN" -instr-profile="$PROFDATA" \
              "${ROOT}/flex-ui/common/" | tail -1 | awk '{print $7}' | tr -d '%')
else
  lcov --capture --directory "$BUILD_DIR" --output-file "${BUILD_DIR}/coverage.info"
  lcov --extract "${BUILD_DIR}/coverage.info" "*/flex-ui/common/*" \
       --output-file "${BUILD_DIR}/coverage.flexui.info"
  PERCENT=$(lcov --summary "${BUILD_DIR}/coverage.flexui.info" 2>&1 \
              | awk '/lines/ {print $2}' | tr -d '%' | head -1)
fi

echo "Line coverage for flex-ui/common/: ${PERCENT}%"
THRESHOLD=80
PERCENT_INT=${PERCENT%.*}
if (( PERCENT_INT < THRESHOLD )); then
  echo "FAIL: coverage ${PERCENT}% below threshold ${THRESHOLD}%"
  exit 1
fi
echo "PASS: coverage gate met (${PERCENT}% >= ${THRESHOLD}%)"
```

```bash
chmod +x scripts/flex-ui/coverage.sh
```

- [ ] **Step 19.2: Run coverage**

```bash
./scripts/flex-ui/coverage.sh
```

Expected: `PASS: coverage gate met (≥ 80% >= 80%)`. If under 80%, identify uncovered units and add focused tests until the gate passes. **Do not** lower the threshold.

- [ ] **Step 19.3: Commit**

```bash
git add scripts/flex-ui/coverage.sh
git commit -m "test(flex-ui): add coverage gate (≥80% line coverage on flex-ui/common/)"
```

---

### Task 20: Coexistence gate (AGenUI tree untouched)

**Files:** none (verification only)

- [ ] **Step 20.1: Diff against master to verify no AGenUI files touched**

```bash
git diff master --stat -- core/ platforms/ tests/cpp/ playground/ scripts/harmony/ scripts/android/ scripts/ios/ agent_sdks/ samples/ skills/
```

Expected: empty output (no changes). If anything appears, revert it — `flex-ui/` work must not bleed into the AGenUI tree.

- [ ] **Step 20.2: Confirm new artifacts under `flex-ui/`, `tests/flex-ui/`, `scripts/flex-ui/`, and `docs/superpowers/plans/`**

```bash
git diff master --stat -- flex-ui/ tests/flex-ui/ scripts/flex-ui/ docs/superpowers/plans/ .gitignore | tail
```

Expected: substantial additions to those paths only. Plan document itself appears in `docs/superpowers/plans/`.

- [ ] **Step 20.3: Run the full host gate once more, end-to-end**

```bash
./scripts/flex-ui/build.sh                   # ASan + UBSan
./scripts/flex-ui/build.sh --tsan-only       # TSan
./scripts/flex-ui/coverage.sh                # ≥80% coverage
```

Expected: all three green.

- [ ] **Step 20.4: Final commit (housekeeping only — no code change)**

If nothing changed in this task, skip the commit. Otherwise:

```bash
git add -A
git commit -m "chore(flex-ui): verify W1-W2 coexistence + green host gates"
```

---

## Self-Review

### Spec coverage

| Spec requirement | Covered by |
|---|---|
| §4.3 `flex-ui/common/` directory + `NOTICE` | Task 1 |
| §4.4 Hippy footstone absorption + namespace rename + attribution | Tasks 4-7, 11-16 |
| §4.4 Hippy footstone files: every file listed in NOTICE | Tasks 4-7, 11-16 (full enumeration in NOTICE template) |
| §8.1 logging at architectural boundaries | Tasks 7-10 |
| §8.2 `FLEXUI/<subsystem>/<event>` tag convention | Task 9 (`FLEXUI_TLOG`) |
| §8.3 log levels (ERROR/WARN/INFO/DEBUG/VERBOSE) | Tasks 7, 9 (LogSeverity from absorbed log_level.h; FLEXUI_TLOG accepts level) |
| §8.6 cross-platform log abstraction; subsystem code never calls platform APIs directly | Task 8 (LogSinkRegistry) + Task 10 (Harmony/Android sink factories) |
| §6.4 unit test coverage ≥80% on `flex-ui/core/` and ≥70% on `flex-ui/components/` | Task 19 (gate on `flex-ui/common/` ≥80%; the `core/` / `components/` gates land in their respective W3-W4 / W7-W8 plans) |
| §7.1 GoogleTest + ASan/UBSan default; TSan curated subset | Task 3 (ASan default), Task 18 (TSan target) |
| §5.1 AGenUI tree untouched, parallel coexistence | Task 20 |

W3-W6/W7-W8/W9-W10 spec items (JS engine, vdom, components, frontends, E2E) are explicitly out of scope for this plan (declared at top of the document).

### Placeholder scan

No "TBD", "TODO", or "fill in later" remain. Cases where the absorbed Hippy API signature is unknown until the executor inspects the source (e.g. Task 11 step 11.3, Task 12 step 12.3) are called out with an explicit instruction: "the executor reads … to confirm" + a working test skeleton that exercises the observable behavior. This is honest, not vague — the actual API matters less than asserting behavior.

### Type consistency

- `LogRecord` defined in Task 8 (log_sink.h) — `subsystem`, `event`, `message` are `std::string`, `level` is `LogSeverity`. Used by `FLEXUI_TLOG` in Task 9 (log_tag.h) constructing `LogRecord{level, subsystem, event, oss.str()}` — matches.
- `LogSinkRegistry::Instance().Emit(LogRecord{...})` — same signature used in Task 8 tests, Task 9 macro, Task 10 platform sinks.
- `Error::Ok()` / `Error(code, message)` — used in Task 17 tests with matching signatures.
- `HippyValue` → `FlexUIValue` rename consistent across Tasks 15 and 16; serializer/deserializer include path matches.

### Scope check

This plan covers exactly the W1-W2 boundary defined in spec §5.2. The deliverable runs working software (a static library + green test suite) on its own without dependency on later W3-W10 plans. Subsequent plans (W3-W4 core kernel) will declare a dependency on this plan being complete.

---

*End of W1-W2 absorption plan.*
