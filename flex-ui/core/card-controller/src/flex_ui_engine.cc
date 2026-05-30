#include "flexui/core/card-controller/flex_ui_engine.h"

#include "flexui/core/js-engine/js_engine_factory.h"
#include "flexui/core/bridge/inprocess_bridge.h"
#include "flexui/common/log_tag.h"
#include "flexui/components/components_base_package.h"
#include "flexui/api/api_base_package.h"
#include "flexui/core/card-controller/frontends/card_js/card_js_frontend.h"

namespace flexui::core::card_controller {

FlexUIEngine& FlexUIEngine::Instance() {
  static FlexUIEngine engine;
  return engine;
}

flexui::common::Error FlexUIEngine::Init(const FlexUIEngineConfig& cfg) {
  if (initialized_) {
    return flexui::common::Error(flexui::common::ErrorCode::kAlreadyExists,
                                 "engine already initialized");
  }
  FLEXUI_TLOG(Engine, Init, INFO) << "backend=" << static_cast<int>(cfg.backend);
  engine_ = js_engine::MakeJsEngine(cfg.backend);
  if (!engine_) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal,
                                 "MakeJsEngine returned null");
  }
  js_engine::EngineConfig ecfg;
  ecfg.memory_limit_mb = cfg.memory_limit_mb;
  auto err = engine_->Initialize(ecfg);
  if (!err.ok()) return err;

  plugins_ = std::make_unique<plugin_host::PluginHost>();
  scopes_  = std::make_unique<scope_manager::ScopeManager>(engine_);
  commit_  = std::make_unique<commit_pipeline::CommitPipeline>(plugins_.get());

  js_runner_ = std::make_shared<flexui::common::TaskRunner>("flexui-js");
  ui_runner_ = std::make_shared<flexui::common::TaskRunner>("flexui-ui");
  bridge_ = std::make_shared<bridge::InProcessBridge>(js_runner_, ui_runner_);

  // Auto-register built-in packages.
  {
    auto pkg = flexui::components::MakeComponentsBasePackage();
    auto e = plugins_->Install(std::move(pkg));
    if (!e.ok()) return e;
  }
  {
    auto pkg = flexui::api::MakeApiBasePackage();
    auto e = plugins_->Install(std::move(pkg));
    if (!e.ok()) return e;
  }
  {
    plugin_host::FlexUIPlugin p;
    p.name = "flexui-frontend-card-js";
    p.frontends.push_back(
        frontends::card_js::MakeCardJsRegistration());
    auto e = plugins_->Install(std::move(p));
    if (!e.ok()) return e;
  }

  initialized_ = true;
  return flexui::common::Error::Ok();
}

flexui::common::Error FlexUIEngine::Install(plugin_host::FlexUIPlugin plugin) {
  if (!initialized_) return flexui::common::Error(
      flexui::common::ErrorCode::kEngineNotInitialized, "engine not initialized");
  return plugins_->Install(std::move(plugin));
}

flexui::common::Error FlexUIEngine::Uninstall(const std::string& n) {
  if (!initialized_) return flexui::common::Error(
      flexui::common::ErrorCode::kEngineNotInitialized, "engine not initialized");
  return plugins_->Uninstall(n);
}

void FlexUIEngine::Shutdown() {
  FLEXUI_TLOG(Engine, Shutdown, INFO);
  commit_.reset();
  scopes_.reset();
  plugins_.reset();
  bridge_.reset();
  js_runner_.reset();
  ui_runner_.reset();
  if (engine_) engine_->Shutdown();
  engine_.reset();
  initialized_ = false;
}

js_engine::IJsEngine& FlexUIEngine::js_engine() { return *engine_; }
scope_manager::ScopeManager& FlexUIEngine::scope_manager() { return *scopes_; }
plugin_host::PluginHost& FlexUIEngine::plugin_host() { return *plugins_; }
commit_pipeline::CommitPipeline& FlexUIEngine::commit_pipeline() { return *commit_; }
bridge::IBridge& FlexUIEngine::bridge() { return *bridge_; }
flexui::common::TaskRunner& FlexUIEngine::js_runner() { return *js_runner_; }
flexui::common::TaskRunner& FlexUIEngine::ui_runner() { return *ui_runner_; }

}  // namespace flexui::core::card_controller
