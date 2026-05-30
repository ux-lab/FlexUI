/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * FlexUI Mutation type. Output of Diff and input of commit-pipeline.
 * The set is fixed: Create, UpdateProps, UpdateLayout, Move, Delete.
 */
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::reconciler {

struct CreateMutation {
  uint32_t node_id;
  uint32_t parent_id;
  uint32_t index;
  std::string view_name;
};

struct UpdatePropsMutation {
  uint32_t node_id;
  std::vector<std::pair<std::string, flexui::common::FlexUIValue>> diff;
  std::vector<std::string> deleted_keys;
};

struct UpdateLayoutMutation {
  uint32_t node_id;
  float x, y, width, height;
};

struct MoveMutation {
  uint32_t node_id;
  uint32_t new_parent_id;
  uint32_t new_index;
};

struct DeleteMutation {
  uint32_t node_id;
};

using Mutation = std::variant<
    CreateMutation,
    UpdatePropsMutation,
    UpdateLayoutMutation,
    MoveMutation,
    DeleteMutation>;

using MutationList = std::vector<Mutation>;

}  // namespace flexui::core::reconciler
