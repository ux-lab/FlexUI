/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/js-engine/js_engine_factory.h"

namespace flexui::core::js_engine {

class QuickJSEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    engine_ = MakeJsEngine(JsEngineBackend::kQuickJS);
    ASSERT_TRUE(engine_);
    ASSERT_TRUE(engine_->Initialize({}).ok());
  }
  void TearDown() override { engine_.reset(); }

  std::unique_ptr<IJsEngine> engine_;
};

TEST_F(QuickJSEngineTest, BackendNameIsQuickJS) {
  EXPECT_STREQ(engine_->BackendName(), "QuickJS");
}

TEST_F(QuickJSEngineTest, EvalNumericExpression) {
  auto ctx = engine_->CreateContext();
  ASSERT_TRUE(ctx);
  flexui::common::Error err = flexui::common::Error::Ok();
  auto v = ctx->Eval("1 + 2", "<test>", &err);
  EXPECT_TRUE(err.ok());
  EXPECT_TRUE(v->IsNumber());
  EXPECT_DOUBLE_EQ(v->ToNumber(), 3.0);
}

TEST_F(QuickJSEngineTest, EvalSyntaxErrorReportedAsException) {
  auto ctx = engine_->CreateContext();
  flexui::common::Error err = flexui::common::Error::Ok();
  auto v = ctx->Eval("function( {", "<test>", &err);
  EXPECT_FALSE(err.ok());
  EXPECT_EQ(err.code(), flexui::common::ErrorCode::kJsException);
}

TEST_F(QuickJSEngineTest, InjectedFunctionCallable) {
  auto ctx = engine_->CreateContext();
  int call_count = 0;
  ctx->InjectGlobalFunction("addOne", [&call_count](IJsContext& c,
      const std::vector<std::shared_ptr<IJsValue>>& args) {
    ++call_count;
    double in = args.empty() ? 0.0 : args[0]->ToNumber();
    return c.NewNumber(in + 1);
  });
  flexui::common::Error err = flexui::common::Error::Ok();
  auto v = ctx->Eval("addOne(41)", "<test>", &err);
  EXPECT_TRUE(err.ok());
  EXPECT_DOUBLE_EQ(v->ToNumber(), 42.0);
  EXPECT_EQ(call_count, 1);
}

TEST_F(QuickJSEngineTest, FlexUIValueRoundTrip) {
  auto ctx = engine_->CreateContext();
  using F = flexui::common::FlexUIValue;
  F input(F::FlexUIValueObjectType{
      {"name", F(std::string("flex"))},
      {"count", F(7.0)},
  });
  auto jv = ctx->FromFlexUIValue(input);
  auto out = jv->ToFlexUIValue();
  EXPECT_EQ(out.ToObjectChecked().at("name").ToStringChecked(), "flex");
  EXPECT_DOUBLE_EQ(out.ToObjectChecked().at("count").ToDoubleChecked(), 7.0);
}

TEST_F(QuickJSEngineTest, MultipleContextsIsolateState) {
  auto a = engine_->CreateContext();
  auto b = engine_->CreateContext();
  flexui::common::Error err = flexui::common::Error::Ok();
  a->Eval("var x = 1", "<a>", &err);
  b->Eval("var x = 99", "<b>", &err);
  EXPECT_DOUBLE_EQ(a->Eval("x", "<a>", &err)->ToNumber(), 1.0);
  EXPECT_DOUBLE_EQ(b->Eval("x", "<b>", &err)->ToNumber(), 99.0);
}

}  // namespace flexui::core::js_engine
