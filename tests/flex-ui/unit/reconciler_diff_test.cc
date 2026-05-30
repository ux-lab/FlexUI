/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Unit tests for DiffUtils (absorbed from Hippy) and the Mutation typedef.
 * The DiffProps API returns DiffValue = tuple<shared_ptr<DomValueMap>, shared_ptr<vector<string>>>:
 *   get<0> = update_props map
 *   get<1> = delete_props vector
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "flexui/core/reconciler/diff_utils.h"
#include "flexui/core/reconciler/mutation.h"

namespace flexui::core::reconciler {

// DomValueMap shorthand
using Map = DomValueMap;

TEST(ReconcilerDiffTest, IdenticalMapsProduceNoChanges) {
  auto old_map = std::make_shared<Map>();
  auto new_map = std::make_shared<Map>();
  (*old_map)["color"] = std::make_shared<FlexUIValue>(std::string("#000"));
  (*new_map)["color"] = std::make_shared<FlexUIValue>(std::string("#000"));

  auto result = DiffUtils::DiffProps(*old_map, *new_map, /*skip_style_diff=*/false);
  auto update_props = std::get<0>(result);
  auto delete_props = std::get<1>(result);
  EXPECT_TRUE(update_props == nullptr || update_props->empty());
  EXPECT_TRUE(delete_props == nullptr || delete_props->empty());
}

TEST(ReconcilerDiffTest, ChangedPropProducesUpdate) {
  auto old_map = std::make_shared<Map>();
  auto new_map = std::make_shared<Map>();
  (*old_map)["color"] = std::make_shared<FlexUIValue>(std::string("#000"));
  (*new_map)["color"] = std::make_shared<FlexUIValue>(std::string("#fff"));

  auto result = DiffUtils::DiffProps(*old_map, *new_map, /*skip_style_diff=*/false);
  auto update_props = std::get<0>(result);
  ASSERT_NE(update_props, nullptr);
  EXPECT_EQ(update_props->size(), 1u);
  EXPECT_NE(update_props->find("color"), update_props->end());
}

TEST(ReconcilerDiffTest, RemovedPropAppearsInDeleteList) {
  auto old_map = std::make_shared<Map>();
  auto new_map = std::make_shared<Map>();
  (*old_map)["opacity"] = std::make_shared<FlexUIValue>(1.0);
  // new_map doesn't have "opacity"

  auto result = DiffUtils::DiffProps(*old_map, *new_map, /*skip_style_diff=*/false);
  auto delete_props = std::get<1>(result);
  ASSERT_NE(delete_props, nullptr);
  ASSERT_EQ(delete_props->size(), 1u);
  EXPECT_EQ((*delete_props)[0], "opacity");
}

TEST(ReconcilerDiffTest, NewPropAppearsInUpdateList) {
  auto old_map = std::make_shared<Map>();
  auto new_map = std::make_shared<Map>();
  (*new_map)["fontSize"] = std::make_shared<FlexUIValue>(14.0);

  auto result = DiffUtils::DiffProps(*old_map, *new_map, /*skip_style_diff=*/false);
  auto update_props = std::get<0>(result);
  ASSERT_NE(update_props, nullptr);
  EXPECT_EQ(update_props->size(), 1u);
  EXPECT_NE(update_props->find("fontSize"), update_props->end());
}

TEST(ReconcilerDiffTest, SkipStyleDiffReturnsBothEmpty) {
  auto old_map = std::make_shared<Map>();
  auto new_map = std::make_shared<Map>();
  (*old_map)["color"] = std::make_shared<FlexUIValue>(std::string("#000"));
  (*new_map)["color"] = std::make_shared<FlexUIValue>(std::string("#fff"));

  auto result = DiffUtils::DiffProps(*old_map, *new_map, /*skip_style_diff=*/true);
  auto update_props = std::get<0>(result);
  auto delete_props = std::get<1>(result);
  EXPECT_TRUE(update_props == nullptr || update_props->empty());
  EXPECT_TRUE(delete_props == nullptr || delete_props->empty());
}

TEST(MutationTypedefTest, VariantHoldsAllTypes) {
  MutationList list;
  list.push_back(CreateMutation{1, 0, 0, "View"});
  list.push_back(UpdatePropsMutation{1, {}, {}});
  list.push_back(UpdateLayoutMutation{1, 0.f, 0.f, 100.f, 50.f});
  list.push_back(MoveMutation{1, 2, 0});
  list.push_back(DeleteMutation{1});
  EXPECT_EQ(list.size(), 5u);
  EXPECT_TRUE(std::holds_alternative<CreateMutation>(list[0]));
  EXPECT_TRUE(std::holds_alternative<DeleteMutation>(list[4]));
}

}  // namespace flexui::core::reconciler
