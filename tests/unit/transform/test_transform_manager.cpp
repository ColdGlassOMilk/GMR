#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "gmr/transform.hpp"
#include "test_fixtures.hpp"
#include <cmath>

using namespace gmr;

constexpr float PI = 3.14159265358979323846f;

TEST_CASE_METHOD(EngineTestFixture, "TransformManager create and destroy", "[transform]") {
    auto& tm = TransformManager::instance();

    SECTION("create returns valid handle") {
        TransformHandle h = tm.create();
        REQUIRE(tm.valid(h));
        REQUIRE(tm.count() == 1);
    }

    SECTION("multiple creates return unique handles") {
        TransformHandle h1 = tm.create();
        TransformHandle h2 = tm.create();
        TransformHandle h3 = tm.create();

        REQUIRE(h1 != h2);
        REQUIRE(h2 != h3);
        REQUIRE(tm.count() == 3);
    }

    SECTION("destroy invalidates handle") {
        TransformHandle h = tm.create();
        REQUIRE(tm.valid(h));

        tm.destroy(h);

        REQUIRE_FALSE(tm.valid(h));
        REQUIRE(tm.count() == 0);
    }

    SECTION("destroy invalid handle is safe") {
        tm.destroy(INVALID_HANDLE);
        tm.destroy(9999);
        REQUIRE(tm.count() == 0);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager get", "[transform]") {
    auto& tm = TransformManager::instance();

    SECTION("get returns state for valid handle") {
        TransformHandle h = tm.create();
        Transform2DState* state = tm.get(h);

        REQUIRE(state != nullptr);
    }

    SECTION("get returns nullptr for invalid handle") {
        REQUIRE(tm.get(INVALID_HANDLE) == nullptr);
        REQUIRE(tm.get(9999) == nullptr);
    }

    SECTION("get returns nullptr after destroy") {
        TransformHandle h = tm.create();
        tm.destroy(h);

        REQUIRE(tm.get(h) == nullptr);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager default transform state", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle h = tm.create();
    Transform2DState* state = tm.get(h);

    SECTION("position defaults to origin") {
        REQUIRE(state->position.x == 0.0f);
        REQUIRE(state->position.y == 0.0f);
    }

    SECTION("rotation defaults to zero") {
        REQUIRE(state->rotation == 0.0f);
    }

    SECTION("scale defaults to 1,1") {
        REQUIRE(state->scale.x == 1.0f);
        REQUIRE(state->scale.y == 1.0f);
    }

    SECTION("origin defaults to 0,0") {
        REQUIRE(state->origin.x == 0.0f);
        REQUIRE(state->origin.y == 0.0f);
    }

    SECTION("parallax defaults to 1.0") {
        REQUIRE(state->parallax == 1.0f);
    }

    SECTION("parent defaults to invalid") {
        REQUIRE(state->parent == INVALID_HANDLE);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager world position - no parent", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle h = tm.create();
    Transform2DState* state = tm.get(h);

    state->position = {100.0f, 200.0f};

    Vec2 world_pos = tm.get_world_position(h);

    REQUIRE_APPROX(world_pos.x, 100.0f);
    REQUIRE_APPROX(world_pos.y, 200.0f);
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager world position - with parent", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle parent = tm.create();
    TransformHandle child = tm.create();

    Transform2DState* p = tm.get(parent);
    Transform2DState* c = tm.get(child);

    p->position = {100.0f, 100.0f};
    c->position = {50.0f, 50.0f};

    tm.set_parent(child, parent);

    Vec2 world_pos = tm.get_world_position(child);

    REQUIRE_APPROX(world_pos.x, 150.0f);
    REQUIRE_APPROX(world_pos.y, 150.0f);
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager world rotation - with parent", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle parent = tm.create();
    TransformHandle child = tm.create();

    Transform2DState* p = tm.get(parent);
    Transform2DState* c = tm.get(child);

    p->rotation = PI / 4.0f;  // 45 degrees
    c->rotation = PI / 4.0f;  // 45 degrees

    tm.set_parent(child, parent);

    float world_rot = tm.get_world_rotation(child);

    // Should combine to 90 degrees
    REQUIRE_APPROX(world_rot, PI / 2.0f);
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager world scale - with parent", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle parent = tm.create();
    TransformHandle child = tm.create();

    Transform2DState* p = tm.get(parent);
    Transform2DState* c = tm.get(child);

    p->scale = {2.0f, 2.0f};
    c->scale = {3.0f, 3.0f};

    tm.set_parent(child, parent);

    Vec2 world_scale = tm.get_world_scale(child);

    // Scales multiply
    REQUIRE_APPROX(world_scale.x, 6.0f);
    REQUIRE_APPROX(world_scale.y, 6.0f);
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager hierarchy - set_parent", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle parent = tm.create();
    TransformHandle child = tm.create();

    SECTION("set_parent establishes relationship") {
        tm.set_parent(child, parent);

        Transform2DState* c = tm.get(child);
        REQUIRE(c->parent == parent);
    }

    SECTION("set_parent updates children list") {
        tm.set_parent(child, parent);

        auto children = tm.get_children(parent);
        REQUIRE(children.size() == 1);
        REQUIRE(children[0] == child);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager hierarchy - clear_parent", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle parent = tm.create();
    TransformHandle child = tm.create();
    tm.set_parent(child, parent);

    tm.clear_parent(child);

    SECTION("clears parent reference") {
        Transform2DState* c = tm.get(child);
        REQUIRE(c->parent == INVALID_HANDLE);
    }

    SECTION("removes from children list") {
        auto children = tm.get_children(parent);
        REQUIRE(children.empty());
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager dirty flag - mark_dirty", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle h = tm.create();

    SECTION("new transform is dirty") {
        Transform2DState* state = tm.get(h);
        REQUIRE(state->dirty);
    }

    SECTION("getting world matrix clears dirty flag") {
        tm.get_world_matrix(h);  // Force computation

        Transform2DState* state = tm.get(h);
        REQUIRE_FALSE(state->dirty);
    }

    SECTION("mark_dirty sets dirty flag") {
        tm.get_world_matrix(h);  // Clear dirty
        tm.mark_dirty(h);

        Transform2DState* state = tm.get(h);
        REQUIRE(state->dirty);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager world forward and right", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle h = tm.create();
    Transform2DState* state = tm.get(h);

    // Note: In this coordinate system:
    // - forward = (cos(rotation), sin(rotation))
    // - right = (-sin(rotation), cos(rotation))
    // At zero rotation: forward=(1,0), right=(0,1)

    SECTION("zero rotation - forward is (1, 0)") {
        state->rotation = 0.0f;
        tm.mark_dirty(h);  // Ensure world matrix is recalculated

        Vec2 forward = tm.get_world_forward(h);

        REQUIRE_APPROX(forward.x, 1.0f);
        REQUIRE_APPROX(forward.y, 0.0f);
    }

    SECTION("zero rotation - right is (0, 1)") {
        state->rotation = 0.0f;
        tm.mark_dirty(h);

        Vec2 right = tm.get_world_right(h);

        REQUIRE_APPROX(right.x, 0.0f);
        REQUIRE_APPROX(right.y, 1.0f);
    }

    SECTION("90 degree rotation - forward is (0, 1)") {
        state->rotation = PI / 2.0f;
        tm.mark_dirty(h);

        Vec2 forward = tm.get_world_forward(h);

        REQUIRE_APPROX(forward.x, 0.0f);
        REQUIRE_APPROX(forward.y, 1.0f);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager utility functions", "[transform]") {
    auto& tm = TransformManager::instance();

    TransformHandle h = tm.create();
    Transform2DState* state = tm.get(h);

    SECTION("snap_position_to_grid snaps to grid") {
        state->position = {23.7f, 41.2f};

        tm.snap_position_to_grid(h, 10.0f);

        REQUIRE_APPROX(state->position.x, 20.0f);
        REQUIRE_APPROX(state->position.y, 40.0f);
    }

    SECTION("round_position_to_pixel rounds to integers") {
        state->position = {23.7f, 41.2f};

        tm.round_position_to_pixel(h);

        REQUIRE_APPROX(state->position.x, 24.0f);
        REQUIRE_APPROX(state->position.y, 41.0f);
    }
}

TEST_CASE("TransformManager static lerp functions", "[transform]") {
    SECTION("lerp_position interpolates correctly") {
        Vec2 a{0.0f, 0.0f};
        Vec2 b{100.0f, 200.0f};

        Vec2 mid = TransformManager::lerp_position(a, b, 0.5f);

        REQUIRE_APPROX(mid.x, 50.0f);
        REQUIRE_APPROX(mid.y, 100.0f);
    }

    SECTION("lerp_rotation interpolates correctly") {
        float a = 0.0f;
        float b = PI;

        float mid = TransformManager::lerp_rotation(a, b, 0.5f);

        REQUIRE_APPROX(mid, PI / 2.0f);
    }

    SECTION("lerp_scale interpolates correctly") {
        Vec2 a{1.0f, 1.0f};
        Vec2 b{3.0f, 5.0f};

        Vec2 mid = TransformManager::lerp_scale(a, b, 0.5f);

        REQUIRE_APPROX(mid.x, 2.0f);
        REQUIRE_APPROX(mid.y, 3.0f);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "TransformManager clear", "[transform]") {
    auto& tm = TransformManager::instance();

    tm.create();
    tm.create();
    tm.create();

    REQUIRE(tm.count() == 3);

    tm.clear();

    REQUIRE(tm.count() == 0);
}
