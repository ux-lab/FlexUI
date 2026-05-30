/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include "flexui/core/plugin-host/plugin.h"

namespace flexui::components {

// Returns the ComponentsBase plugin (registers Text, Image, View, Button,
// ScrollView). FlexUIEngine::Init() installs it automatically.
flexui::core::plugin_host::FlexUIPlugin MakeComponentsBasePackage();

}  // namespace flexui::components
