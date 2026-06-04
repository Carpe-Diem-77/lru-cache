#include <benchmark/benchmark.h>

#include "lru_cache.h"

// TODO: replace with full benchmark suite per v4 spec:
//   - Get hit, hot key
//   - Get miss
//   - Put new key, under capacity
//   - Put causing eviction
//   - Emplace vs Put
//   - Mixed 80/20 read/write, Zipfian keys
//   - N threads on shared cache

static void BM_PutGet(benchmark::State& state) {
    LruCache<int, int> cache(1024);
    int key = 0;
    for (auto _ : state) {
        cache.put(key % 1024, key);
        benchmark::DoNotOptimize(cache.get(key % 1024));
        ++key;
    }
}
BENCHMARK(BM_PutGet);

BENCHMARK_MAIN();
