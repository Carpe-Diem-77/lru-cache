#include "lru_cache.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

#pragma region Helper Classes
void EXPECT_CACHE_HIT(LruCache<int, int> &c, int key, int expected) {
  auto v = c.get(key);
  EXPECT_NE(v, nullptr);
  EXPECT_EQ(*v, expected);
}

void EXPECT_CACHE_MISS(LruCache<int, int> &c, int key) {
  auto v = c.get(key);
  EXPECT_EQ(v, nullptr);
}

LruCache<int, int> PrimedCap3_V3() {
  LruCache<int, int> c(3);
  c.put(1, 100);
  c.put(2, 200);
  c.put(3, 300);
  return c;
}
#pragma endregion

TEST(V3_Basics, GetMissReturnsFalse) {
  LruCache<int, int> c(2);
  EXPECT_CACHE_MISS(c, 7);
}

TEST(V3_Basics, PutThenGet) {
  LruCache<int, int> c(2);
  c.put(1, 100);
  EXPECT_CACHE_HIT(c, 1, 100);
}

TEST(V3_Basics, PutOverwritesValueForSameKey) {
  LruCache<int, int> c(2);
  c.put(1, 100);
  c.put(1, 200);
  EXPECT_CACHE_HIT(c, 1, 200);
}

TEST(V3_Basics, MultipleIndependentKeys) {
  LruCache<int, int> c(3);
  c.put(1, 100);
  c.put(2, 200);
  c.put(3, 300);
  EXPECT_CACHE_HIT(c, 1, 100);
  EXPECT_CACHE_HIT(c, 2, 200);
  EXPECT_CACHE_HIT(c, 3, 300);
}

TEST(V3_Basics, InsertingBeyondCapacityEvictsLeastRecentlyUsed) {
  auto cache = PrimedCap3_V3();
  cache.put(4, 400);

  EXPECT_CACHE_MISS(cache, 1);
  EXPECT_CACHE_HIT(cache, 2, 200);
  EXPECT_CACHE_HIT(cache, 3, 300);
  EXPECT_CACHE_HIT(cache, 4, 400);
}

TEST(V3_Basics, GetPromotesKeyToMostRecentlyUsed) {
  auto cache = PrimedCap3_V3();
  EXPECT_CACHE_HIT(cache, 1, 100);
  cache.put(4, 400);

  EXPECT_CACHE_HIT(cache, 1, 100);
  EXPECT_CACHE_MISS(cache, 2);
  EXPECT_CACHE_HIT(cache, 3, 300);
  EXPECT_CACHE_HIT(cache, 4, 400);
}

TEST(V3_Basics, PutOnExistingKeyPromotesItToMostRecentlyUsed) {
  auto cache = PrimedCap3_V3();
  cache.put(1, 999);
  cache.put(4, 400);

  EXPECT_CACHE_HIT(cache, 1, 999);
  EXPECT_CACHE_MISS(cache, 2);
  EXPECT_CACHE_HIT(cache, 3, 300);
  EXPECT_CACHE_HIT(cache, 4, 400);
}

TEST(V3_Basics, GetOfMiddleNodeMovesItToFrontWithoutCorruption) {
  auto cache = PrimedCap3_V3();
  EXPECT_CACHE_HIT(cache, 2, 200);
  cache.put(4, 400);

  EXPECT_CACHE_MISS(cache, 1);
  EXPECT_CACHE_HIT(cache, 2, 200);
  EXPECT_CACHE_HIT(cache, 3, 300);
  EXPECT_CACHE_HIT(cache, 4, 400);
}

TEST(V3_EdgeCases, CapacityOneRepeatedEvictionsDoNotCrash) {
  LruCache<int, int> c(1);
  for (int i = 0; i < 10; ++i) {
    c.put(i, i * 10);
    EXPECT_CACHE_HIT(c, i, i * 10);
  }

  for (int i = 0; i < 9; ++i) {
    SCOPED_TRACE("key i=" + std::to_string(i));
    EXPECT_CACHE_MISS(c, i);
  }
}

TEST(V3_CopyAssignment, SelfCopyAssignmentWontBreak) {
  LruCache<int, int> a(3);
  a.put(1, 1);
  a = a;

  EXPECT_EQ(a.capacity(), 3);
  EXPECT_CACHE_HIT(a, 1, 1);
  EXPECT_CACHE_MISS(a, 2);
}

TEST(V3_CopyAssignment, CopyAssignmentClearOriginalData) {
  LruCache<int, int> a(3), b(4);
  a.put(1, 1);
  b.put(2, 2);
  b = a;

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 1, 1);
  EXPECT_CACHE_MISS(b, 2);
}

TEST(V3_CopyAssignment, CopyKeepLruOrder) {
  LruCache<int, int> a(3);
  a.put(1, 1);
  a.put(2, 2);
  a.put(3, 3);
  LruCache<int, int> b(a);

  // 1 would be evicted from b
  b.put(4, 4);

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 2, 2);
  EXPECT_CACHE_MISS(b, 1);
}

TEST(V3_CopyCtor, CopyCtorBasic) {
  LruCache<int, int> a(3);
  a.put(1, 1);
  a.put(2, 2);
  LruCache<int, int> b(a);

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 1, 1);
  EXPECT_CACHE_HIT(b, 2, 2);
}

// If something wrong, this would fire at compile time
TEST(V3_MoveSemantics, NoThrowForMove) {
  static_assert(std::is_nothrow_move_constructible_v<LruCache<int, int>>);
  static_assert(std::is_nothrow_move_assignable_v<LruCache<int, int>>);
}

TEST(V3_SmartPointer, GetReturnsConstQualifiedValue) {
  using GetResult = decltype(*std::declval<LruCache<int, int> &>().get(0));
  static_assert(std::is_const_v<std::remove_reference_t<GetResult>>);
}

TEST(V3_Emplace, PerfectForwardingTest) {
  LruCache<int, std::unique_ptr<int>> cache(3);
  cache.emplace(0, std::make_unique<int>(3));

  auto p = cache.get(0);
  EXPECT_EQ(**p, 3);
}

TEST(V3_SmartPointer, AccessEvictedItemTest) {
  auto cache = PrimedCap3_V3();
  auto value = cache.get(1);
  cache.put(4, 400);
  cache.put(5, 500);
  cache.put(6, 600);
  EXPECT_EQ(*value, 100);
}