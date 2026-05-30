// Copyright (c) 2026 the FlexUI authors. Licensed under the Apache License,
// Version 2.0.
//
// Extended string_view tests to push coverage above the 80% gate (Task 19).
// Focused on UTF-8, UTF-16, UTF-32 constructors, assignment operators,
// destructor paths, move semantics, hash, and StringViewUtils conversions —
// all uncovered by the original Task 5 tests.

#include <gtest/gtest.h>
#include <string>
#include <unordered_set>

#include "flexui/common/string_view.h"
#include "flexui/common/string_view_utils.h"

namespace flexui::common {

// ── UTF-8 constructors ────────────────────────────────────────────────────────

TEST(StringViewUtf8, ConstructFromCharPtr) {
  using u8 = string_view::char8_t_;
  const u8* s = reinterpret_cast<const u8*>("hello");
  string_view sv(s);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf8);
}

TEST(StringViewUtf8, ConstructFromCharPtrWithLength) {
  using u8 = string_view::char8_t_;
  const u8* s = reinterpret_cast<const u8*>("hello");
  string_view sv(s, 3);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf8);
}

TEST(StringViewUtf8, ConstructFromU8String) {
  string_view::u8string us(reinterpret_cast<const string_view::char8_t_*>("hi"));
  string_view sv(us);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf8);
}

TEST(StringViewUtf8, ConstructFromMoveU8String) {
  string_view::u8string us(reinterpret_cast<const string_view::char8_t_*>("move"));
  string_view sv(std::move(us));
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf8);
}

TEST(StringViewUtf8, IsEmptyTrue) {
  string_view::u8string empty_u8;
  string_view sv(empty_u8);
  EXPECT_TRUE(StringViewUtils::IsEmpty(sv));
}

TEST(StringViewUtf8, IsEmptyFalse) {
  string_view::u8string us(reinterpret_cast<const string_view::char8_t_*>("x"));
  string_view sv(us);
  EXPECT_FALSE(StringViewUtils::IsEmpty(sv));
}

// ── UTF-16 constructors ───────────────────────────────────────────────────────

TEST(StringViewUtf16, ConstructFromCharPtr) {
  string_view sv(u"hello");
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf16);
}

TEST(StringViewUtf16, ConstructFromCharPtrWithLength) {
  string_view sv(u"hello", 3);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf16);
  EXPECT_EQ(sv.utf16_value().size(), 3u);
}

TEST(StringViewUtf16, ConstructFromU16String) {
  std::u16string s = u"world";
  string_view sv(s);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf16);
  EXPECT_EQ(sv.utf16_value(), s);
}

TEST(StringViewUtf16, ConstructFromMoveU16String) {
  std::u16string s = u"move";
  string_view sv(std::move(s));
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf16);
}

TEST(StringViewUtf16, IsEmptyTrue) {
  string_view sv(std::u16string{});
  EXPECT_TRUE(StringViewUtils::IsEmpty(sv));
}

TEST(StringViewUtf16, IsEmptyFalse) {
  string_view sv(u"abc");
  EXPECT_FALSE(StringViewUtils::IsEmpty(sv));
}

TEST(StringViewUtf16, GetLength) {
  string_view sv(u"hello");
  EXPECT_EQ(StringViewUtils::GetLength(sv), 5u);
}

// ── UTF-32 constructors ───────────────────────────────────────────────────────

TEST(StringViewUtf32, ConstructFromCharPtr) {
  string_view sv(U"hello");
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf32);
}

TEST(StringViewUtf32, ConstructFromCharPtrWithLength) {
  string_view sv(U"hello", 3);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf32);
  EXPECT_EQ(sv.utf32_value().size(), 3u);
}

TEST(StringViewUtf32, ConstructFromU32String) {
  std::u32string s = U"world";
  string_view sv(s);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf32);
}

TEST(StringViewUtf32, ConstructFromMoveU32String) {
  std::u32string s = U"move";
  string_view sv(std::move(s));
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf32);
}

TEST(StringViewUtf32, IsEmptyTrue) {
  string_view sv(std::u32string{});
  EXPECT_TRUE(StringViewUtils::IsEmpty(sv));
}

TEST(StringViewUtf32, IsEmptyFalse) {
  string_view sv(U"abc");
  EXPECT_FALSE(StringViewUtils::IsEmpty(sv));
}

TEST(StringViewUtf32, GetLength) {
  string_view sv(U"hello");
  EXPECT_EQ(StringViewUtils::GetLength(sv), 5u);
}

// ── Unknown encoding ──────────────────────────────────────────────────────────

TEST(StringViewUnknown, DefaultConstructIsEmpty) {
  string_view sv;
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Unknown);
  EXPECT_TRUE(StringViewUtils::IsEmpty(sv));
}

// ── Copy and assignment ───────────────────────────────────────────────────────

TEST(StringViewAssign, CopyConstructUtf8) {
  string_view::u8string us(reinterpret_cast<const string_view::char8_t_*>("copy"));
  string_view a(us);
  string_view b(a);
  EXPECT_EQ(b.encoding(), string_view::Encoding::Utf8);
}

TEST(StringViewAssign, CopyConstructUtf16) {
  string_view a(u"copy");
  string_view b(a);
  EXPECT_EQ(b.encoding(), string_view::Encoding::Utf16);
  EXPECT_EQ(b.utf16_value(), u"copy");
}

TEST(StringViewAssign, CopyConstructUtf32) {
  string_view a(U"copy");
  string_view b(a);
  EXPECT_EQ(b.encoding(), string_view::Encoding::Utf32);
}

TEST(StringViewAssign, AssignCharPtr) {
  string_view sv("first");
  sv = "second";
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Latin1);
  EXPECT_EQ(sv.latin1_value(), "second");
}

TEST(StringViewAssign, AssignStdString) {
  string_view sv("first");
  std::string s("second");
  sv = s;
  EXPECT_EQ(sv.latin1_value(), "second");
}

TEST(StringViewAssign, AssignUtf16FromLatin1) {
  string_view sv("latin1");
  sv = u"utf16";
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf16);
}

TEST(StringViewAssign, AssignUtf32FromUtf16) {
  string_view sv(u"utf16");
  sv = U"utf32";
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf32);
}

TEST(StringViewAssign, AssignLatin1FromUtf32) {
  string_view sv(U"utf32");
  sv = "latin1";
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Latin1);
}

TEST(StringViewAssign, SelfAssign) {
  string_view sv("same");
  // operator= handles self-assignment — must not crash or corrupt
  sv = sv;  // NOLINT
  EXPECT_EQ(sv.latin1_value(), "same");
}

TEST(StringViewAssign, AssignSameEncodingLatin1) {
  string_view a("first");
  string_view b("second");
  a = b;
  EXPECT_EQ(a.latin1_value(), "second");
}

TEST(StringViewAssign, AssignSameEncodingUtf16) {
  string_view a(u"first");
  string_view b(u"second");
  a = b;
  EXPECT_EQ(a.utf16_value(), u"second");
}

TEST(StringViewAssign, AssignU16StringToLatin1) {
  string_view sv("latin1");
  std::u16string us = u"wide";
  sv = us;
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf16);
}

TEST(StringViewAssign, AssignU32StringToLatin1) {
  string_view sv("latin1");
  std::u32string us = U"wide32";
  sv = us;
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf32);
}

// ── Hash ─────────────────────────────────────────────────────────────────────

TEST(StringViewHash, Latin1HashConsistent) {
  string_view a("key");
  string_view b("key");
  std::hash<string_view> h;
  EXPECT_EQ(h(a), h(b));
}

TEST(StringViewHash, Utf16HashConsistent) {
  string_view a(u"key");
  string_view b(u"key");
  std::hash<string_view> h;
  EXPECT_EQ(h(a), h(b));
}

TEST(StringViewHash, Utf32HashConsistent) {
  string_view a(U"key");
  string_view b(U"key");
  std::hash<string_view> h;
  EXPECT_EQ(h(a), h(b));
}

TEST(StringViewHash, UnknownHashIsZero) {
  string_view sv;
  std::hash<string_view> h;
  EXPECT_EQ(h(sv), 0u);
}

TEST(StringViewHash, UsableInUnorderedSet) {
  std::unordered_set<string_view> s;
  s.insert(string_view("a"));
  s.insert(string_view("b"));
  s.insert(string_view("a"));
  EXPECT_EQ(s.size(), 2u);
}

// ── new_from_utf8 factory ─────────────────────────────────────────────────────

TEST(StringViewFactory, NewFromUtf8CStr) {
  // new_from_utf8 takes const u8_type* (char8_t on platforms with __cpp_char8_t).
  // Use a u8"" literal which decays to const char8_t* on C++20-capable compilers.
  auto sv = string_view::new_from_utf8(u8"test");
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf8);
}

TEST(StringViewFactory, NewFromUtf8CStrWithLen) {
  auto sv = string_view::new_from_utf8(u8"hello", 3);
  EXPECT_EQ(sv.encoding(), string_view::Encoding::Utf8);
}

// ── Equality operator ─────────────────────────────────────────────────────────

TEST(StringViewEquality, Utf16Equal) {
  string_view a(u"same");
  string_view b(u"same");
  EXPECT_EQ(a, b);
}

TEST(StringViewEquality, Utf32Equal) {
  string_view a(U"same");
  string_view b(U"same");
  EXPECT_EQ(a, b);
}

TEST(StringViewEquality, DifferentEncodingsNotEqual) {
  string_view a("latin1");
  string_view b(u"latin1");
  // Different encoding → not equal
  EXPECT_NE(a, b);
}

}  // namespace flexui::common
