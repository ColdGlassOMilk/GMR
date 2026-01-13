#ifndef GMR_EMITTER_STATE_HPP
#define GMR_EMITTER_STATE_HPP

#include "gmr/particle/particle_types.hpp"
#include "gmr/particle/emitter_config.hpp"
#include "gmr/types.hpp"
#include <mruby.h>
#include <vector>

namespace gmr {
namespace particle {

// Runtime state for a particle emitter instance
struct EmitterState {
    // Configuration (shared reference, not owned)
    const EmitterConfig* config{nullptr};

    // Position/attachment
    Vec2 position{0.0f, 0.0f};
    TransformHandle attached_transform{INVALID_HANDLE};
    Vec2 last_position{0.0f, 0.0f}; // For interpolating spawn positions

    // Particle pool (owned, preallocated)
    std::vector<ParticleState> particles;
    size_t alive_count{0};
    size_t first_dead_index{0}; // Optimization: track where dead particles start

    // Emission state
    float spawn_accumulator{0.0f}; // Fractional particles to spawn
    float burst_timer{0.0f};
    bool emitting{false};       // Is actively spawning
    bool active{true};          // Is being updated (false = marked for removal)
    bool one_shot{false};       // Stop emitting after initial burst
    bool drawn_this_frame{false}; // Track if Ruby called draw() this frame

    // Ruby callbacks
    mrb_value on_complete{mrb_nil_value()}; // Called when all particles die (one-shot)
    mrb_value ruby_emitter_obj{mrb_nil_value()}; // GC reference

    EmitterState() = default;

    // Initialize with config (allocates particle pool)
    void init(const EmitterConfig* cfg) {
        config = cfg;
        if (cfg) {
            particles.resize(cfg->max_particles);
            for (auto& p : particles) {
                p.alive = false;
            }
        }
        alive_count = 0;
        first_dead_index = 0;
        spawn_accumulator = 0.0f;
        burst_timer = 0.0f;
        emitting = false;
        active = true;
        one_shot = false;
    }

    // Reset emitter to initial state (clears all particles)
    void reset() {
        for (auto& p : particles) {
            p.alive = false;
        }
        alive_count = 0;
        first_dead_index = 0;
        spawn_accumulator = 0.0f;
        burst_timer = 0.0f;
    }

    // Find and return a dead particle slot, or nullptr if pool is full
    ParticleState* spawn_particle() {
        // Fast path: check from first_dead_index
        for (size_t i = first_dead_index; i < particles.size(); ++i) {
            if (!particles[i].alive) {
                first_dead_index = i + 1;
                particles[i].alive = true;
                alive_count++;
                return &particles[i];
            }
        }

        // Slow path: wrap around and search from beginning
        for (size_t i = 0; i < first_dead_index; ++i) {
            if (!particles[i].alive) {
                particles[i].alive = true;
                alive_count++;
                return &particles[i];
            }
        }

        // Pool exhausted
        return nullptr;
    }

    // Compact particles to remove gaps (optional, for ordered effects)
    void compact() {
        size_t write_idx = 0;
        for (size_t i = 0; i < particles.size(); ++i) {
            if (particles[i].alive) {
                if (i != write_idx) {
                    particles[write_idx] = particles[i];
                    particles[i].alive = false;
                }
                write_idx++;
            }
        }
        first_dead_index = write_idx;
    }
};

} // namespace particle
} // namespace gmr

#endif // GMR_EMITTER_STATE_HPP
