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
