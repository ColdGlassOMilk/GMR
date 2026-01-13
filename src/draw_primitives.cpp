#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/camera.hpp"
#include "gmr/state.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/resources/tilemap_manager.hpp"
#include "gmr/resources/font_manager.hpp"
#include "raylib.h"
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
        // Apply subpixel inset to prevent texture bleeding from adjacent frames.
        // When sprites are rendered at non-integer screen positions (due to world-space
        // coordinates and camera scaling), GPU texture sampling can bleed pixels from
        // adjacent frames in the spritesheet. A half-texel inset prevents this.
        constexpr float TEXEL_INSET = 0.5f;
        source = {
            sprite->source_rect.x + TEXEL_INSET,
            sprite->source_rect.y + TEXEL_INSET,
            sprite->source_rect.width - TEXEL_INSET * 2.0f,
            sprite->source_rect.height - TEXEL_INSET * 2.0f
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

    // ASSET_PPU: The pixels-per-unit at which all assets were designed.
    // This decouples sprite world size from camera settings.
    // A 24px sprite = 1 world unit, matching tilemap (1 tile = 1 world unit).
    // Camera.pixels_per_unit only affects view scale, NOT sprite intrinsic size.
    constexpr float ASSET_PPU = 24.0f;

    // Calculate destination rectangle in world units using ORIGINAL sprite dimensions
    // (source.width/height have texel inset applied, so use original rect values)
    float original_width = sprite->use_source_rect ? sprite->source_rect.width : static_cast<float>(texture->width);
    float original_height = sprite->use_source_rect ? sprite->source_rect.height : static_cast<float>(texture->height);
    float dest_w = std::abs(original_width) * world_scale.x / ASSET_PPU;
    float dest_h = std::abs(original_height) * world_scale.y / ASSET_PPU;

    // Apply parallax scrolling if rendering within a camera
    if (active_camera_ != INVALID_CAMERA_HANDLE) {
        auto* cam = CameraManager::instance().get(active_camera_);
        if (cam) {
            // parallax=1.0: moves with world (default)
            // parallax=0.0: fixed to screen (stationary relative to camera)
            // parallax=0.5: moves at half camera speed (distant background)
            float parallax = transform->parallax;
            if (parallax != 1.0f) {
                // Add offset to counteract camera movement proportionally
                // When camera moves by X, parallax object appears to move by X*parallax
                // We achieve this by offsetting position: pos += target * (1 - parallax)
                float offset_factor = 1.0f - parallax;
                world_pos.x += cam->target.x * offset_factor;
                world_pos.y += cam->target.y * offset_factor;
            }
        }
    }

    Rectangle dest = {world_pos.x, world_pos.y, dest_w, dest_h};

    // CRITICAL FIX: Transform matrix already bakes origin offset into world_pos
    // (see transform.cpp:26-27: tx = x - (origin_x * m.a + origin_y * m.b))
    // This means world_pos is the top-left corner AFTER accounting for the rotated origin.
    // DrawTexturePro should rotate the texture around the top-left (origin={0,0}),
    // but we still need to pass the rotation angle to visually rotate the texture.
    Vector2 origin = {0, 0};
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

    // ASSET_PPU: Must match the constant in draw_sprite() for consistency.
    // All assets designed at 24 PPU: a 24px tile = 1 world unit.
    constexpr float ASSET_PPU = 24.0f;

    // World-space tile rendering:
    // - tile_width/tile_height are in PIXELS (for texture sampling)
    // - Tile world size = tile_pixel_size / ASSET_PPU
    // - The camera's effective_scale converts world units to screen pixels
    float tile_world_size = static_cast<float>(tilemap->tile_width) / ASSET_PPU;

    // Draw each tile
    for (int32_t ty = start_y; ty < end_y; ++ty) {
        for (int32_t tx = start_x; tx < end_x; ++tx) {
            int32_t tile_index = tilemap->get(tx, ty);
            if (tile_index < 0) continue;  // Skip empty tiles

            // Calculate source rect from tileset (in pixels for texture sampling)
            int tileset_x = (tile_index % tileset_cols) * tilemap->tile_width;
            int tileset_y = (tile_index / tileset_cols) * tilemap->tile_height;

            Rectangle source = {
                static_cast<float>(tileset_x),
                static_cast<float>(tileset_y),
                static_cast<float>(tilemap->tile_width),
                static_cast<float>(tilemap->tile_height)
            };

            // Calculate dest position in world units
            // offset_x/y are in world units, tile positions are grid indices (0, 1, 2...)
            float dest_x, dest_y;
            if (cmd.tilemap.use_region) {
                dest_x = cmd.tilemap.offset_x + (tx - start_x) * tile_world_size;
                dest_y = cmd.tilemap.offset_y + (ty - start_y) * tile_world_size;
            } else {
                dest_x = cmd.tilemap.offset_x + tx * tile_world_size;
                dest_y = cmd.tilemap.offset_y + ty * tile_world_size;
            }

            // Destination size in world units (1 tile = 1 world unit)
            // Add a tiny overlap to prevent gaps from floating-point rounding errors
            // The overlap is 1 pixel in world units (1/effective_scale), ensuring
            // adjacent tiles touch without visible gaps
            float overlap = 0.01f;  // Small world-unit overlap to cover rounding errors

            Rectangle dest = {
                dest_x,
                dest_y,
                tile_world_size + overlap,
                tile_world_size + overlap
            };

            DrawTexturePro(*texture, source, dest, Vector2{0, 0}, 0, tint);
        }
    }
}

void DrawQueue::draw_rect(const DrawCommand& cmd) {
    ::Color color = to_raylib(cmd.rect.color);

    if (cmd.rect.transform != INVALID_HANDLE) {
        // TRANSFORM-BASED RENDERING
        auto* transform = TransformManager::instance().get(cmd.rect.transform);
        if (!transform) return;

        Vec2 world_pos = TransformManager::instance().get_world_position(cmd.rect.transform);
        float world_rot = TransformManager::instance().get_world_rotation(cmd.rect.transform);
        Vec2 world_scale = TransformManager::instance().get_world_scale(cmd.rect.transform);

        float w = cmd.rect.width * std::abs(world_scale.x);
        float h = cmd.rect.height * std::abs(world_scale.y);

        if (world_rot != 0.0f) {
            // Rotated rect - use DrawRectanglePro
            // Transform matrix already bakes origin offset into world_pos
            // (same as sprites - see draw_sprite comments)
            Rectangle rect = {world_pos.x, world_pos.y, w, h};
            Vector2 origin = {0, 0};
            float rotation_degrees = world_rot * (180.0f / 3.14159265358979323846f);
            DrawRectanglePro(rect, origin, rotation_degrees, color);
        } else {
            // Fast path: no rotation/origin
            if (cmd.rect.filled) {
                DrawRectangle(
                    static_cast<int>(world_pos.x),
                    static_cast<int>(world_pos.y),
                    static_cast<int>(w),
                    static_cast<int>(h),
                    color
                );
            } else {
                DrawRectangleLines(
                    static_cast<int>(world_pos.x),
                    static_cast<int>(world_pos.y),
                    static_cast<int>(w),
                    static_cast<int>(h),
                    color
                );
            }
        }
    } else {
        // SCREEN-SPACE RENDERING (backward compat)
        // In surface mode, screen-space primitives need to bypass the surface camera transform
        suspend_surface_camera();

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

        restore_surface_camera();
    }
}

void DrawQueue::draw_circle(const DrawCommand& cmd) {
    if (cmd.circle.transform != INVALID_HANDLE) {
        // TRANSFORM-BASED RENDERING
        auto* transform = TransformManager::instance().get(cmd.circle.transform);
        if (!transform) return;

        Vec2 world_pos = TransformManager::instance().get_world_position(cmd.circle.transform);
        Vec2 world_scale = TransformManager::instance().get_world_scale(cmd.circle.transform);

        // Use average scale for radius (circles don't support non-uniform scaling)
        float scale = (std::abs(world_scale.x) + std::abs(world_scale.y)) * 0.5f;
        float scaled_radius = cmd.circle.radius * scale;

        ::Color color = to_raylib(cmd.circle.color);

        if (cmd.circle.gradient) {
            ::Color color2 = to_raylib(cmd.circle.color2);
            DrawCircleGradient(
                static_cast<int>(world_pos.x),
                static_cast<int>(world_pos.y),
                scaled_radius,
                color,
                color2
            );
        } else if (cmd.circle.filled) {
            DrawCircle(
                static_cast<int>(world_pos.x),
                static_cast<int>(world_pos.y),
                scaled_radius,
                color
            );
        } else {
            DrawCircleLines(
                static_cast<int>(world_pos.x),
                static_cast<int>(world_pos.y),
                scaled_radius,
                color
            );
        }
    } else {
        // SCREEN-SPACE RENDERING (backward compat)
        // In surface mode, screen-space primitives need to bypass the surface camera transform
        suspend_surface_camera();

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

        restore_surface_camera();
    }
}

void DrawQueue::draw_line(const DrawCommand& cmd) {
    ::Color color = to_raylib(cmd.line.color);

    if (cmd.line.transform != INVALID_HANDLE) {
        // TRANSFORM-BASED RENDERING
        auto* transform = TransformManager::instance().get(cmd.line.transform);
        if (!transform) return;

        // Get world matrix to transform local-space endpoints
        Vec2 world_pos = TransformManager::instance().get_world_position(cmd.line.transform);
        float world_rot = TransformManager::instance().get_world_rotation(cmd.line.transform);
        Vec2 world_scale = TransformManager::instance().get_world_scale(cmd.line.transform);

        // Transform local-space endpoints to world space
        // Apply rotation and scale, then translate
        float cos_r = std::cos(world_rot);
        float sin_r = std::sin(world_rot);

        // Transform point 1
        float local_x1 = cmd.line.x1 - transform->origin.x;
        float local_y1 = cmd.line.y1 - transform->origin.y;
        float world_x1 = world_pos.x + (local_x1 * cos_r - local_y1 * sin_r) * world_scale.x;
        float world_y1 = world_pos.y + (local_x1 * sin_r + local_y1 * cos_r) * world_scale.y;

        // Transform point 2
        float local_x2 = cmd.line.x2 - transform->origin.x;
        float local_y2 = cmd.line.y2 - transform->origin.y;
        float world_x2 = world_pos.x + (local_x2 * cos_r - local_y2 * sin_r) * world_scale.x;
        float world_y2 = world_pos.y + (local_x2 * sin_r + local_y2 * cos_r) * world_scale.y;

        if (cmd.line.thickness > 1.0f) {
            DrawLineEx(
                Vector2{world_x1, world_y1},
                Vector2{world_x2, world_y2},
                cmd.line.thickness,
                color
            );
        } else {
            DrawLine(
                static_cast<int>(world_x1),
                static_cast<int>(world_y1),
                static_cast<int>(world_x2),
                static_cast<int>(world_y2),
                color
            );
        }
    } else {
        // SCREEN-SPACE RENDERING (backward compat)
        // In surface mode, screen-space primitives need to bypass the surface camera transform
        suspend_surface_camera();

        if (cmd.line.thickness > 1.0f) {
            DrawLineEx(
                Vector2{cmd.line.x1, cmd.line.y1},
                Vector2{cmd.line.x2, cmd.line.y2},
                cmd.line.thickness,
                color
            );
        } else {
            DrawLine(
                static_cast<int>(cmd.line.x1),
                static_cast<int>(cmd.line.y1),
                static_cast<int>(cmd.line.x2),
                static_cast<int>(cmd.line.y2),
                color
            );
        }

        restore_surface_camera();
    }
}

void DrawQueue::draw_triangle(const DrawCommand& cmd) {
    ::Color color = to_raylib(cmd.triangle.color);

    if (cmd.triangle.transform != INVALID_HANDLE) {
        // TRANSFORM-BASED RENDERING
        auto* transform = TransformManager::instance().get(cmd.triangle.transform);
        if (!transform) return;

        // Get world matrix to transform local-space vertices
        Vec2 world_pos = TransformManager::instance().get_world_position(cmd.triangle.transform);
        float world_rot = TransformManager::instance().get_world_rotation(cmd.triangle.transform);
        Vec2 world_scale = TransformManager::instance().get_world_scale(cmd.triangle.transform);

        // Transform local-space vertices to world space
        float cos_r = std::cos(world_rot);
        float sin_r = std::sin(world_rot);

        // Transform vertex 1
        float local_x1 = cmd.triangle.x1 - transform->origin.x;
        float local_y1 = cmd.triangle.y1 - transform->origin.y;
        float world_x1 = world_pos.x + (local_x1 * cos_r - local_y1 * sin_r) * world_scale.x;
        float world_y1 = world_pos.y + (local_x1 * sin_r + local_y1 * cos_r) * world_scale.y;

        // Transform vertex 2
        float local_x2 = cmd.triangle.x2 - transform->origin.x;
        float local_y2 = cmd.triangle.y2 - transform->origin.y;
        float world_x2 = world_pos.x + (local_x2 * cos_r - local_y2 * sin_r) * world_scale.x;
        float world_y2 = world_pos.y + (local_x2 * sin_r + local_y2 * cos_r) * world_scale.y;

        // Transform vertex 3
        float local_x3 = cmd.triangle.x3 - transform->origin.x;
        float local_y3 = cmd.triangle.y3 - transform->origin.y;
        float world_x3 = world_pos.x + (local_x3 * cos_r - local_y3 * sin_r) * world_scale.x;
        float world_y3 = world_pos.y + (local_x3 * sin_r + local_y3 * cos_r) * world_scale.y;

        Vector2 v1 = {world_x1, world_y1};
        Vector2 v2 = {world_x2, world_y2};
        Vector2 v3 = {world_x3, world_y3};

        if (cmd.triangle.filled) {
            DrawTriangle(v1, v2, v3, color);
        } else {
            DrawTriangleLines(v1, v2, v3, color);
        }
    } else {
        // SCREEN-SPACE RENDERING (backward compat)
        // In surface mode, screen-space primitives need to bypass the surface camera transform
        suspend_surface_camera();

        Vector2 v1 = {cmd.triangle.x1, cmd.triangle.y1};
        Vector2 v2 = {cmd.triangle.x2, cmd.triangle.y2};
        Vector2 v3 = {cmd.triangle.x3, cmd.triangle.y3};

        if (cmd.triangle.filled) {
            DrawTriangle(v1, v2, v3, color);
        } else {
            DrawTriangleLines(v1, v2, v3, color);
        }

        restore_surface_camera();
    }
}

void DrawQueue::draw_texture_quad(const DrawCommand& cmd) {
    auto* texture = TextureManager::instance().get(cmd.texture_quad.texture);
    if (!texture) {
        static int warn_count = 0;
        if (warn_count++ < 5) {
            TraceLog(LOG_WARNING, "draw_texture_quad: texture handle %d not found!", cmd.texture_quad.texture);
        }
        return;
    }

    // Apply subpixel inset to prevent texture bleeding from adjacent frames.
    constexpr float TEXEL_INSET = 0.5f;
    Rectangle source = {
        cmd.texture_quad.src_x + TEXEL_INSET,
        cmd.texture_quad.src_y + TEXEL_INSET,
        cmd.texture_quad.src_w - TEXEL_INSET * 2.0f,
        cmd.texture_quad.src_h - TEXEL_INSET * 2.0f
    };

    // World position and size
    float world_x = cmd.texture_quad.x;
    float world_y = cmd.texture_quad.y;
    float world_size = cmd.texture_quad.size;

    // If camera is active, raylib's BeginMode2D handles transformation.
    // If no camera active, we need to manually transform world->screen.
    float screen_x, screen_y, screen_size;

    if (active_camera_ != INVALID_CAMERA_HANDLE) {
        // Camera is active - use world coordinates directly, raylib transforms
        screen_x = world_x;
        screen_y = world_y;
        screen_size = world_size;
    } else {
        // No camera active - manually transform using last_camera_ if available
        Camera2DState* cam = (last_camera_ != INVALID_CAMERA_HANDLE)
            ? CameraManager::instance().get(last_camera_) : nullptr;

        if (cam) {
            Vec2 screen_pos = cam->world_to_screen({world_x, world_y});
            float scale = cam->get_effective_scale();
            screen_x = screen_pos.x;
            screen_y = screen_pos.y;
            screen_size = world_size * scale;
        } else {
            // No camera at all - fallback to world coords as pixels (probably wrong)
            screen_x = world_x;
            screen_y = world_y;
            screen_size = world_size * ASSET_PPU;
        }
    }

    float half_size = screen_size * 0.5f;
    Rectangle dest = {
        screen_x - half_size,
        screen_y - half_size,
        screen_size,
        screen_size
    };

    Vector2 origin = {half_size, half_size};
    float rotation_degrees = cmd.texture_quad.rotation * (180.0f / 3.14159265358979323846f);

    ::Color tint{cmd.texture_quad.color.r, cmd.texture_quad.color.g,
                 cmd.texture_quad.color.b, cmd.texture_quad.color.a};

    // Use additive blending for particle effects (makes black transparent)
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTexturePro(*texture, source, dest, origin, rotation_degrees, tint);
    EndBlendMode();
}

void DrawQueue::draw_text(const DrawCommand& cmd) {
    ::Color color = to_raylib(cmd.text.color);

    // Get font (custom or default)
    Font font;
    if (cmd.text.font != INVALID_HANDLE) {
        auto* custom_font = FontManager::instance().get(cmd.text.font);
        if (custom_font) {
            font = *custom_font;
        } else {
            font = GetFontDefault();
        }
    } else {
        font = GetFontDefault();
    }

    if (cmd.text.transform != INVALID_HANDLE) {
        // WORLD-SPACE RENDERING (with Transform2D)
        // Font size is in WORLD UNITS (like sprites and tilemaps)
        // The camera's effective_scale converts world units to screen pixels
        auto* transform = TransformManager::instance().get(cmd.text.transform);
        if (!transform) return;

        Vec2 world_pos = TransformManager::instance().get_world_position(cmd.text.transform);
        Vec2 world_scale = TransformManager::instance().get_world_scale(cmd.text.transform);

        // Apply parallax scrolling (same as sprites)
        if (active_camera_ != INVALID_CAMERA_HANDLE) {
            auto* cam = CameraManager::instance().get(active_camera_);
            if (cam) {
                float parallax = transform->parallax;
                if (parallax != 1.0f) {
                    float offset_factor = 1.0f - parallax;
                    world_pos.x += cam->target.x * offset_factor;
                    world_pos.y += cam->target.y * offset_factor;
                }
            }
        }

        // Convert font size from world units to pixels, applying transform scale
        float avg_scale = (std::abs(world_scale.x) + std::abs(world_scale.y)) * 0.5f;
        float font_size_world = static_cast<float>(cmd.text.font_size) * avg_scale;

        // Get camera effective scale to convert world units to screen pixels
        float effective_scale = 1.0f;
        if (active_camera_ != INVALID_CAMERA_HANDLE) {
            auto* cam = CameraManager::instance().get(active_camera_);
            if (cam) {
                effective_scale = cam->get_effective_scale();
            }
        }

        // Final pixel size = world_size * effective_scale
        float font_size_pixels = font_size_world * effective_scale;
        float spacing = font_size_pixels / 10.0f;

        Vector2 position = {world_pos.x, world_pos.y};

        DrawTextEx(font, cmd.text.content.c_str(), position, font_size_pixels, spacing, color);
    } else {
        // SCREEN-SPACE RENDERING with auto-scaling
        // Developer specifies size at 360p baseline, engine auto-scales to virtual resolution
        // This means: at 360p, size 14 = 14px. At 1080p, size 14 = 42px. At 128p, size 14 = 5px.
        auto& state = State::instance();
        float ui_scale = state.ui_scale();

        float font_size_scaled = static_cast<float>(cmd.text.font_size) * ui_scale;
        float spacing = font_size_scaled / 10.0f;
        Vector2 position = {cmd.text.x * ui_scale, cmd.text.y * ui_scale};

        // In surface mode, screen-space primitives need to bypass the surface camera transform
        suspend_surface_camera();
        DrawTextEx(font, cmd.text.content.c_str(), position, font_size_scaled, spacing, color);
        restore_surface_camera();
    }
}

} // namespace gmr
