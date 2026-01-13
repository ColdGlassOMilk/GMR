#ifndef GMR_PARTICLE_TYPES_HPP
#define GMR_PARTICLE_TYPES_HPP

#include "gmr/types.hpp"
#include <cstdint>

namespace gmr {
namespace particle {

// Pool capacity constants
constexpr size_t DEFAULT_POOL_SIZE = 256;
constexpr size_t MAX_POOL_SIZE = 16384;

// Handle type for emitters
using EmitterHandle = int32_t;
constexpr EmitterHandle INVALID_EMITTER_HANDLE = -1;

// Compact per-particle data (~96 bytes)
// Designed for cache-friendly iteration with no pointers or virtual methods
struct ParticleState {
    // Position and motion (24 bytes)
    Vec2 position{0.0f, 0.0f};
    Vec2 velocity{0.0f, 0.0f};
    Vec2 acceleration{0.0f, 0.0f};

    // Lifetime (8 bytes)
    float lifetime{0.0f};       // Time remaining (counts down)
    float max_lifetime{1.0f};   // Initial lifetime (for normalization)

    // Visual properties (20 bytes)
    float size{1.0f};           // Current size (world units)
    float start_size{1.0f};     // Initial size
    float end_size{1.0f};       // Target size at death
    float rotation{0.0f};       // Radians
    float angular_velocity{0.0f}; // Radians per second

    // Color over lifetime (12 bytes)
    Color start_color{255, 255, 255, 255};
    Color end_color{255, 255, 255, 0};
    Color color{255, 255, 255, 255}; // Current interpolated color

    // Flags (4 bytes)
    bool alive{false};
    uint8_t _padding[3]{0, 0, 0};

    // Spritesheet frame (for textured particles)
    uint32_t frame_index{0};    // Index into spritesheet (0 to cols*rows-1)

    // Custom data for extensibility (16 bytes)
    float custom[4]{0.0f, 0.0f, 0.0f, 0.0f};

    ParticleState() = default;
};

} // namespace particle
} // namespace gmr

#endif // GMR_PARTICLE_TYPES_HPP
