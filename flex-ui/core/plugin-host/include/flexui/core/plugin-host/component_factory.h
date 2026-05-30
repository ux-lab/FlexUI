#pragma once
#include <memory>
#include <string>

#include "flexui/common/flexui_value.h"

namespace flexui::core::commit_pipeline { class ComponentInstance; }

namespace flexui::core::plugin_host {

enum class ComponentImplementation { kCapi, kEtsBuilder };

class ComponentContext;   // forward; defined in commit-pipeline

struct ComponentFactory {
  std::string name;
  ComponentImplementation implementation = ComponentImplementation::kCapi;
  std::function<std::unique_ptr<commit_pipeline::ComponentInstance>(
      ComponentContext&)> create;
};

}  // namespace flexui::core::plugin_host
