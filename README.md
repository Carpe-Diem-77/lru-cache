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