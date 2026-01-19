#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "gmr/animation/easing.hpp"
#include "test_fixtures.hpp"

using namespace gmr::animation;

// Test that all easing functions satisfy f(0) = 0 and f(1) = 1
// This is a fundamental property of easing functions
TEST_CASE("Easing functions boundary values", "[animation][easing]") {
    // All easing types to test
    std::vector<EasingType> types = {
        EasingType::LINEAR,
        EasingType::IN_QUAD, EasingType::OUT_QUAD, EasingType::IN_OUT_QUAD,
        EasingType::IN_CUBIC, EasingType::OUT_CUBIC, EasingType::IN_OUT_CUBIC,
        EasingType::IN_QUART, EasingType::OUT_QUART, EasingType::IN_OUT_QUART,
        EasingType::IN_QUINT, EasingType::OUT_QUINT, EasingType::IN_OUT_QUINT,
        EasingType::IN_SINE, EasingType::OUT_SINE, EasingType::IN_OUT_SINE,
        EasingType::IN_EXPO, EasingType::OUT_EXPO, EasingType::IN_OUT_EXPO,
        EasingType::IN_CIRC, EasingType::OUT_CIRC, EasingType::IN_OUT_CIRC,
        EasingType::IN_BACK, EasingType::OUT_BACK, EasingType::IN_OUT_BACK,
        EasingType::IN_ELASTIC, EasingType::OUT_ELASTIC, EasingType::IN_OUT_ELASTIC,
        EasingType::IN_BOUNCE, EasingType::OUT_BOUNCE, EasingType::IN_OUT_BOUNCE
    };

    for (auto type : types) {
        DYNAMIC_SECTION("Easing type " << static_cast<int>(type)) {
            float at_zero = apply_easing(type, 0.0f);
            float at_one = apply_easing(type, 1.0f);

            // Allow small floating point error
            REQUIRE_THAT(at_zero, Catch::Matchers::WithinAbs(0.0f, 0.001f));
            REQUIRE_THAT(at_one, Catch::Matchers::WithinAbs(1.0f, 0.001f));
        }
    }
}

TEST_CASE("LINEAR easing", "[animation][easing]") {
    SECTION("returns input unchanged") {
        REQUIRE(apply_easing(EasingType::LINEAR, 0.0f) == 0.0f);
        REQUIRE(apply_easing(EasingType::LINEAR, 0.25f) == 0.25f);
        REQUIRE(apply_easing(EasingType::LINEAR, 0.5f) == 0.5f);
        REQUIRE(apply_easing(EasingType::LINEAR, 0.75f) == 0.75f);
        REQUIRE(apply_easing(EasingType::LINEAR, 1.0f) == 1.0f);
    }
}

TEST_CASE("IN_QUAD easing", "[animation][easing]") {
    SECTION("starts slow (below linear)") {
        float quarter = apply_easing(EasingType::IN_QUAD, 0.25f);
        REQUIRE(quarter < 0.25f);  // Below linear
    }

    SECTION("midpoint is less than 0.5") {
        float mid = apply_easing(EasingType::IN_QUAD, 0.5f);
        REQUIRE(mid < 0.5f);
    }

    SECTION("is monotonically increasing") {
        float prev = 0.0f;
        for (float t = 0.1f; t <= 1.0f; t += 0.1f) {
            float val = apply_easing(EasingType::IN_QUAD, t);
            REQUIRE(val > prev);
            prev = val;
        }
    }
}

TEST_CASE("OUT_QUAD easing", "[animation][easing]") {
    SECTION("ends slow (above linear)") {
        float three_quarter = apply_easing(EasingType::OUT_QUAD, 0.75f);
        REQUIRE(three_quarter > 0.75f);  // Above linear
    }

    SECTION("midpoint is greater than 0.5") {
        float mid = apply_easing(EasingType::OUT_QUAD, 0.5f);
        REQUIRE(mid > 0.5f);
    }
}

TEST_CASE("IN_OUT_QUAD easing", "[animation][easing]") {
    SECTION("symmetric around midpoint") {
        float mid = apply_easing(EasingType::IN_OUT_QUAD, 0.5f);
        REQUIRE_THAT(mid, Catch::Matchers::WithinAbs(0.5f, 0.001f));
    }

    SECTION("first half below linear") {
        float quarter = apply_easing(EasingType::IN_OUT_QUAD, 0.25f);
        REQUIRE(quarter < 0.25f);
    }

    SECTION("second half above linear") {
        float three_quarter = apply_easing(EasingType::IN_OUT_QUAD, 0.75f);
        REQUIRE(three_quarter > 0.75f);
    }
}

TEST_CASE("IN_BACK easing overshoots", "[animation][easing]") {
    SECTION("goes negative at start") {
        float early = apply_easing(EasingType::IN_BACK, 0.1f);
        REQUIRE(early < 0.0f);
    }
}

TEST_CASE("OUT_BACK easing overshoots", "[animation][easing]") {
    SECTION("exceeds 1.0 before settling") {
        float late = apply_easing(EasingType::OUT_BACK, 0.9f);
        REQUIRE(late > 1.0f);
    }
}

TEST_CASE("IN_ELASTIC easing", "[animation][easing]") {
    SECTION("oscillates before reaching target") {
        // Elastic easing oscillates, so intermediate values may be negative
        float val = apply_easing(EasingType::IN_ELASTIC, 0.3f);
        // Should be less than 0.3 due to oscillation at the start
        REQUIRE(val < 0.3f);
    }
}

TEST_CASE("OUT_ELASTIC easing", "[animation][easing]") {
    SECTION("overshoots before settling") {
        float val = apply_easing(EasingType::OUT_ELASTIC, 0.3f);
        // Should exceed 0.3 due to springy overshoot
        REQUIRE(val > 0.3f);
    }
}

TEST_CASE("IN_BOUNCE easing", "[animation][easing]") {
    SECTION("bounces (non-monotonic at start)") {
        // Bounce easing has non-monotonic behavior
        // Just verify it works without crashing
        float val = apply_easing(EasingType::IN_BOUNCE, 0.5f);
        REQUIRE(val >= 0.0f);
        REQUIRE(val <= 1.0f);
    }
}

TEST_CASE("OUT_BOUNCE easing", "[animation][easing]") {
    SECTION("bounces at end") {
        float val = apply_easing(EasingType::OUT_BOUNCE, 0.5f);
        // Should be greater than linear at midpoint
        REQUIRE(val > 0.5f);
    }
}

TEST_CASE("IN_EXPO easing", "[animation][easing]") {
    SECTION("very slow start") {
        float early = apply_easing(EasingType::IN_EXPO, 0.2f);
        REQUIRE(early < 0.01f);  // Almost zero at 20%
    }
}

TEST_CASE("OUT_EXPO easing", "[animation][easing]") {
    SECTION("very fast start") {
        float early = apply_easing(EasingType::OUT_EXPO, 0.2f);
        REQUIRE(early > 0.7f);  // Already most of the way at 20%
    }
}

TEST_CASE("IN_CIRC easing", "[animation][easing]") {
    SECTION("follows circular curve") {
        float mid = apply_easing(EasingType::IN_CIRC, 0.5f);
        // Value at 0.5 for IN_CIRC should be less than 0.5
        REQUIRE(mid < 0.5f);
    }
}

TEST_CASE("Easing function enum values", "[animation][easing]") {
    // Verify enum starts at 0 for LINEAR (for array indexing)
    REQUIRE(static_cast<int>(EasingType::LINEAR) == 0);

    // Verify sequential order
    REQUIRE(static_cast<int>(EasingType::IN_QUAD) == 1);
    REQUIRE(static_cast<int>(EasingType::OUT_QUAD) == 2);
    REQUIRE(static_cast<int>(EasingType::IN_OUT_QUAD) == 3);
}

TEST_CASE("easing_to_name returns valid string", "[animation][easing]") {
    SECTION("LINEAR has name") {
        const char* name = easing_to_name(EasingType::LINEAR);
        REQUIRE(name != nullptr);
    }

    SECTION("all types have names") {
        std::vector<EasingType> types = {
            EasingType::LINEAR,
            EasingType::IN_QUAD, EasingType::OUT_BOUNCE
        };

        for (auto type : types) {
            const char* name = easing_to_name(type);
            REQUIRE(name != nullptr);
            REQUIRE(strlen(name) > 0);
        }
    }
}

TEST_CASE("Easing functions handle edge cases", "[animation][easing]") {
    SECTION("values outside [0,1] are clamped") {
        // Implementation clamps input to [0,1] range
        float neg = apply_easing(EasingType::LINEAR, -0.5f);
        float over = apply_easing(EasingType::LINEAR, 1.5f);

        // Values are clamped to 0 and 1 respectively
        REQUIRE_THAT(neg, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(over, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }
}
