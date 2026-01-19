#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "gmr/transform.hpp"
#include "test_fixtures.hpp"
#include <cmath>

using namespace gmr;

// PI constant for rotation tests
constexpr float PI = 3.14159265358979323846f;

TEST_CASE("Matrix2D identity", "[math][transform]") {
    auto m = Matrix2D::identity();

    REQUIRE(m.a == 1.0f);
    REQUIRE(m.b == 0.0f);
    REQUIRE(m.c == 0.0f);
    REQUIRE(m.d == 1.0f);
    REQUIRE(m.tx == 0.0f);
    REQUIRE(m.ty == 0.0f);
}

TEST_CASE("Matrix2D from_transform - translation only", "[math][transform]") {
    auto m = Matrix2D::from_transform(10.0f, 20.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);

    REQUIRE_APPROX(m.a, 1.0f);
    REQUIRE_APPROX(m.d, 1.0f);
    REQUIRE_APPROX(m.tx, 10.0f);
    REQUIRE_APPROX(m.ty, 20.0f);
}

TEST_CASE("Matrix2D from_transform - scale only", "[math][transform]") {
    auto m = Matrix2D::from_transform(0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 0.0f);

    REQUIRE_APPROX(m.a, 2.0f);
    REQUIRE_APPROX(m.d, 3.0f);
    REQUIRE_APPROX(m.b, 0.0f);
    REQUIRE_APPROX(m.c, 0.0f);
}

TEST_CASE("Matrix2D from_transform - rotation 90 degrees", "[math][transform]") {
    float rotation = PI / 2.0f;  // 90 degrees
    auto m = Matrix2D::from_transform(0.0f, 0.0f, rotation, 1.0f, 1.0f, 0.0f, 0.0f);

    // cos(90) = 0, sin(90) = 1
    REQUIRE_APPROX(m.a, 0.0f);   // cos
    REQUIRE_APPROX(m.b, -1.0f);  // -sin
    REQUIRE_APPROX(m.c, 1.0f);   // sin
    REQUIRE_APPROX(m.d, 0.0f);   // cos
}

TEST_CASE("Matrix2D from_transform - rotation 180 degrees", "[math][transform]") {
    float rotation = PI;  // 180 degrees
    auto m = Matrix2D::from_transform(0.0f, 0.0f, rotation, 1.0f, 1.0f, 0.0f, 0.0f);

    // cos(180) = -1, sin(180) = 0
    REQUIRE_APPROX(m.a, -1.0f);
    REQUIRE_APPROX(m.b, 0.0f);
    REQUIRE_APPROX(m.c, 0.0f);
    REQUIRE_APPROX(m.d, -1.0f);
}

TEST_CASE("Matrix2D from_transform - origin offset", "[math][transform]") {
    // Position at (100, 100), origin at (50, 50)
    // Visual position should still be at (100, 100), but pivot is centered
    auto m = Matrix2D::from_transform(100.0f, 100.0f, 0.0f, 1.0f, 1.0f, 50.0f, 50.0f);

    // Translation is position minus origin (when no rotation/scale)
    REQUIRE_APPROX(m.tx, 50.0f);
    REQUIRE_APPROX(m.ty, 50.0f);
}

TEST_CASE("Matrix2D transform_point - identity", "[math][transform]") {
    auto m = Matrix2D::identity();
    Vec2 p{5.0f, 7.0f};

    Vec2 result = m.transform_point(p);

    REQUIRE_APPROX(result.x, 5.0f);
    REQUIRE_APPROX(result.y, 7.0f);
}

TEST_CASE("Matrix2D transform_point - translation", "[math][transform]") {
    auto m = Matrix2D::from_transform(10.0f, 20.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    Vec2 p{5.0f, 5.0f};

    Vec2 result = m.transform_point(p);

    REQUIRE_APPROX(result.x, 15.0f);
    REQUIRE_APPROX(result.y, 25.0f);
}

TEST_CASE("Matrix2D transform_point - scale", "[math][transform]") {
    auto m = Matrix2D::from_transform(0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 0.0f);
    Vec2 p{5.0f, 5.0f};

    Vec2 result = m.transform_point(p);

    REQUIRE_APPROX(result.x, 10.0f);
    REQUIRE_APPROX(result.y, 15.0f);
}

TEST_CASE("Matrix2D transform_point - rotation 90 degrees", "[math][transform]") {
    auto m = Matrix2D::from_transform(0.0f, 0.0f, PI / 2.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    Vec2 p{1.0f, 0.0f};  // Point on positive x-axis

    Vec2 result = m.transform_point(p);

    // Rotating (1, 0) by 90 degrees should give (0, 1)
    REQUIRE_APPROX(result.x, 0.0f);
    REQUIRE_APPROX(result.y, 1.0f);
}

TEST_CASE("Matrix2D multiplication - translation composition", "[math][transform]") {
    auto m1 = Matrix2D::from_transform(10.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    auto m2 = Matrix2D::from_transform(0.0f, 20.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);

    auto result = m1 * m2;

    // Composed translation should add up
    REQUIRE_APPROX(result.tx, 10.0f);
    REQUIRE_APPROX(result.ty, 20.0f);
}

TEST_CASE("Matrix2D multiplication - scale then translate", "[math][transform]") {
    auto scale = Matrix2D::from_transform(0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 0.0f, 0.0f);
    auto translate = Matrix2D::from_transform(10.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);

    // translate * scale: first scale, then translate
    auto result = translate * scale;
    Vec2 p{1.0f, 0.0f};
    Vec2 transformed = result.transform_point(p);

    // 1 * 2 = 2, then + 10 = 12
    REQUIRE_APPROX(transformed.x, 12.0f);
}

TEST_CASE("Matrix2D multiplication - translate then scale", "[math][transform]") {
    auto scale = Matrix2D::from_transform(0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 0.0f, 0.0f);
    auto translate = Matrix2D::from_transform(10.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);

    // scale * translate: first translate, then scale
    auto result = scale * translate;
    Vec2 p{1.0f, 0.0f};
    Vec2 transformed = result.transform_point(p);

    // (1 + 10) * 2 = 22
    REQUIRE_APPROX(transformed.x, 22.0f);
}

TEST_CASE("Matrix2D inverse - identity", "[math][transform]") {
    auto m = Matrix2D::identity();
    auto inv = m.inverse();

    REQUIRE_APPROX(inv.a, 1.0f);
    REQUIRE_APPROX(inv.d, 1.0f);
    REQUIRE_APPROX(inv.tx, 0.0f);
    REQUIRE_APPROX(inv.ty, 0.0f);
}

TEST_CASE("Matrix2D inverse - translation", "[math][transform]") {
    auto m = Matrix2D::from_transform(10.0f, 20.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    auto inv = m.inverse();

    REQUIRE_APPROX(inv.tx, -10.0f);
    REQUIRE_APPROX(inv.ty, -20.0f);
}

TEST_CASE("Matrix2D inverse - scale", "[math][transform]") {
    auto m = Matrix2D::from_transform(0.0f, 0.0f, 0.0f, 2.0f, 4.0f, 0.0f, 0.0f);
    auto inv = m.inverse();

    REQUIRE_APPROX(inv.a, 0.5f);
    REQUIRE_APPROX(inv.d, 0.25f);
}

TEST_CASE("Matrix2D inverse - round trip", "[math][transform]") {
    // Create a complex transform
    auto m = Matrix2D::from_transform(10.0f, 20.0f, PI / 4.0f, 2.0f, 3.0f, 5.0f, 5.0f);
    Vec2 original{7.0f, 11.0f};

    Vec2 transformed = m.transform_point(original);
    Vec2 back = m.transform_inverse_point(transformed);

    REQUIRE_APPROX(back.x, original.x);
    REQUIRE_APPROX(back.y, original.y);
}

TEST_CASE("Matrix2D is_invertible", "[math][transform]") {
    SECTION("identity is invertible") {
        auto m = Matrix2D::identity();
        REQUIRE(m.is_invertible());
    }

    SECTION("normal transform is invertible") {
        auto m = Matrix2D::from_transform(10.0f, 20.0f, 0.5f, 2.0f, 3.0f, 0.0f, 0.0f);
        REQUIRE(m.is_invertible());
    }

    SECTION("zero scale is not invertible") {
        Matrix2D m{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        REQUIRE_FALSE(m.is_invertible());
    }
}

TEST_CASE("Matrix2D transform_direction - ignores translation", "[math][transform]") {
    auto m = Matrix2D::from_transform(100.0f, 200.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    Vec2 dir{1.0f, 0.0f};

    Vec2 result = m.transform_direction(dir);

    // Direction should not be affected by translation
    REQUIRE_APPROX(result.x, 1.0f);
    REQUIRE_APPROX(result.y, 0.0f);
}

TEST_CASE("Matrix2D transform_direction - applies rotation", "[math][transform]") {
    auto m = Matrix2D::from_transform(0.0f, 0.0f, PI / 2.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    Vec2 dir{1.0f, 0.0f};  // Right direction

    Vec2 result = m.transform_direction(dir);

    // Rotating right by 90 degrees should give up
    REQUIRE_APPROX(result.x, 0.0f);
    REQUIRE_APPROX(result.y, 1.0f);
}

TEST_CASE("Matrix2D transform_direction - applies scale", "[math][transform]") {
    auto m = Matrix2D::from_transform(0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 0.0f);
    Vec2 dir{1.0f, 1.0f};

    Vec2 result = m.transform_direction(dir);

    REQUIRE_APPROX(result.x, 2.0f);
    REQUIRE_APPROX(result.y, 3.0f);
}
