/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * JSVM backend smoke test.
 * - On HarmonyOS (FLEXUI_OHOS): asserts the engine initializes and can eval.
 * - On host (no FLEXUI_OHOS): asserts MakeJsEngine(kJsvm) returns nullptr.
 */
#include <gtest/gtest.h>
#include "flexui/core/js-engine/js_engine_factory.h"

namespace flexui::core::js_engine {

#if defined(FLEXUI_OHOS)

TEST(JsvmEngineTest, EvalNumericExpression) {
  auto eng = MakeJsEngine(JsEngineBackend::kJsvm);
  ASSERT_TRUE(eng);
  ASSERT_TRUE(eng->Initialize({}).ok());
  auto ctx = eng->CreateContext();
  ASSERT_TRUE(ctx);
  flexui::common::Error err = flexui::common::Error::Ok();
  auto v = ctx->Eval("1 + 2", "<test>", &err);
  EXPECT_TRUE(err.ok());
  EXPECT_DOUBLE_EQ(v->ToNumber(), 3.0);
}

#else

TEST(JsvmEngineTest, NullOnHost) {
  auto eng = MakeJsEngine(JsEngineBackend::kJsvm);
  EXPECT_EQ(eng, nullptr);
}

#endif

}  // namespace flexui::core::js_engine
