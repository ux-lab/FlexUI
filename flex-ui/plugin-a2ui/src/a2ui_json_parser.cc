/*
 * Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
 * Version 2.0.
 *
 * Minimal JSON parser: text -> FlexUIValue.
 * Supports: null, true/false, number, string (no escape sequences beyond \"), object, array.
 */
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "flexui/common/flexui_value.h"

namespace flexui::plugin_a2ui {

namespace {

struct Parser {
  const char* p;
  const char* end;

  void skip() { while (p < end && std::isspace((unsigned char)*p)) ++p; }

  flexui::common::FlexUIValue parse();
  flexui::common::FlexUIValue parseString();
  flexui::common::FlexUIValue parseNumber();
  flexui::common::FlexUIValue parseObject();
  flexui::common::FlexUIValue parseArray();
};

flexui::common::FlexUIValue Parser::parseString() {
  // assumes *p == '"'
  ++p;
  std::string s;
  while (p < end && *p != '"') {
    if (*p == '\\' && p + 1 < end) {
      ++p;
      switch (*p) {
        case '"': s += '"'; break;
        case '\\': s += '\\'; break;
        case 'n': s += '\n'; break;
        case 't': s += '\t'; break;
        default: s += *p; break;
      }
    } else {
      s += *p;
    }
    ++p;
  }
  if (p < end) ++p;  // consume closing "
  return flexui::common::FlexUIValue(std::move(s));
}

flexui::common::FlexUIValue Parser::parseNumber() {
  const char* start = p;
  if (*p == '-') ++p;
  while (p < end && std::isdigit((unsigned char)*p)) ++p;
  if (p < end && *p == '.') {
    ++p;
    while (p < end && std::isdigit((unsigned char)*p)) ++p;
  }
  double d = std::strtod(start, nullptr);
  return flexui::common::FlexUIValue(d);
}

flexui::common::FlexUIValue Parser::parseObject() {
  ++p;  // consume '{'
  flexui::common::FlexUIValue::FlexUIValueObjectType obj;
  skip();
  while (p < end && *p != '}') {
    skip();
    if (*p != '"') break;
    auto key = parseString();
    skip();
    if (p < end && *p == ':') ++p;
    skip();
    auto val = parse();
    obj[key.ToStringChecked()] = std::move(val);
    skip();
    if (p < end && *p == ',') ++p;
    skip();
  }
  if (p < end) ++p;  // consume '}'
  return flexui::common::FlexUIValue(std::move(obj));
}

flexui::common::FlexUIValue Parser::parseArray() {
  ++p;  // consume '['
  flexui::common::FlexUIValue::FlexUIValueArrayType arr;
  skip();
  while (p < end && *p != ']') {
    arr.push_back(parse());
    skip();
    if (p < end && *p == ',') ++p;
    skip();
  }
  if (p < end) ++p;  // consume ']'
  return flexui::common::FlexUIValue(std::move(arr));
}

flexui::common::FlexUIValue Parser::parse() {
  skip();
  if (p >= end) return flexui::common::FlexUIValue();
  char c = *p;
  if (c == '"') return parseString();
  if (c == '{') return parseObject();
  if (c == '[') return parseArray();
  if (c == 't') { p += 4; return flexui::common::FlexUIValue(true); }
  if (c == 'f') { p += 5; return flexui::common::FlexUIValue(false); }
  if (c == 'n') { p += 4; return flexui::common::FlexUIValue(); }  // null -> undefined
  if (c == '-' || std::isdigit((unsigned char)c)) return parseNumber();
  return flexui::common::FlexUIValue();
}

}  // namespace

flexui::common::FlexUIValue ParseA2UIJson(const std::string& text) {
  Parser p{text.data(), text.data() + text.size()};
  return p.parse();
}

}  // namespace flexui::plugin_a2ui
