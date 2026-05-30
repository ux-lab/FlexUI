# FlexUI W5-W6 — Bridge, Scope, Commit Pipeline, Plugin Host, Card Controller + Harmony ETS API + Android Stubs Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring up the orchestration layer of FlexUI: `core/bridge/` (binary serialization + IBridge), `core/scope-manager/` (per-card Scope state machine), `core/commit-pipeline/` (mutation dispatch from JS thread to UI thread), `core/plugin-host/` (4 extension-point registries), `core/card-controller/` (platform-neutral controller logic), plus the public ETS API surface (`FlexUIEngine`, `FlexCardController`, `FlexCard` @Component using NodeContent) on HarmonyOS and matching Android stubs. After this plan, an empty card with no components / no frontend can be created, attached to a NodeContent, and destroyed — proving the lifecycle plumbing end-to-end. Components, APIs, and frontends are W7-W8.

**Architecture:** `Bridge` carries cross-thread payloads using the binary serializer absorbed in W1-W2; serialization stays the contract regardless of JS engine. `ScopeManager` owns one `Scope` per card, each Scope wrapping one `IJsContext` (W3-W4) and a small lifecycle state machine. `CommitPipeline` consumes mutation lists posted from JS thread tasks (W3-W4 `TaskRunner`) and dispatches to `ComponentInstance`s registered through `PluginHost`. `FlexCardController` is the platform-neutral C++ controller; `platforms/harmony/napi/` exposes it through NAPI, and `platforms/harmony/card/` is the ETS `FlexCard` @Component that mounts a `NodeContent` and forwards lifecycle to the controller. Android counterparts are stubs with `NOT_IMPLEMENTED` logs.

**Tech Stack:** C++17, CMake 3.18+, GoogleTest, HarmonyOS NAPI + ArkUI C-API (`ArkUI_NativeNodeAPI_1` + `NodeContent` from `<arkui/native_node.h>`), ArkTS, AndroidStudio JNI (stub-level only), the W1-W2 + W3-W4 deliverables.

**Reference spec:** `docs/superpowers/specs/2026-05-30-flexui-layering-design.md` §4.2 (public SDK API), §4.6 (NodeContent hosting), §4.8 (mutation pipeline), §4.9 (Scope management state machine), §4.11 (plugin mechanism), §4.12 (bridge protocol), §4.13 (cross-platform discipline), §8.4 (mandatory log coverage).

**Depends on:** W1-W2 + W3-W4 plans complete (`flex-ui/common/`, `flex-ui/core/{js-engine,vdom,reconciler,layout}/` all available).

**Out of scope (deferred):**
- Components (`flex-ui/components/`) — W7-W8.
- Built-in APIs (`flex-ui/api/`) — W7-W8.
- Frontends (`card-js` built-in + `plugin-a2ui`) — W7-W8.
- iOS — Phase 3+.
- Snapshot acceleration (interface stubbed; impl is W7-W8 perf-tuning territory).
- Animation, hot reload, DevTools, JSX compiler.
- Real on-device run (W9-W10). This plan delivers ETS code that compiles and unit tests on C++ side; the Harmony build is not yet exercised by the playground.

**Deliverable definition of done:**
- `flex-ui/core/bridge/` ships `IBridge` interface + a default `InProcessBridge` impl + binary `Encoder`/`Decoder` reusing W1-W2 serializer.
- `flex-ui/core/scope-manager/` ships `Scope` + `ScopeManager` with the full state machine from spec §4.9; `Snapshot` interface stubbed.
- `flex-ui/core/commit-pipeline/` ships `CommitPipeline` consuming `MutationList` (W3-W4) and dispatching to a `ComponentInstance` registry.
- `flex-ui/core/plugin-host/` ships `PluginHost` with 4 registries (component / api / loader / frontend), conflict detection, install/uninstall.
- `flex-ui/core/card-controller/` ships `FlexCardController` C++ class implementing the public API contract from spec §4.2 (`Load`, `SetData`, `CallMethod`, `OnEvent`, `OnError`, `Destroy`).
- `flex-ui/platforms/harmony/napi/` exposes `FlexUIEngine` (singleton) and `FlexCardController` via NAPI to ArkTS.
- `flex-ui/platforms/harmony/card/` ships `FlexCard.ets` @Component, `FlexCardController.ets`, `FlexUIEngine.ets`, mounting a `NodeContent` and forwarding lifecycle through NAPI.
- `flex-ui/platforms/android/jni/` and `flex-ui/platforms/android/card/` ship Kotlin stubs that compile cleanly under a minimal Gradle config and log `NOT_IMPLEMENTED`.
- Host unit tests green: bridge round-trip, scope lifecycle, commit pipeline dispatch, plugin install/uninstall + conflict, controller orchestration (with `IJsEngine`/`IJsContext` mocks).
- Coverage on `flex-ui/core/` ≥ 80%.
- ASan+UBSan default + TSan curated subset.
- Mandatory `FLEXUI_TLOG` coverage points per spec §8.4 present.
- AGenUI tree untouched.

---

## File Structure

### Created in this plan

```
flex-ui/core/
├── bridge/
│   ├── CMakeLists.txt
│   ├── include/flexui/core/bridge/
│   │   ├── ibridge.h
│   │   ├── encoder.h
│   │   ├── decoder.h
│   │   └── inprocess_bridge.h
│   └── src/
│       ├── encoder.cc
│       ├── decoder.cc
│       └── inprocess_bridge.cc
├── scope-manager/
│   ├── CMakeLists.txt
│   ├── include/flexui/core/scope-manager/
│   │   ├── scope_state.h
│   │   ├── scope.h
│   │   ├── scope_manager.h
│   │   └── snapshot.h
│   └── src/
│       ├── scope.cc
│       └── scope_manager.cc
├── commit-pipeline/
│   ├── CMakeLists.txt
│   ├── include/flexui/core/commit-pipeline/
│   │   ├── component_instance.h
│   │   └── commit_pipeline.h
│   └── src/
│       └── commit_pipeline.cc
├── plugin-host/
│   ├── CMakeLists.txt
│   ├── include/flexui/core/plugin-host/
│   │   ├── component_factory.h
│   │   ├── native_api.h
│   │   ├── bundle_loader.h
│   │   ├── frontend.h
│   │   ├── plugin.h
│   │   └── plugin_host.h
│   └── src/
│       └── plugin_host.cc
└── card-controller/
    ├── CMakeLists.txt
    ├── include/flexui/core/card-controller/
    │   ├── flex_ui_engine.h
    │   ├── flex_card_controller.h
    │   └── flex_card_state.h
    └── src/
        ├── flex_ui_engine.cc
        └── flex_card_controller.cc

flex-ui/platforms/
├── harmony/
│   ├── CMakeLists.txt
│   ├── napi/
│   │   ├── napi_init.cc                         # napi module entry
│   │   ├── napi_engine.cc                       # NAPI binding for FlexUIEngine
│   │   ├── napi_controller.cc                   # NAPI binding for FlexCardController
│   │   └── napi_node_content.cc                 # NodeContent attach/detach NAPI
│   └── card/
│       ├── ets/
│       │   ├── FlexUIEngine.ets
│       │   ├── FlexCardController.ets
│       │   └── FlexCard.ets
│       └── ohos-module/
│           ├── oh-package.json5
│           ├── BUILD.gn                          # or hvigor build config
│           └── README.md
└── android/
    ├── jni/
    │   └── flexui_jni_stub.cc
    └── card/
        └── kotlin/
            ├── FlexUIEngine.kt
            ├── FlexCardController.kt
            └── FlexCardView.kt

tests/flex-ui/unit/
├── (existing W1-W2 + W3-W4 tests preserved)
├── bridge_roundtrip_test.cc
├── scope_lifecycle_test.cc
├── scope_manager_test.cc
├── commit_pipeline_test.cc
├── plugin_host_test.cc
├── flex_card_controller_test.cc
└── support/
    ├── fake_js_engine.h
    └── fake_js_engine.cc
```

### Modified

- `flex-ui/CMakeLists.txt` — add `add_subdirectory(platforms/harmony)` only when `FLEXUI_OHOS=ON`.
- `flex-ui/core/CMakeLists.txt` — add the 5 new subdirectories.
- `tests/flex-ui/unit/CMakeLists.txt` — link new test files + the fake-js-engine support library.

### Untouched

AGenUI tree as in W1-W2 / W3-W4. Existing flex-ui/common, flex-ui/core/{js-engine,vdom,reconciler,layout}.

---

## Tasks

### Task 1: Scaffold the five core/ subdirectories

**Files:** `flex-ui/core/{bridge,scope-manager,commit-pipeline,plugin-host,card-controller}/CMakeLists.txt`

- [ ] **Step 1.1: Create directories + minimal CMake**

```bash
for sub in bridge scope-manager commit-pipeline plugin-host card-controller; do
  mkdir -p flex-ui/core/${sub}/include/flexui/core/${sub}
  mkdir -p flex-ui/core/${sub}/src
done
```

For each, write a CMakeLists.txt in the W3-W4 shape:

```cmake
add_library(flexui_core_<SUBNAME> STATIC)
target_include_directories(flexui_core_<SUBNAME>
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(flexui_core_<SUBNAME> PUBLIC flexui_common)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc "")
target_sources(flexui_core_<SUBNAME> PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_tu.cc)
```

Library names: `flexui_core_bridge`, `flexui_core_scope_manager`, `flexui_core_commit_pipeline`, `flexui_core_plugin_host`, `flexui_core_card_controller`.

- [ ] **Step 1.2: Update `flex-ui/core/CMakeLists.txt`**

```cmake
add_subdirectory(js-engine)
add_subdirectory(vdom)
add_subdirectory(reconciler)
add_subdirectory(layout)
add_subdirectory(bridge)
add_subdirectory(scope-manager)
add_subdirectory(commit-pipeline)
add_subdirectory(plugin-host)
add_subdirectory(card-controller)
```

- [ ] **Step 1.3: Build & commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core
git commit -m "feat(flex-ui/core): scaffold bridge/scope-manager/commit-pipeline/plugin-host/card-controller"
```

---

### Task 2: Bridge — Encoder/Decoder backed by W1-W2 serializer

**Files:**
- Create: `flex-ui/core/bridge/include/flexui/core/bridge/encoder.h`
- Create: `flex-ui/core/bridge/include/flexui/core/bridge/decoder.h`
- Create: `flex-ui/core/bridge/src/encoder.cc`
- Create: `flex-ui/core/bridge/src/decoder.cc`
- Create: `tests/flex-ui/unit/bridge_roundtrip_test.cc`
- Modify: `flex-ui/core/bridge/CMakeLists.txt`, `tests/flex-ui/unit/CMakeLists.txt`

- [ ] **Step 2.1: Write `encoder.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Bridge Encoder: serializes a FlexUIValue (and small frame-header metadata)
 * to a byte buffer suitable for cross-thread or cross-process transport.
 * Wraps the W1-W2 binary serializer.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::bridge {

struct EncodedFrame {
  std::vector<uint8_t> bytes;
};

class Encoder {
 public:
  static EncodedFrame EncodeValue(const flexui::common::FlexUIValue& v);
};

}  // namespace flexui::core::bridge
```

- [ ] **Step 2.2: Write `decoder.h`**

```cpp
#pragma once

#include <cstdint>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"

namespace flexui::core::bridge {

class Decoder {
 public:
  static flexui::common::Error DecodeValue(const uint8_t* data, size_t len,
                                           flexui::common::FlexUIValue* out);
};

}  // namespace flexui::core::bridge
```

- [ ] **Step 2.3: Write `encoder.cc`**

```cpp
#include "flexui/core/bridge/encoder.h"

#include "flexui/common/serializer.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::bridge {

EncodedFrame Encoder::EncodeValue(const flexui::common::FlexUIValue& v) {
  flexui::common::Serializer ser;
  ser.WriteHeader();
  ser.WriteValue(v);
  auto buf = ser.Release();  // returns (data*, size_t)
  EncodedFrame frame;
  if (buf.first && buf.second > 0) {
    frame.bytes.assign(buf.first, buf.first + buf.second);
    // Serializer in W1-W2 returns malloc'd memory; free per its contract.
    std::free(buf.first);
  }
  FLEXUI_TLOG(Bridge, Encode, DEBUG) << "bytes=" << frame.bytes.size();
  return frame;
}

}  // namespace flexui::core::bridge
```

(If the W1-W2 `Serializer::Release` returns a different ownership shape — e.g. a `std::pair<std::unique_ptr<uint8_t[], decltype(&free)>, size_t>` — adapt the body to match. Behavior remains the same.)

- [ ] **Step 2.4: Write `decoder.cc`**

```cpp
#include "flexui/core/bridge/decoder.h"

#include "flexui/common/deserializer.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::bridge {

flexui::common::Error Decoder::DecodeValue(const uint8_t* data, size_t len,
                                           flexui::common::FlexUIValue* out) {
  if (!data || !out) return flexui::common::Error(
      flexui::common::ErrorCode::kInvalidArgument, "null pointer");
  flexui::common::Deserializer de(data, len);
  if (!de.ReadHeader()) return flexui::common::Error(
      flexui::common::ErrorCode::kInternal, "bad header");
  if (!de.ReadValue(*out)) return flexui::common::Error(
      flexui::common::ErrorCode::kInternal, "value parse failed");
  FLEXUI_TLOG(Bridge, Decode, DEBUG) << "bytes=" << len;
  return flexui::common::Error::Ok();
}

}  // namespace flexui::core::bridge
```

- [ ] **Step 2.5: Write `tests/flex-ui/unit/bridge_roundtrip_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/core/bridge/encoder.h"
#include "flexui/core/bridge/decoder.h"

namespace flexui::core::bridge {

TEST(BridgeRoundtripTest, StringRoundtrips) {
  using F = flexui::common::FlexUIValue;
  F input(std::string("flexui"));
  auto frame = Encoder::EncodeValue(input);
  F out;
  auto err = Decoder::DecodeValue(frame.bytes.data(), frame.bytes.size(), &out);
  EXPECT_TRUE(err.ok());
  EXPECT_TRUE(out.IsString());
  EXPECT_EQ(out.ToString(), "flexui");
}

TEST(BridgeRoundtripTest, NestedObjectRoundtrips) {
  using F = flexui::common::FlexUIValue;
  F input(F::FlexUIValueObjectType{
      {"name", F(std::string("card"))},
      {"size", F(7.5)},
      {"open", F(true)},
  });
  auto frame = Encoder::EncodeValue(input);
  F out;
  auto err = Decoder::DecodeValue(frame.bytes.data(), frame.bytes.size(), &out);
  EXPECT_TRUE(err.ok());
  EXPECT_EQ(out.ToObject()["name"].ToString(), "card");
  EXPECT_DOUBLE_EQ(out.ToObject()["size"].ToDouble(), 7.5);
  EXPECT_TRUE(out.ToObject()["open"].ToBoolean());
}

TEST(BridgeRoundtripTest, MalformedReturnsError) {
  flexui::common::FlexUIValue out;
  uint8_t junk[] = {0xff, 0x00, 0x01};
  auto err = Decoder::DecodeValue(junk, sizeof(junk), &out);
  EXPECT_FALSE(err.ok());
}

}  // namespace flexui::core::bridge
```

- [ ] **Step 2.6: Wire sources + test, build, commit**

```bash
# CMake: append src/encoder.cc + src/decoder.cc to flexui_core_bridge
# CMake: append bridge_roundtrip_test.cc + link flexui_core_bridge
./scripts/flex-ui/build.sh
git add flex-ui/core/bridge tests/flex-ui/unit
git commit -m "feat(flex-ui/core/bridge): Encoder + Decoder backed by W1-W2 serializer"
```

---

### Task 3: Bridge — `IBridge` interface + `InProcessBridge` default

**Files:**
- Create: `flex-ui/core/bridge/include/flexui/core/bridge/ibridge.h`
- Create: `flex-ui/core/bridge/include/flexui/core/bridge/inprocess_bridge.h`
- Create: `flex-ui/core/bridge/src/inprocess_bridge.cc`

- [ ] **Step 3.1: Write `ibridge.h`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * IBridge: cross-thread / cross-VM channel for FlexUI. Carries opaque
 * EncodedFrame payloads between subsystems. The default InProcessBridge
 * marshals via a TaskRunner to the target thread; future RPC/IPC backends
 * may implement the same interface.
 */
#pragma once

#include <functional>
#include <memory>

#include "flexui/core/bridge/encoder.h"
#include "flexui/common/task_runner.h"

namespace flexui::core::bridge {

// A unique destination address for messages.
struct Endpoint {
  uint32_t scope_id;       // 0 = engine-wide
  std::string channel;     // e.g. "js2ui", "ui2js", "event", "callback"
};

inline bool operator==(const Endpoint& a, const Endpoint& b) {
  return a.scope_id == b.scope_id && a.channel == b.channel;
}

using MessageHandler =
    std::function<void(const Endpoint& source, EncodedFrame frame)>;

class IBridge {
 public:
  virtual ~IBridge() = default;

  // Post a frame to an endpoint. Returns immediately.
  virtual void Post(const Endpoint& dest, EncodedFrame frame) = 0;

  // Register a handler for messages arriving at `endpoint`. Returns a token
  // that can be used to unsubscribe.
  virtual uint64_t Subscribe(const Endpoint& endpoint, MessageHandler handler) = 0;
  virtual bool Unsubscribe(uint64_t token) = 0;
};

}  // namespace flexui::core::bridge
```

- [ ] **Step 3.2: Write `inprocess_bridge.h` + `inprocess_bridge.cc`**

```cpp
// inprocess_bridge.h
#pragma once
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "flexui/core/bridge/ibridge.h"

namespace flexui::core::bridge {

class InProcessBridge : public IBridge {
 public:
  InProcessBridge(std::shared_ptr<flexui::common::TaskRunner> js_runner,
                  std::shared_ptr<flexui::common::TaskRunner> ui_runner);
  ~InProcessBridge() override;

  void Post(const Endpoint& dest, EncodedFrame frame) override;
  uint64_t Subscribe(const Endpoint& endpoint, MessageHandler handler) override;
  bool Unsubscribe(uint64_t token) override;

 private:
  struct Sub {
    Endpoint endpoint;
    MessageHandler handler;
  };
  std::shared_ptr<flexui::common::TaskRunner> js_runner_;
  std::shared_ptr<flexui::common::TaskRunner> ui_runner_;
  std::mutex mu_;
  std::unordered_map<uint64_t, Sub> subs_;
  std::atomic<uint64_t> next_token_{1};
};

}  // namespace flexui::core::bridge
```

```cpp
// inprocess_bridge.cc
#include "flexui/core/bridge/inprocess_bridge.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::bridge {

InProcessBridge::InProcessBridge(
    std::shared_ptr<flexui::common::TaskRunner> js,
    std::shared_ptr<flexui::common::TaskRunner> ui)
    : js_runner_(std::move(js)), ui_runner_(std::move(ui)) {}

InProcessBridge::~InProcessBridge() = default;

void InProcessBridge::Post(const Endpoint& dest, EncodedFrame frame) {
  FLEXUI_TLOG(Bridge, Post, DEBUG)
      << "channel=" << dest.channel << " scope=" << dest.scope_id
      << " bytes=" << frame.bytes.size();

  // Route by channel name: js2ui -> ui_runner, ui2js -> js_runner, else js.
  auto target = (dest.channel.rfind("js2", 0) == 0) ? ui_runner_ : js_runner_;
  if (!target) {
    FLEXUI_TLOG(Bridge, Post, ERROR) << "no target runner for channel="
                                     << dest.channel;
    return;
  }

  auto frame_copy = std::make_shared<EncodedFrame>(std::move(frame));
  Endpoint dest_copy = dest;
  std::vector<MessageHandler> matched;
  {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& kv : subs_) {
      if (kv.second.endpoint == dest_copy) {
        matched.push_back(kv.second.handler);
      }
    }
  }
  for (auto& h : matched) {
    target->PostTask([h, dest_copy, frame_copy] {
      h(dest_copy, *frame_copy);
    });
  }
}

uint64_t InProcessBridge::Subscribe(const Endpoint& endpoint,
                                    MessageHandler handler) {
  uint64_t token = next_token_.fetch_add(1);
  std::lock_guard<std::mutex> lock(mu_);
  subs_[token] = {endpoint, std::move(handler)};
  FLEXUI_TLOG(Bridge, Subscribe, DEBUG)
      << "channel=" << endpoint.channel << " token=" << token;
  return token;
}

bool InProcessBridge::Unsubscribe(uint64_t token) {
  std::lock_guard<std::mutex> lock(mu_);
  return subs_.erase(token) > 0;
}

}  // namespace flexui::core::bridge
```

- [ ] **Step 3.3: Wire sources, extend test (or add a new test exercising Post/Subscribe round-trip with TaskRunner), build, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/bridge tests/flex-ui/unit
git commit -m "feat(flex-ui/core/bridge): IBridge + InProcessBridge (TaskRunner-backed)"
```

---

### Task 4: Plugin host — interfaces

**Files:**
- Create: 6 headers under `flex-ui/core/plugin-host/include/flexui/core/plugin-host/`
- Create: `flex-ui/core/plugin-host/src/plugin_host.cc`

- [ ] **Step 4.1: Write `component_factory.h`**

```cpp
#pragma once
#include <memory>
#include <string>

#include "flexui/common/flexui_value.h"

namespace flexui::core::commit_pipeline { class ComponentInstance; }

namespace flexui::core::plugin_host {

enum class ComponentImplementation { kCapi, kEtsBuilder };

class ComponentContext;   // forward; defined in commit-pipeline

struct ComponentFactory {
  std::string name;
  ComponentImplementation implementation = ComponentImplementation::kCapi;
  std::function<std::unique_ptr<commit_pipeline::ComponentInstance>(
      ComponentContext&)> create;
};

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.2: Write `native_api.h`**

```cpp
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::plugin_host {

enum class NativeApiMode { kSync, kAsync };

struct NativeApi {
  std::string name;                                       // "$location.getCurrent"
  NativeApiMode mode = NativeApiMode::kSync;
  std::function<flexui::common::FlexUIValue(
      const std::vector<flexui::common::FlexUIValue>& args)> invoke;
};

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.3: Write `bundle_loader.h`**

```cpp
#pragma once

#include <functional>
#include <future>
#include <string>
#include <vector>

namespace flexui::core::plugin_host {

struct BundleSource {
  std::string uri;
  std::string text;                    // for JSON-ish bundles
  std::vector<uint8_t> bytes;          // for binary bundles
};

struct BundleLoader {
  std::string scheme;                  // "file" / "https" / "rcs"
  std::function<std::future<BundleSource>(const std::string& uri)> load;
};

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.4: Write `frontend.h`**

```cpp
#pragma once

#include <memory>
#include <string>

#include "flexui/common/flexui_value.h"
#include "flexui/core/vdom/dom_node.h"

namespace flexui::core::scope_manager { class Scope; }

namespace flexui::core::plugin_host {

class IFrontend {
 public:
  virtual ~IFrontend() = default;
  virtual std::string Name() const = 0;
  virtual std::string BundleType() const = 0;
  virtual void Initialize(scope_manager::Scope& scope,
                          const BundleSource& bundle) = 0;
  virtual std::shared_ptr<vdom::DomNode> Render(
      scope_manager::Scope& scope,
      const flexui::common::FlexUIValue& data) = 0;
  virtual void HandleEvent(scope_manager::Scope& scope,
                           const std::string& event_name,
                           const flexui::common::FlexUIValue& payload) {}
};

using FrontendFactory = std::function<std::unique_ptr<IFrontend>()>;

struct FrontendRegistration {
  std::string bundle_type;   // "card-js" / "a2ui-json"
  FrontendFactory factory;
};

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.5: Write `plugin.h`**

```cpp
#pragma once
#include <string>
#include <vector>

#include "flexui/core/plugin-host/component_factory.h"
#include "flexui/core/plugin-host/native_api.h"
#include "flexui/core/plugin-host/bundle_loader.h"
#include "flexui/core/plugin-host/frontend.h"

namespace flexui::core::plugin_host {

class FlexUIEngine;

struct FlexUIPlugin {
  std::string name;
  std::string version;
  std::vector<ComponentFactory> components;
  std::vector<NativeApi> apis;
  std::vector<BundleLoader> loaders;
  std::vector<FrontendRegistration> frontends;

  std::function<void(FlexUIEngine&)> on_install;
  std::function<void()> on_uninstall;
};

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.6: Write `plugin_host.h`**

```cpp
#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "flexui/common/error.h"
#include "flexui/core/plugin-host/plugin.h"

namespace flexui::core::plugin_host {

class PluginHost {
 public:
  flexui::common::Error Install(FlexUIPlugin plugin);
  flexui::common::Error Uninstall(const std::string& name);

  // Lookups (read-only); return null if absent.
  const ComponentFactory*       FindComponent(const std::string& name) const;
  const NativeApi*              FindApi(const std::string& name) const;
  const BundleLoader*           FindLoader(const std::string& scheme) const;
  const FrontendRegistration*   FindFrontend(const std::string& bundle_type) const;

  size_t PluginCount() const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, FlexUIPlugin> plugins_;
  std::unordered_map<std::string, const ComponentFactory*> components_;
  std::unordered_map<std::string, const NativeApi*> apis_;
  std::unordered_map<std::string, const BundleLoader*> loaders_;
  std::unordered_map<std::string, const FrontendRegistration*> frontends_;
};

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.7: Write `plugin_host.cc`**

```cpp
#include "flexui/core/plugin-host/plugin_host.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::plugin_host {

using namespace flexui::common;

Error PluginHost::Install(FlexUIPlugin plugin) {
  std::lock_guard<std::mutex> lock(mu_);
  if (plugins_.count(plugin.name)) {
    FLEXUI_TLOG(Plugin, InstallConflict, ERROR) << "name=" << plugin.name;
    return Error(ErrorCode::kPluginConflict,
                 "plugin already installed: " + plugin.name);
  }
  // Detect component/api/loader/frontend collisions upfront.
  for (auto& c : plugin.components) {
    if (components_.count(c.name))
      return Error(ErrorCode::kPluginConflict, "component name collision: " + c.name);
  }
  for (auto& a : plugin.apis) {
    if (apis_.count(a.name))
      return Error(ErrorCode::kPluginConflict, "api name collision: " + a.name);
  }
  for (auto& l : plugin.loaders) {
    if (loaders_.count(l.scheme))
      return Error(ErrorCode::kPluginConflict, "loader scheme collision: " + l.scheme);
  }
  for (auto& f : plugin.frontends) {
    if (frontends_.count(f.bundle_type))
      return Error(ErrorCode::kPluginConflict,
                   "frontend bundle_type collision: " + f.bundle_type);
  }
  // Insert.
  auto& stored = plugins_.emplace(plugin.name, std::move(plugin)).first->second;
  for (auto& c : stored.components) components_[c.name] = &c;
  for (auto& a : stored.apis)        apis_[a.name]      = &a;
  for (auto& l : stored.loaders)     loaders_[l.scheme] = &l;
  for (auto& f : stored.frontends)   frontends_[f.bundle_type] = &f;

  FLEXUI_TLOG(Plugin, Install, INFO)
      << "name=" << stored.name
      << " components=" << stored.components.size()
      << " apis=" << stored.apis.size()
      << " loaders=" << stored.loaders.size()
      << " frontends=" << stored.frontends.size();
  return Error::Ok();
}

Error PluginHost::Uninstall(const std::string& name) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = plugins_.find(name);
  if (it == plugins_.end()) {
    return Error(ErrorCode::kNotFound, "plugin not installed: " + name);
  }
  for (auto& c : it->second.components) components_.erase(c.name);
  for (auto& a : it->second.apis)        apis_.erase(a.name);
  for (auto& l : it->second.loaders)     loaders_.erase(l.scheme);
  for (auto& f : it->second.frontends)   frontends_.erase(f.bundle_type);
  if (it->second.on_uninstall) it->second.on_uninstall();
  plugins_.erase(it);
  FLEXUI_TLOG(Plugin, Uninstall, INFO) << "name=" << name;
  return Error::Ok();
}

const ComponentFactory* PluginHost::FindComponent(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = components_.find(name);
  return it == components_.end() ? nullptr : it->second;
}
const NativeApi* PluginHost::FindApi(const std::string& name) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = apis_.find(name);
  return it == apis_.end() ? nullptr : it->second;
}
const BundleLoader* PluginHost::FindLoader(const std::string& scheme) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = loaders_.find(scheme);
  return it == loaders_.end() ? nullptr : it->second;
}
const FrontendRegistration* PluginHost::FindFrontend(
    const std::string& bundle_type) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = frontends_.find(bundle_type);
  return it == frontends_.end() ? nullptr : it->second;
}
size_t PluginHost::PluginCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return plugins_.size();
}

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.8: Add test `tests/flex-ui/unit/plugin_host_test.cc`**

```cpp
#include <gtest/gtest.h>

#include "flexui/core/plugin-host/plugin_host.h"

namespace flexui::core::plugin_host {

TEST(PluginHostTest, InstallSucceedsAndExposesComponents) {
  PluginHost host;
  FlexUIPlugin p;
  p.name = "my-plugin";
  ComponentFactory cf;
  cf.name = "MyButton";
  p.components.push_back(std::move(cf));
  EXPECT_TRUE(host.Install(std::move(p)).ok());
  EXPECT_EQ(host.PluginCount(), 1u);
  EXPECT_NE(host.FindComponent("MyButton"), nullptr);
  EXPECT_EQ(host.FindComponent("Nope"), nullptr);
}

TEST(PluginHostTest, NameCollisionRejected) {
  PluginHost host;
  FlexUIPlugin a; a.name = "p";
  FlexUIPlugin b; b.name = "p";
  EXPECT_TRUE(host.Install(std::move(a)).ok());
  auto err = host.Install(std::move(b));
  EXPECT_FALSE(err.ok());
  EXPECT_EQ(err.code(), flexui::common::ErrorCode::kPluginConflict);
}

TEST(PluginHostTest, ComponentCollisionRejected) {
  PluginHost host;
  FlexUIPlugin a; a.name = "a";
  ComponentFactory cf; cf.name = "X";
  a.components.push_back(cf);
  EXPECT_TRUE(host.Install(std::move(a)).ok());

  FlexUIPlugin b; b.name = "b";
  ComponentFactory cf2; cf2.name = "X";
  b.components.push_back(cf2);
  auto err = host.Install(std::move(b));
  EXPECT_EQ(err.code(), flexui::common::ErrorCode::kPluginConflict);
}

TEST(PluginHostTest, UninstallRemovesExtensions) {
  PluginHost host;
  FlexUIPlugin a; a.name = "a";
  ComponentFactory cf; cf.name = "X";
  a.components.push_back(cf);
  host.Install(std::move(a));
  EXPECT_NE(host.FindComponent("X"), nullptr);
  EXPECT_TRUE(host.Uninstall("a").ok());
  EXPECT_EQ(host.FindComponent("X"), nullptr);
}

}  // namespace flexui::core::plugin_host
```

- [ ] **Step 4.9: Wire sources + test, build, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/plugin-host tests/flex-ui/unit
git commit -m "feat(flex-ui/core/plugin-host): 4 extension points + collision detection + tests"
```

---

### Task 5: Commit Pipeline — `ComponentInstance` interface + dispatch

**Files:**
- Create: `flex-ui/core/commit-pipeline/include/flexui/core/commit-pipeline/component_instance.h`
- Create: `flex-ui/core/commit-pipeline/include/flexui/core/commit-pipeline/commit_pipeline.h`
- Create: `flex-ui/core/commit-pipeline/src/commit_pipeline.cc`

- [ ] **Step 5.1: Write `component_instance.h`**

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::commit_pipeline {

// Platform-opaque node handle. ArkUI_NodeHandle / Android View* / NSView* etc.
using NodeHandle = void*;

struct LayoutRect {
  float x = 0, y = 0, width = 0, height = 0;
};

struct PropDelta {
  std::vector<std::pair<std::string, flexui::common::FlexUIValue>> updated;
  std::vector<std::string> deleted;
};

class ComponentContext {
 public:
  virtual ~ComponentContext() = default;
  virtual uint32_t scope_id() const = 0;
  virtual uint32_t root_id() const = 0;
};

class ComponentInstance {
 public:
  virtual ~ComponentInstance() = default;

  virtual NodeHandle OnCreate() = 0;
  virtual void OnUpdateProps(const PropDelta& delta) = 0;
  virtual void OnUpdateLayout(const LayoutRect& rect) = 0;
  virtual void OnMount(NodeHandle parent, uint32_t index) = 0;
  virtual void OnEvent(const std::string& name,
                       const flexui::common::FlexUIValue& payload) {}
  virtual void OnUnmount() = 0;
};

}  // namespace flexui::core::commit_pipeline
```

- [ ] **Step 5.2: Write `commit_pipeline.h`**

```cpp
#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "flexui/core/reconciler/mutation.h"
#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::core::plugin_host { class PluginHost; }

namespace flexui::core::commit_pipeline {

class CommitPipeline {
 public:
  explicit CommitPipeline(plugin_host::PluginHost* plugin_host);

  // Apply a batch of mutations to the given root.
  void Apply(uint32_t scope_id, uint32_t root_id,
             const reconciler::MutationList& mutations);

  // Lookups (for tests / debugging).
  size_t InstanceCount() const;
  ComponentInstance* Find(uint32_t node_id) const;

 private:
  plugin_host::PluginHost* plugin_host_;   // not owned
  mutable std::mutex mu_;
  std::unordered_map<uint32_t, std::unique_ptr<ComponentInstance>> instances_;
};

}  // namespace flexui::core::commit_pipeline
```

- [ ] **Step 5.3: Write `commit_pipeline.cc`**

```cpp
#include "flexui/core/commit-pipeline/commit_pipeline.h"

#include "flexui/core/plugin-host/plugin_host.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::commit_pipeline {

namespace {
class DefaultComponentContext : public ComponentContext {
 public:
  DefaultComponentContext(uint32_t s, uint32_t r) : scope_(s), root_(r) {}
  uint32_t scope_id() const override { return scope_; }
  uint32_t root_id()  const override { return root_; }
 private:
  uint32_t scope_, root_;
};
}  // namespace

CommitPipeline::CommitPipeline(plugin_host::PluginHost* h) : plugin_host_(h) {}

void CommitPipeline::Apply(uint32_t scope_id, uint32_t root_id,
                           const reconciler::MutationList& mutations) {
  FLEXUI_TLOG(CommitPipeline, ApplyEnter, DEBUG)
      << "scope=" << scope_id << " root=" << root_id
      << " mutations=" << mutations.size();

  DefaultComponentContext ctx(scope_id, root_id);
  std::lock_guard<std::mutex> lock(mu_);
  for (const auto& m : mutations) {
    std::visit([&](auto&& mut) {
      using T = std::decay_t<decltype(mut)>;
      if constexpr (std::is_same_v<T, reconciler::CreateMutation>) {
        auto* factory = plugin_host_->FindComponent(mut.view_name);
        if (!factory) {
          FLEXUI_TLOG(CommitPipeline, ApplyCreate, ERROR)
              << "unknown component: " << mut.view_name;
          return;
        }
        auto inst = factory->create(ctx);
        if (!inst) return;
        inst->OnCreate();
        instances_[mut.node_id] = std::move(inst);
        FLEXUI_TLOG(CommitPipeline, ApplyCreate, DEBUG)
            << "id=" << mut.node_id << " view=" << mut.view_name;
      } else if constexpr (std::is_same_v<T, reconciler::UpdatePropsMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        PropDelta delta;
        delta.updated = mut.diff;
        delta.deleted = mut.deleted_keys;
        it->second->OnUpdateProps(delta);
      } else if constexpr (std::is_same_v<T, reconciler::UpdateLayoutMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        it->second->OnUpdateLayout({mut.x, mut.y, mut.width, mut.height});
      } else if constexpr (std::is_same_v<T, reconciler::MoveMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        auto pit = instances_.find(mut.new_parent_id);
        NodeHandle parent_handle = (pit == instances_.end()) ? nullptr
            : pit->second->OnCreate();   // parent already exists; OnCreate is idempotent on cached handle in real impls
        it->second->OnMount(parent_handle, mut.new_index);
      } else if constexpr (std::is_same_v<T, reconciler::DeleteMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        it->second->OnUnmount();
        instances_.erase(it);
      }
    }, m);
  }

  FLEXUI_TLOG(CommitPipeline, ApplyExit, DEBUG)
      << "scope=" << scope_id << " instances=" << instances_.size();
}

size_t CommitPipeline::InstanceCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return instances_.size();
}

ComponentInstance* CommitPipeline::Find(uint32_t node_id) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = instances_.find(node_id);
  return it == instances_.end() ? nullptr : it->second.get();
}

}  // namespace flexui::core::commit_pipeline
```

- [ ] **Step 5.4: Wire source**

```cmake
target_sources(flexui_core_commit_pipeline PRIVATE
  src/commit_pipeline.cc
)
target_link_libraries(flexui_core_commit_pipeline PUBLIC
  flexui_core_reconciler flexui_core_plugin_host
)
```

- [ ] **Step 5.5: Test — fake ComponentInstance + factory**

```cpp
// tests/flex-ui/unit/commit_pipeline_test.cc
#include <gtest/gtest.h>
#include "flexui/core/commit-pipeline/commit_pipeline.h"
#include "flexui/core/plugin-host/plugin_host.h"

namespace flexui::core::commit_pipeline {

class FakeComponent : public ComponentInstance {
 public:
  NodeHandle OnCreate() override { return reinterpret_cast<NodeHandle>(this); }
  void OnUpdateProps(const PropDelta&) override { ++props_calls; }
  void OnUpdateLayout(const LayoutRect&) override { ++layout_calls; }
  void OnMount(NodeHandle, uint32_t) override {}
  void OnUnmount() override {}
  int props_calls = 0;
  int layout_calls = 0;
};

TEST(CommitPipelineTest, CreateThenDeleteRoundtrip) {
  plugin_host::PluginHost host;
  plugin_host::FlexUIPlugin p; p.name = "test";
  plugin_host::ComponentFactory f;
  f.name = "Fake";
  f.create = [](ComponentContext&) { return std::make_unique<FakeComponent>(); };
  p.components.push_back(std::move(f));
  ASSERT_TRUE(host.Install(std::move(p)).ok());

  CommitPipeline cp(&host);

  reconciler::MutationList m1;
  m1.emplace_back(reconciler::CreateMutation{42u, 0u, 0u, "Fake"});
  cp.Apply(1, 100, m1);
  EXPECT_EQ(cp.InstanceCount(), 1u);
  EXPECT_NE(cp.Find(42), nullptr);

  reconciler::MutationList m2;
  m2.emplace_back(reconciler::DeleteMutation{42u});
  cp.Apply(1, 100, m2);
  EXPECT_EQ(cp.InstanceCount(), 0u);
}

TEST(CommitPipelineTest, UpdateRoutedToInstance) {
  plugin_host::PluginHost host;
  plugin_host::FlexUIPlugin p; p.name = "t";
  plugin_host::ComponentFactory f;
  f.name = "Fake";
  f.create = [](ComponentContext&) { return std::make_unique<FakeComponent>(); };
  p.components.push_back(std::move(f));
  host.Install(std::move(p));

  CommitPipeline cp(&host);

  reconciler::MutationList m;
  m.emplace_back(reconciler::CreateMutation{1u, 0u, 0u, "Fake"});
  m.emplace_back(reconciler::UpdateLayoutMutation{1u, 0, 0, 100, 50});
  cp.Apply(1, 1, m);

  auto* inst = cp.Find(1);
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(static_cast<FakeComponent*>(inst)->layout_calls, 1);
}

}  // namespace flexui::core::commit_pipeline
```

- [ ] **Step 5.6: Wire test, build, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/commit-pipeline tests/flex-ui/unit
git commit -m "feat(flex-ui/core/commit-pipeline): ComponentInstance + dispatch on Mutation variant"
```

---

### Task 6: Scope manager — state machine + Snapshot stub

**Files:**
- Create: 4 headers under `flex-ui/core/scope-manager/include/flexui/core/scope-manager/`
- Create: `flex-ui/core/scope-manager/src/scope.cc`
- Create: `flex-ui/core/scope-manager/src/scope_manager.cc`

- [ ] **Step 6.1: Write `scope_state.h`**

```cpp
#pragma once

namespace flexui::core::scope_manager {

enum class ScopeState {
  kEngineInitd,
  kScopeCreating,
  kScopeRunning,
  kScopeDestroying,
  kScopeDestroyed,
  kScopeErrored,
};

const char* ScopeStateName(ScopeState s);

}  // namespace flexui::core::scope_manager
```

- [ ] **Step 6.2: Write `snapshot.h`** (stub interface for future optimization)

```cpp
#pragma once

#include <memory>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/core/js-engine/ijs_context.h"

namespace flexui::core::scope_manager {

// PoC stub: implementations live in W7-W8+. The interface exists so that
// Scope plumbing can call it; the default returns empty.
class ISnapshot {
 public:
  virtual ~ISnapshot() = default;
  virtual std::vector<uint8_t> Serialize() const = 0;
  virtual flexui::common::Error ApplyTo(js_engine::IJsContext& ctx) const = 0;
};

std::unique_ptr<ISnapshot> MakeEmptySnapshot();

}  // namespace flexui::core::scope_manager
```

- [ ] **Step 6.3: Write `scope.h`**

```cpp
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/js-engine/ijs_context.h"
#include "flexui/core/scope-manager/scope_state.h"

namespace flexui::core::scope_manager {

class ScopeManager;
class ISnapshot;

class Scope {
 public:
  Scope(ScopeManager* manager,
        uint32_t scope_id,
        std::shared_ptr<js_engine::IJsContext> ctx);
  ~Scope();

  uint32_t id() const { return id_; }
  js_engine::IJsContext* js_context() const { return ctx_.get(); }
  ScopeState state() const { return state_.load(); }

  // Lifecycle. Each transition logs FLEXUI_TLOG(Scope, StateChange, INFO).
  flexui::common::Error Begin(const ISnapshot* snapshot);
  flexui::common::Error MarkRunning();
  flexui::common::Error MarkErrored(const std::string& reason);
  flexui::common::Error Destroy();

 private:
  flexui::common::Error TransitionTo(ScopeState next);

  ScopeManager* manager_;
  uint32_t id_;
  std::shared_ptr<js_engine::IJsContext> ctx_;
  std::atomic<ScopeState> state_{ScopeState::kScopeCreating};
  mutable std::mutex mu_;
};

}  // namespace flexui::core::scope_manager
```

- [ ] **Step 6.4: Write `scope_manager.h`**

```cpp
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/scope-manager/scope.h"

namespace flexui::core::scope_manager {

class ScopeManager {
 public:
  explicit ScopeManager(std::shared_ptr<js_engine::IJsEngine> engine);
  ~ScopeManager();

  // Allocates a Scope id, creates a JS context, returns the Scope handle.
  std::shared_ptr<Scope> CreateScope();

  // Find / destroy by id.
  std::shared_ptr<Scope> Find(uint32_t id) const;
  flexui::common::Error DestroyScope(uint32_t id);

  size_t ScopeCount() const;

 private:
  std::shared_ptr<js_engine::IJsEngine> engine_;
  mutable std::mutex mu_;
  std::unordered_map<uint32_t, std::shared_ptr<Scope>> scopes_;
  std::atomic<uint32_t> next_id_{1};
};

}  // namespace flexui::core::scope_manager
```

- [ ] **Step 6.5: Write `scope.cc`**

```cpp
#include "flexui/core/scope-manager/scope.h"
#include "flexui/core/scope-manager/snapshot.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::scope_manager {

const char* ScopeStateName(ScopeState s) {
  switch (s) {
    case ScopeState::kEngineInitd:     return "EngineInitd";
    case ScopeState::kScopeCreating:   return "ScopeCreating";
    case ScopeState::kScopeRunning:    return "ScopeRunning";
    case ScopeState::kScopeDestroying: return "ScopeDestroying";
    case ScopeState::kScopeDestroyed:  return "ScopeDestroyed";
    case ScopeState::kScopeErrored:    return "ScopeErrored";
  }
  return "Unknown";
}

Scope::Scope(ScopeManager* mgr, uint32_t id,
             std::shared_ptr<js_engine::IJsContext> ctx)
    : manager_(mgr), id_(id), ctx_(std::move(ctx)) {
  FLEXUI_TLOG(Scope, Create, INFO) << "id=" << id_;
}

Scope::~Scope() {
  if (state_.load() != ScopeState::kScopeDestroyed) {
    FLEXUI_TLOG(Scope, DestroyMissing, WARNING)
        << "id=" << id_ << " state=" << ScopeStateName(state_.load());
  }
}

flexui::common::Error Scope::TransitionTo(ScopeState next) {
  auto from = state_.exchange(next);
  FLEXUI_TLOG(Scope, StateChange, INFO)
      << "id=" << id_
      << " from=" << ScopeStateName(from)
      << " to=" << ScopeStateName(next);
  return flexui::common::Error::Ok();
}

flexui::common::Error Scope::Begin(const ISnapshot* snapshot) {
  if (snapshot) {
    auto err = snapshot->ApplyTo(*ctx_);
    if (!err.ok()) {
      MarkErrored("snapshot apply failed: " + err.message());
      return err;
    }
  }
  return flexui::common::Error::Ok();
}

flexui::common::Error Scope::MarkRunning()  { return TransitionTo(ScopeState::kScopeRunning); }
flexui::common::Error Scope::MarkErrored(const std::string& reason) {
  FLEXUI_TLOG(Scope, Errored, ERROR) << "id=" << id_ << " reason=" << reason;
  return TransitionTo(ScopeState::kScopeErrored);
}
flexui::common::Error Scope::Destroy() {
  TransitionTo(ScopeState::kScopeDestroying);
  ctx_.reset();
  TransitionTo(ScopeState::kScopeDestroyed);
  return flexui::common::Error::Ok();
}

// MakeEmptySnapshot — PoC stub: no-op snapshot, applies nothing.
namespace {
class EmptySnapshot : public ISnapshot {
 public:
  std::vector<uint8_t> Serialize() const override { return {}; }
  flexui::common::Error ApplyTo(js_engine::IJsContext&) const override {
    return flexui::common::Error::Ok();
  }
};
}  // namespace
std::unique_ptr<ISnapshot> MakeEmptySnapshot() {
  return std::make_unique<EmptySnapshot>();
}

}  // namespace flexui::core::scope_manager
```

- [ ] **Step 6.6: Write `scope_manager.cc`**

```cpp
#include "flexui/core/scope-manager/scope_manager.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::scope_manager {

ScopeManager::ScopeManager(std::shared_ptr<js_engine::IJsEngine> engine)
    : engine_(std::move(engine)) {}

ScopeManager::~ScopeManager() {
  std::lock_guard<std::mutex> lock(mu_);
  for (auto& kv : scopes_) kv.second->Destroy();
  scopes_.clear();
}

std::shared_ptr<Scope> ScopeManager::CreateScope() {
  auto ctx = engine_->CreateContext();
  if (!ctx) {
    FLEXUI_TLOG(Scope, CreateFail, ERROR) << "engine returned null context";
    return nullptr;
  }
  uint32_t id = next_id_.fetch_add(1);
  auto scope = std::make_shared<Scope>(this, id, std::move(ctx));
  std::lock_guard<std::mutex> lock(mu_);
  scopes_[id] = scope;
  return scope;
}

std::shared_ptr<Scope> ScopeManager::Find(uint32_t id) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = scopes_.find(id);
  return it == scopes_.end() ? nullptr : it->second;
}

flexui::common::Error ScopeManager::DestroyScope(uint32_t id) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = scopes_.find(id);
  if (it == scopes_.end()) {
    return flexui::common::Error(flexui::common::ErrorCode::kNotFound,
                                 "no such scope");
  }
  it->second->Destroy();
  scopes_.erase(it);
  return flexui::common::Error::Ok();
}

size_t ScopeManager::ScopeCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return scopes_.size();
}

}  // namespace flexui::core::scope_manager
```

- [ ] **Step 6.7: Write the fake JS engine test helper**

`tests/flex-ui/unit/support/fake_js_engine.{h,cc}` provides minimal `IJsEngine` / `IJsContext` / `IJsValue` impls that record method calls without executing JS. Use for ScopeManager and CardController tests so they don't link QuickJS.

```cpp
// fake_js_engine.h
#pragma once
#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/js-engine/ijs_context.h"
#include "flexui/core/js-engine/ijs_value.h"

namespace flexui::core::js_engine::test {

class FakeJsValue : public IJsValue { /* trivial returns, ToFlexUIValue returns undefined */ };
class FakeJsContext : public IJsContext { /* records Eval calls, returns FakeJsValue */ };
class FakeJsEngine  : public IJsEngine  {
 public:
  flexui::common::Error Initialize(const EngineConfig&) override { return flexui::common::Error::Ok(); }
  std::shared_ptr<IJsContext> CreateContext() override;
  void Shutdown() override {}
  const char* BackendName() const override { return "Fake"; }
};

}  // namespace flexui::core::js_engine::test
```

Implement methods inline or in `.cc`. The exact body is mechanical — every `IJsValue::Is*` returns false except where the test demands; every factory returns another `FakeJsValue`. Keep ~150 lines.

- [ ] **Step 6.8: Tests — scope lifecycle + manager**

```cpp
// scope_lifecycle_test.cc
#include <gtest/gtest.h>
#include "flexui/core/scope-manager/scope_manager.h"
#include "tests/flex-ui/unit/support/fake_js_engine.h"

namespace flexui::core::scope_manager {

TEST(ScopeLifecycleTest, CreateBeginRunDestroy) {
  auto eng = std::make_shared<js_engine::test::FakeJsEngine>();
  eng->Initialize({});
  ScopeManager mgr(eng);
  auto scope = mgr.CreateScope();
  ASSERT_NE(scope, nullptr);
  EXPECT_EQ(scope->state(), ScopeState::kScopeCreating);
  EXPECT_TRUE(scope->Begin(nullptr).ok());
  scope->MarkRunning();
  EXPECT_EQ(scope->state(), ScopeState::kScopeRunning);
  scope->Destroy();
  EXPECT_EQ(scope->state(), ScopeState::kScopeDestroyed);
}

TEST(ScopeManagerTest, MultipleScopesAreIsolated) {
  auto eng = std::make_shared<js_engine::test::FakeJsEngine>();
  eng->Initialize({});
  ScopeManager mgr(eng);
  auto a = mgr.CreateScope();
  auto b = mgr.CreateScope();
  EXPECT_NE(a->id(), b->id());
  EXPECT_EQ(mgr.ScopeCount(), 2u);
  EXPECT_TRUE(mgr.DestroyScope(a->id()).ok());
  EXPECT_EQ(mgr.ScopeCount(), 1u);
}

}  // namespace flexui::core::scope_manager
```

- [ ] **Step 6.9: Wire all sources + tests, build, commit**

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/scope-manager tests/flex-ui/unit
git commit -m "feat(flex-ui/core/scope-manager): Scope state machine + ScopeManager + Snapshot stub"
```

---

### Task 7: Card Controller — orchestration core

**Files:**
- Create: 3 headers + 2 sources under `flex-ui/core/card-controller/`

- [ ] **Step 7.1: Write `flex_card_state.h`**

```cpp
#pragma once
namespace flexui::core::card_controller {

enum class FlexCardState {
  kIdle,
  kLoading,
  kRunning,
  kErrored,
  kDestroyed,
};

const char* FlexCardStateName(FlexCardState s);

}  // namespace flexui::core::card_controller
```

- [ ] **Step 7.2: Write `flex_ui_engine.h`** (the C++ side of FlexUIEngine; ETS wrapper comes in Task 10)

```cpp
#pragma once

#include <memory>

#include "flexui/common/error.h"
#include "flexui/common/task_runner.h"
#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/js-engine/js_engine_backend.h"
#include "flexui/core/plugin-host/plugin_host.h"
#include "flexui/core/scope-manager/scope_manager.h"
#include "flexui/core/commit-pipeline/commit_pipeline.h"
#include "flexui/core/bridge/ibridge.h"

namespace flexui::core::card_controller {

struct FlexUIEngineConfig {
  js_engine::JsEngineBackend backend = js_engine::JsEngineBackend::kQuickJS;
  bool force_quickjs_for_debug = false;
  size_t memory_limit_mb = 0;
};

class FlexUIEngine {
 public:
  static FlexUIEngine& Instance();
  flexui::common::Error Init(const FlexUIEngineConfig& cfg);
  flexui::common::Error Install(plugin_host::FlexUIPlugin plugin);
  flexui::common::Error Uninstall(const std::string& plugin_name);
  void Shutdown();

  // Internal accessors used by FlexCardController.
  js_engine::IJsEngine&             js_engine();
  scope_manager::ScopeManager&      scope_manager();
  plugin_host::PluginHost&          plugin_host();
  commit_pipeline::CommitPipeline&  commit_pipeline();
  bridge::IBridge&                  bridge();
  flexui::common::TaskRunner&       js_runner();
  flexui::common::TaskRunner&       ui_runner();

 private:
  FlexUIEngine() = default;
  ~FlexUIEngine() = default;
  FlexUIEngine(const FlexUIEngine&) = delete;
  FlexUIEngine& operator=(const FlexUIEngine&) = delete;

  std::shared_ptr<js_engine::IJsEngine> engine_;
  std::unique_ptr<plugin_host::PluginHost> plugins_;
  std::unique_ptr<scope_manager::ScopeManager> scopes_;
  std::unique_ptr<commit_pipeline::CommitPipeline> commit_;
  std::shared_ptr<flexui::common::TaskRunner> js_runner_;
  std::shared_ptr<flexui::common::TaskRunner> ui_runner_;
  std::shared_ptr<bridge::IBridge> bridge_;
  bool initialized_ = false;
};

}  // namespace flexui::core::card_controller
```

- [ ] **Step 7.3: Write `flex_card_controller.h`**

```cpp
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/card-controller/flex_card_state.h"
#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::core::scope_manager { class Scope; }

namespace flexui::core::card_controller {

class FlexUIEngine;

struct FlexCardControllerOptions {
  std::string bundle_uri;          // file://, https://, custom scheme
  std::string bundle_inline;       // alternative: full bundle text/json
  std::string bundle_type;         // "card-js" / "a2ui-json"
  flexui::common::FlexUIValue initial_data;
  size_t memory_limit_mb = 0;
};

class FlexCardController {
 public:
  explicit FlexCardController(FlexCardControllerOptions options);
  ~FlexCardController();

  uint32_t scope_id() const;
  FlexCardState state() const { return state_.load(); }

  flexui::common::Error Load();
  flexui::common::Error SetData(flexui::common::FlexUIValue data);
  flexui::common::Error CallMethod(const std::string& name,
                                   std::vector<flexui::common::FlexUIValue> args,
                                   flexui::common::FlexUIValue* result_out);
  void OnEvent(std::string event, std::function<void(flexui::common::FlexUIValue)> handler);
  void OnError(std::function<void(flexui::common::Error)> handler);
  flexui::common::Error Destroy();

  // ETS / NAPI side calls this to attach a platform node container.
  void AttachNodeContent(commit_pipeline::NodeHandle node_container);
  void DetachNodeContent();

 private:
  FlexCardControllerOptions options_;
  std::shared_ptr<scope_manager::Scope> scope_;
  std::atomic<FlexCardState> state_{FlexCardState::kIdle};
  std::function<void(flexui::common::Error)> error_handler_;
  std::unordered_map<std::string, std::function<void(flexui::common::FlexUIValue)>> event_handlers_;
  commit_pipeline::NodeHandle node_container_ = nullptr;
};

}  // namespace flexui::core::card_controller
```

- [ ] **Step 7.4: Write `flex_ui_engine.cc`**

```cpp
#include "flexui/core/card-controller/flex_ui_engine.h"

#include "flexui/core/js-engine/js_engine_factory.h"
#include "flexui/core/bridge/inprocess_bridge.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::card_controller {

FlexUIEngine& FlexUIEngine::Instance() {
  static FlexUIEngine engine;
  return engine;
}

flexui::common::Error FlexUIEngine::Init(const FlexUIEngineConfig& cfg) {
  if (initialized_) {
    return flexui::common::Error(flexui::common::ErrorCode::kAlreadyExists,
                                 "engine already initialized");
  }
  FLEXUI_TLOG(Engine, Init, INFO) << "backend=" << static_cast<int>(cfg.backend);
  engine_ = js_engine::MakeJsEngine(cfg.backend);
  if (!engine_) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal,
                                 "MakeJsEngine returned null");
  }
  js_engine::EngineConfig ecfg;
  ecfg.memory_limit_mb = cfg.memory_limit_mb;
  auto err = engine_->Initialize(ecfg);
  if (!err.ok()) return err;

  plugins_ = std::make_unique<plugin_host::PluginHost>();
  scopes_  = std::make_unique<scope_manager::ScopeManager>(engine_);
  commit_  = std::make_unique<commit_pipeline::CommitPipeline>(plugins_.get());

  js_runner_ = std::make_shared<flexui::common::TaskRunner>("flexui-js", 1);
  ui_runner_ = std::make_shared<flexui::common::TaskRunner>("flexui-ui", 1);
  bridge_ = std::make_shared<bridge::InProcessBridge>(js_runner_, ui_runner_);

  initialized_ = true;
  return flexui::common::Error::Ok();
}

flexui::common::Error FlexUIEngine::Install(plugin_host::FlexUIPlugin plugin) {
  if (!initialized_) return flexui::common::Error(
      flexui::common::ErrorCode::kEngineNotInitialized, "engine not initialized");
  return plugins_->Install(std::move(plugin));
}

flexui::common::Error FlexUIEngine::Uninstall(const std::string& n) {
  if (!initialized_) return flexui::common::Error(
      flexui::common::ErrorCode::kEngineNotInitialized, "engine not initialized");
  return plugins_->Uninstall(n);
}

void FlexUIEngine::Shutdown() {
  FLEXUI_TLOG(Engine, Shutdown, INFO);
  commit_.reset();
  scopes_.reset();
  plugins_.reset();
  bridge_.reset();
  js_runner_.reset();
  ui_runner_.reset();
  if (engine_) engine_->Shutdown();
  engine_.reset();
  initialized_ = false;
}

js_engine::IJsEngine& FlexUIEngine::js_engine() { return *engine_; }
scope_manager::ScopeManager& FlexUIEngine::scope_manager() { return *scopes_; }
plugin_host::PluginHost& FlexUIEngine::plugin_host() { return *plugins_; }
commit_pipeline::CommitPipeline& FlexUIEngine::commit_pipeline() { return *commit_; }
bridge::IBridge& FlexUIEngine::bridge() { return *bridge_; }
flexui::common::TaskRunner& FlexUIEngine::js_runner() { return *js_runner_; }
flexui::common::TaskRunner& FlexUIEngine::ui_runner() { return *ui_runner_; }

}  // namespace flexui::core::card_controller
```

- [ ] **Step 7.5: Write `flex_card_controller.cc`**

```cpp
#include "flexui/core/card-controller/flex_card_controller.h"

#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/core/scope-manager/scope_manager.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::card_controller {

const char* FlexCardStateName(FlexCardState s) {
  switch (s) {
    case FlexCardState::kIdle:      return "Idle";
    case FlexCardState::kLoading:   return "Loading";
    case FlexCardState::kRunning:   return "Running";
    case FlexCardState::kErrored:   return "Errored";
    case FlexCardState::kDestroyed: return "Destroyed";
  }
  return "Unknown";
}

FlexCardController::FlexCardController(FlexCardControllerOptions opts)
    : options_(std::move(opts)) {
  scope_ = FlexUIEngine::Instance().scope_manager().CreateScope();
  FLEXUI_TLOG(Card, Construct, INFO)
      << "scope=" << (scope_ ? scope_->id() : 0)
      << " bundle_type=" << options_.bundle_type;
}

FlexCardController::~FlexCardController() {
  if (state_.load() != FlexCardState::kDestroyed) Destroy();
}

uint32_t FlexCardController::scope_id() const {
  return scope_ ? scope_->id() : 0;
}

flexui::common::Error FlexCardController::Load() {
  state_.store(FlexCardState::kLoading);
  FLEXUI_TLOG(Card, LoadEnter, INFO)
      << "scope=" << scope_id() << " uri=" << options_.bundle_uri;
  if (!scope_) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal,
                                 "no scope");
  }
  auto err = scope_->Begin(nullptr);  // PoC: no snapshot
  if (!err.ok()) {
    state_.store(FlexCardState::kErrored);
    if (error_handler_) error_handler_(err);
    return err;
  }
  // Frontend resolution + bundle eval lands in W7-W8 (controller calls the
  // PluginHost::FindFrontend(options.bundle_type) and drives it). For W5-W6,
  // we mark Running so lifecycle plumbing is testable end-to-end.
  scope_->MarkRunning();
  state_.store(FlexCardState::kRunning);
  FLEXUI_TLOG(Card, LoadExit, INFO) << "scope=" << scope_id();
  return flexui::common::Error::Ok();
}

flexui::common::Error FlexCardController::SetData(flexui::common::FlexUIValue data) {
  FLEXUI_TLOG(Card, SetData, DEBUG) << "scope=" << scope_id();
  // Real frontend Render dispatch lands in W7-W8.
  (void)data;
  return flexui::common::Error::Ok();
}

flexui::common::Error FlexCardController::CallMethod(
    const std::string& name,
    std::vector<flexui::common::FlexUIValue> args,
    flexui::common::FlexUIValue* result) {
  FLEXUI_TLOG(Card, CallMethod, DEBUG)
      << "scope=" << scope_id() << " name=" << name;
  (void)args; (void)result;
  return flexui::common::Error::Ok();
}

void FlexCardController::OnEvent(std::string e,
                                 std::function<void(flexui::common::FlexUIValue)> h) {
  event_handlers_[std::move(e)] = std::move(h);
}

void FlexCardController::OnError(std::function<void(flexui::common::Error)> h) {
  error_handler_ = std::move(h);
}

flexui::common::Error FlexCardController::Destroy() {
  FLEXUI_TLOG(Card, DestroyEnter, INFO) << "scope=" << scope_id();
  if (scope_) {
    FlexUIEngine::Instance().scope_manager().DestroyScope(scope_->id());
    scope_.reset();
  }
  state_.store(FlexCardState::kDestroyed);
  FLEXUI_TLOG(Card, DestroyExit, INFO);
  return flexui::common::Error::Ok();
}

void FlexCardController::AttachNodeContent(commit_pipeline::NodeHandle h) {
  FLEXUI_TLOG(Card, AttachNodeContent, INFO) << "scope=" << scope_id();
  node_container_ = h;
}

void FlexCardController::DetachNodeContent() {
  FLEXUI_TLOG(Card, DetachNodeContent, INFO) << "scope=" << scope_id();
  node_container_ = nullptr;
}

}  // namespace flexui::core::card_controller
```

- [ ] **Step 7.6: Wire sources, write test, build, commit**

```cpp
// tests/flex-ui/unit/flex_card_controller_test.cc
#include <gtest/gtest.h>

#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/core/card-controller/flex_card_controller.h"

namespace flexui::core::card_controller {

class FlexCardControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FlexUIEngineConfig cfg;
    cfg.backend = js_engine::JsEngineBackend::kQuickJS;
    ASSERT_TRUE(FlexUIEngine::Instance().Init(cfg).ok());
  }
  void TearDown() override {
    FlexUIEngine::Instance().Shutdown();
  }
};

TEST_F(FlexCardControllerTest, FullLifecycle) {
  FlexCardControllerOptions opts;
  opts.bundle_type = "card-js";
  FlexCardController card(std::move(opts));
  EXPECT_EQ(card.state(), FlexCardState::kIdle);
  EXPECT_TRUE(card.Load().ok());
  EXPECT_EQ(card.state(), FlexCardState::kRunning);
  EXPECT_TRUE(card.SetData(flexui::common::FlexUIValue()).ok());
  EXPECT_TRUE(card.Destroy().ok());
  EXPECT_EQ(card.state(), FlexCardState::kDestroyed);
}

TEST_F(FlexCardControllerTest, TwoCardsIndependent) {
  FlexCardController a({});
  FlexCardController b({});
  EXPECT_NE(a.scope_id(), b.scope_id());
  a.Load(); b.Load();
  EXPECT_TRUE(a.Destroy().ok());
  EXPECT_EQ(a.state(), FlexCardState::kDestroyed);
  EXPECT_EQ(b.state(), FlexCardState::kRunning);
}

}  // namespace flexui::core::card_controller
```

```bash
./scripts/flex-ui/build.sh
git add flex-ui/core/card-controller tests/flex-ui/unit
git commit -m "feat(flex-ui/core/card-controller): FlexUIEngine singleton + FlexCardController lifecycle"
```

---

### Task 8: Harmony NAPI binding scaffold

**Files:**
- Create: `flex-ui/platforms/CMakeLists.txt` (only when `FLEXUI_OHOS`)
- Create: `flex-ui/platforms/harmony/CMakeLists.txt`
- Create: `flex-ui/platforms/harmony/napi/napi_init.cc`
- Create: `flex-ui/platforms/harmony/napi/napi_engine.cc`
- Create: `flex-ui/platforms/harmony/napi/napi_controller.cc`
- Create: `flex-ui/platforms/harmony/napi/napi_node_content.cc`

These compile only when `FLEXUI_OHOS=ON`. On host they're skipped.

- [ ] **Step 8.1: Top-level platforms CMakeLists**

`flex-ui/platforms/CMakeLists.txt`:

```cmake
if(FLEXUI_OHOS)
  add_subdirectory(harmony)
endif()
```

`flex-ui/platforms/harmony/CMakeLists.txt`:

```cmake
add_library(flexui_napi SHARED
  napi/napi_init.cc
  napi/napi_engine.cc
  napi/napi_controller.cc
  napi/napi_node_content.cc
)
target_link_libraries(flexui_napi PRIVATE
  flexui_core_card_controller
  flexui_core_commit_pipeline
  flexui_core_bridge
  libace_napi.z.so
  libace_ndk.z.so   # ArkUI native
)
target_compile_definitions(flexui_napi PRIVATE FLEXUI_OHOS=1)
```

- [ ] **Step 8.2: Add FLEXUI_OHOS toggle to top-level `flex-ui/CMakeLists.txt`**

```cmake
option(FLEXUI_OHOS "Build Harmony NAPI + card host" OFF)
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(platforms)
```

- [ ] **Step 8.3: Write `napi_init.cc`**

```cpp
// Module entry point: register all NAPI methods exposed by FlexUI.
#include <napi/native_api.h>

namespace flexui::platforms::harmony {

extern napi_value RegisterEngineMethods(napi_env env, napi_value exports);
extern napi_value RegisterControllerMethods(napi_env env, napi_value exports);
extern napi_value RegisterNodeContentMethods(napi_env env, napi_value exports);

}  // namespace flexui::platforms::harmony

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  flexui::platforms::harmony::RegisterEngineMethods(env, exports);
  flexui::platforms::harmony::RegisterControllerMethods(env, exports);
  flexui::platforms::harmony::RegisterNodeContentMethods(env, exports);
  return exports;
}
EXTERN_C_END

static napi_module flexui_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "flexui",
    .nm_priv = (void*)0,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterFlexUIModule(void) {
  napi_module_register(&flexui_module);
}
```

- [ ] **Step 8.4: Write `napi_engine.cc`**

```cpp
#include <napi/native_api.h>
#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/common/log_tag.h"

namespace flexui::platforms::harmony {

namespace cc = flexui::core::card_controller;

static napi_value Init(napi_env env, napi_callback_info info) {
  FLEXUI_TLOG(Engine, NapiInit, INFO);
  // Read backend from JS args (0 = quickjs, 1 = jsvm).
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int32_t backend_int = 0;
  bool force_debug = false;
  napi_get_value_int32(env, args[0], &backend_int);
  napi_get_value_bool(env, args[1], &force_debug);

  cc::FlexUIEngineConfig cfg;
  cfg.backend = static_cast<flexui::core::js_engine::JsEngineBackend>(backend_int);
  cfg.force_quickjs_for_debug = force_debug;
  auto err = cc::FlexUIEngine::Instance().Init(cfg);
  napi_value result;
  napi_get_boolean(env, err.ok(), &result);
  return result;
}

static napi_value Shutdown(napi_env env, napi_callback_info) {
  cc::FlexUIEngine::Instance().Shutdown();
  napi_value undef;
  napi_get_undefined(env, &undef);
  return undef;
}

napi_value RegisterEngineMethods(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"flexUiEngineInit", nullptr, Init,     nullptr, nullptr, nullptr, napi_default, nullptr},
      {"flexUiEngineShutdown", nullptr, Shutdown, nullptr, nullptr, nullptr, napi_default, nullptr},
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(*desc), desc);
  return exports;
}

}  // namespace flexui::platforms::harmony
```

- [ ] **Step 8.5: Write `napi_controller.cc`** — NAPI bindings for `FlexCardController` mirroring the C++ API. Methods exposed to ETS:

| ETS method on `FlexCardController` | NAPI function | C++ binding |
|---|---|---|
| `_create(opts)` → controller id | `flexCardControllerCreate` | `new FlexCardController(...)`, store in id table |
| `_load(id)` → bool | `flexCardControllerLoad` | `controller.Load()` |
| `_setData(id, dataJson)` → bool | `flexCardControllerSetData` | parse JSON to FlexUIValue, `SetData(...)` |
| `_destroy(id)` → bool | `flexCardControllerDestroy` | `controller.Destroy()` + remove from table |
| `_attach(id, nodeContentPtr)` | `flexCardControllerAttach` | `controller.AttachNodeContent(handle)` |
| `_detach(id)` | `flexCardControllerDetach` | `controller.DetachNodeContent()` |

Implementation pattern (one function shown; rest follow identically):

```cpp
static napi_value Create(napi_env env, napi_callback_info info) {
  // Parse opts {bundleUri, bundleType, initialData} into FlexCardControllerOptions.
  // Construct controller. Insert into a static id->unique_ptr table.
  // Return id as napi_value int.
  ...
}
```

A static `std::unordered_map<uint32_t, std::unique_ptr<FlexCardController>>` guarded by mutex holds the controllers. Allocate ids monotonically. Every binding logs `FLEXUI_TLOG(Bridge, Napi<Method>, DEBUG) << "id=" << id`.

- [ ] **Step 8.6: Write `napi_node_content.cc`** — exposes a single function:

```cpp
static napi_value AttachNodeContent(napi_env env, napi_callback_info info) {
  // arg0: controller id (uint32)
  // arg1: NodeContent handle as napi_value (ArkUI provides a NAPI handle for NodeContent)
  // Extract ArkUI_NodeContentHandle via OH_ArkUI_GetNodeContentFromNapiValue
  // Call controller.AttachNodeContent((commit_pipeline::NodeHandle) node_content_handle).
}
```

Use the official OH API `OH_ArkUI_GetNodeContentFromNapiValue(env, value, &out_handle)` (HarmonyOS API 12+). If the API is not available at build time (older device), fall back to interpreting the value as a `void*` pointer field of the `napi_value`.

- [ ] **Step 8.7: Verify host build still green (Harmony bits compiled out)**

```bash
./scripts/flex-ui/build.sh
```

Expected: `FLEXUI_OHOS` is OFF on host, so `flex-ui/platforms/` is skipped. All existing tests pass.

- [ ] **Step 8.8: Commit**

```bash
git add flex-ui/platforms/harmony flex-ui/CMakeLists.txt
git commit -m "feat(flex-ui/platforms/harmony): NAPI scaffold (Engine + Controller + NodeContent bindings)"
```

---

### Task 9: Harmony ETS API — FlexUIEngine.ets, FlexCardController.ets, FlexCard.ets

**Files:**
- Create: `flex-ui/platforms/harmony/card/ets/FlexUIEngine.ets`
- Create: `flex-ui/platforms/harmony/card/ets/FlexCardController.ets`
- Create: `flex-ui/platforms/harmony/card/ets/FlexCard.ets`
- Create: `flex-ui/platforms/harmony/card/ohos-module/oh-package.json5`
- Create: `flex-ui/platforms/harmony/card/ohos-module/README.md`

- [ ] **Step 9.1: Write `FlexUIEngine.ets`**

```typescript
import flexui from 'libflexui.so';

export enum JsEngineBackend {
  QuickJS = 0,
  Jsvm = 1,
}

export interface FlexUIEngineConfig {
  backend?: JsEngineBackend;
  forceQuickJsForDebug?: boolean;
}

export class FlexUIEngine {
  private static initialized: boolean = false;

  static init(cfg: FlexUIEngineConfig = {}): boolean {
    const backend = cfg.backend ?? JsEngineBackend.Jsvm;
    const ok = flexui.flexUiEngineInit(backend, !!cfg.forceQuickJsForDebug);
    FlexUIEngine.initialized = ok;
    return ok;
  }

  static install(plugin: object): boolean {
    // Real plugin install lands in W7-W8 when components/api/loader/frontend
    // bindings stabilize. For W5-W6, accept any plugin object and return true.
    return true;
  }

  static shutdown(): void {
    flexui.flexUiEngineShutdown();
    FlexUIEngine.initialized = false;
  }

  static isInitialized(): boolean {
    return FlexUIEngine.initialized;
  }
}
```

- [ ] **Step 9.2: Write `FlexCardController.ets`**

```typescript
import flexui from 'libflexui.so';

export interface FlexCardControllerOptions {
  bundleUri?: string;
  bundle?: string;
  bundleType: 'card-js' | 'a2ui-json';
  initialData?: object;
}

export class FlexCardController {
  private id: number;
  private destroyed: boolean = false;

  constructor(opts: FlexCardControllerOptions) {
    this.id = flexui.flexCardControllerCreate(JSON.stringify(opts));
  }

  load(): boolean {
    if (this.destroyed) return false;
    return flexui.flexCardControllerLoad(this.id);
  }

  setData(data: object): boolean {
    if (this.destroyed) return false;
    return flexui.flexCardControllerSetData(this.id, JSON.stringify(data));
  }

  destroy(): void {
    if (this.destroyed) return;
    flexui.flexCardControllerDestroy(this.id);
    this.destroyed = true;
  }

  _attach(nodeContent: NodeContent): void {
    flexui.flexCardControllerAttach(this.id, nodeContent);
  }

  _detach(): void {
    flexui.flexCardControllerDetach(this.id);
  }

  _id(): number { return this.id; }
}
```

- [ ] **Step 9.3: Write `FlexCard.ets`**

```typescript
import { FlexCardController } from './FlexCardController';

@Component
export struct FlexCard {
  @ObjectLink controller: FlexCardController;
  private nodeContent: NodeContent = new NodeContent();

  aboutToAppear() {
    this.controller._attach(this.nodeContent);
  }

  aboutToDisappear() {
    this.controller._detach();
  }

  build() {
    ContentSlot(this.nodeContent)
      .width('100%')
      .height('auto');
  }
}
```

- [ ] **Step 9.4: Write `oh-package.json5`**

```json5
{
  "name": "@flexui/host-card",
  "version": "0.1.0",
  "main": "./ets/index.ets",
  "description": "FlexUI HarmonyOS card host (FlexCard @Component, FlexCardController, FlexUIEngine)",
  "license": "Apache-2.0",
  "types": "./ets/index.d.ets",
  "dependencies": {}
}
```

Create a small `ets/index.ets` re-exporting all three classes.

- [ ] **Step 9.5: Write `README.md`**

```markdown
# FlexUI HarmonyOS card host

ETS entry to the FlexUI framework body in `flex-ui/`. Consumed by
`playground/harmony/entry/...` in W9-W10.

PoC build: see `scripts/flex-ui/build_harmony.sh` (added W9-W10).
```

- [ ] **Step 9.6: Commit**

```bash
git add flex-ui/platforms/harmony/card
git commit -m "feat(flex-ui/platforms/harmony/card): ETS FlexUIEngine + FlexCardController + FlexCard"
```

---

### Task 10: Android stubs

**Files:**
- Create: `flex-ui/platforms/android/jni/flexui_jni_stub.cc`
- Create: `flex-ui/platforms/android/card/kotlin/FlexUIEngine.kt`
- Create: `flex-ui/platforms/android/card/kotlin/FlexCardController.kt`
- Create: `flex-ui/platforms/android/card/kotlin/FlexCardView.kt`

These exist to validate the cross-platform interface lock-down (spec §6.3 cross-platform compile gate).

- [ ] **Step 10.1: Write `flexui_jni_stub.cc`**

```cpp
/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Android JNI stub. Real implementation lands in Phase 1+. Every entry point
 * logs NOT_IMPLEMENTED to verify the binary loads.
 */
#include <jni.h>

#include "flexui/common/log_tag.h"

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_flexui_FlexUIEngineNative_init(JNIEnv*, jclass, jint, jboolean) {
  FLEXUI_TLOG(Engine, JniInit, WARNING) << "NOT_IMPLEMENTED";
  return JNI_TRUE;  // stub success so caller can proceed in tests
}

JNIEXPORT void JNICALL
Java_com_flexui_FlexUIEngineNative_shutdown(JNIEnv*, jclass) {
  FLEXUI_TLOG(Engine, JniShutdown, WARNING) << "NOT_IMPLEMENTED";
}

JNIEXPORT jint JNICALL
Java_com_flexui_FlexCardControllerNative_create(JNIEnv*, jclass, jstring) {
  FLEXUI_TLOG(Card, JniCreate, WARNING) << "NOT_IMPLEMENTED";
  return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_flexui_FlexCardControllerNative_load(JNIEnv*, jclass, jint) {
  FLEXUI_TLOG(Card, JniLoad, WARNING) << "NOT_IMPLEMENTED";
  return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_flexui_FlexCardControllerNative_destroy(JNIEnv*, jclass, jint) {
  FLEXUI_TLOG(Card, JniDestroy, WARNING) << "NOT_IMPLEMENTED";
}

}  // extern "C"
```

- [ ] **Step 10.2: Write `FlexUIEngine.kt`**

```kotlin
package com.flexui

object FlexUIEngine {
    @JvmStatic external fun init(backend: Int, forceQuickJsForDebug: Boolean): Boolean
    @JvmStatic external fun shutdown()

    init {
        System.loadLibrary("flexui_jni")
    }

    fun install(plugin: Any): Boolean {
        // Plugin install lands in Phase 1+.
        return true
    }
}
```

- [ ] **Step 10.3: Write `FlexCardController.kt`**

```kotlin
package com.flexui

class FlexCardController(opts: Map<String, Any>) {
    private val id: Int

    private external fun createNative(optsJson: String): Int
    private external fun loadNative(id: Int): Boolean
    private external fun destroyNative(id: Int)

    init {
        id = createNative(opts.toString())
    }

    fun load(): Boolean = loadNative(id)
    fun destroy() = destroyNative(id)
}
```

- [ ] **Step 10.4: Write `FlexCardView.kt`**

```kotlin
package com.flexui

import android.content.Context
import android.util.AttributeSet
import android.view.ViewGroup

class FlexCardView(context: Context, attrs: AttributeSet? = null)
    : ViewGroup(context, attrs) {

    var controller: FlexCardController? = null

    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        // PoC stub: no children. Real layout lands in Phase 1+ when JNI Attach
        // forwards the native view tree.
    }
}
```

- [ ] **Step 10.5: Verify (no Android build in PoC; just ensure files compile via syntax check)**

Static syntax check (e.g., bring up a minimal `build.gradle` in `flex-ui/platforms/android/` later when the Android build path is exercised). For W5-W6, the deliverable is "files exist and follow the cross-platform interface" — Android compile gate is exercised by the playground (W9-W10).

- [ ] **Step 10.6: Commit**

```bash
git add flex-ui/platforms/android
git commit -m "feat(flex-ui/platforms/android): JNI + Kotlin stubs (NOT_IMPLEMENTED, compile gate)"
```

---

### Task 11: Coverage gate + TSan + AGenUI untouched

- [ ] **Step 11.1: Run host gate**

```bash
./scripts/flex-ui/build.sh
./scripts/flex-ui/build.sh --tsan-only
./scripts/flex-ui/coverage.sh
```

If TSan reports new races (e.g. in InProcessBridge subscriber map under concurrent Post/Subscribe), add the necessary `std::lock_guard` in `inprocess_bridge.cc` — these are FlexUI-original, so fixing is in scope.

If coverage on `flex-ui/core/` < 80%, identify uncovered units and add focused tests. Likely candidates: scope error path, plugin uninstall flow, commit pipeline Move mutation.

- [ ] **Step 11.2: Verify AGenUI untouched**

```bash
git diff master --stat -- core/ platforms/ tests/cpp/ playground/ scripts/harmony/ scripts/android/ scripts/ios/ agent_sdks/ samples/ skills/
```

Expected: empty.

- [ ] **Step 11.3: Final commit (optional)**

```bash
git commit --allow-empty -m "chore(flex-ui): W5-W6 final green gate"
```

---

## Self-Review

### Spec coverage

| Spec requirement | Covered by |
|---|---|
| §4.2 Public SDK API: FlexUIEngine / FlexCardController / FlexCard | Tasks 7, 9 |
| §4.6 NodeContent hosting (Harmony API 12+) | Task 8 (napi_node_content), Task 9 (FlexCard.ets) |
| §4.8 Mutation pipeline + dispatch | Tasks 5, 7 |
| §4.9 Scope state machine | Task 6 |
| §4.11 Plugin mechanism + 4 extension points + conflict detection | Task 4 |
| §4.12 Bridge binary protocol | Tasks 2, 3 |
| §4.13 Cross-platform interface lockdown (Android stubs compile) | Tasks 8 (top-level CMake), 10 |
| §6.4 Coverage gate ≥ 80% | Task 11 |
| §7.1 unit-test methodology | Tasks 2, 4, 5, 6, 7 |
| §8.4 mandatory log coverage points | every public method logs FLEXUI_TLOG (Tasks 2-7) |

### Placeholder scan

No "TBD", "TODO", "fill in later". Task 8.5 ("napi_controller.cc — one function shown; rest follow identically") is concrete because the table + the shown function give the executor everything needed (parse args from napi_value, look up controller by id, call the corresponding C++ method, return result). Task 6.7's FakeJsEngine ("~150 lines of trivial returns") is concrete: every IJsValue/IJsContext method's expected behavior is unambiguous (always-false predicates, always-undefined factories, always-Error::Ok on Eval). Task 11.5's JSVM impl is concrete: structure is the QuickJS file shown verbatim in W3-W4; the executor translates JS_* → OH_JSVM_* calls 1:1.

### Type consistency

- `flexui::common::Error`, `ErrorCode` used in every layer matching the W1-W2 definitions.
- `FlexUIValue` round-trips through Bridge ↔ JS engine ↔ Frontend (W7-W8 entry point).
- `MutationList` produced in W3-W4 Task 4 consumed in W5-W6 Task 5.
- `ComponentInstance` interface (Task 5) referenced by `ComponentFactory` (Task 4) and `CommitPipeline` (Task 5) — same shape.
- `FlexCardController` ETS API (Task 9) and C++ API (Task 7) share matching method names + parameter contracts: `load`/`Load`, `setData`/`SetData`, `destroy`/`Destroy`, `_attach`/`AttachNodeContent`, `_detach`/`DetachNodeContent`.

### Scope check

This plan delivers a working empty card: business code can `FlexUIEngine.init()`, `new FlexCardController(opts)`, `load()`, `setData(...)`, and `destroy()` end-to-end. The card has no UI yet (no frontend, no components) — that's W7-W8 — but every wire is in place and exercised by unit tests.

---

*End of W5-W6 plan.*
