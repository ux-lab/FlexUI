/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Data binding: {{ dotted.path }} interpolation applied to a FlexUIValue template.
 */
#include <regex>
#include <string>
#include <vector>

#include "flexui/common/flexui_value.h"

namespace flexui::plugin_a2ui {

namespace {

// Lookup a dotted path in data: "user.name" -> data["user"]["name"]
const flexui::common::FlexUIValue* LookupPath(
    const flexui::common::FlexUIValue& data, const std::string& path) {
  std::vector<std::string> parts;
  std::string cur;
  for (char c : path) {
    if (c == '.') { parts.push_back(cur); cur.clear(); }
    else cur += c;
  }
  parts.push_back(cur);
  const flexui::common::FlexUIValue* v = &data;
  for (auto& part : parts) {
    if (!v->IsObject()) return nullptr;
    auto& obj = v->ToObjectChecked();
    auto it = obj.find(part);
    if (it == obj.end()) return nullptr;
    v = &it->second;
  }
  return v;
}

// Replace {{ path }} placeholders in a string.
std::string BindString(const std::string& tmpl,
                       const flexui::common::FlexUIValue& data) {
  static const std::regex kBinding(R"(\{\{\s*([\w.]+)\s*\}\})");
  std::string result;
  auto begin = std::sregex_iterator(tmpl.begin(), tmpl.end(), kBinding);
  auto end   = std::sregex_iterator();
  size_t last = 0;
  for (auto it = begin; it != end; ++it) {
    auto& m = *it;
    result += tmpl.substr(last, m.position() - last);
    const auto* v = LookupPath(data, m[1].str());
    if (v) {
      if (v->IsString()) result += v->ToStringChecked();
      else if (v->IsNumber()) result += std::to_string(v->ToDoubleChecked());
      else if (v->IsBoolean()) result += v->ToBooleanChecked() ? "true" : "false";
    }
    last = static_cast<size_t>(m.position()) + m.length();
  }
  result += tmpl.substr(last);
  return result;
}

flexui::common::FlexUIValue BindValue(const flexui::common::FlexUIValue& tmpl,
                                     const flexui::common::FlexUIValue& data) {
  if (tmpl.IsString()) {
    const std::string& s = tmpl.ToStringChecked();
    if (s.find("{{") != std::string::npos) {
      return flexui::common::FlexUIValue(BindString(s, data));
    }
    return tmpl;
  }
  if (tmpl.IsObject()) {
    flexui::common::FlexUIValue::FlexUIValueObjectType out;
    for (auto& kv : tmpl.ToObjectChecked()) {
      out[kv.first] = BindValue(kv.second, data);
    }
    return flexui::common::FlexUIValue(std::move(out));
  }
  if (tmpl.IsArray()) {
    flexui::common::FlexUIValue::FlexUIValueArrayType out;
    for (auto& v : tmpl.ToArrayChecked()) {
      out.push_back(BindValue(v, data));
    }
    return flexui::common::FlexUIValue(std::move(out));
  }
  return tmpl;
}

}  // namespace

flexui::common::FlexUIValue ApplyDataBinding(
    const flexui::common::FlexUIValue& tmpl,
    const flexui::common::FlexUIValue& data) {
  return BindValue(tmpl, data);
}

}  // namespace flexui::plugin_a2ui
