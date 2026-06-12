#include "lru_cache.h"
#include <atomic>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

#pragma region Helper Classes
void EXPECT_CACHE_HIT(LruCache<int, int> &c, int key, int expected) {
  auto v = c.get(key);
  EXPECT_NE(v, nullptr);
  EXPECT_EQ(*v, expected);
}

void EXPECT_CACHE_HIT(LruCache<int, std::unique_ptr<int>> &c, int key,
                      int expected) {
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

LruCache<int, int> InitializeCache(int capacity, int size) {
  LruCache<int, int> c(capacity);
  for (int i = 1; i <= size; ++i)
    c.emplace(i, i);
  return c;
}
#pragma endregion

TEST(DefaultCtor, InvalidCapacity) {
  // Can not just write EXPECT_DEATH(LruCache<int, int> c(0), ""); because of
  // the "," in template it will see 3 arguments instead of 2.
  using Cache = LruCache<int, int>;
  EXPECT_DEATH(Cache c(0), "");
}

TEST(CopyCtorAndAssignment, CopyCtorBasic) {
  auto a = InitializeCache(3, 3);
  LruCache<int, int> b(a);

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 1, 1);
  EXPECT_CACHE_HIT(b, 2, 2);
  EXPECT_CACHE_HIT(b, 3, 3);
}

TEST(CopyCtorAndAssignment, CopyCtorKeepLruOrder) {
  auto a = InitializeCache(3, 3);
  LruCache<int, int> b(a);

  // 1 would be evicted from b
  b.put(4, 4);

  EXPECT_EQ(a.capacity(), b.capacity());
  EXPECT_CACHE_HIT(b, 2, 2);
  EXPECT_CACHE_MISS(b, 1);
}

TEST(CopyCtorAndAssignment, SelfCopyAssignmentWontBreak) {
  auto a = InitializeCache(3, 3);
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
  auto a = InitializeCache(3, 3);
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
  auto a = InitializeCache(3, 3);
  EXPECT_CACHE_HIT(a, 1, 1);
  EXPECT_CACHE_HIT(a, 2, 2);
  EXPECT_CACHE_HIT(a, 3, 3);
  EXPECT_CACHE_MISS(a, 4);
}

TEST(PutGetOperations, AccessEvictedItemTest) {
  auto cache = InitializeCache(3, 3);
  auto value = cache.get(1);
  cache.put(4, 4);
  cache.put(5, 5);
  cache.put(6, 6);
  EXPECT_EQ(*value, 1);
}

TEST(PutGetOperations, PutOverwritesValueForSameKey) {
  auto a = InitializeCache(3, 3);
  a.put(1, 1);
  EXPECT_CACHE_HIT(a, 1, 1);
}

TEST(PutGetOperations, InsertingBeyondCapacityEvictsLeastRecentlyUsed) {
  auto cache = InitializeCache(3, 3);
  cache.put(4, 4);

  EXPECT_CACHE_MISS(cache, 1);
  EXPECT_CACHE_HIT(cache, 2, 2);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_HIT(cache, 4, 4);
}

TEST(PutGetOperations, GetPromotesKeyToMostRecentlyUsed) {
  auto cache = InitializeCache(3, 3);
  EXPECT_CACHE_HIT(cache, 1, 1);
  cache.put(4, 4);

  EXPECT_CACHE_HIT(cache, 4, 4);
  EXPECT_CACHE_HIT(cache, 1, 1);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_MISS(cache, 2);
}

TEST(PutGetOperations, GetOfMiddleNodeMovesItToFrontWithoutCorruption) {
  auto cache = InitializeCache(3, 3);
  EXPECT_CACHE_HIT(cache, 2, 2);
  cache.put(4, 4);

  EXPECT_CACHE_HIT(cache, 4, 4);
  EXPECT_CACHE_HIT(cache, 2, 2);
  EXPECT_CACHE_HIT(cache, 3, 3);
  EXPECT_CACHE_MISS(cache, 1);
}

TEST(PutGetOperations, PutOnExistingKeyPromotesItToMostRecentlyUsed) {
  auto cache = InitializeCache(3, 3);
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

  for (int i = 0; i < 9; ++i)
    EXPECT_CACHE_MISS(c, i);
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

// --- Thread Safety ---
// Run with -fsanitize=thread to catch data races.

template <typename F> void RunThreads(int n, F make_task) {
  std::vector<std::thread> threads;
  threads.reserve(n);
  for (int i = 0; i < n; ++i)
    threads.emplace_back(make_task(i));
  for (auto &t : threads)
    t.join();
}

TEST(ThreadSafety, ConcurrentGetSameKey) {
  auto cache = InitializeCache(10, 1);
  std::atomic<int> wrong_results{0};

  RunThreads(8, [&](int) {
    return [&]() {
      for (int j = 0; j < 1000; ++j) {
        auto v = cache.get(1);
        if (!v || *v != 1)
          ++wrong_results;
      }
    };
  });

  EXPECT_EQ(wrong_results.load(), 0);
}

TEST(ThreadSafety, ConcurrentGetDifferentKey) {
  auto cache = InitializeCache(10, 10);
  RunThreads(8, [&](int) {
    return [&]() {
      for (int j = 0; j < 1000; ++j)
        auto _ = cache.get(j % 10);
    };
  });
}

TEST(ThreadSafety, ConcurrentEmplaceSameKey) {
  auto cache = InitializeCache(1000, 300);
  RunThreads(8, [&](int i) {
    return [&cache, i]() {
      int base = i * 10000;
      for (int j = 0; j < 1000; ++j)
        cache.emplace(1, base + j);
    };
  });

  // No matter which thread run at last, the assert should be true
  EXPECT_EQ(*cache.get(1) % 1000, 999);
}

TEST(ThreadSafety, ConcurrentPutDifferentKey) {
  LruCache<int, int> cache(100);

  RunThreads(8, [&](int i) {
    return [&cache, i]() {
      int base = i * 10000;
      for (int j = 0; j < 1000; ++j)
        cache.put(j, base + j);
    };
  });
}

TEST(ThreadSafety, HitMissCountShouldBeAtomic) {
  auto cache = InitializeCache(100, 100);

  RunThreads(8, [&](int) {
    return [&]() {
      for (int j = 0; j < 200; ++j)
        auto _ = cache.get(j);
    };
  });

  EXPECT_EQ(cache.hit_cnt(), 800);
  EXPECT_EQ(cache.miss_cnt(), 800);
}

TEST(ThreadSafety, ConcurrentWritePeekTest) {
  auto cache = InitializeCache(100, 100);

  RunThreads(8, [&](int i) {
    return [&cache, i]() {
      if (i < 4) {
        for (int j = 0; j < 200; ++j)
          auto _ = cache.peek(j);
      } else {
        for (int j = 0; j < 200; ++j)
          cache.put(j, j);
      }
    };
  });
}

TEST(ThreadSafety, ShardPtrSurviveWhenOtherThreadEviction) {
  auto cache = InitializeCache(100, 100);
  auto res = cache.get(33);

  RunThreads(1, [&](int) {
    return [&]() {
      for (int j = 0; j < 200; ++j)
        cache.put(j + 100, j + 100);
    };
  });

  EXPECT_EQ(*res, 33);
}
