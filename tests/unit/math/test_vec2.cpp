#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "gmr/types.hpp"
#include "test_fixtures.hpp"

using namespace gmr;

TEST_CASE("Vec2 default construction", "[math][vec2]") {
    Vec2 v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
}

TEST_CASE("Vec2 parameterized construction", "[math][vec2]") {
    Vec2 v{3.0f, 4.0f};
    REQUIRE(v.x == 3.0f);
    REQUIRE(v.y == 4.0f);
}

TEST_CASE("Vec2 addition", "[math][vec2]") {
    Vec2 a{1.0f, 2.0f};
    Vec2 b{3.0f, 4.0f};

    Vec2 result = a + b;

    REQUIRE(result.x == 4.0f);
    REQUIRE(result.y == 6.0f);
}

TEST_CASE("Vec2 subtraction", "[math][vec2]") {
    Vec2 a{5.0f, 7.0f};
    Vec2 b{2.0f, 3.0f};

    Vec2 result = a - b;

    REQUIRE(result.x == 3.0f);
    REQUIRE(result.y == 4.0f);
}

TEST_CASE("Vec2 scalar multiplication", "[math][vec2]") {
    Vec2 v{3.0f, 4.0f};

    Vec2 result = v * 2.0f;

    REQUIRE(result.x == 6.0f);
    REQUIRE(result.y == 8.0f);
}

TEST_CASE("Vec2 scalar division", "[math][vec2]") {
    Vec2 v{6.0f, 8.0f};

    Vec2 result = v / 2.0f;

    REQUIRE(result.x == 3.0f);
    REQUIRE(result.y == 4.0f);
}

TEST_CASE("Vec2 negative values", "[math][vec2]") {
    Vec2 v{-3.0f, -4.0f};

    REQUIRE(v.x == -3.0f);
    REQUIRE(v.y == -4.0f);
}

TEST_CASE("Vec2 operations preserve original", "[math][vec2]") {
    Vec2 a{1.0f, 2.0f};
    Vec2 b{3.0f, 4.0f};

    Vec2 result = a + b;

    // Original vectors should be unchanged
    REQUIRE(a.x == 1.0f);
    REQUIRE(a.y == 2.0f);
    REQUIRE(b.x == 3.0f);
    REQUIRE(b.y == 4.0f);
}

TEST_CASE("Vec2 chained operations", "[math][vec2]") {
    Vec2 a{1.0f, 1.0f};
    Vec2 b{2.0f, 2.0f};

    Vec2 result = (a + b) * 2.0f;

    REQUIRE(result.x == 6.0f);
    REQUIRE(result.y == 6.0f);
}
