#pragma once

#include <memory>

#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/js-engine/js_engine_backend.h"

namespace flexui::core::js_engine {

std::unique_ptr<IJsEngine> MakeJsEngine(JsEngineBackend backend);

// Convenience: SelectBackend + MakeJsEngine.
std::unique_ptr<IJsEngine> MakeDefaultJsEngine(bool force_quickjs_for_debug = false);

}  // namespace flexui::core::js_engine
