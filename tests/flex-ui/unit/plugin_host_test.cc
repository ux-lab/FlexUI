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
