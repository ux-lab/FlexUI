#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::core::commit_pipeline {

// Platform-opaque node handle. ArkUI_NodeHandle / Android View* / NSView* etc.
using NodeHandle = void*;

struct LayoutRect {
  float x = 0, y = 0, width = 0, height = 0;
};

struct PropDelta {
  std::vector<std::pair<std::string, flexui::common::FlexUIValue>> updated;
  std::vector<std::string> deleted;
};

class ComponentContext {
 public:
  virtual ~ComponentContext() = default;
  virtual uint32_t scope_id() const = 0;
  virtual uint32_t root_id() const = 0;
};

class ComponentInstance {
 public:
  virtual ~ComponentInstance() = default;

  virtual NodeHandle OnCreate() = 0;
  virtual void OnUpdateProps(const PropDelta& delta) = 0;
  virtual void OnUpdateLayout(const LayoutRect& rect) = 0;
  virtual void OnMount(NodeHandle parent, uint32_t index) = 0;
  virtual void OnEvent(const std::string& name,
                       const flexui::common::FlexUIValue& payload) {}
  virtual void OnUnmount() = 0;
};

}  // namespace flexui::core::commit_pipeline
