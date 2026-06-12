#include "lru_cache.h"
#include <benchmark/benchmark.h>
#include <cmath>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <unordered_map>

static constexpr int kCacheSize = 1024;

static std::discrete_distribution<int> ZipfDistribution(int n,
                                                        double skew = 1.0) {
  std::vector<double> weights(n);
  for (int i = 0; i < n; ++i)
    weights[i] = 1.0 / std::pow(i + 1, skew);
  return std::discrete_distribution<int>(weights.begin(), weights.end());
}

// --- Baseline: std::unordered_map + std::mutex ---

struct BaselineMap {
  std::unordered_map<int, int> map;
  std::mutex mtx;

  std::optional<int> get(int key) {
    std::lock_guard guard(mtx);
    auto it = map.find(key);
    return it != map.end() ? std::optional{it->second} : std::nullopt;
  }

  void put(int key, int value) {
    std::lock_guard guard(mtx);
    map[key] = value;
  }
};

// --- Get hit ---

static void BM_GetHit(benchmark::State &state) {
  LruCache<int, int> cache(kCacheSize);
  cache.put(42, 42);
  for (auto _ : state)
    benchmark::DoNotOptimize(cache.get(42));
}
BENCHMARK(BM_GetHit);

static void BM_GetHit_Baseline(benchmark::State &state) {
  BaselineMap m;
  m.put(42, 42);
  for (auto _ : state)
    benchmark::DoNotOptimize(m.get(42));
}
BENCHMARK(BM_GetHit_Baseline);

// --- Get miss ---

static void BM_GetMiss(benchmark::State &state) {
  LruCache<int, int> cache(kCacheSize);
  for (auto _ : state)
    benchmark::DoNotOptimize(cache.get(kCacheSize + 1));
}
BENCHMARK(BM_GetMiss);

static void BM_GetMiss_Baseline(benchmark::State &state) {
  BaselineMap m;
  for (auto _ : state)
    benchmark::DoNotOptimize(m.get(kCacheSize + 1));
}
BENCHMARK(BM_GetMiss_Baseline);

// --- Put under capacity ---

static void BM_PutUnderCapacity(benchmark::State &state) {
  LruCache<int, int> cache(kCacheSize);
  int key = 0;
  for (auto _ : state)
    cache.put(key++ % (kCacheSize / 2), key);
}
BENCHMARK(BM_PutUnderCapacity);

static void BM_PutUnderCapacity_Baseline(benchmark::State &state) {
  BaselineMap m;
  int key = 0;
  for (auto _ : state)
    m.put(key++ % (kCacheSize / 2), key);
}
BENCHMARK(BM_PutUnderCapacity_Baseline);

// --- Put with eviction ---

static void BM_PutEviction(benchmark::State &state) {
  LruCache<int, int> cache(kCacheSize);
  int key = 0;
  for (auto _ : state)
    cache.put(key++ % (kCacheSize * 2), key);
}
BENCHMARK(BM_PutEviction);

// Baseline grows unbounded — eviction cost is LRU-specific, no baseline
// equivalent.

// --- Emplace vs Put ---

static void BM_Emplace(benchmark::State &state) {
  LruCache<int, int> cache(kCacheSize);
  int key = 0;
  for (auto _ : state)
    cache.emplace(key++ % kCacheSize, key);
}
BENCHMARK(BM_Emplace);

static void BM_Put(benchmark::State &state) {
  LruCache<int, int> cache(kCacheSize);
  int key = 0;
  for (auto _ : state)
    cache.put(key++ % kCacheSize, key);
}
BENCHMARK(BM_Put);

// --- Mixed 80/20 Zipfian ---

static void BM_Mixed8020Zipfian(benchmark::State &state) {
  LruCache<int, int> cache(kCacheSize);
  for (int i = 0; i < kCacheSize; ++i)
    cache.put(i, i);

  std::mt19937 rng(42);
  auto key_dist = ZipfDistribution(kCacheSize * 2);
  std::uniform_int_distribution<int> op_dist(0, 9);

  for (auto _ : state) {
    int key = key_dist(rng);
    if (op_dist(rng) < 8)
      benchmark::DoNotOptimize(cache.get(key));
    else
      cache.put(key, key);
  }
}
BENCHMARK(BM_Mixed8020Zipfian);

static void BM_Mixed8020Zipfian_Baseline(benchmark::State &state) {
  BaselineMap m;
  for (int i = 0; i < kCacheSize; ++i)
    m.put(i, i);

  std::mt19937 rng(42);
  auto key_dist = ZipfDistribution(kCacheSize * 2);
  std::uniform_int_distribution<int> op_dist(0, 9);

  for (auto _ : state) {
    int key = key_dist(rng);
    if (op_dist(rng) < 8)
      benchmark::DoNotOptimize(m.get(key));
    else
      m.put(key, key);
  }
}
BENCHMARK(BM_Mixed8020Zipfian_Baseline);

// --- Concurrent shared cache ---

static void BM_ConcurrentSharedCache(benchmark::State &state) {
  static std::unique_ptr<LruCache<int, int>> cache;
  static std::once_flag init_flag;
  std::call_once(init_flag, [] {
    cache = std::make_unique<LruCache<int, int>>(kCacheSize);
    for (int i = 0; i < kCacheSize; ++i)
      cache->put(i, i);
  });

  std::mt19937 rng(state.thread_index());
  auto key_dist = ZipfDistribution(kCacheSize * 2);

  for (auto _ : state)
    benchmark::DoNotOptimize(cache->get(key_dist(rng)));
}
BENCHMARK(BM_ConcurrentSharedCache)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8);

static void BM_ConcurrentSharedCache_Baseline(benchmark::State &state) {
  static std::unique_ptr<BaselineMap> m;
  static std::once_flag init_flag;
  std::call_once(init_flag, [] {
    m = std::make_unique<BaselineMap>();
    for (int i = 0; i < kCacheSize; ++i)
      m->put(i, i);
  });

  std::mt19937 rng(state.thread_index());
  auto key_dist = ZipfDistribution(kCacheSize * 2);

  for (auto _ : state)
    benchmark::DoNotOptimize(m->get(key_dist(rng)));
}
BENCHMARK(BM_ConcurrentSharedCache_Baseline)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8);

BENCHMARK_MAIN();
