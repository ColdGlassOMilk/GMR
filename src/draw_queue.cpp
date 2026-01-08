#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/camera.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/resources/tilemap_manager.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace gmr {

// Convert our Color to raylib Color
static ::Color to_raylib(const Color& c) {
    return ::Color{c.r, c.g, c.b, c.a};
}

// Convert DrawColor to raylib Color
static ::Color to_raylib(const DrawColor& c) {
    return ::Color{c.r, c.g, c.b, c.a};
}

DrawQueue& DrawQueue::instance() {
    static DrawQueue instance;
    return instance;
}

void DrawQueue::begin_frame() {
    next_draw_order_ = 0;
    camera_stack_.clear();
    active_camera_ = INVALID_CAMERA_HANDLE;
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

void DrawQueue::queue_camera_begin(CameraHandle handle) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::CAMERA_BEGIN;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.camera_handle = handle;
    commands_.push_back(cmd);

    camera_stack_.push_back(handle);
    next_draw_order_++;
}

void DrawQueue::queue_camera_end() {
    if (camera_stack_.empty()) return;  // Safety check

    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::CAMERA_END;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.camera_handle = camera_stack_.back();
    commands_.push_back(cmd);

    camera_stack_.pop_back();
    next_draw_order_++;
}

void DrawQueue::queue_tilemap(TilemapHandle handle, float offset_x, float offset_y, const DrawColor& tint) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::TILEMAP;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.tilemap.handle = handle;
    cmd.tilemap.offset_x = offset_x;
    cmd.tilemap.offset_y = offset_y;
    cmd.tilemap.tint = tint;
    cmd.tilemap.use_region = false;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_tilemap_region(TilemapHandle handle, float offset_x, float offset_y,
                                      int32_t region_x, int32_t region_y, int32_t region_w, int32_t region_h,
                                      const DrawColor& tint) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::TILEMAP;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.tilemap.handle = handle;
    cmd.tilemap.offset_x = offset_x;
    cmd.tilemap.offset_y = offset_y;
    cmd.tilemap.tint = tint;
    cmd.tilemap.use_region = true;
    cmd.tilemap.region_x = region_x;
    cmd.tilemap.region_y = region_y;
    cmd.tilemap.region_w = region_w;
    cmd.tilemap.region_h = region_h;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_rect(float x, float y, float w, float h, const DrawColor& color, bool filled) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::RECT;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.rect.x = x;
    cmd.rect.y = y;
    cmd.rect.w = w;
    cmd.rect.h = h;
    cmd.rect.color = color;
    cmd.rect.filled = filled;
    cmd.rect.rotation = 0;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_rect_rotated(float x, float y, float w, float h, float rotation, const DrawColor& color) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::RECT;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.rect.x = x;
    cmd.rect.y = y;
    cmd.rect.w = w;
    cmd.rect.h = h;
    cmd.rect.color = color;
    cmd.rect.filled = true;
    cmd.rect.rotation = rotation;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_circle(float x, float y, float radius, const DrawColor& color, bool filled) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::CIRCLE;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.circle.x = x;
    cmd.circle.y = y;
    cmd.circle.radius = radius;
    cmd.circle.color = color;
    cmd.circle.filled = filled;
    cmd.circle.gradient = false;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_circle_gradient(float x, float y, float radius, const DrawColor& inner, const DrawColor& outer) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::CIRCLE;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.circle.x = x;
    cmd.circle.y = y;
    cmd.circle.radius = radius;
    cmd.circle.color = inner;
    cmd.circle.color2 = outer;
    cmd.circle.filled = true;
    cmd.circle.gradient = true;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_line(float x1, float y1, float x2, float y2, const DrawColor& color, float thickness) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::LINE;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.line.x1 = x1;
    cmd.line.y1 = y1;
    cmd.line.x2 = x2;
    cmd.line.y2 = y2;
    cmd.line.color = color;
    cmd.line.thickness = thickness;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_triangle(float x1, float y1, float x2, float y2, float x3, float y3,
                                const DrawColor& color, bool filled) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::TRIANGLE;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.triangle.x1 = x1;
    cmd.triangle.y1 = y1;
    cmd.triangle.x2 = x2;
    cmd.triangle.y2 = y2;
    cmd.triangle.x3 = x3;
    cmd.triangle.y3 = y3;
    cmd.triangle.color = color;
    cmd.triangle.filled = filled;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::queue_text(float x, float y, const std::string& content, int font_size, const DrawColor& color) {
    float z_value = DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_);

    DrawCommand cmd;
    cmd.type = DrawCommand::Type::TEXT;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_;
    cmd.text.x = x;
    cmd.text.y = y;
    cmd.text.font_size = font_size;
    cmd.text.color = color;
    cmd.text.content = content;
    commands_.push_back(cmd);

    next_draw_order_++;
}

void DrawQueue::apply_camera_begin(CameraHandle handle) {
    // End any existing camera mode first
    if (active_camera_ != INVALID_CAMERA_HANDLE) {
        EndMode2D();
    }

    auto* cam = CameraManager::instance().get(handle);
    if (cam) {
        ::Camera2D raylib_cam = {};
        // Round camera target to nearest pixel for crisp pixel art rendering
        // This prevents subpixel jitter while keeping smooth camera movement
        raylib_cam.target = {std::round(cam->target.x), std::round(cam->target.y)};
        raylib_cam.offset = {cam->offset.x + cam->shake_offset.x,
                             cam->offset.y + cam->shake_offset.y};
        raylib_cam.rotation = cam->rotation;
        raylib_cam.zoom = cam->zoom;

        BeginMode2D(raylib_cam);
        active_camera_ = handle;
    }
}

void DrawQueue::apply_camera_end() {
    if (active_camera_ != INVALID_CAMERA_HANDLE) {
        EndMode2D();
        active_camera_ = INVALID_CAMERA_HANDLE;
    }
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
            case DrawCommand::Type::CAMERA_BEGIN:
                apply_camera_begin(cmd.camera_handle);
                break;
            case DrawCommand::Type::CAMERA_END:
                apply_camera_end();
                break;
            case DrawCommand::Type::SPRITE:
                draw_sprite(cmd);
                break;
            case DrawCommand::Type::TILEMAP:
                draw_tilemap(cmd);
                break;
            case DrawCommand::Type::RECT:
                draw_rect(cmd);
                break;
            case DrawCommand::Type::CIRCLE:
                draw_circle(cmd);
                break;
            case DrawCommand::Type::LINE:
                draw_line(cmd);
                break;
            case DrawCommand::Type::TRIANGLE:
                draw_triangle(cmd);
                break;
            case DrawCommand::Type::TEXT:
                draw_text(cmd);
                break;
        }
    }

    // Safety: close any unclosed cameras
    while (active_camera_ != INVALID_CAMERA_HANDLE) {
        apply_camera_end();
    }

    // Clear for next frame
    commands_.clear();
    camera_stack_.clear();
}

void DrawQueue::clear() {
    commands_.clear();
    next_draw_order_ = 0;
    camera_stack_.clear();
    active_camera_ = INVALID_CAMERA_HANDLE;
}

void DrawQueue::draw_sprite(const DrawCommand& cmd) {
    auto* sprite = SpriteManager::instance().get(cmd.sprite_handle);
    if (!sprite) return;

    auto* texture = TextureManager::instance().get(sprite->texture);
    if (!texture) return;

    // Get transform (required)
    auto* transform = TransformManager::instance().get(sprite->transform);
    if (!transform) return;

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

    // Get world transform from Transform2D (uses cached values - fast!)
    Vec2 world_pos = TransformManager::instance().get_world_position(sprite->transform);
    float world_rot = TransformManager::instance().get_world_rotation(sprite->transform);
    Vec2 world_scale = TransformManager::instance().get_world_scale(sprite->transform);

    // Calculate destination rectangle
    // Note: Camera target is rounded to pixels, so no need to round sprite positions
    float dest_w = std::abs(source.width) * world_scale.x;
    float dest_h = std::abs(source.height) * world_scale.y;
    Rectangle dest = {world_pos.x, world_pos.y, dest_w, dest_h};

    // Origin for rotation (in dest space) - from transform's origin
    Vector2 origin = {
        transform->origin.x * world_scale.x,
        transform->origin.y * world_scale.y
    };

    // Convert radians to degrees for raylib
    float rotation_degrees = world_rot * (180.0f / 3.14159265358979323846f);

    // Draw with tint
    DrawTexturePro(*texture, source, dest, origin, rotation_degrees, to_raylib(sprite->color));
}

void DrawQueue::draw_tilemap(const DrawCommand& cmd) {
    auto* tilemap = TilemapManager::instance().get(cmd.tilemap.handle);
    if (!tilemap) return;

    auto* texture = TextureManager::instance().get(tilemap->tileset);
    if (!texture) return;

    ::Color tint = to_raylib(cmd.tilemap.tint);

    // Calculate tileset dimensions (tiles per row in tileset)
    int tileset_cols = texture->width / tilemap->tile_width;

    // Determine iteration bounds
    int32_t start_x = 0, start_y = 0;
    int32_t end_x = tilemap->width, end_y = tilemap->height;

    if (cmd.tilemap.use_region) {
        start_x = std::max(0, cmd.tilemap.region_x);
        start_y = std::max(0, cmd.tilemap.region_y);
        end_x = std::min(tilemap->width, cmd.tilemap.region_x + cmd.tilemap.region_w);
        end_y = std::min(tilemap->height, cmd.tilemap.region_y + cmd.tilemap.region_h);
    }

    // Draw each tile
    for (int32_t ty = start_y; ty < end_y; ++ty) {
        for (int32_t tx = start_x; tx < end_x; ++tx) {
            int32_t tile_index = tilemap->get(tx, ty);
            if (tile_index < 0) continue;  // Skip empty tiles

            // Calculate source rect from tileset
            int tileset_x = (tile_index % tileset_cols) * tilemap->tile_width;
            int tileset_y = (tile_index / tileset_cols) * tilemap->tile_height;

            Rectangle source = {
                static_cast<float>(tileset_x),
                static_cast<float>(tileset_y),
                static_cast<float>(tilemap->tile_width),
                static_cast<float>(tilemap->tile_height)
            };

            // Calculate dest position (adjust for region offset if using region)
            float dest_x = cmd.tilemap.offset_x + (tx - start_x) * tilemap->tile_width;
            float dest_y = cmd.tilemap.offset_y + (ty - start_y) * tilemap->tile_height;

            // When using full tilemap (no region), use absolute tile position
            if (!cmd.tilemap.use_region) {
                dest_x = cmd.tilemap.offset_x + tx * tilemap->tile_width;
                dest_y = cmd.tilemap.offset_y + ty * tilemap->tile_height;
            }

            Rectangle dest = {
                dest_x,
                dest_y,
                static_cast<float>(tilemap->tile_width),
                static_cast<float>(tilemap->tile_height)
            };

            DrawTexturePro(*texture, source, dest, Vector2{0, 0}, 0, tint);
        }
    }
}

void DrawQueue::draw_rect(const DrawCommand& cmd) {
    ::Color color = to_raylib(cmd.rect.color);

    if (cmd.rect.rotation != 0) {
        // Rotated rectangle
        Rectangle rect = {cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h};
        Vector2 origin = {cmd.rect.w / 2, cmd.rect.h / 2};
        DrawRectanglePro(rect, origin, cmd.rect.rotation, color);
    } else if (cmd.rect.filled) {
        DrawRectangle(
            static_cast<int>(cmd.rect.x),
            static_cast<int>(cmd.rect.y),
            static_cast<int>(cmd.rect.w),
            static_cast<int>(cmd.rect.h),
            color
        );
    } else {
        DrawRectangleLines(
            static_cast<int>(cmd.rect.x),
            static_cast<int>(cmd.rect.y),
            static_cast<int>(cmd.rect.w),
            static_cast<int>(cmd.rect.h),
            color
        );
    }
}

void DrawQueue::draw_circle(const DrawCommand& cmd) {
    if (cmd.circle.gradient) {
        DrawCircleGradient(
            static_cast<int>(cmd.circle.x),
            static_cast<int>(cmd.circle.y),
            cmd.circle.radius,
            to_raylib(cmd.circle.color),
            to_raylib(cmd.circle.color2)
        );
    } else if (cmd.circle.filled) {
        DrawCircle(
            static_cast<int>(cmd.circle.x),
            static_cast<int>(cmd.circle.y),
            cmd.circle.radius,
            to_raylib(cmd.circle.color)
        );
    } else {
        DrawCircleLines(
            static_cast<int>(cmd.circle.x),
            static_cast<int>(cmd.circle.y),
            cmd.circle.radius,
            to_raylib(cmd.circle.color)
        );
    }
}

void DrawQueue::draw_line(const DrawCommand& cmd) {
    if (cmd.line.thickness > 1.0f) {
        DrawLineEx(
            Vector2{cmd.line.x1, cmd.line.y1},
            Vector2{cmd.line.x2, cmd.line.y2},
            cmd.line.thickness,
            to_raylib(cmd.line.color)
        );
    } else {
        DrawLine(
            static_cast<int>(cmd.line.x1),
            static_cast<int>(cmd.line.y1),
            static_cast<int>(cmd.line.x2),
            static_cast<int>(cmd.line.y2),
            to_raylib(cmd.line.color)
        );
    }
}

void DrawQueue::draw_triangle(const DrawCommand& cmd) {
    Vector2 v1 = {cmd.triangle.x1, cmd.triangle.y1};
    Vector2 v2 = {cmd.triangle.x2, cmd.triangle.y2};
    Vector2 v3 = {cmd.triangle.x3, cmd.triangle.y3};
    ::Color color = to_raylib(cmd.triangle.color);

    if (cmd.triangle.filled) {
        DrawTriangle(v1, v2, v3, color);
    } else {
        DrawTriangleLines(v1, v2, v3, color);
    }
}

void DrawQueue::draw_text(const DrawCommand& cmd) {
    DrawText(
        cmd.text.content.c_str(),
        static_cast<int>(cmd.text.x),
        static_cast<int>(cmd.text.y),
        cmd.text.font_size,
        to_raylib(cmd.text.color)
    );
}

} // namespace gmr
