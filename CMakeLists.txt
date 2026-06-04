cmake_minimum_required(VERSION 3.20)
project(lru-cache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit build/compile_commands.json so editors (VS Code IntelliSense, clangd)
# pick up include paths and flags automatically.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- library under test ---
# Header-only template library: no .cpp to compile, so use INTERFACE.
# Include dirs propagate to consumers; warnings are set on the test exe
# (an INTERFACE lib has no compilation of its own to apply them to).
add_library(lru_cache INTERFACE)
target_include_directories(lru_cache INTERFACE src)

# --- tests + benchmarks (FetchContent) ---
# FetchContent downloads + builds gtest as part of this project at configure
# time. No system install required; the pinned GIT_TAG makes the build
# reproducible. On Windows/MSVC, gtest_force_shared_crt makes gtest use the
# same CRT as the rest of the project (otherwise: link errors).
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(lru_cache_bench bench/lru_cache_bench.cpp)
target_link_libraries(lru_cache_bench PRIVATE lru_cache benchmark::benchmark)

add_executable(lru_cache_test
  test/lru_cache_tests.cpp
)

target_compile_options(lru_cache_test PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(lru_cache_test PRIVATE
  lru_cache
  GTest::gtest_main   # provides `int main()` so we don't have to write one
)

# `gtest_discover_tests` queries the binary for each TEST/TEST_F at build time
# and registers them individually with CTest. Run them with `ctest` from the
# build dir, or run the binary directly to see gtest's own output.
include(GoogleTest)
gtest_discover_tests(lru_cache_test)
