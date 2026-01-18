#ifndef GMR_TEST_FIXTURES_HPP
#define GMR_TEST_FIXTURES_HPP

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// Forward declarations to avoid including heavy headers in every test
namespace gmr {
    class TransformManager;
    class NodeManager;
    class DrawQueue;
    class SpriteManager;
    class TextureManager;
    class SoundManager;
    class MusicManager;
    class FontManager;
    class ShaderManager;
    class CameraManager;
    namespace particle { class ParticleManager; }
    namespace input { class InputManager; }
    namespace animation { class AnimationManager; }
    namespace timer { class TimerManager; }
    namespace sequence { class SequenceManager; }
    namespace spatial { class SpatialHash; }
    namespace state_machine { class StateMachineManager; }
    namespace event { class EventQueue; }
    namespace ui { class UIManager; }
}

// Base test fixture that resets all singleton managers between tests
// Use with TEST_CASE_METHOD(EngineTestFixture, "test name", "[tags]")
struct EngineTestFixture {
    EngineTestFixture();
    ~EngineTestFixture();

    // Helper to reset specific managers (called by constructor)
    void reset_all_managers();
};

// Lightweight fixture for math-only tests (no manager reset needed)
struct MathTestFixture {
    // No setup/teardown needed for pure math tests
};

// Floating point comparison tolerance for transform math
constexpr float FLOAT_EPSILON = 0.0001f;

// Macro for approximate float comparison
#define REQUIRE_APPROX(actual, expected) \
    REQUIRE_THAT(actual, Catch::Matchers::WithinAbs(expected, FLOAT_EPSILON))

#endif // GMR_TEST_FIXTURES_HPP
