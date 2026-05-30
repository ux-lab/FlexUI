/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include <gtest/gtest.h>

#include "flexui/components/components_base_package.h"

namespace flexui::components {

TEST(ComponentsBasePackageTest, MakeComponentsBasePackageHasFiveComponents) {
  auto pkg = MakeComponentsBasePackage();
  EXPECT_EQ(pkg.name, "flexui-components-base");
  EXPECT_EQ(pkg.components.size(), 5u);
}

TEST(ComponentsBasePackageTest, AllFactoriesAreCapiMode) {
  auto pkg = MakeComponentsBasePackage();
  for (auto& f : pkg.components) {
    EXPECT_EQ(f.implementation,
              flexui::core::plugin_host::ComponentImplementation::kCapi);
    EXPECT_TRUE(f.create != nullptr);
  }
}

TEST(ComponentsBasePackageTest, ComponentNames) {
  auto pkg = MakeComponentsBasePackage();
  std::vector<std::string> names;
  for (auto& f : pkg.components) names.push_back(f.name);
  EXPECT_NE(std::find(names.begin(), names.end(), "Text"), names.end());
  EXPECT_NE(std::find(names.begin(), names.end(), "Image"), names.end());
  EXPECT_NE(std::find(names.begin(), names.end(), "View"), names.end());
  EXPECT_NE(std::find(names.begin(), names.end(), "Button"), names.end());
  EXPECT_NE(std::find(names.begin(), names.end(), "ScrollView"), names.end());
}

}  // namespace flexui::components
