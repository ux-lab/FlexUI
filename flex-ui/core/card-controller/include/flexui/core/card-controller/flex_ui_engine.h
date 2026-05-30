/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <memory>
#include <string>

#include "flexui/common/error.h"
#include "flexui/common/task_runner.h"
#include "flexui/core/js-engine/ijs_engine.h"
#include "flexui/core/js-engine/js_engine_backend.h"
#include "flexui/core/plugin-host/plugin_host.h"
#include "flexui/core/scope-manager/scope_manager.h"
#include "flexui/core/commit-pipeline/commit_pipeline.h"
#include "flexui/core/bridge/ibridge.h"

namespace flexui::core::card_controller {

struct FlexUIEngineConfig {
  js_engine::JsEngineBackend backend = js_engine::JsEngineBackend::kQuickJS;
  bool force_quickjs_for_debug = false;
  size_t memory_limit_mb = 0;
};

class FlexUIEngine {
 public:
  static FlexUIEngine& Instance();
  flexui::common::Error Init(const FlexUIEngineConfig& cfg);
  flexui::common::Error Install(plugin_host::FlexUIPlugin plugin);
  flexui::common::Error Uninstall(const std::string& plugin_name);
  void Shutdown();

  // Internal accessors used by FlexCardController.
  js_engine::IJsEngine&             js_engine();
  scope_manager::ScopeManager&      scope_manager();
  plugin_host::PluginHost&          plugin_host();
  commit_pipeline::CommitPipeline&  commit_pipeline();
  bridge::IBridge&                  bridge();
  flexui::common::TaskRunner&       js_runner();
  flexui::common::TaskRunner&       ui_runner();

 private:
  FlexUIEngine() = default;
  ~FlexUIEngine() = default;
  FlexUIEngine(const FlexUIEngine&) = delete;
  FlexUIEngine& operator=(const FlexUIEngine&) = delete;

  std::shared_ptr<js_engine::IJsEngine> engine_;
  std::unique_ptr<plugin_host::PluginHost> plugins_;
  std::unique_ptr<scope_manager::ScopeManager> scopes_;
  std::unique_ptr<commit_pipeline::CommitPipeline> commit_;
  std::shared_ptr<flexui::common::TaskRunner> js_runner_;
  std::shared_ptr<flexui::common::TaskRunner> ui_runner_;
  std::shared_ptr<bridge::IBridge> bridge_;
  bool initialized_ = false;
};

}  // namespace flexui::core::card_controller
