/*
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2022 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Modified by the FlexUI authors. This file is derived from
 * dom/include/dom/dom_node.h in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Namespace: `hippy::dom` -> `flexui::core::vdom`.
 *   - Include paths: `footstone/` -> `flexui/common/`, `dom/` -> `flexui/core/vdom/`.
 *   - Type rename: `HippyValue` -> `FlexUIValue` (all occurrences).
 *   - `footstone::TaskRunner` -> `flexui::common::TaskRunner`.
 *   - Removed absent-dep includes with FlexUI markers (see below).
 *   - Removed types/methods that depend on removed Hippy subsystems with markers.
 */

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

// FlexUI: removed Hippy dependency on dom_listener (orchestration moved to commit-pipeline/).
// #include "dom/dom_listener.h"
// FlexUI: removed Hippy dependency on dom_event (event dispatch moved to bridge/).
// #include "dom/dom_event.h"
// FlexUI: removed Hippy dependency on render_manager (renderer is per-platform under platforms/).
// #include "dom/render_manager.h"
// FlexUI: removed Hippy dependency on dom_action_interceptor (no equivalent in FlexUI).
// #include "dom/dom_action_interceptor.h"

#include "flexui/common/check.h"
#include "flexui/common/flexui_value.h"
#include "flexui/core/vdom/dom_argument.h"

namespace flexui::core::vdom {

// Forward declarations
class DomNode;

// FlexUI: CallFunctionCallback moved here from removed dom_listener.h.
using CallFunctionCallback = std::function<void(std::shared_ptr<DomArgument>)>;

// FlexUI: LayoutResult moved here from removed dom_listener.h.
struct LayoutResult {
  float left = 0;
  float top = 0;
  float width = 0;
  float height = 0;
  float marginLeft = 0;
  float marginTop = 0;
  float marginRight = 0;
  float marginBottom = 0;
  float paddingLeft = 0;
  float paddingTop = 0;
  float paddingRight = 0;
  float paddingBottom = 0;
};

// FlexUI: kLayoutEvent moved here from removed dom_listener.h.
constexpr char kLayoutEvent[] = "layout";

constexpr uint32_t kCapture = 0;
constexpr uint32_t kBubble = 1;
constexpr uint32_t kInvalidId = 0;
constexpr int32_t kInvalidIndex = -1;

enum RelativeType {
  kFront = -1,
  kDefault = 0,
  kBack = 1,
};

struct DiffInfo {
  bool skip_style_diff;
  DiffInfo(bool skip_style_diff) : skip_style_diff(skip_style_diff) {}

 private:
  friend std::ostream& operator<<(std::ostream& os, const DiffInfo& diff_info);
};

struct RefInfo {
  uint32_t ref_id;
  int32_t relative_to_ref = RelativeType::kDefault;
  RefInfo(uint32_t id, int32_t ref) : ref_id(id), relative_to_ref(ref) {}

 private:
  friend std::ostream& operator<<(std::ostream& os, const RefInfo& ref_info);
};

struct DomInfo {
  std::shared_ptr<DomNode> dom_node;
  std::shared_ptr<RefInfo> ref_info;
  std::shared_ptr<DiffInfo> diff_info;
  DomInfo(std::shared_ptr<DomNode> node, std::shared_ptr<RefInfo> ref,
          std::shared_ptr<DiffInfo> diff)
      : dom_node(node), ref_info(ref), diff_info(diff) {}

 private:
  friend std::ostream& operator<<(std::ostream& os, const DomInfo& dom_info);
};

// FlexUI: DomEventListenerInfo kept for internal event-listener bookkeeping.
// EventCallback is self-contained (no DomEvent dependency) — calls are
// dispatched by the bridge layer (W5-W6).
// FlexUI: removed EventCallback typedef that depended on DomEvent.
// using EventCallback = std::function<void(const std::shared_ptr<DomEvent>&)>;

struct DomEventListenerInfo {
  uint64_t id;
  // FlexUI: removed EventCallback cb; — DomEvent dispatch moved to bridge/.
  // Listener bookkeeping retained; callback will be re-introduced as
  // flexui::bridge::EventCallback in W5-W6.
  DomEventListenerInfo(uint64_t id) : id(id) {}
};

class DomNode : public std::enable_shared_from_this<DomNode> {
 public:
  using FlexUIValue = flexui::common::FlexUIValue;

  DomNode(uint32_t id, uint32_t pid, int32_t index, std::string tag_name,
          std::string view_name,
          std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> style_map,
          std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> dom_ext_map);

  DomNode(uint32_t id, uint32_t pid);
  DomNode();
  virtual ~DomNode();

  // RenderNode info after layer optimization
  struct RenderInfo {
    uint32_t id = kInvalidId;
    uint32_t pid = kInvalidId;
    int32_t index = kInvalidIndex;
    int32_t depth = kInvalidIndex;
  };

  inline std::shared_ptr<DomNode> GetParent() { return parent_.lock(); }
  inline void SetParent(std::shared_ptr<DomNode> parent) { parent_ = parent; }
  inline uint32_t GetChildCount() const {
    return flexui::common::check::checked_numeric_cast<size_t, uint32_t>(children_.size());
  }
  inline void SetTagName(const std::string& tag_name) { tag_name_ = tag_name; }
  inline const std::string& GetTagName() { return tag_name_; }
  inline void SetViewName(const std::string& view_name) { view_name_ = view_name; }
  inline const std::string& GetViewName() { return view_name_; }
  inline void SetId(uint32_t id) { id_ = id; }
  inline uint32_t GetId() const { return id_; }
  inline void SetPid(uint32_t pid) { pid_ = pid; }
  inline uint32_t GetPid() const { return pid_; }
  inline const RenderInfo& GetRenderInfo() const { return render_info_; }
  inline void SetRenderInfo(const RenderInfo& render_info) { render_info_ = render_info; }
  inline bool IsLayoutOnly() const { return layout_only_; }
  inline void SetLayoutOnly(bool layout_only) { layout_only_ = layout_only; }
  inline bool IsVirtual() { return is_virtual_; }
  inline void SetIsVirtual(bool is_virtual) { is_virtual_ = is_virtual; }
  inline bool IsEnableEliminated() { return enable_eliminated_; }
  inline void SetEnableEliminated(bool enable_eliminated) { enable_eliminated_ = enable_eliminated; }
  inline void SetIndex(int32_t index) { index_ = index; }
  inline int32_t GetIndex() const { return index_; }

  // FlexUI: removed AddEventListener/RemoveEventListener that depended on
  // EventCallback (which required DomEvent); event dispatch moves to bridge/ in W5-W6.

  void MarkWillChange(bool flag);
  int32_t GetSelfIndex();
  int32_t GetChildIndex(uint32_t id);
  int32_t GetSelfDepth();

  int32_t IndexOf(const std::shared_ptr<DomNode>& child);
  std::shared_ptr<DomNode> GetChildAt(size_t index);
  const std::vector<std::shared_ptr<DomNode>>& GetChildren() { return children_; }
  int32_t AddChildByRefInfo(const std::shared_ptr<DomInfo>& dom_node);
  std::shared_ptr<DomNode> RemoveChildAt(int32_t index);
  std::shared_ptr<DomNode> RemoveChildById(uint32_t id);

  // FlexUI: DoLayout removed — layout engine absorbed in T5 (W3-W4).
  // void DoLayout();
  // void DoLayout(std::vector<std::shared_ptr<DomNode>>& changed_nodes);

  // FlexUI: ParseLayoutStyleInfo / UpdateLayoutStyleInfo / ResetLayoutCache /
  // GetLayoutInfoFromRoot / TransferLayoutOutputsRecursive / GetLayoutSize /
  // SetLayoutSize / SetLayoutOrigin removed — layout engine absorbed in T5 (W3-W4).

  const LayoutResult& GetLayoutResult() const { return layout_; }
  const LayoutResult& GetRenderLayoutResult() const { return render_layout_; }

  // FlexUI: GetEventListener removed — event dispatch moved to bridge/ in W5-W6.
  // std::vector<std::shared_ptr<DomEventListenerInfo>> GetEventListener(
  //     const std::string& name, bool is_capture);

  const std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>&
  GetStyleMap() const {
    return style_map_;
  }
  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>&
  GetStyleMap() {
    return style_map_;
  }
  void SetStyleMap(
      std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> style) {
    style_map_ = style;
  }

  // FlexUI: CallFunction removed — render_manager dependency removed; will be
  // re-introduced via platform bridge in W5-W6.
  // void CallFunction(const std::string& name, const DomArgument& param,
  //                   const CallFunctionCallback& cb);

  const std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>
  GetExtStyle() {
    return dom_ext_map_;
  }
  void SetExtStyleMap(
      std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> style) {
    dom_ext_map_ = style;
  }
  const std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>
  GetDiffStyle() {
    return diff_;
  }
  void SetDiffStyle(
      std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> diff) {
    diff_ = std::move(diff);
  }
  const std::shared_ptr<std::vector<std::string>> GetDeleteProps() { return delete_props_; }
  void SetDeleteProps(std::shared_ptr<std::vector<std::string>> delete_props) {
    delete_props_ = delete_props;
  }

  // FlexUI: GetCallback removed together with CallFunction.
  // CallFunctionCallback GetCallback(const std::string& name, uint32_t id);

  bool HasEventListeners();

  void EmplaceStyleMap(const std::string& key, const FlexUIValue& value);
  void EmplaceStyleMapAndGetDiff(
      const std::string& key, const FlexUIValue& value,
      std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& diff);

  void UpdateProperties(
      const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style,
      const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_dom_ext);

  void UpdateDomNodeStyleAndParseLayoutInfo(
      const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style);

  FlexUIValue Serialize() const;
  bool Deserialize(FlexUIValue value);

  // FlexUI: HandleEvent removed — DomEvent dispatch moved to bridge/ in W5-W6.
  // virtual void HandleEvent(const std::shared_ptr<DomEvent>& event);

 private:
  void UpdateDiff(
      const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style,
      const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_dom_ext);
  void UpdateDomExt(
      const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_dom_ext);
  void UpdateStyle(
      const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style);
  void UpdateObjectStyle(FlexUIValue& style_map, const FlexUIValue& update_style);
  bool ReplaceStyle(FlexUIValue& object, const std::string& key, const FlexUIValue& value);

  friend std::ostream& operator<<(std::ostream& os, const DomNode& node);

 private:
  uint32_t id_{};
  uint32_t pid_{};
  int32_t index_{};
  std::string tag_name_;
  std::string view_name_;
  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> style_map_;
  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> dom_ext_map_;
  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> diff_;
  std::shared_ptr<std::vector<std::string>> delete_props_;

  LayoutResult layout_;
  LayoutResult render_layout_;
  bool is_virtual_{};
  bool layout_only_ = false;
  bool is_layout_width_nan_ = false;
  bool is_layout_height_nan_ = false;
  bool enable_eliminated_ = true;

  std::weak_ptr<DomNode> parent_;
  std::vector<std::shared_ptr<DomNode>> children_;

  RenderInfo render_info_;
  uint32_t current_callback_id_{};

  // FlexUI: func_cb_map_ removed — CallFunction moved to bridge/ in W5-W6.
  // FlexUI: event_listener_map_ retained for bookkeeping; EventCallback
  // generics re-wired in W5-W6.
  std::shared_ptr<std::unordered_map<std::string,
      std::array<std::vector<std::shared_ptr<DomEventListenerInfo>>, 2>>>
      event_listener_map_;
};

}  // namespace flexui::core::vdom
