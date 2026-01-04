#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace gmr {

// Convert our Color to raylib Color
static ::Color to_raylib(const Color& c) {
    return ::Color{c.r, c.g, c.b, c.a};
}

DrawQueue& DrawQueue::instance() {
    static DrawQueue instance;
    return instance;
}

void DrawQueue::begin_frame() {
    next_draw_order_ = 0;
}

void DrawQueue::queue_sprite(SpriteHandle handle) {
    auto* sprite = SpriteManager::instance().get(handle);
    if (!sprite) return;

    float z_value;
    if (sprite->z.has_value()) {
        // Use explicit z
        z_value = sprite->z.value();
    } else {
        // Use draw order - add to a large base to ensure draw-order sprites
        // sort after explicit z sprites (unless explicit z is very high)
        z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);
    }

    commands_.emplace_back(
        DrawCommand::Type::SPRITE,
        z_value,
        next_draw_order_,
        handle
    );

    next_draw_order_++;
}

void DrawQueue::flush() {
    if (commands_.empty()) return;

    // Stable sort by z value
    // Sprites with same z maintain their draw order (first drawn = first rendered)
    std::stable_sort(commands_.begin(), commands_.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            return a.z < b.z;
        });

    // Execute all draw commands
    for (const auto& cmd : commands_) {
        switch (cmd.type) {
            case DrawCommand::Type::SPRITE:
                draw_sprite(cmd);
                break;
        }
    }

    // Clear for next frame
    commands_.clear();
}

void DrawQueue::clear() {
    commands_.clear();
    next_draw_order_ = 0;
}

void DrawQueue::draw_sprite(const DrawCommand& cmd) {
    auto* sprite = SpriteManager::instance().get(cmd.sprite_handle);
    if (!sprite) return;

    auto* texture = TextureManager::instance().get(sprite->texture);
    if (!texture) return;

    // Determine source rectangle
    Rectangle source;
    if (sprite->use_source_rect) {
        source = {
            sprite->source_rect.x,
            sprite->source_rect.y,
            sprite->source_rect.width,
            sprite->source_rect.height
        };
    } else {
        source = {
            0, 0,
            static_cast<float>(texture->width),
            static_cast<float>(texture->height)
        };
    }

    // Apply flip by negating source dimensions
    if (sprite->flip_x) source.width = -source.width;
    if (sprite->flip_y) source.height = -source.height;

    // Calculate world position/rotation/scale if parented
    Vec2 world_pos = sprite->position;
    float world_rot = sprite->rotation;
    Vec2 world_scale = sprite->scale;

    if (sprite->parent_transform != INVALID_HANDLE) {
        const Matrix2D& parent_mat = TransformManager::instance()
            .get_world_matrix(sprite->parent_transform);

        // Transform position by parent matrix
        world_pos = parent_mat.transform_point(sprite->position);

        // For full matrix decomposition we'd need more complex math
        // For now, extract rotation from parent matrix (assuming uniform scale)
        float parent_rot = std::atan2(parent_mat.c, parent_mat.a);
        world_rot = parent_rot + sprite->rotation;

        // Extract scale from parent (assuming no shear)
        float parent_scale_x = std::sqrt(parent_mat.a * parent_mat.a + parent_mat.c * parent_mat.c);
        float parent_scale_y = std::sqrt(parent_mat.b * parent_mat.b + parent_mat.d * parent_mat.d);
        world_scale.x = parent_scale_x * sprite->scale.x;
        world_scale.y = parent_scale_y * sprite->scale.y;
    }

    // Calculate destination rectangle
    float dest_w = std::abs(source.width) * world_scale.x;
    float dest_h = std::abs(source.height) * world_scale.y;
    Rectangle dest = {world_pos.x, world_pos.y, dest_w, dest_h};

    // Origin for rotation (in dest space)
    Vector2 origin = {
        sprite->origin.x * world_scale.x,
        sprite->origin.y * world_scale.y
    };

    // Convert radians to degrees for raylib
    float rotation_degrees = world_rot * (180.0f / 3.14159265358979323846f);

    // Draw with tint
    DrawTexturePro(*texture, source, dest, origin, rotation_degrees, to_raylib(sprite->color));
}

} // namespace gmr
