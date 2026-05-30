#include "flexui/core/scope-manager/scope_manager.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::scope_manager {

ScopeManager::ScopeManager(std::shared_ptr<js_engine::IJsEngine> engine)
    : engine_(std::move(engine)) {}

ScopeManager::~ScopeManager() {
  std::lock_guard<std::mutex> lock(mu_);
  for (auto& kv : scopes_) kv.second->Destroy();
  scopes_.clear();
}

std::shared_ptr<Scope> ScopeManager::CreateScope() {
  auto ctx = engine_->CreateContext();
  if (!ctx) {
    FLEXUI_TLOG(Scope, CreateFail, ERROR) << "engine returned null context";
    return nullptr;
  }
  uint32_t id = next_id_.fetch_add(1);
  auto scope = std::make_shared<Scope>(this, id, std::move(ctx));
  std::lock_guard<std::mutex> lock(mu_);
  scopes_[id] = scope;
  return scope;
}

std::shared_ptr<Scope> ScopeManager::Find(uint32_t id) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = scopes_.find(id);
  return it == scopes_.end() ? nullptr : it->second;
}

flexui::common::Error ScopeManager::DestroyScope(uint32_t id) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = scopes_.find(id);
  if (it == scopes_.end()) {
    return flexui::common::Error(flexui::common::ErrorCode::kNotFound,
                                 "no such scope");
  }
  it->second->Destroy();
  scopes_.erase(it);
  return flexui::common::Error::Ok();
}

size_t ScopeManager::ScopeCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return scopes_.size();
}

}  // namespace flexui::core::scope_manager
