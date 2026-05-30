#include "flexui/core/card-controller/flex_card_controller.h"

#include <memory>

#include "flexui/core/card-controller/flex_ui_engine.h"
#include "flexui/core/scope-manager/scope_manager.h"
#include "flexui/core/plugin-host/frontend.h"
#include "flexui/core/vdom/dom_node.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::card_controller {

const char* FlexCardStateName(FlexCardState s) {
  switch (s) {
    case FlexCardState::kIdle:      return "Idle";
    case FlexCardState::kLoading:   return "Loading";
    case FlexCardState::kRunning:   return "Running";
    case FlexCardState::kErrored:   return "Errored";
    case FlexCardState::kDestroyed: return "Destroyed";
  }
  return "Unknown";
}

FlexCardController::FlexCardController(FlexCardControllerOptions opts)
    : options_(std::move(opts)) {
  scope_ = FlexUIEngine::Instance().scope_manager().CreateScope();
  FLEXUI_TLOG(Card, Construct, INFO)
      << "scope=" << (scope_ ? scope_->id() : 0)
      << " bundle_type=" << options_.bundle_type;
}

FlexCardController::~FlexCardController() {
  if (state_.load() != FlexCardState::kDestroyed) Destroy();
}

uint32_t FlexCardController::scope_id() const {
  return scope_ ? scope_->id() : 0;
}

flexui::common::Error FlexCardController::Load() {
  state_.store(FlexCardState::kLoading);
  FLEXUI_TLOG(Card, LoadEnter, INFO)
      << "scope=" << scope_id() << " uri=" << options_.bundle_uri;
  if (!scope_) {
    return flexui::common::Error(flexui::common::ErrorCode::kInternal, "no scope");
  }
  auto err = scope_->Begin(nullptr);
  if (!err.ok()) {
    state_.store(FlexCardState::kErrored);
    if (error_handler_) error_handler_(err);
    return err;
  }

  // Resolve frontend.
  const auto* reg = FlexUIEngine::Instance().plugin_host()
                        .FindFrontend(options_.bundle_type);
  if (!reg) {
    auto e = flexui::common::Error(flexui::common::ErrorCode::kNotFound,
                                   "no frontend for bundle_type: " + options_.bundle_type);
    state_.store(FlexCardState::kErrored);
    if (error_handler_) error_handler_(e);
    return e;
  }
  frontend_ = reg->factory();

  plugin_host::BundleSource src;
  src.uri  = options_.bundle_uri;
  src.text = options_.bundle_inline;
  frontend_->Initialize(*scope_, src);

  last_dom_tree_ = frontend_->Render(*scope_, options_.initial_data);

  scope_->MarkRunning();
  state_.store(FlexCardState::kRunning);
  FLEXUI_TLOG(Card, LoadExit, INFO) << "scope=" << scope_id();
  return flexui::common::Error::Ok();
}

flexui::common::Error FlexCardController::SetData(flexui::common::FlexUIValue data) {
  FLEXUI_TLOG(Card, SetData, DEBUG) << "scope=" << scope_id();
  if (!frontend_ || !scope_) return flexui::common::Error::Ok();
  last_dom_tree_ = frontend_->Render(*scope_, data);
  return flexui::common::Error::Ok();
}

std::shared_ptr<flexui::core::vdom::DomNode> FlexCardController::DebugLastDomTree() const {
  return last_dom_tree_;
}

flexui::common::Error FlexCardController::CallMethod(
    const std::string& name,
    std::vector<flexui::common::FlexUIValue> args,
    flexui::common::FlexUIValue* result) {
  FLEXUI_TLOG(Card, CallMethod, DEBUG)
      << "scope=" << scope_id() << " name=" << name;
  (void)args; (void)result;
  return flexui::common::Error::Ok();
}

void FlexCardController::OnEvent(std::string e,
                                 std::function<void(flexui::common::FlexUIValue)> h) {
  event_handlers_[std::move(e)] = std::move(h);
}

void FlexCardController::OnError(std::function<void(flexui::common::Error)> h) {
  error_handler_ = std::move(h);
}

flexui::common::Error FlexCardController::Destroy() {
  FLEXUI_TLOG(Card, DestroyEnter, INFO) << "scope=" << scope_id();
  if (scope_) {
    FlexUIEngine::Instance().scope_manager().DestroyScope(scope_->id());
    scope_.reset();
  }
  state_.store(FlexCardState::kDestroyed);
  FLEXUI_TLOG(Card, DestroyExit, INFO);
  return flexui::common::Error::Ok();
}

void FlexCardController::AttachNodeContent(commit_pipeline::NodeHandle h) {
  FLEXUI_TLOG(Card, AttachNodeContent, INFO) << "scope=" << scope_id();
  node_container_ = h;
}

void FlexCardController::DetachNodeContent() {
  FLEXUI_TLOG(Card, DetachNodeContent, INFO) << "scope=" << scope_id();
  node_container_ = nullptr;
}

}  // namespace flexui::core::card_controller
