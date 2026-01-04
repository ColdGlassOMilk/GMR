#ifndef GMR_SPRITE_HPP
#define GMR_SPRITE_HPP

#include "gmr/types.hpp"
#include "gmr/transform.hpp"
#include <unordered_map>
#include <optional>

namespace gmr {

// Sprite state - represents a textured quad with transform
struct SpriteState {
    TextureHandle texture{INVALID_HANDLE};
    Rect source_rect{0, 0, 0, 0};
    bool use_source_rect{false};

    // Embedded transform (owned by sprite, not a handle)
    Vec2 position{0.0f, 0.0f};
    float rotation{0.0f};            // Radians internally
    Vec2 scale{1.0f, 1.0f};
    Vec2 origin{0.0f, 0.0f};         // Pivot point

    // Optional parent transform for hierarchy
    TransformHandle parent_transform{INVALID_HANDLE};

    // Visual properties
    Color color{255, 255, 255, 255}; // Tint
    bool flip_x{false};
    bool flip_y{false};

    // Z-ordering: optional explicit z, or use draw order
    std::optional<float> z;          // If set, overrides draw order

    SpriteState() = default;
};

// Singleton Sprite manager
class SpriteManager {
public:
    static SpriteManager& instance();

    // Lifecycle
    SpriteHandle create();
    void destroy(SpriteHandle handle);
    SpriteState* get(SpriteHandle handle);
    bool valid(SpriteHandle handle) const;

    // Clear all (for cleanup/reload)
    void clear();

    // Debug info
    size_t count() const { return sprites_.size(); }

private:
    SpriteManager() = default;

    std::unordered_map<SpriteHandle, SpriteState> sprites_;
    SpriteHandle next_id_{0};
};

} // namespace gmr

#endif
