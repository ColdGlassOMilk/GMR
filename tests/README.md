# GMR Engine Test Suite

Automated unit tests for the GMR game engine using [Catch2](https://github.com/catchorg/Catch2).

## Building Tests

Tests are disabled by default. Enable them with:

```bash
# Configure with tests enabled
cmake -B build -DGMR_BUILD_TESTS=ON

# Build (includes test executable)
cmake --build build

# Or build just the tests
cmake --build build --target gmr_tests
```

## Running Tests

```bash
# Run all tests via CTest
ctest --test-dir build --output-on-failure

# Or run test executable directly (more detailed output)
./build/tests/bin/gmr_tests

# Run specific tests by tag
./build/tests/bin/gmr_tests [math]
./build/tests/bin/gmr_tests [transform]
./build/tests/bin/gmr_tests [node]

# Run a specific test by name
./build/tests/bin/gmr_tests "Matrix2D identity"

# List all tests
./build/tests/bin/gmr_tests --list-tests
```

## Test Structure

```
tests/
├── CMakeLists.txt           # Test build configuration
├── README.md                # This file
├── helpers/
│   ├── test_fixtures.hpp    # Shared test fixtures
│   └── test_fixtures.cpp    # Fixture implementation
└── unit/
    ├── math/
    │   ├── test_matrix2d.cpp    # 2D matrix operations
    │   └── test_vec2.cpp        # Vector operations
    ├── transform/
    │   └── test_transform_manager.cpp  # Transform hierarchy
    ├── node/
    │   └── test_node_manager.cpp       # Scene graph nodes
    ├── resources/
    │   └── test_resource_manager.cpp   # Resource pooling
    ├── draw_queue/
    │   └── test_draw_queue_sorting.cpp # Render ordering
    ├── particle/
    │   └── test_particle_lifecycle.cpp # Particle system
    ├── input/
    │   └── test_input_context.cpp      # Input handling
    └── animation/
        └── test_easing.cpp             # Easing functions
```

## Test Categories (Tags)

- `[math]` - Pure mathematical operations (Matrix2D, Vec2)
- `[transform]` - Transform manager and hierarchy
- `[node]` - Node manager and scene graph
- `[resources]` - Resource loading and reference counting
- `[draw_queue]` - Render command queuing
- `[particle]` - Particle system lifecycle
- `[input]` - Input handling and contexts
- `[animation]` - Animation and easing functions
- `[config]` - Configuration structures

## Writing New Tests

### 1. Create Test File

Place tests in the appropriate subdirectory under `tests/unit/`:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "test_fixtures.hpp"
#include "gmr/your_header.hpp"

TEST_CASE("Feature description", "[tag1][tag2]") {
    // Test code
    REQUIRE(actual == expected);
}
```

### 2. Use Fixtures for Singleton Reset

Most managers are singletons. Use `EngineTestFixture` to reset them between tests:

```cpp
TEST_CASE_METHOD(EngineTestFixture, "Test with clean state", "[tag]") {
    auto& manager = SomeManager::instance();
    // Manager is reset before this test runs
}
```

### 3. Test Sections for Variations

Use `SECTION` for related test cases:

```cpp
TEST_CASE("Resource loading", "[resources]") {
    SECTION("first load creates resource") {
        // ...
    }

    SECTION("duplicate load returns same handle") {
        // ...
    }
}
```

### 4. Floating Point Comparison

Use the provided macro for float comparison:

```cpp
#include "test_fixtures.hpp"

REQUIRE_APPROX(actual, expected);  // Uses FLOAT_EPSILON tolerance
```

Or Catch2 matchers directly:

```cpp
REQUIRE_THAT(value, Catch::Matchers::WithinAbs(expected, 0.0001f));
```

### 5. Add to CMakeLists.txt

Add your test file to `tests/CMakeLists.txt`:

```cmake
set(TEST_SOURCES
    # ... existing tests ...
    unit/subsystem/test_new_feature.cpp
)
```

## Testing Philosophy

### What We Test

- Pure math operations (transforms, matrices, vectors, easing)
- Manager lifecycle (create, destroy, clear)
- Data structure correctness (sorting, hierarchy, reference counting)
- State transitions (input contexts, particle states)
- API contracts (handle validity, parameter validation)

### What We Don't Test

- GPU rendering output
- Audio playback
- Platform-specific windowing
- Ruby script execution (requires mruby runtime)
- File I/O (use mock resource managers instead)

### Test Independence

Each test must:
- Run independently of other tests
- Not rely on test execution order
- Clean up after itself (fixtures handle this)
- Be deterministic (no random seeds unless seeded explicitly)

## Mocking Strategy

### Resource Managers

Create mock subclasses that don't require raylib:

```cpp
class MockResourceManager : public ResourceManager<int32_t, MockResource> {
protected:
    std::optional<MockResource> load_resource(const std::string& path) override {
        return MockResource{path};  // No file I/O
    }
    void unload_resource(MockResource& r) override { /* no-op */ }
};
```

### Singleton Managers

Pass `nullptr` for `mrb_state*` in clear methods - they guard against null:

```cpp
SomeManager::instance().clear(nullptr);  // Safe for unit tests
```

## CI Integration

Tests are designed to run headless in CI environments:

- No GPU required
- No window opened
- Deterministic execution
- Clear pass/fail exit codes
- XML output available via `--reporter junit`

```bash
# Generate JUnit XML for CI
./build/tests/bin/gmr_tests --reporter junit -o test-results.xml
```
