# lru-cache

## Dependencies

| Tool | Purpose |
|------|---------|
| `cmake` ≥ 3.20 | Build system |
| `g++` ≥ 11 | C++20 compiler |
| `git` | FetchContent downloads (googletest, googlebenchmark) |

```bash
sudo apt update && sudo apt install -y cmake g++ git
```

## Commands

```bash
# Build and run tests
## Configure
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug

## Build
cmake --build ./build

## Run tests
ctest --test-dir ./build --output-on-failure

# Build and run tests with tsan
cmake -S . -B ./build-tsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON

## Build
cmake --build ./build-tsan

## Run tests
ctest --test-dir ./build-tsan --output-on-failure
```

## Report and Dashboard

Check code coverage at https://app.codecov.io/github/carpe-diem-77/lru-cache.
Check benchmark at https://carpe-diem-77.github.io/lru-cache/.