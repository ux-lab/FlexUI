# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## MCP Tools: code-review-graph

**IMPORTANT: This project has a knowledge graph. ALWAYS use the
code-review-graph MCP tools BEFORE using Grep/Glob/Read to explore
the codebase.** The graph is faster, cheaper (fewer tokens), and gives
you structural context (callers, dependents, test coverage) that file
scanning cannot.

### When to use graph tools FIRST

- **Exploring code**: `semantic_search_nodes` or `query_graph` instead of Grep
- **Understanding impact**: `get_impact_radius` instead of manually tracing imports
- **Code review**: `detect_changes` + `get_review_context` instead of reading entire files
- **Finding relationships**: `query_graph` with callers_of/callees_of/imports_of/tests_for
- **Architecture questions**: `get_architecture_overview` + `list_communities`

Fall back to Grep/Glob/Read **only** when the graph doesn't cover what you need.

### Key Tools

| Tool | Use when |
|------|----------|
| `detect_changes` | Reviewing code changes — gives risk-scored analysis |
| `get_review_context` | Need source snippets for review — token-efficient |
| `get_impact_radius` | Understanding blast radius of a change |
| `get_affected_flows` | Finding which execution paths are impacted |
| `query_graph` | Tracing callers, callees, imports, tests, dependencies |
| `semantic_search_nodes` | Finding functions/classes by name or keyword |
| `get_architecture_overview` | Understanding high-level codebase structure |
| `refactor_tool` | Planning renames, finding dead code |

### Workflow

1. The graph auto-updates on file changes (via hooks).
2. Use `detect_changes` for code review.
3. Use `get_affected_flows` to understand impact.
4. Use `query_graph` pattern="tests_for" to check coverage.

---

## What is This Project

This repository is a **fork of [AGenUI](https://github.com/AGenUI/AGenUI)**. In addition to the upstream A2UI surface-mode rendering, this fork adds **FlexCard** — a view-based card rendering mode designed to avoid the performance problems (frame drops, layout jank) that the surface/waterfall mode causes in feed/list scenarios.

**FlexCard priority:** HarmonyOS and Android are the primary targets. iOS support may be added later if needed. When implementing or reviewing FlexCard-related code, focus effort on `platforms/harmony/` and `platforms/android/` first.

**AGenUI** is an A2UI SDK for building native generative UI across iOS, Android, and HarmonyOS. It implements the [A2UI v0.9 protocol](https://github.com/google/A2UI), rendering LLM-generated streaming UI in real time. The architecture is a **shared C++ core engine** + three platform-specific rendering layers.

## Architecture

```
core/                  ← Shared C++ engine (parser, differ, layout, FunctionCall)
  include/             ← Public C++ headers consumed by platform bridges
  src/
    module/            ← Engine entry, surface manager, thread manager
    stream/            ← Streaming content parser, stream plugins (markdown, text, composite)
    surface/           ← Surface coordinator, message parser, virtual DOM, component manager,
    │                     data values, style defaults, token parser, yoga layout nodes
    function_call/     ← FunctionCall framework, config, resolution, builtins
    style_parser/      ← Color and edge insets CSS-style parsers
    jni/               ← Android JNI bridge layer (compiled into core)

platforms/
  ios/AGenUI/          ← Swift + Obj-C++ bridge (AGenUIEngineBridge.mm, AGenUIEngineSurfaceManagerBridge.mm)
    Classes/Bridge/    ← Obj-C++ JNI-style bridge to the C++ API
    Classes/Render/    ← Native SwiftUI/UIKit component renderers
  android/src/         ← Kotlin/Java JNI bridge + View-based component renderers
  harmony/agenui/      ← ArkTS NAPI bridge + ArkUI component renderers

playground/            ← Standalone demo apps per platform
scripts/               ← Per-platform build scripts
tests/cpp/             ← gtest suite for the C++ core
agent_sdks/go/         ← Go SDK for server-side A2UI agent generation
samples/go/            ← Go sample apps
skills/a2ui-generation/← Agent Skill for LLM-driven A2UI generation
```

### Data flow

1. An LLM streams A2UI JSON to the engine via `AGenUIEngine::feedData`
2. The **stream layer** (`agenui_streaming_content_parser`) extracts A2UI protocol messages incrementally
3. The **surface layer** (`agenui_surface`) parses messages into a virtual DOM, resolves data bindings and design tokens, computes Yoga layout, and diffs against the prior snapshot
4. The engine dispatches render commands to the platform layer via `IAGenUIDispatcher`
5. Platform renderers (Swift/Kotlin/ArkTS) translate dispatched render commands into native views

### Key interfaces (core/include/)

| Header | Purpose |
|--------|---------|
| `agenui_engine.h` | Main engine: `feedData`, `dispatch`, lifecycle |
| `agenui_surface_manager_interface.h` | Create/destroy surfaces |
| `agenui_platform_function.h` | Register platform-side FunctionCall handlers |
| `agenui_message_listener.h` | Callbacks for parsed messages and errors |
| `agenui_logger_interface.h` | Pluggable `IRuntimeLogger` (inject per-platform logging) |
| `agenui_measurement.h` | Platform-provided text/component measurement |

## Build Commands

### C++ Core Tests (host — macOS)

```bash
# Quick: ASan + UBSan (recommended day-to-day)
./tests/cpp/ci/run_tests.sh

# ASan only
./tests/cpp/ci/run_tests.sh --asan-only

# TSan (diagnostic; 11 known races)
./tests/cpp/ci/run_tests.sh --tsan-only

# No sanitizers (fastest)
./tests/cpp/ci/run_tests.sh --no-san

# CMake directly
cmake -S tests/cpp -B tests/cpp/build -j4
cmake --build tests/cpp/build -j4
ctest --test-dir tests/cpp/build --output-on-failure

# Run a single test suite binary
./tests/cpp/build/agenui_unit_tests --gtest_filter='EngineLifecycleTest.*'
```

### Android

```bash
./scripts/android/build.sh                  # Release AAR → dist/android/release/
./scripts/android/build.sh --debug          # Debug AAR
./scripts/android/build.sh --publish-local  # Publish to ~/.m2
./scripts/android/build.sh --clean          # Clean then build
```

Playground: open `playground/android/` in Android Studio. Toggle `agenui.sdk.source=true` in `gradle.properties` to use source directly (supports breakpoints in C++/Java).

### iOS

```bash
./scripts/ios/build.sh                      # XCFramework (Release)
./scripts/ios/build.sh -t framework -c Debug
./scripts/ios/build.sh --pod-install
```

Playground: `cd playground/ios/Playground && pod install && open Playground.xcworkspace`

### HarmonyOS

```bash
./scripts/harmony/build.sh                  # HAR (Release) → default output
./scripts/harmony/build.sh --mode debug
./scripts/harmony/build.sh -o /path/to/output
```

**Playground (always use `scripts/dev.sh` — never run hvigorw or hdc directly):**

```bash
./scripts/dev.sh auto              # build + install + start in one step
./scripts/dev.sh build             # build the playground .hap (debug)
./scripts/dev.sh build --mode release
./scripts/dev.sh install           # install .hap onto connected device
./scripts/dev.sh start             # launch the app on device
./scripts/dev.sh --device <id> auto   # target a specific HDC device
```

> `scripts/dev.sh` handles one-retry on hvigorw/hdc failures (both tools
> occasionally need a second invocation on daemon cold-start or device-connection settle).

> **Do not commit** `playground/harmony/build-profile.json5` or
> `playground/harmony/AppScope/app.json5` — these files contain local signing
> config and bundle name overrides. Both are listed in `.gitignore`.

## Code Style

- **C++**: Google C++ Style Guide
- **Swift**: Google Swift Style Guide + Apple Swift API Design Guidelines
- **Java/Kotlin**: Google Java Style Guide
- **ArkTS**: OpenHarmony coding conventions

## Testing

The C++ test suite lives in `tests/cpp/` and covers ~165 tests across these suites:

| Suite | Location | Coverage |
|-------|----------|---------|
| Integration | `integration/` | Public engine + SurfaceManager API (85+ tests) |
| Unit | `unit/` | Parser, dispatcher, stream, style, function_call (60+ tests) |
| Concurrency | `concurrency/` | Thread-safety + lifecycle races (11 tests) |
| Stress | `stress/` | In-process pressure (~2 tests) |
| Sanitizer | `sanitizer/` | ASan + TSan targeted tests |

Key fixtures and helpers: `tests/cpp/support/` — `ScopedSurfaceManager`, `MockMessageListener`, `WaitForWorkerIdle()`.

To add a new test: create a `.cpp` in the appropriate subdirectory, then re-run CMake configure (globs resolved at configure time).

### TSan known status

The TSan target reports **11 data races** between main thread and the worker thread. These are diagnostic output; they do not gate merges. See `tests/cpp/DESIGN.md §5` for proposed fixes.

## Branch Strategy

| Branch | Purpose |
|--------|---------|
| `main` | Tracks upstream [AGenUI](https://github.com/AGenUI/AGenUI) — sync upstream changes here only |
| `master` | FlexUI development — all FlexCard and fork-specific features land here |

**Rule:** Never develop FlexUI features on `main`. Never merge FlexUI-specific commits back to `main`. To pull in upstream updates, merge/rebase `main` into `master`.

## PR Workflow

1. Branch from `master`: `fix/123-description` or `feat/description`
2. Build and test on affected platforms
3. Open PR against `master` — one maintainer approval required
4. For large changes (new platform, engine refactor, new component category): open an issue first

## Catalog File

`agenui_catalog.json` at repo root is a freestanding A2UI v0.9 catalog (25 components, 14 functions, all `$ref`s inlined). Supply it as `catalogId` in `supportedCatalogIds` when configuring an LLM agent to drive A2UI generation.

## A2UI Generation Skill

`skills/a2ui-generation/` is a standalone Agent Skill for LLM-driven UI generation. Install it with:

```bash
npx skills add AGenUI/AGenUI
```

---

## Coding Guidelines

Behavioral guidelines to reduce common LLM coding mistakes.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

### 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them — don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it — don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: every changed line should trace directly to the user's request.

### 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.
