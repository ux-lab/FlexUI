/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 */
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "flexui/common/error.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/card-controller/flex_card_state.h"
#include "flexui/core/commit-pipeline/component_instance.h"

namespace flexui::core::scope_manager { class Scope; }

namespace flexui::core::card_controller {

class FlexUIEngine;

struct FlexCardControllerOptions {
  std::string bundle_uri;          // file://, https://, custom scheme
  std::string bundle_inline;       // alternative: full bundle text/json
  std::string bundle_type;         // "card-js" / "a2ui-json"
  flexui::common::FlexUIValue initial_data;
  size_t memory_limit_mb = 0;
};

class FlexCardController {
 public:
  explicit FlexCardController(FlexCardControllerOptions options);
  ~FlexCardController();

  uint32_t scope_id() const;
  FlexCardState state() const { return state_.load(); }

  flexui::common::Error Load();
  flexui::common::Error SetData(flexui::common::FlexUIValue data);
  flexui::common::Error CallMethod(const std::string& name,
                                   std::vector<flexui::common::FlexUIValue> args,
                                   flexui::common::FlexUIValue* result_out);
  void OnEvent(std::string event,
               std::function<void(flexui::common::FlexUIValue)> handler);
  void OnError(std::function<void(flexui::common::Error)> handler);
  flexui::common::Error Destroy();

  // ETS / NAPI side calls this to attach a platform node container.
  void AttachNodeContent(commit_pipeline::NodeHandle node_container);
  void DetachNodeContent();

 private:
  FlexCardControllerOptions options_;
  std::shared_ptr<scope_manager::Scope> scope_;
  std::atomic<FlexCardState> state_{FlexCardState::kIdle};
  std::function<void(flexui::common::Error)> error_handler_;
  std::unordered_map<std::string,
                     std::function<void(flexui::common::FlexUIValue)>>
      event_handlers_;
  commit_pipeline::NodeHandle node_container_ = nullptr;
};

}  // namespace flexui::core::card_controller
