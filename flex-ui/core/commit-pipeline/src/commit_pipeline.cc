#include "flexui/core/commit-pipeline/commit_pipeline.h"

#include "flexui/core/plugin-host/plugin_host.h"
#include "flexui/common/log_tag.h"

namespace flexui::core::commit_pipeline {

namespace {
class DefaultComponentContext : public ComponentContext {
 public:
  DefaultComponentContext(uint32_t s, uint32_t r) : scope_(s), root_(r) {}
  uint32_t scope_id() const override { return scope_; }
  uint32_t root_id()  const override { return root_; }
 private:
  uint32_t scope_, root_;
};
}  // namespace

CommitPipeline::CommitPipeline(plugin_host::PluginHost* h) : plugin_host_(h) {}

void CommitPipeline::Apply(uint32_t scope_id, uint32_t root_id,
                           const reconciler::MutationList& mutations) {
  FLEXUI_TLOG(CommitPipeline, ApplyEnter, DEBUG)
      << "scope=" << scope_id << " root=" << root_id
      << " mutations=" << mutations.size();

  DefaultComponentContext ctx(scope_id, root_id);
  std::lock_guard<std::mutex> lock(mu_);
  for (const auto& m : mutations) {
    std::visit([&](auto&& mut) {
      using T = std::decay_t<decltype(mut)>;
      if constexpr (std::is_same_v<T, reconciler::CreateMutation>) {
        auto* factory = plugin_host_->FindComponent(mut.view_name);
        if (!factory) {
          FLEXUI_TLOG(CommitPipeline, ApplyCreate, ERROR)
              << "unknown component: " << mut.view_name;
          return;
        }
        if (!factory->create) return;
        auto inst = factory->create(ctx);
        if (!inst) return;
        inst->OnCreate();
        instances_[mut.node_id] = std::move(inst);
        FLEXUI_TLOG(CommitPipeline, ApplyCreate, DEBUG)
            << "id=" << mut.node_id << " view=" << mut.view_name;
      } else if constexpr (std::is_same_v<T, reconciler::UpdatePropsMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        PropDelta delta;
        delta.updated = mut.diff;
        delta.deleted = mut.deleted_keys;
        it->second->OnUpdateProps(delta);
      } else if constexpr (std::is_same_v<T, reconciler::UpdateLayoutMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        it->second->OnUpdateLayout({mut.x, mut.y, mut.width, mut.height});
      } else if constexpr (std::is_same_v<T, reconciler::MoveMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        // Parent handle: look up parent instance; use nullptr if not found
        NodeHandle parent_handle = nullptr;
        auto pit = instances_.find(mut.new_parent_id);
        if (pit != instances_.end()) {
          parent_handle = pit->second->OnCreate();
        }
        it->second->OnMount(parent_handle, mut.new_index);
      } else if constexpr (std::is_same_v<T, reconciler::DeleteMutation>) {
        auto it = instances_.find(mut.node_id);
        if (it == instances_.end()) return;
        it->second->OnUnmount();
        instances_.erase(it);
      }
    }, m);
  }

  FLEXUI_TLOG(CommitPipeline, ApplyExit, DEBUG)
      << "scope=" << scope_id << " instances=" << instances_.size();
}

size_t CommitPipeline::InstanceCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return instances_.size();
}

ComponentInstance* CommitPipeline::Find(uint32_t node_id) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = instances_.find(node_id);
  return it == instances_.end() ? nullptr : it->second.get();
}

}  // namespace flexui::core::commit_pipeline
