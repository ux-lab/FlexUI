/*
 * Copyright (C) 2024 The FlexUI Authors.
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

#include "flexui/common/persistent_object_map.h"

#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

using flexui::common::PersistentObjectMap;

TEST(PersistentObjectMapTest, InsertAndFind) {
  PersistentObjectMap<int, std::string> map;
  EXPECT_TRUE(map.Insert(1, "one"));
  EXPECT_TRUE(map.Insert(2, "two"));

  std::string val;
  EXPECT_TRUE(map.Find(1, val));
  EXPECT_EQ(val, "one");

  EXPECT_TRUE(map.Find(2, val));
  EXPECT_EQ(val, "two");
}

TEST(PersistentObjectMapTest, InsertDuplicateReturnsFalse) {
  PersistentObjectMap<int, std::string> map;
  EXPECT_TRUE(map.Insert(1, "one"));
  // Inserting same key again should fail
  EXPECT_FALSE(map.Insert(1, "ONE"));
  // Value should remain unchanged
  std::string val;
  EXPECT_TRUE(map.Find(1, val));
  EXPECT_EQ(val, "one");
}

TEST(PersistentObjectMapTest, FindMissingKeyReturnsFalse) {
  PersistentObjectMap<int, std::string> map;
  std::string val;
  EXPECT_FALSE(map.Find(99, val));
}

TEST(PersistentObjectMapTest, Erase) {
  PersistentObjectMap<int, std::string> map;
  EXPECT_TRUE(map.Insert(1, "one"));
  EXPECT_TRUE(map.Erase(1));

  std::string val;
  EXPECT_FALSE(map.Find(1, val));
}

TEST(PersistentObjectMapTest, EraseMissingKeyReturnsFalse) {
  PersistentObjectMap<int, std::string> map;
  EXPECT_FALSE(map.Erase(42));
}

TEST(PersistentObjectMapTest, Clear) {
  PersistentObjectMap<int, std::string> map;
  EXPECT_TRUE(map.Insert(1, "one"));
  EXPECT_TRUE(map.Insert(2, "two"));
  map.Clear();

  std::string val;
  EXPECT_FALSE(map.Find(1, val));
  EXPECT_FALSE(map.Find(2, val));
}

TEST(PersistentObjectMapTest, Emplace) {
  PersistentObjectMap<int, std::string> map;
  EXPECT_TRUE(map.Emplace(10, "ten"));
  std::string val;
  EXPECT_TRUE(map.Find(10, val));
  EXPECT_EQ(val, "ten");
  // Emplace same key should fail
  EXPECT_FALSE(map.Emplace(10, "TEN"));
}

TEST(PersistentObjectMapTest, ThreadSafeInsertFind) {
  PersistentObjectMap<int, int> map;
  constexpr int kCount = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kCount; ++i) {
    threads.emplace_back([&map, i]() { map.Insert(i, i * 10); });
  }
  for (auto& t : threads) t.join();

  for (int i = 0; i < kCount; ++i) {
    int val = -1;
    if (map.Find(i, val)) {
      EXPECT_EQ(val, i * 10);
    }
  }
}
