#include "flexui/core/scope-manager/scope.h"
#include "flexui/core/scope-manager/snapshot.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::scope_manager {

const char* ScopeStateName(ScopeState s) {
  switch (s) {
    case ScopeState::kEngineInitd:     return "EngineInitd";
    case ScopeState::kScopeCreating:   return "ScopeCreating";
    case ScopeState::kScopeRunning:    return "ScopeRunning";
    case ScopeState::kScopeDestroying: return "ScopeDestroying";
    case ScopeState::kScopeDestroyed:  return "ScopeDestroyed";
    case ScopeState::kScopeErrored:    return "ScopeErrored";
  }
  return "Unknown";
}

Scope::Scope(ScopeManager* mgr, uint32_t id,
             std::shared_ptr<js_engine::IJsContext> ctx)
    : manager_(mgr), id_(id), ctx_(std::move(ctx)) {
  FLEXUI_TLOG(Scope, Create, INFO) << "id=" << id_;
}

Scope::~Scope() {
  if (state_.load() != ScopeState::kScopeDestroyed) {
    FLEXUI_TLOG(Scope, DestroyMissing, WARNING)
        << "id=" << id_ << " state=" << ScopeStateName(state_.load());
  }
}

flexui::common::Error Scope::TransitionTo(ScopeState next) {
  auto from = state_.exchange(next);
  FLEXUI_TLOG(Scope, StateChange, INFO)
      << "id=" << id_
      << " from=" << ScopeStateName(from)
      << " to=" << ScopeStateName(next);
  return flexui::common::Error::Ok();
}

flexui::common::Error Scope::Begin(const ISnapshot* snapshot) {
  if (snapshot) {
    auto err = snapshot->ApplyTo(*ctx_);
    if (!err.ok()) {
      MarkErrored("snapshot apply failed: " + err.message());
      return err;
    }
  }
  return flexui::common::Error::Ok();
}

flexui::common::Error Scope::MarkRunning()  { return TransitionTo(ScopeState::kScopeRunning); }
flexui::common::Error Scope::MarkErrored(const std::string& reason) {
  FLEXUI_TLOG(Scope, Errored, ERROR) << "id=" << id_ << " reason=" << reason;
  return TransitionTo(ScopeState::kScopeErrored);
}
flexui::common::Error Scope::Destroy() {
  TransitionTo(ScopeState::kScopeDestroying);
  ctx_.reset();
  TransitionTo(ScopeState::kScopeDestroyed);
  return flexui::common::Error::Ok();
}

// MakeEmptySnapshot — PoC stub: no-op snapshot, applies nothing.
namespace {
class EmptySnapshot : public ISnapshot {
 public:
  std::vector<uint8_t> Serialize() const override { return {}; }
  flexui::common::Error ApplyTo(js_engine::IJsContext&) const override {
    return flexui::common::Error::Ok();
  }
};
}  // namespace
std::unique_ptr<ISnapshot> MakeEmptySnapshot() {
  return std::make_unique<EmptySnapshot>();
}

}  // namespace flexui::core::scope_manager
