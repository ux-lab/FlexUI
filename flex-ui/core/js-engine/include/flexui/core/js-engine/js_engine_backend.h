#pragma once

namespace flexui::core::js_engine {

enum class JsEngineBackend { kQuickJS, kJsvm };

// Auto-select backend per platform:
//   - HarmonyOS (FLEXUI_OHOS set):  kJsvm (debug fallback: kQuickJS via flag)
//   - Android, host:                kQuickJS
JsEngineBackend SelectBackend(bool force_quickjs_for_debug);

}  // namespace flexui::core::js_engine
