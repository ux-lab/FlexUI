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
