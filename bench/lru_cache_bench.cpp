#include <benchmark/benchmark.h>

// TODO: add benchmarks per v4 spec:
//   - Get hit, hot key
//   - Get miss
//   - Put new key, under capacity
//   - Put causing eviction
//   - Emplace vs Put
//   - Mixed 80/20 read/write, Zipfian keys
//   - N threads on shared cache

BENCHMARK_MAIN();
