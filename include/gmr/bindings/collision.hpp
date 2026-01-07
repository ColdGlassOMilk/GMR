#ifndef GMR_BINDINGS_COLLISION_HPP
#define GMR_BINDINGS_COLLISION_HPP

#include <mruby.h>
#include "gmr/resources/tilemap_manager.hpp"
#include <cmath>

namespace gmr {

// Collision result structure - used by all collision resolution functions
struct CollisionResult {
    float x;           // Resolved X position
    float y;           // Resolved Y position
    float vx;          // Resolved X velocity
    float vy;          // Resolved Y velocity
    bool hit_left;     // Collided with wall on left
    bool hit_right;    // Collided with wall on right
    bool hit_top;      // Collided with ceiling
    bool hit_bottom;   // Landed on ground

    CollisionResult()
        : x(0), y(0), vx(0), vy(0)
        , hit_left(false), hit_right(false)
        , hit_top(false), hit_bottom(false) {}
};

// Collision resolution functions
namespace collision {

// Resolve collision between a rectangle hitbox and a tilemap
// x, y: hitbox position in tilemap local coordinates
// w, h: hitbox dimensions
// vx, vy: current velocity (for directional checks)
// tilemap: the tilemap to check against
inline CollisionResult tilemap_resolve(
    float x, float y, float w, float h,
    float vx, float vy,
    const TilemapData& tilemap)
{
    CollisionResult result;
    result.x = x;
    result.y = y;
    result.vx = vx;
    result.vy = vy;

    float tile_w = static_cast<float>(tilemap.tile_width);
    float tile_h = static_cast<float>(tilemap.tile_height);

    // Vertical collision (check based on movement direction)
    if (vy < 0) {
        // Moving up - check ceiling
        int tx_start = static_cast<int>(std::floor((x + 2) / tile_w));
        int tx_end = static_cast<int>(std::floor((x + w - 3) / tile_w));
        int ty = static_cast<int>(std::floor(y / tile_h));

        for (int tx = tx_start; tx <= tx_end; ++tx) {
            if (tilemap.is_solid(tx, ty)) {
                float tile_bottom = static_cast<float>((ty + 1)) * tile_h;
                if (y < tile_bottom) {
                    result.y = tile_bottom;
                    result.vy = 0;
                    result.hit_top = true;
                    break;
                }
            }
        }
    } else if (vy >= 0) {
        // Moving down or stationary - check ground
        int tx_start = static_cast<int>(std::floor((x + 2) / tile_w));
        int tx_end = static_cast<int>(std::floor((x + w - 3) / tile_w));
        int ty = static_cast<int>(std::floor((y + h) / tile_h));

        for (int tx = tx_start; tx <= tx_end; ++tx) {
            if (tilemap.is_solid(tx, ty)) {
                float tile_top = static_cast<float>(ty) * tile_h;
                if (y + h >= tile_top) {
                    result.y = tile_top - h;
                    result.vy = 0;
                    result.hit_bottom = true;
                    break;
                }
            }
        }
    }

    // Horizontal collision (use resolved Y position)
    float check_y = result.y;
    if (vx < 0) {
        // Moving left - check left wall
        int ty_start = static_cast<int>(std::floor((check_y + 4) / tile_h));
        int ty_end = static_cast<int>(std::floor((check_y + h - 4) / tile_h));
        int tx = static_cast<int>(std::floor(x / tile_w));

        for (int ty = ty_start; ty <= ty_end; ++ty) {
            if (tilemap.is_solid(tx, ty)) {
                float tile_right = static_cast<float>((tx + 1)) * tile_w;
                if (x < tile_right) {
                    result.x = tile_right;
                    result.vx = 0;
                    result.hit_left = true;
                    break;
                }
            }
        }
    } else if (vx > 0) {
        // Moving right - check right wall
        int ty_start = static_cast<int>(std::floor((check_y + 4) / tile_h));
        int ty_end = static_cast<int>(std::floor((check_y + h - 4) / tile_h));
        int tx = static_cast<int>(std::floor((x + w) / tile_w));

        for (int ty = ty_start; ty <= ty_end; ++ty) {
            if (tilemap.is_solid(tx, ty)) {
                float tile_left = static_cast<float>(tx) * tile_w;
                if (x + w > tile_left) {
                    result.x = tile_left - w;
                    result.vx = 0;
                    result.hit_right = true;
                    break;
                }
            }
        }
    }

    return result;
}

} // namespace collision

namespace bindings {

void register_collision(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
