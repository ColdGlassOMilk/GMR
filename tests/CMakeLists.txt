# GMR Engine Test Suite
# Requires GMR_BUILD_TESTS=ON when configuring CMake

# Fetch Catch2 testing framework
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
)
FetchContent_MakeAvailable(Catch2)

# Test sources
set(TEST_SOURCES
    # Helpers
    helpers/test_fixtures.cpp

    # Unit tests - Math
    unit/math/test_matrix2d.cpp
    unit/math/test_vec2.cpp

    # Unit tests - Transform
    unit/transform/test_transform_manager.cpp

    # Unit tests - Node
    unit/node/test_node_manager.cpp

    # Unit tests - Resources
    unit/resources/test_resource_manager.cpp

    # Unit tests - Draw Queue
    unit/draw_queue/test_draw_queue_sorting.cpp

    # Unit tests - Particle
    unit/particle/test_particle_lifecycle.cpp

    # Unit tests - Input
    unit/input/test_input_context.cpp

    # Unit tests - Animation
    unit/animation/test_easing.cpp
)

# Create test executable
add_executable(gmr_tests ${TEST_SOURCES})

# Link against engine core and Catch2
target_link_libraries(gmr_tests PRIVATE
    gmr_core
    Catch2::Catch2WithMain
)

# Test-specific compile definitions
target_compile_definitions(gmr_tests PRIVATE GMR_TESTING)

# Include test helpers directory
target_include_directories(gmr_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers
)

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(gmr_tests)

# Output test executable to tests/bin directory
set_target_properties(gmr_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/bin
)
