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
