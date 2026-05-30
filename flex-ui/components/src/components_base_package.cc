/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#include "flexui/components/components_base_package.h"

#include "flexui/common/log_tag.h"

namespace flexui::components {

// Forward decls — defined in each component's .cc.
flexui::core::plugin_host::ComponentFactory MakeTextFactory();
flexui::core::plugin_host::ComponentFactory MakeImageFactory();
flexui::core::plugin_host::ComponentFactory MakeViewFactory();
flexui::core::plugin_host::ComponentFactory MakeButtonFactory();
flexui::core::plugin_host::ComponentFactory MakeScrollViewFactory();

flexui::core::plugin_host::FlexUIPlugin MakeComponentsBasePackage() {
  FLEXUI_TLOG(Plugin, MakeComponentsBase, INFO);
  flexui::core::plugin_host::FlexUIPlugin p;
  p.name = "flexui-components-base";
  p.version = "0.1.0";
  p.components = {
    MakeTextFactory(),
    MakeImageFactory(),
    MakeViewFactory(),
    MakeButtonFactory(),
    MakeScrollViewFactory(),
  };
  return p;
}

}  // namespace flexui::components
