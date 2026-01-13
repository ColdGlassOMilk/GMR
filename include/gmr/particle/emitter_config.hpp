#ifndef GMR_EMITTER_CONFIG_HPP
#define GMR_EMITTER_CONFIG_HPP

#include "gmr/types.hpp"
#include "gmr/particle/particle_types.hpp"
#include "gmr/animation/easing.hpp"
#include <string>
#include <cstdlib>
#include <cmath>

namespace gmr {
namespace particle {

// Range for randomized float values
struct FloatRange {
    float min{0.0f};
    float max{0.0f};

    FloatRange() = default;
    FloatRange(float v) : min(v), max(v) {}
    FloatRange(float min_, float max_) : min(min_), max(max_) {}

    // Returns random value in [min, max]
    float random() const {
        if (min >= max) return min;
        float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return min + t * (max - min);
    }

    // Returns interpolated value at t in [0, 1]
    float lerp(float t) const {
        return min + t * (max - min);
    }
};

// Range for Vec2 values
struct Vec2Range {
    Vec2 min{0.0f, 0.0f};
    Vec2 max{0.0f, 0.0f};

    Vec2Range() = default;
    Vec2Range(Vec2 v) : min(v), max(v) {}
    Vec2Range(Vec2 min_, Vec2 max_) : min(min_), max(max_) {}

    Vec2 random() const {
        float tx = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float ty = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return Vec2{
            min.x + tx * (max.x - min.x),
            min.y + ty * (max.y - min.y)
        };
    }
};

// Range for Color values
struct ColorRange {
    Color min{255, 255, 255, 255};
    Color max{255, 255, 255, 255};

    ColorRange() = default;
    ColorRange(Color c) : min(c), max(c) {}
    ColorRange(Color min_, Color max_) : min(min_), max(max_) {}

    Color random() const {
        float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return Color{
            static_cast<uint8_t>(min.r + t * (max.r - min.r)),
            static_cast<uint8_t>(min.g + t * (max.g - min.g)),
            static_cast<uint8_t>(min.b + t * (max.b - min.b)),
            static_cast<uint8_t>(min.a + t * (max.a - min.a))
        };
    }
};

// Emission shape for spawn positions
enum class EmissionShape : uint8_t {
    POINT = 0,      // All particles spawn at emitter position
    CIRCLE,         // Random within circle radius
    CIRCLE_EDGE,    // Only on circle circumference
    RECTANGLE,      // Random within rectangle
    RECTANGLE_EDGE, // Only on rectangle edges
    LINE            // Along a line segment
};

// How velocity direction is determined
enum class VelocityMode : uint8_t {
    DIRECTIONAL = 0,  // Fixed direction + spread angle
    RADIAL,           // Away from emitter center
    TANGENTIAL,       // Perpendicular to radial
    RANDOM            // Completely random direction
};

// Emitter configuration - loaded from JSON or constructed in code
struct EmitterConfig {
    // Identification
    std::string name;

    // Texture
    std::string texture_path;   // Path to particle texture (empty = use primitive)
    Rect source_rect{0, 0, 0, 0}; // Source rect (0 dimensions = full texture)
    TextureHandle texture{INVALID_HANDLE};  // Loaded texture handle (set during load)
    int spritesheet_cols{1};    // Number of columns in spritesheet
    int spritesheet_rows{1};    // Number of rows in spritesheet
    float frame_width{0};       // Pixel width of each frame (auto-detected if 0)
    float frame_height{0};      // Pixel height of each frame (auto-detected if 0)

    // Emission rate
    float spawn_rate{10.0f};    // Particles per second (continuous)
    int burst_count{0};         // Particles per burst (0 = no burst)
    float burst_interval{0.0f}; // Seconds between bursts (0 = burst on start only)

    // Pool size
    size_t max_particles{DEFAULT_POOL_SIZE};

    // Lifetime
    FloatRange lifetime{1.0f, 2.0f};

    // Spawn position shape
    EmissionShape shape{EmissionShape::POINT};
    float shape_radius{0.0f};   // For CIRCLE shapes
    Vec2 shape_size{0.0f, 0.0f}; // For RECTANGLE/LINE shapes

    // Initial velocity
    VelocityMode velocity_mode{VelocityMode::DIRECTIONAL};
    float direction{0.0f};      // Base direction in radians (0 = right)
    FloatRange spread{0.0f};    // Angular spread in radians
    FloatRange speed{1.0f, 2.0f};

    // Acceleration/forces
    Vec2 gravity{0.0f, 0.0f};   // Constant acceleration
    float radial_accel{0.0f};   // Acceleration toward/away from emitter
    float tangent_accel{0.0f};  // Tangential acceleration
    float drag{0.0f};           // Velocity damping (0-1 per second)

    // Size over lifetime
    FloatRange start_size{0.5f, 1.0f};
    FloatRange end_size{0.0f, 0.5f};
    animation::EasingType size_easing{animation::EasingType::LINEAR};

    // Rotation
    FloatRange start_rotation{0.0f};
    FloatRange angular_velocity{0.0f};

    // Color over lifetime
    ColorRange start_color{Color{255, 255, 255, 255}};
    ColorRange end_color{Color{255, 255, 255, 0}};
    animation::EasingType color_easing{animation::EasingType::LINEAR};

    // Rendering
    uint8_t layer{150};         // RenderLayer::EFFECTS
    float z{0.0f};              // Z-depth within layer
    bool additive_blend{false}; // Use additive blending

    // World space behavior
    bool world_space{true};     // true = particles ignore emitter movement
                                // false = particles move with emitter

    // Time scaling
    bool scaled{true};          // Respect Time.scale

    EmitterConfig() = default;
};

} // namespace particle
} // namespace gmr

#endif // GMR_EMITTER_CONFIG_HPP
