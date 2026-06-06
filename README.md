# lru-cache

## Commands

```bash
# Configure
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build ./build

# Run tests
ctest --test-dir ./build --output-on-failure
```

Check code coverage at https://app.codecov.io/github/carpe-diem-77/lru-cache.
Check benchmark at https://carpe-diem-77.github.io/lru-cache/.