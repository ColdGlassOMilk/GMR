#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "gmr/particle/particle_types.hpp"
#include "gmr/particle/emitter_config.hpp"
#include "gmr/particle/emitter_state.hpp"
#include "test_fixtures.hpp"
#include <cstdlib>

using namespace gmr;
using namespace gmr::particle;

// Seed random for deterministic tests
struct DeterministicRandomFixture {
    DeterministicRandomFixture() {
        std::srand(12345);
    }
};

TEST_CASE("ParticleState defaults", "[particle]") {
    ParticleState p;

    SECTION("particle is not alive by default") {
        REQUIRE_FALSE(p.alive);
    }

    SECTION("position defaults to origin") {
        REQUIRE(p.position.x == 0.0f);
        REQUIRE(p.position.y == 0.0f);
    }

    SECTION("velocity defaults to zero") {
        REQUIRE(p.velocity.x == 0.0f);
        REQUIRE(p.velocity.y == 0.0f);
    }

    SECTION("lifetime defaults to zero") {
        REQUIRE(p.lifetime == 0.0f);
        REQUIRE(p.max_lifetime == 1.0f);
    }

    SECTION("size defaults to 1.0") {
        REQUIRE(p.size == 1.0f);
    }

    SECTION("color defaults to white") {
        REQUIRE(p.color.r == 255);
        REQUIRE(p.color.g == 255);
        REQUIRE(p.color.b == 255);
        REQUIRE(p.color.a == 255);
    }
}

TEST_CASE("FloatRange", "[particle][config]") {
    SECTION("single value constructor") {
        FloatRange r{5.0f};
        REQUIRE(r.min == 5.0f);
        REQUIRE(r.max == 5.0f);
    }

    SECTION("range constructor") {
        FloatRange r{1.0f, 10.0f};
        REQUIRE(r.min == 1.0f);
        REQUIRE(r.max == 10.0f);
    }

    SECTION("lerp at 0 returns min") {
        FloatRange r{0.0f, 100.0f};
        REQUIRE(r.lerp(0.0f) == 0.0f);
    }

    SECTION("lerp at 1 returns max") {
        FloatRange r{0.0f, 100.0f};
        REQUIRE(r.lerp(1.0f) == 100.0f);
    }

    SECTION("lerp at 0.5 returns midpoint") {
        FloatRange r{0.0f, 100.0f};
        REQUIRE(r.lerp(0.5f) == 50.0f);
    }

    SECTION("random returns value in range") {
        std::srand(12345);
        FloatRange r{0.0f, 100.0f};

        for (int i = 0; i < 100; i++) {
            float val = r.random();
            REQUIRE(val >= 0.0f);
            REQUIRE(val <= 100.0f);
        }
    }

    SECTION("random with same min/max returns that value") {
        FloatRange r{42.0f};
        REQUIRE(r.random() == 42.0f);
    }
}

TEST_CASE("ColorRange", "[particle][config]") {
    SECTION("single color constructor") {
        Color c{128, 64, 32, 255};
        ColorRange r{c};
        REQUIRE(r.min.r == 128);
        REQUIRE(r.max.r == 128);
    }

    SECTION("random returns color in range") {
        std::srand(12345);
        Color min{0, 0, 0, 0};
        Color max{255, 255, 255, 255};
        ColorRange r{min, max};

        for (int i = 0; i < 100; i++) {
            Color c = r.random();
            REQUIRE(c.r >= 0);
            REQUIRE(c.r <= 255);
            REQUIRE(c.g >= 0);
            REQUIRE(c.g <= 255);
        }
    }
}

TEST_CASE("EmitterConfig defaults", "[particle][config]") {
    EmitterConfig cfg;

    SECTION("spawn rate defaults to 10") {
        REQUIRE(cfg.spawn_rate == 10.0f);
    }

    SECTION("max particles defaults to DEFAULT_POOL_SIZE") {
        REQUIRE(cfg.max_particles == DEFAULT_POOL_SIZE);
    }

    SECTION("emission shape defaults to POINT") {
        REQUIRE(cfg.shape == EmissionShape::POINT);
    }

    SECTION("velocity mode defaults to DIRECTIONAL") {
        REQUIRE(cfg.velocity_mode == VelocityMode::DIRECTIONAL);
    }

    SECTION("world space defaults to true") {
        REQUIRE(cfg.world_space == true);
    }

    SECTION("scaled defaults to true") {
        REQUIRE(cfg.scaled == true);
    }
}

TEST_CASE("EmitterState init", "[particle][emitter]") {
    EmitterConfig cfg;
    cfg.max_particles = 100;

    EmitterState emitter;
    emitter.init(&cfg);

    SECTION("allocates particle pool") {
        REQUIRE(emitter.particles.size() == 100);
    }

    SECTION("all particles are dead initially") {
        for (const auto& p : emitter.particles) {
            REQUIRE_FALSE(p.alive);
        }
    }

    SECTION("alive count is zero") {
        REQUIRE(emitter.alive_count == 0);
    }

    SECTION("emitting is false") {
        REQUIRE_FALSE(emitter.emitting);
    }

    SECTION("active is true") {
        REQUIRE(emitter.active);
    }

    SECTION("spawn accumulator is zero") {
        REQUIRE(emitter.spawn_accumulator == 0.0f);
    }
}

TEST_CASE("EmitterState spawn_particle", "[particle][emitter]") {
    EmitterConfig cfg;
    cfg.max_particles = 3;

    EmitterState emitter;
    emitter.init(&cfg);

    SECTION("returns particle and marks it alive") {
        ParticleState* p = emitter.spawn_particle();

        REQUIRE(p != nullptr);
        REQUIRE(p->alive == true);
    }

    SECTION("increments alive_count") {
        REQUIRE(emitter.alive_count == 0);

        emitter.spawn_particle();
        REQUIRE(emitter.alive_count == 1);

        emitter.spawn_particle();
        REQUIRE(emitter.alive_count == 2);
    }

    SECTION("returns nullptr when pool exhausted") {
        emitter.spawn_particle();
        emitter.spawn_particle();
        emitter.spawn_particle();

        ParticleState* p = emitter.spawn_particle();
        REQUIRE(p == nullptr);
    }

    SECTION("reuses dead particle slots") {
        ParticleState* p1 = emitter.spawn_particle();
        emitter.spawn_particle();
        emitter.spawn_particle();

        // Kill first particle
        p1->alive = false;
        emitter.alive_count--;

        // Should be able to spawn again
        ParticleState* p4 = emitter.spawn_particle();
        REQUIRE(p4 != nullptr);
    }
}

TEST_CASE("EmitterState reset", "[particle][emitter]") {
    EmitterConfig cfg;
    cfg.max_particles = 10;

    EmitterState emitter;
    emitter.init(&cfg);

    // Spawn some particles
    emitter.spawn_particle();
    emitter.spawn_particle();
    emitter.spawn_particle();
    emitter.spawn_accumulator = 5.5f;
    emitter.burst_timer = 2.0f;

    REQUIRE(emitter.alive_count == 3);

    emitter.reset();

    SECTION("kills all particles") {
        for (const auto& p : emitter.particles) {
            REQUIRE_FALSE(p.alive);
        }
    }

    SECTION("resets alive count") {
        REQUIRE(emitter.alive_count == 0);
    }

    SECTION("resets spawn accumulator") {
        REQUIRE(emitter.spawn_accumulator == 0.0f);
    }

    SECTION("resets burst timer") {
        REQUIRE(emitter.burst_timer == 0.0f);
    }
}

TEST_CASE("EmitterState compact", "[particle][emitter]") {
    EmitterConfig cfg;
    cfg.max_particles = 5;

    EmitterState emitter;
    emitter.init(&cfg);

    // Spawn 5 particles
    ParticleState* p0 = emitter.spawn_particle();
    ParticleState* p1 = emitter.spawn_particle();
    ParticleState* p2 = emitter.spawn_particle();
    ParticleState* p3 = emitter.spawn_particle();
    ParticleState* p4 = emitter.spawn_particle();

    // Kill some to create gaps
    p1->alive = false;
    p3->alive = false;
    emitter.alive_count = 3;

    emitter.compact();

    SECTION("moves alive particles to front") {
        REQUIRE(emitter.particles[0].alive == true);
        REQUIRE(emitter.particles[1].alive == true);
        REQUIRE(emitter.particles[2].alive == true);
    }

    SECTION("updates first_dead_index") {
        REQUIRE(emitter.first_dead_index == 3);
    }

    SECTION("remaining slots are dead") {
        REQUIRE(emitter.particles[3].alive == false);
        REQUIRE(emitter.particles[4].alive == false);
    }
}

TEST_CASE("EmissionShape enum values", "[particle][config]") {
    // Verify enum values for serialization compatibility
    REQUIRE(static_cast<uint8_t>(EmissionShape::POINT) == 0);
    REQUIRE(static_cast<uint8_t>(EmissionShape::CIRCLE) == 1);
    REQUIRE(static_cast<uint8_t>(EmissionShape::CIRCLE_EDGE) == 2);
    REQUIRE(static_cast<uint8_t>(EmissionShape::RECTANGLE) == 3);
    REQUIRE(static_cast<uint8_t>(EmissionShape::RECTANGLE_EDGE) == 4);
    REQUIRE(static_cast<uint8_t>(EmissionShape::LINE) == 5);
}

TEST_CASE("VelocityMode enum values", "[particle][config]") {
    // Verify enum values for serialization compatibility
    REQUIRE(static_cast<uint8_t>(VelocityMode::DIRECTIONAL) == 0);
    REQUIRE(static_cast<uint8_t>(VelocityMode::RADIAL) == 1);
    REQUIRE(static_cast<uint8_t>(VelocityMode::TANGENTIAL) == 2);
    REQUIRE(static_cast<uint8_t>(VelocityMode::RANDOM) == 3);
}

TEST_CASE("Particle pool capacity", "[particle][config]") {
    SECTION("DEFAULT_POOL_SIZE is reasonable") {
        REQUIRE(DEFAULT_POOL_SIZE == 256);
    }

    SECTION("MAX_POOL_SIZE is reasonable") {
        REQUIRE(MAX_POOL_SIZE == 16384);
    }

    SECTION("INVALID_EMITTER_HANDLE is -1") {
        REQUIRE(INVALID_EMITTER_HANDLE == -1);
    }
}
