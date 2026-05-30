/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/plugin_a2ui/a2ui_plugin.h"
#include "flexui/plugin_a2ui/a2ui_json_frontend.h"
#include "flexui/common/log_tag.h"

namespace flexui::plugin_a2ui {

flexui::core::plugin_host::FlexUIPlugin MakeA2UIPlugin() {
  FLEXUI_TLOG(Plugin, MakeA2UI, INFO);
  flexui::core::plugin_host::FlexUIPlugin p;
  p.name = "flexui-a2ui";
  p.version = "0.1.0-mvp";
  p.frontends.push_back({"a2ui-json", []() {
    return std::make_unique<A2UIJsonFrontend>();
  }});
  return p;
}

}  // namespace flexui::plugin_a2ui
