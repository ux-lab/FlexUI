#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "flexui/core/reconciler/mutation.h"
#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::core::plugin_host { class PluginHost; }

namespace flexui::core::commit_pipeline {

class CommitPipeline {
 public:
  explicit CommitPipeline(plugin_host::PluginHost* plugin_host);

  // Apply a batch of mutations to the given root.
  void Apply(uint32_t scope_id, uint32_t root_id,
             const reconciler::MutationList& mutations);

  // Lookups (for tests / debugging).
  size_t InstanceCount() const;
  ComponentInstance* Find(uint32_t node_id) const;

 private:
  plugin_host::PluginHost* plugin_host_;   // not owned
  mutable std::mutex mu_;
  std::unordered_map<uint32_t, std::unique_ptr<ComponentInstance>> instances_;
};

}  // namespace flexui::core::commit_pipeline
