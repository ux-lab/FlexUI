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
 * dom/src/dom/dom_node.cc in the Hippy project.
 *
 * The FlexUI modifications:
 *   - Namespace: `hippy::dom` -> `flexui::core::vdom`.
 *   - Include paths: `footstone/` -> `flexui/common/`, `dom/` -> `flexui/core/vdom/`.
 *   - Type rename: `HippyValue` -> `FlexUIValue`.
 *   - Removed includes for absent Hippy subsystems:
 *       dom/render_manager.h, dom/root_node.h, dom/scene.h, dom/diff_utils.h,
 *       dom/animation/animation_manager.h, footstone/serializer.h (used here
 *       only in DomNode serialization via FlexUIValue which serializes itself).
 *   - Methods that referenced RootNode / RenderManager / DomEvent had their
 *     bodies replaced with FlexUI stubs (FLEXUI_TLOG + sensible defaults).
 *     See inline comments for each stub.
 *   - DomNode constructor signature simplified: LayoutNode params removed
 *     (layout engine absorbed in T5, W3-W4).
 *   - UpdateProperties / UpdateDiff: DiffUtils::DiffProps removed — diff
 *     logic migrated to commit-pipeline/ in W5-W6; simple UpdateStyle/UpdateDomExt
 *     called directly.
 */

#include "flexui/core/vdom/dom_node.h"

#include <algorithm>
#include <map>
#include <utility>

#include "flexui/common/check.h"
#include "flexui/common/log_tag.h"
#include "flexui/common/logging.h"

namespace flexui::core::vdom {

using FlexUIValue = flexui::common::FlexUIValue;
using FlexUIValueObjectType = flexui::common::FlexUIValue::FlexUIValueObjectType;

constexpr char kLayoutLayoutKey[] = "layout";
constexpr char kLayoutXKey[] = "x";
constexpr char kLayoutYKey[] = "y";
constexpr char kLayoutWidthKey[] = "width";
constexpr char kLayoutHeightKey[] = "height";

constexpr char kNodePropertyId[] = "id";
constexpr char kNodePropertyPid[] = "pId";
constexpr char kNodePropertyIndex[] = "index";
constexpr char kNodePropertyTagName[] = "tagName";
constexpr char kNodePropertyViewName[] = "name";
constexpr char kNodePropertyStyle[] = "style";
constexpr char kNodePropertyExt[] = "ext";

constexpr char kNodeWillChangeKey[] = "willChange";

const std::map<int32_t, std::string> kRelativeTypeMap = {
    {-1, "kFront"},
    {0, "kDefault"},
    {1, "kBack"},
};

// ---------------------------------------------------------------------------
// Constructors / destructor
// ---------------------------------------------------------------------------

DomNode::DomNode(uint32_t id, uint32_t pid, int32_t index, std::string tag_name,
                 std::string view_name,
                 std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> style_map,
                 std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>> dom_ext_map)
    : id_(id),
      pid_(pid),
      index_(index),
      tag_name_(std::move(tag_name)),
      view_name_(std::move(view_name)),
      style_map_(std::move(style_map)),
      dom_ext_map_(std::move(dom_ext_map)),
      is_virtual_(false),
      current_callback_id_(0),
      event_listener_map_(nullptr) {}

DomNode::DomNode(uint32_t id, uint32_t pid)
    : DomNode(id, pid, 0, "", "", nullptr, nullptr) {}

DomNode::DomNode() : DomNode(0, 0) {}

DomNode::~DomNode() = default;

// ---------------------------------------------------------------------------
// Child management
// ---------------------------------------------------------------------------

int32_t DomNode::IndexOf(const std::shared_ptr<DomNode>& child) {
  for (size_t i = 0; i < children_.size(); i++) {
    if (children_[i] == child) {
      return flexui::common::check::checked_numeric_cast<size_t, int32_t>(i);
    }
  }
  return kInvalidIndex;
}

std::shared_ptr<DomNode> DomNode::GetChildAt(size_t index) {
  if (index >= children_.size()) {
    return nullptr;
  }
  return children_[index];
}

int32_t DomNode::AddChildByRefInfo(const std::shared_ptr<DomInfo>& dom_info) {
  std::shared_ptr<RefInfo>& ref_info = dom_info->ref_info;
  if (ref_info) {
    if (children_.size() == 0) {
      children_.push_back(dom_info->dom_node);
    } else {
      for (uint32_t i = 0; i < children_.size(); ++i) {
        auto& child = children_[i];
        if (ref_info->ref_id == child->GetId()) {
          if (ref_info->relative_to_ref == RelativeType::kFront) {
            children_.insert(
                children_.begin() +
                    flexui::common::check::checked_numeric_cast<uint32_t, int32_t>(i),
                dom_info->dom_node);
          } else {
            children_.insert(
                children_.begin() +
                    flexui::common::check::checked_numeric_cast<uint32_t, int32_t>(i + 1),
                dom_info->dom_node);
          }
          break;
        }
        if (i == children_.size() - 1) {
          children_.push_back(dom_info->dom_node);
          break;
        }
      }
    }
  } else {
    children_.push_back(dom_info->dom_node);
  }
  dom_info->dom_node->SetParent(shared_from_this());
  return dom_info->dom_node->GetSelfIndex();
}

int32_t DomNode::GetChildIndex(uint32_t id) {
  int32_t index = -1;
  for (uint32_t i = 0; i < children_.size(); ++i) {
    auto& child = children_[i];
    if (child && child->GetId() == id) {
      index = static_cast<int32_t>(i);
      break;
    }
  }
  return index;
}

void DomNode::MarkWillChange(bool flag) {
  if (!dom_ext_map_) {
    dom_ext_map_ =
        std::make_shared<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
  }
  (*dom_ext_map_)[kNodeWillChangeKey] = std::make_shared<FlexUIValue>(flag);
}

int32_t DomNode::GetSelfIndex() {
  auto parent = parent_.lock();
  if (parent) {
    return parent->GetChildIndex(id_);
  }
  return -1;
}

int32_t DomNode::GetSelfDepth() {
  if (auto parent = parent_.lock()) {
    return 1 + parent->GetSelfDepth();
  }
  return 1;
}

std::shared_ptr<DomNode> DomNode::RemoveChildAt(int32_t index) {
  auto child =
      children_[flexui::common::check::checked_numeric_cast<int32_t, unsigned long>(index)];
  child->SetParent(nullptr);
  children_.erase(children_.begin() + index);
  return child;
}

std::shared_ptr<DomNode> DomNode::RemoveChildById(uint32_t id) {
  auto it = children_.begin();
  while (it != children_.end()) {
    auto child = *it;
    if (id == child->GetId()) {
      child->SetParent(nullptr);
      children_.erase(it);
      return child;
    }
    it++;
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Event listener bookkeeping (stub — full dispatch in W5-W6)
// ---------------------------------------------------------------------------

bool DomNode::HasEventListeners() {
  return event_listener_map_ != nullptr && !event_listener_map_->empty();
}

// ---------------------------------------------------------------------------
// Style map helpers
// ---------------------------------------------------------------------------

void DomNode::EmplaceStyleMap(const std::string& key, const FlexUIValue& value) {
  if (!style_map_) {
    style_map_ =
        std::make_shared<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
  }
  auto iter = style_map_->find(key);
  if (iter != style_map_->end()) {
    iter->second = std::make_shared<FlexUIValue>(value);
  } else {
    for (auto& style : *style_map_) {
      auto replaced = ReplaceStyle(*style.second, key, value);
      if (replaced) {
        return;
      }
    }
    style_map_->insert({key, std::make_shared<FlexUIValue>(value)});
  }
}

void DomNode::EmplaceStyleMapAndGetDiff(
    const std::string& key, const FlexUIValue& value,
    std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& diff) {
  if (!style_map_) {
    style_map_ =
        std::make_shared<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
  }
  auto it = style_map_->find(key);
  if (it != style_map_->end()) {
    it->second = std::make_shared<FlexUIValue>(value);
    diff[key] = it->second;
  } else {
    for (auto& style : *style_map_) {
      auto replaced = ReplaceStyle(*style.second, key, value);
      if (replaced) {
        diff[style.first] = style.second;
        return;
      }
    }
    diff[key] = std::make_shared<FlexUIValue>(value);
    style_map_->insert({key, diff[key]});
  }
}

// ---------------------------------------------------------------------------
// UpdateProperties — FlexUI: DiffUtils::DiffProps removed; diff logic
// migrates to commit-pipeline/ in W5-W6. Calls UpdateStyle/UpdateDomExt directly.
// ---------------------------------------------------------------------------

void DomNode::UpdateProperties(
    const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style,
    const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_dom_ext) {
  // FlexUI: body simplified — RootNode::UpdateRenderNode removed; will be
  // re-introduced via commit-pipeline/ in W5-W6.
  UpdateStyle(update_style);
  UpdateDomExt(update_dom_ext);
}

void DomNode::UpdateDomNodeStyleAndParseLayoutInfo(
    const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style) {
  UpdateStyle(update_style);
  // FlexUI: ParseLayoutStyleInfo stub kept; layout engine in T5.
}

void DomNode::UpdateDiff(
    const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style,
    const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_dom_ext) {
  // FlexUI: body removed; DiffUtils::DiffProps migrates to commit-pipeline/ in W5-W6.
  // Stub kept to preserve ABI of absorbed header until plumbing lands.
  FLEXUI_TLOG(Vdom, NotImplemented, ERROR) << __PRETTY_FUNCTION__;
  (void)update_style;
  (void)update_dom_ext;
}

void DomNode::UpdateDomExt(
    const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_dom_ext) {
  if (update_dom_ext.empty()) return;

  for (const auto& v : update_dom_ext) {
    if (this->dom_ext_map_ == nullptr) {
      this->dom_ext_map_ =
          std::make_shared<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
    }

    auto iter = this->dom_ext_map_->find(v.first);
    if (iter == this->dom_ext_map_->end()) {
      this->dom_ext_map_->insert({v.first, std::make_shared<FlexUIValue>(*v.second)});
      continue;
    }

    if (v.second->IsObject() && iter->second->IsObject()) {
      this->UpdateObjectStyle(*iter->second, *v.second);
    } else {
      iter->second = std::make_shared<FlexUIValue>(*v.second);
    }
  }
}

void DomNode::UpdateStyle(
    const std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>& update_style) {
  if (update_style.empty()) return;

  for (const auto& v : update_style) {
    if (this->style_map_ == nullptr) {
      this->style_map_ =
          std::make_shared<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
    }

    auto iter = this->style_map_->find(v.first);
    if (iter == this->style_map_->end()) {
      this->style_map_->insert({v.first, std::make_shared<FlexUIValue>(*v.second)});
      continue;
    }

    if (v.second->IsObject() && iter->second->IsObject()) {
      this->UpdateObjectStyle(*iter->second, *v.second);
    } else {
      iter->second = std::make_shared<FlexUIValue>(*v.second);
    }
  }
}

void DomNode::UpdateObjectStyle(FlexUIValue& style_map, const FlexUIValue& update_style) {
  FLEXUI_DCHECK(style_map.IsObject());
  FLEXUI_DCHECK(update_style.IsObject());

  auto style_object = style_map.ToObjectChecked();
  for (auto& v : update_style.ToObjectChecked()) {
    auto iter = style_object.find(v.first);
    if (iter == style_object.end()) {
      style_object[v.first] = v.second;
      continue;
    }
    if (v.second.IsObject() && iter->second.IsObject()) {
      UpdateObjectStyle(iter->second, v.second);
    } else {
      iter->second = v.second;
    }
  }
}

bool DomNode::ReplaceStyle(FlexUIValue& style, const std::string& key,
                            const FlexUIValue& value) {
  if (style.IsObject()) {
    auto& object = style.ToObjectChecked();
    if (object.find(key) != object.end()) {
      object.at(key) = value;
      return true;
    }
    bool replaced = false;
    for (auto& o : object) {
      replaced = ReplaceStyle(o.second, key, value);
      if (replaced) break;
    }
    return replaced;
  }

  if (style.IsArray()) {
    auto& array = style.ToArrayChecked();
    bool replaced = false;
    for (auto& a : array) {
      replaced = ReplaceStyle(a, key, value);
      if (replaced) break;
    }
    return replaced;
  }

  return false;
}

// ---------------------------------------------------------------------------
// ostream operators
// ---------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const RefInfo& ref_info) {
  os << "{";
  os << "\"ref_id\": " << ref_info.ref_id << ", ";
  os << "\"relative_to_ref\": \""
     << kRelativeTypeMap.find(ref_info.relative_to_ref)->second << "\"";
  os << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const DiffInfo& diff_info) {
  os << "{";
  os << "\"skip_style_diff\": " << diff_info.skip_style_diff;
  os << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const DomNode& node) {
  os << "{";
  os << "\"id\": " << node.id_ << ", ";
  os << "\"pid\": " << node.pid_ << ", ";
  os << "\"view name\": \"" << node.view_name_ << "\", ";
  if (node.style_map_ != nullptr) {
    os << "\"style\": {";
    for (const auto& s : *node.style_map_) {
      os << "\"" << s.first << "\": " << *s.second << ", ";
    }
    os << "}, ";
  }
  if (node.dom_ext_map_ != nullptr) {
    os << "\"ext style\": {";
    for (const auto& e : *node.dom_ext_map_) {
      os << "\"" << e.first << "\": " << *e.second << ", ";
    }
    os << "}, ";
  }
  os << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const DomInfo& dom_info) {
  auto dom_node = dom_info.dom_node;
  auto ref_info = dom_info.ref_info;
  auto diff_info = dom_info.diff_info;
  os << "{";
  if (ref_info != nullptr) {
    os << "\"ref info\": " << *ref_info << ", ";
  }
  if (diff_info != nullptr) {
    os << "\"diff info\": " << *diff_info << ", ";
  }
  if (dom_node != nullptr) {
    os << "\"dom node\": " << *dom_node << ", ";
  }
  os << "}";
  return os;
}

// ---------------------------------------------------------------------------
// Serialize / Deserialize
// ---------------------------------------------------------------------------

FlexUIValue DomNode::Serialize() const {
  FlexUIValueObjectType result;

  result[kNodePropertyId] = FlexUIValue(id_);
  result[kNodePropertyPid] = FlexUIValue(pid_);
  result[kNodePropertyIndex] = FlexUIValue(index_);
  result[kNodePropertyTagName] = FlexUIValue(tag_name_);
  result[kNodePropertyViewName] = FlexUIValue(view_name_);

  if (style_map_) {
    FlexUIValueObjectType style_map_value;
    for (const auto& value : *style_map_) {
      style_map_value[value.first] = *value.second;
    }
    result[kNodePropertyStyle] = FlexUIValue(std::move(style_map_value));
  }

  if (dom_ext_map_) {
    FlexUIValueObjectType dom_ext_map_value;
    for (const auto& value : *dom_ext_map_) {
      dom_ext_map_value[value.first] = *value.second;
    }
    result[kNodePropertyExt] = FlexUIValue(std::move(dom_ext_map_value));
  }

  return FlexUIValue(std::move(result));
}

bool DomNode::Deserialize(FlexUIValue value) {
  FLEXUI_DCHECK(value.IsObject());
  if (!value.IsObject()) {
    FLEXUI_LOG(ERROR) << "Deserialize value is not object";
    return false;
  }
  FlexUIValueObjectType dom_node_obj = value.ToObjectChecked();

  uint32_t id;
  auto flag = dom_node_obj[kNodePropertyId].ToUint32(id);
  if (flag) {
    SetId(static_cast<uint32_t>(id));
  } else {
    FLEXUI_LOG(ERROR) << "Deserialize id error";
    return false;
  }

  uint32_t pid;
  flag = dom_node_obj[kNodePropertyPid].ToUint32(pid);
  if (flag) {
    SetPid(static_cast<uint32_t>(pid));
  } else {
    FLEXUI_LOG(ERROR) << "Deserialize pid error";
    return false;
  }

  int32_t index;
  flag = dom_node_obj[kNodePropertyIndex].ToInt32(index);
  if (flag) {
    SetIndex(index);
  } else {
    FLEXUI_LOG(ERROR) << "Deserialize index error";
    return false;
  }

  std::string tag_name;
  flag = dom_node_obj[kNodePropertyTagName].ToString(tag_name);
  if (flag) {
    SetTagName(tag_name);
  }

  std::string view_name;
  flag = dom_node_obj[kNodePropertyViewName].ToString(view_name);
  if (flag) {
    SetViewName(view_name);
  } else {
    FLEXUI_LOG(ERROR) << "Deserialize view_name error";
    return false;
  }

  auto style_obj = dom_node_obj[kNodePropertyStyle];
  if (style_obj.IsObject()) {
    auto style = style_obj.ToObjectChecked();
    auto style_map =
        std::make_shared<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
    for (const auto& p : style) {
      (*style_map)[p.first] = std::make_shared<FlexUIValue>(p.second);
    }
    SetStyleMap(std::move(style_map));
  }

  auto ext_obj = dom_node_obj[kNodePropertyExt];
  if (ext_obj.IsObject()) {
    auto ext = ext_obj.ToObjectChecked();
    auto ext_map =
        std::make_shared<std::unordered_map<std::string, std::shared_ptr<FlexUIValue>>>();
    for (const auto& p : ext) {
      (*ext_map)[p.first] = std::make_shared<FlexUIValue>(p.second);
    }
    SetExtStyleMap(std::move(ext_map));
  }

  return true;
}

}  // namespace flexui::core::vdom
