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

    // Transform reference (required)
    TransformHandle transform{INVALID_HANDLE};

    // Visual properties
    Color color{255, 255, 255, 255}; // Tint
    bool flip_x{false};
    bool flip_y{false};

    // Layer and Z-ordering
    uint8_t layer{100};              // Render layer (default: ENTITIES = 100)
    std::optional<float> z;          // If set, overrides draw order for z-depth within layer

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
