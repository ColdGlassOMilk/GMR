#include <catch2/catch_test_macros.hpp>
#include "gmr/draw_queue.hpp"
#include "gmr/transform.hpp"
#include "test_fixtures.hpp"

using namespace gmr;

// Note: We test observable behavior through the public interface.
// The actual sorting is verified by ensuring commands are queued correctly
// and that the queue behaves as expected. Full sorting verification would
// require either:
// 1. A test accessor (friend class)
// 2. Verifying rendered output (not suitable for unit tests)
// We focus on command queuing, frame management, and camera stack behavior.

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue begin_frame", "[draw_queue]") {
    auto& dq = DrawQueue::instance();

    // Note: begin_frame() resets draw order counter and camera stack,
    // but does NOT clear pending commands. Commands are cleared by flush()
    // at the end of the frame, or explicitly by clear().

    SECTION("resets camera stack") {
        dq.clear();  // Start fresh
        dq.begin_frame();
        dq.queue_camera_begin(1);
        REQUIRE(dq.get_queued_camera() == 1);

        dq.begin_frame();  // New frame resets camera stack
        // After begin_frame, camera stack is cleared
        // get_queued_camera returns last_camera_ when stack is empty
    }

    SECTION("can be called multiple times") {
        dq.clear();
        dq.begin_frame();
        dq.begin_frame();
        dq.begin_frame();
        // Should not crash or cause issues
        REQUIRE(dq.pending_count() == 0);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue clear", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();

    DrawColor white{255, 255, 255, 255};
    dq.queue_rect(0, 0, 10, 10, white);
    dq.queue_circle(0, 0, 5, white);
    dq.queue_line(0, 0, 10, 10, white);

    REQUIRE(dq.pending_count() == 3);

    dq.clear();

    REQUIRE(dq.pending_count() == 0);
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue queue primitives", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();
    DrawColor white{255, 255, 255, 255};

    SECTION("queue_rect adds command") {
        dq.queue_rect(10, 20, 100, 50, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_rect_rotated adds command") {
        dq.queue_rect_rotated(10, 20, 100, 50, 0.5f, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_circle adds command") {
        dq.queue_circle(50, 50, 25, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_circle_gradient adds command") {
        DrawColor inner{255, 0, 0, 255};
        DrawColor outer{0, 0, 255, 255};
        dq.queue_circle_gradient(50, 50, 25, inner, outer);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_line adds command") {
        dq.queue_line(0, 0, 100, 100, white, 2.0f);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_triangle adds command") {
        dq.queue_triangle(0, 0, 50, 100, 100, 0, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_text adds command") {
        dq.queue_text(10, 10, "Hello", 20, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("multiple primitives accumulate") {
        dq.queue_rect(0, 0, 10, 10, white);
        dq.queue_circle(0, 0, 5, white);
        dq.queue_line(0, 0, 10, 10, white);
        dq.queue_triangle(0, 0, 10, 10, 20, 0, white);
        dq.queue_text(0, 0, "Test", 12, white);

        REQUIRE(dq.pending_count() == 5);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue layer parameters", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();
    DrawColor white{255, 255, 255, 255};

    SECTION("primitives can specify layer") {
        // These should all queue without error, using different layers
        dq.queue_rect(0, 0, 10, 10, white, true,
                      static_cast<uint8_t>(RenderLayer::BACKGROUND));
        dq.queue_rect(0, 0, 10, 10, white, true,
                      static_cast<uint8_t>(RenderLayer::WORLD));
        dq.queue_rect(0, 0, 10, 10, white, true,
                      static_cast<uint8_t>(RenderLayer::ENTITIES));
        dq.queue_rect(0, 0, 10, 10, white, true,
                      static_cast<uint8_t>(RenderLayer::EFFECTS));
        dq.queue_rect(0, 0, 10, 10, white, true,
                      static_cast<uint8_t>(RenderLayer::UI));

        REQUIRE(dq.pending_count() == 5);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue z parameters", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();
    DrawColor white{255, 255, 255, 255};

    SECTION("primitives can specify z value") {
        dq.queue_rect(0, 0, 10, 10, white, true, 100, 1.0f);
        dq.queue_rect(0, 0, 10, 10, white, true, 100, 5.0f);
        dq.queue_rect(0, 0, 10, 10, white, true, 100, 10.0f);

        REQUIRE(dq.pending_count() == 3);
    }

    SECTION("negative z values are allowed") {
        dq.queue_rect(0, 0, 10, 10, white, true, 100, -5.0f);
        REQUIRE(dq.pending_count() == 1);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue camera commands", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();

    SECTION("queue_camera_begin adds command") {
        dq.queue_camera_begin(1);  // Arbitrary camera handle
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_camera_end adds command") {
        dq.queue_camera_begin(1);
        dq.queue_camera_end();
        REQUIRE(dq.pending_count() == 2);
    }

    SECTION("camera commands track active camera") {
        dq.queue_camera_begin(42);

        // get_queued_camera should return the current camera
        REQUIRE(dq.get_queued_camera() == 42);
    }

    SECTION("nested cameras work correctly") {
        dq.queue_camera_begin(1);
        dq.queue_camera_begin(2);

        REQUIRE(dq.get_queued_camera() == 2);

        dq.queue_camera_end();
        REQUIRE(dq.get_queued_camera() == 1);

        dq.queue_camera_end();
        // After all cameras ended, should return last used
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue shader commands", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();

    SECTION("queue_shader_begin adds command") {
        dq.queue_shader_begin(1);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_shader_end adds command") {
        dq.queue_shader_begin(1);
        dq.queue_shader_end();
        REQUIRE(dq.pending_count() == 2);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue transform-based primitives", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    auto& tm = TransformManager::instance();
    dq.begin_frame();

    TransformHandle transform = tm.create();
    DrawColor white{255, 255, 255, 255};

    SECTION("queue_rect with transform") {
        dq.queue_rect(transform, 100.0f, 50.0f, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_circle with transform") {
        dq.queue_circle(transform, 25.0f, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_line with transform") {
        dq.queue_line(transform, 0, 0, 100, 100, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_triangle with transform") {
        dq.queue_triangle(transform, 0, 0, 50, 100, 100, 0, white);
        REQUIRE(dq.pending_count() == 1);
    }

    SECTION("queue_text with transform") {
        dq.queue_text(transform, "Hello", 20, white);
        REQUIRE(dq.pending_count() == 1);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue texture quad", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();
    DrawColor white{255, 255, 255, 255};

    SECTION("queue_texture_quad adds command") {
        dq.queue_texture_quad(1, 0, 0, 32, 32, 100, 100, 1.0f, 0.0f, white);
        REQUIRE(dq.pending_count() == 1);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "DrawQueue pending_count", "[draw_queue]") {
    auto& dq = DrawQueue::instance();
    dq.begin_frame();
    DrawColor white{255, 255, 255, 255};

    SECTION("starts at 0 after begin_frame") {
        REQUIRE(dq.pending_count() == 0);
    }

    SECTION("increments with each queued command") {
        for (int i = 0; i < 10; i++) {
            dq.queue_rect(0, 0, 10, 10, white);
            REQUIRE(dq.pending_count() == static_cast<size_t>(i + 1));
        }
    }

    SECTION("returns correct count for mixed commands") {
        dq.queue_rect(0, 0, 10, 10, white);
        dq.queue_camera_begin(1);
        dq.queue_circle(0, 0, 5, white);
        dq.queue_shader_begin(1);
        dq.queue_text(0, 0, "Test", 12, white);
        dq.queue_shader_end();
        dq.queue_camera_end();

        REQUIRE(dq.pending_count() == 7);
    }
}
