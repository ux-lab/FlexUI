#pragma once
#include <memory>
#include <string>

#include "flexui/common/flexui_value.h"

namespace flexui::core::commit_pipeline {
class ComponentInstance;
class ComponentContext;
}

namespace flexui::core::plugin_host {

enum class ComponentImplementation { kCapi, kEtsBuilder };

struct ComponentFactory {
  std::string name;
  ComponentImplementation implementation = ComponentImplementation::kCapi;
  std::function<std::unique_ptr<commit_pipeline::ComponentInstance>(
      commit_pipeline::ComponentContext&)> create;
};

}  // namespace flexui::core::plugin_host
