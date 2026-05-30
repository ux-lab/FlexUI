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
