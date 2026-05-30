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
