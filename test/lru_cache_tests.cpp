#include "lru_cache.h"
#include <gtest/gtest.h>
#include <memory>

#pragma region Helper Classes
void EXPECT_CACHE_HIT(LruCache<int, int> &c, int key, int expected) {
  auto v = c.get(key);
  EXPECT_NE(v, nullptr);
  EXPECT_EQ(*v, expected);
}

void EXPECT_CACHE_HIT(LruCache<int, std::unique_ptr<int>> &c, int key, int expected) {
  auto v = c.get(key);
  EXPECT_NE(v, nullptr);
  EXPECT_EQ(**v, expected);
}

void EXPECT_CACHE_MISS(LruCache<int, int> &c, int key) {
  auto v = c.get(key);
  EXPECT_EQ(v, nullptr);
}

void EXPECT_CACHE_MISS(LruCache<int, std::unique_ptr<int>> &c, int key) {
  auto v = c.get(key);
  EXPECT_EQ(v, nullptr);
}

LruCache<int, int> InitializeCacheWithThreeIntegers() {
  LruCache<int, int> c(3);
  c.put(1, 1);
  c.put(2, 2);
  c.put(3, 3);
  return c;
}
#pragma endregion

TEST(DefaultCtor, InvalidCapacity) {
  // Can not just write EXPECT_DEATH(LruCache<int, int> c(0), ""); because of the "," in template
  // it will see 3 arguments instead of 2.
  using Cache = LruCache<int, int>;
  EXPECT_DEATH(Cache c(0), "");
}

TEST(CopyCtorAndAssignment, CopyCtorBasic) {
  auto a = InitializeCacheWithThreeIntegers();
  LruCache<int, int> b(a);

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 1, 1);
  EXPECT_CACHE_HIT(b, 2, 2);
  EXPECT_CACHE_HIT(b, 3, 3);
}

TEST(CopyCtorAndAssignment, CopyCtorKeepLruOrder) {
  auto a = InitializeCacheWithThreeIntegers();
  LruCache<int, int> b(a);

  // 1 would be evicted from b
  b.put(4, 4);

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 2, 2);
  EXPECT_CACHE_MISS(b, 1);
}

TEST(CopyCtorAndAssignment, SelfCopyAssignmentWontBreak) {
  auto a = InitializeCacheWithThreeIntegers();
  a = a;

  EXPECT_EQ(a.capacity(), 3);
}

TEST(CopyCtorAndAssignment, CopyAssignmentClearOriginalData) {
  LruCache<int, int> a(3), b(4);
  a.put(1, 1);
  b.put(2, 2);
  b = a;

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 1, 1);
  EXPECT_CACHE_MISS(b, 2);
}

TEST(CopyCtorAndAssignment, CopyAssignmentKeepLruOrder) {
  auto a = InitializeCacheWithThreeIntegers();
  LruCache<int, int> b = a;

  // 1 would be evicted from b
  b.put(4, 4);

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 2, 2);
  EXPECT_CACHE_MISS(b, 1);
}

// If something wrong, this would fire at compile time
TEST(MoveCtorAndAssignment, NoThrowForMove) {
  static_assert(std::is_nothrow_move_constructible_v<LruCache<int, int>>);
  static_assert(std::is_nothrow_move_assignable_v<LruCache<int, int>>);
}

TEST(PutGetOperations, BasicOperations) {
  auto a = InitializeCacheWithThreeIntegers();
  EXPECT_CACHE_HIT(a, 1, 1);
  EXPECT_CACHE_HIT(a, 2, 2);
  EXPECT_CACHE_HIT(a, 3, 3);
  EXPECT_CACHE_MISS(a, 4);
}

TEST(PutGetOperations, AccessEvictedItemTest) {
  auto cache = InitializeCacheWithThreeIntegers();
  auto value = cache.get(1);
  cache.put(4, 4);
  cache.put(5, 5);
  cache.put(6, 6);
  EXPECT_EQ(*value, 1);
}

TEST(PutGetOperations, PutOverwritesValueForSameKey) {
  auto a = InitializeCacheWithThreeIntegers();
  a.put(1, 1);
  EXPECT_CACHE_HIT(a, 1, 1);
}

TEST(PutGetOperations, InsertingBeyondCapacityEvictsLeastRecentlyUsed) {
  auto cache = InitializeCacheWithThreeIntegers();
  cache.put(4, 4);

  EXPECT_CACHE_MISS(cache, 1);
  EXPECT_CACHE_HIT(cache, 2, 2);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_HIT(cache, 4, 4);
}

TEST(PutGetOperations, GetPromotesKeyToMostRecentlyUsed) {
  auto cache = InitializeCacheWithThreeIntegers();
  EXPECT_CACHE_HIT(cache, 1, 1);
  cache.put(4, 4);

  EXPECT_CACHE_HIT(cache, 4, 4);
  EXPECT_CACHE_HIT(cache, 1, 1);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_MISS(cache, 2);
}

TEST(PutGetOperations, GetOfMiddleNodeMovesItToFrontWithoutCorruption) {
  auto cache = InitializeCacheWithThreeIntegers();
  EXPECT_CACHE_HIT(cache, 2, 2);
  cache.put(4, 4);

  EXPECT_CACHE_HIT(cache, 4, 4);
  EXPECT_CACHE_HIT(cache, 2, 2);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_MISS(cache, 1);
}

TEST(PutGetOperations, PutOnExistingKeyPromotesItToMostRecentlyUsed) {
  auto cache = InitializeCacheWithThreeIntegers();
  cache.put(1, 999);
  cache.put(4, 4);

  EXPECT_CACHE_HIT(cache, 4, 4);
  EXPECT_CACHE_HIT(cache, 1, 999);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_MISS(cache, 2);
}

TEST(PutGetOperations, CapacityOneRepeatedEvictionsDoNotCrash) {
  LruCache<int, int> c(1);
  for (int i = 0; i < 10; ++i) {
    c.put(i, i * 10);
    EXPECT_CACHE_HIT(c, i, i * 10);
  }

  for (int i = 0; i < 9; ++i) {
    EXPECT_CACHE_MISS(c, i);
  }
}

TEST(EmplaceOperations, PerfectForwardingTest) {
  LruCache<int, std::unique_ptr<int>> cache(3);
  cache.emplace(0, std::make_unique<int>(3));

  EXPECT_CACHE_HIT(cache, 0, 3);
}

TEST(EmplaceOperations, InsertingBeyondCapacityEvictsLeastRecentlyUsed) {
  LruCache<int, std::unique_ptr<int>> cache(3);

  cache.emplace(0, std::make_unique<int>(0));
  cache.emplace(1, std::make_unique<int>(1));
  cache.emplace(2, std::make_unique<int>(2));
  cache.emplace(3, std::make_unique<int>(3));
  cache.emplace(2, std::make_unique<int>(999));

  EXPECT_CACHE_HIT(cache, 2, 999);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_HIT(cache, 1, 1);
  EXPECT_CACHE_MISS(cache, 0);
}