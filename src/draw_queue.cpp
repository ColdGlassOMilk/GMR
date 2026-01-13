#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/camera.hpp"
#include "gmr/resources/shader_manager.hpp"
#include "gmr/rendering/surface_pool.hpp"
#include "raylib.h"
#include <algorithm>

namespace gmr {

DrawQueue& DrawQueue::instance() {
    static DrawQueue instance;
    return instance;
}

DrawCommand& DrawQueue::create_command(DrawCommand::Type type, uint8_t layer, float z) {
    float z_value = (z != 0.0f) ? z : (DRAW_ORDER_Z_BASE + static_cast<float>(next_draw_order_));
    commands_.emplace_back();
    auto& cmd = commands_.back();
    cmd.type = type;
    cmd.layer = layer;
    cmd.z = z_value;
    cmd.draw_order = next_draw_order_++;
    return cmd;
}

void DrawQueue::begin_frame() {
    next_draw_order_ = 0;
    camera_stack_.clear();
    active_camera_ = INVALID_CAMERA_HANDLE;
    last_camera_ = INVALID_CAMERA_HANDLE;
}

void DrawQueue::queue_sprite(SpriteHandle handle) {
    auto* sprite = SpriteManager::instance().get(handle);
    if (!sprite) return;

    // Get layer from sprite
    uint8_t layer = sprite->layer;

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
        layer,
        z_value,
        next_draw_order_,
        handle
    );

    next_draw_order_++;
}

void DrawQueue::queue_camera_begin(CameraHandle handle) {
    auto& cmd = create_command(DrawCommand::Type::CAMERA_BEGIN,
                               static_cast<uint8_t>(RenderLayer::ENTITIES), 0.0f);
    cmd.camera_handle = handle;
    camera_stack_.push_back(handle);
    last_camera_ = handle;  // Remember for particle system
}

void DrawQueue::queue_camera_end() {
    if (camera_stack_.empty()) return;  // Safety check

    auto& cmd = create_command(DrawCommand::Type::CAMERA_END,
                               static_cast<uint8_t>(RenderLayer::ENTITIES), 0.0f);
    cmd.camera_handle = camera_stack_.back();
    camera_stack_.pop_back();
}

void DrawQueue::queue_shader_begin(ShaderHandle handle) {
    auto& cmd = create_command(DrawCommand::Type::SHADER_BEGIN,
                               static_cast<uint8_t>(RenderLayer::ENTITIES), 0.0f);
    cmd.shader_handle = handle;
}

void DrawQueue::queue_shader_end() {
    create_command(DrawCommand::Type::SHADER_END,
                   static_cast<uint8_t>(RenderLayer::ENTITIES), 0.0f);
}

void DrawQueue::queue_tilemap(TilemapHandle handle, float offset_x, float offset_y, const DrawColor& tint) {
    auto& cmd = create_command(DrawCommand::Type::TILEMAP,
                               static_cast<uint8_t>(RenderLayer::WORLD), 0.0f);
    cmd.tilemap.handle = handle;
    cmd.tilemap.offset_x = offset_x;
    cmd.tilemap.offset_y = offset_y;
    cmd.tilemap.tint = tint;
    cmd.tilemap.use_region = false;
}

void DrawQueue::queue_tilemap_region(TilemapHandle handle, float offset_x, float offset_y,
                                      int32_t region_x, int32_t region_y, int32_t region_w, int32_t region_h,
                                      const DrawColor& tint) {
    auto& cmd = create_command(DrawCommand::Type::TILEMAP,
                               static_cast<uint8_t>(RenderLayer::WORLD), 0.0f);
    cmd.tilemap.handle = handle;
    cmd.tilemap.offset_x = offset_x;
    cmd.tilemap.offset_y = offset_y;
    cmd.tilemap.tint = tint;
    cmd.tilemap.use_region = true;
    cmd.tilemap.region_x = region_x;
    cmd.tilemap.region_y = region_y;
    cmd.tilemap.region_w = region_w;
    cmd.tilemap.region_h = region_h;
}

void DrawQueue::queue_rect(float x, float y, float w, float h, const DrawColor& color, bool filled,
                            uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::RECT, layer, z);
    cmd.rect.x = x;
    cmd.rect.y = y;
    cmd.rect.w = w;
    cmd.rect.h = h;
    cmd.rect.color = color;
    cmd.rect.filled = filled;
    cmd.rect.rotation = 0;
}

void DrawQueue::queue_rect_rotated(float x, float y, float w, float h, float rotation, const DrawColor& color,
                                    uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::RECT, layer, z);
    cmd.rect.x = x;
    cmd.rect.y = y;
    cmd.rect.w = w;
    cmd.rect.h = h;
    cmd.rect.color = color;
    cmd.rect.filled = true;
    cmd.rect.rotation = rotation;
}

void DrawQueue::queue_circle(float x, float y, float radius, const DrawColor& color, bool filled,
                              uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::CIRCLE, layer, z);
    cmd.circle.x = x;
    cmd.circle.y = y;
    cmd.circle.radius = radius;
    cmd.circle.color = color;
    cmd.circle.filled = filled;
    cmd.circle.gradient = false;
}

void DrawQueue::queue_circle_gradient(float x, float y, float radius, const DrawColor& inner, const DrawColor& outer,
                                       uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::CIRCLE, layer, z);
    cmd.circle.x = x;
    cmd.circle.y = y;
    cmd.circle.radius = radius;
    cmd.circle.color = inner;
    cmd.circle.color2 = outer;
    cmd.circle.filled = true;
    cmd.circle.gradient = true;
}

void DrawQueue::queue_line(float x1, float y1, float x2, float y2, const DrawColor& color, float thickness,
                            uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::LINE, layer, z);
    cmd.line.x1 = x1;
    cmd.line.y1 = y1;
    cmd.line.x2 = x2;
    cmd.line.y2 = y2;
    cmd.line.color = color;
    cmd.line.thickness = thickness;
}

void DrawQueue::queue_triangle(float x1, float y1, float x2, float y2, float x3, float y3,
                                const DrawColor& color, bool filled,
                                uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::TRIANGLE, layer, z);
    cmd.triangle.x1 = x1;
    cmd.triangle.y1 = y1;
    cmd.triangle.x2 = x2;
    cmd.triangle.y2 = y2;
    cmd.triangle.x3 = x3;
    cmd.triangle.y3 = y3;
    cmd.triangle.color = color;
    cmd.triangle.filled = filled;
}

void DrawQueue::queue_text(float x, float y, const std::string& content, int font_size, const DrawColor& color,
                            uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::TEXT, layer, z);
    cmd.text.x = x;
    cmd.text.y = y;
    cmd.text.font_size = font_size;
    cmd.text.color = color;
    cmd.text.content = content;
}

void DrawQueue::queue_text(float x, float y, const std::string& content, int font_size, const DrawColor& color,
                            FontHandle font, uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::TEXT, layer, z);
    cmd.text.x = x;
    cmd.text.y = y;
    cmd.text.font_size = font_size;
    cmd.text.color = color;
    cmd.text.font = font;
    cmd.text.content = content;
}

// ==================== Transform-based queue functions ====================

void DrawQueue::queue_rect(TransformHandle transform, float width, float height, const DrawColor& color,
                            bool filled, uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::RECT, layer, z);
    cmd.rect.transform = transform;
    cmd.rect.width = width;
    cmd.rect.height = height;
    cmd.rect.color = color;
    cmd.rect.filled = filled;
}

void DrawQueue::queue_circle(TransformHandle transform, float radius, const DrawColor& color,
                              bool filled, uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::CIRCLE, layer, z);
    cmd.circle.transform = transform;
    cmd.circle.radius = radius;
    cmd.circle.color = color;
    cmd.circle.filled = filled;
    cmd.circle.gradient = false;
}

void DrawQueue::queue_triangle(TransformHandle transform, float x1, float y1, float x2, float y2,
                                float x3, float y3, const DrawColor& color, bool filled,
                                uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::TRIANGLE, layer, z);
    cmd.triangle.transform = transform;
    cmd.triangle.x1 = x1;
    cmd.triangle.y1 = y1;
    cmd.triangle.x2 = x2;
    cmd.triangle.y2 = y2;
    cmd.triangle.x3 = x3;
    cmd.triangle.y3 = y3;
    cmd.triangle.color = color;
    cmd.triangle.filled = filled;
}

void DrawQueue::queue_line(TransformHandle transform, float x1, float y1, float x2, float y2,
                            const DrawColor& color, float thickness, uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::LINE, layer, z);
    cmd.line.transform = transform;
    cmd.line.x1 = x1;
    cmd.line.y1 = y1;
    cmd.line.x2 = x2;
    cmd.line.y2 = y2;
    cmd.line.color = color;
    cmd.line.thickness = thickness;
}

void DrawQueue::queue_text(TransformHandle transform, const std::string& content, int font_size,
                            const DrawColor& color, uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::TEXT, layer, z);
    cmd.text.transform = transform;
    cmd.text.font_size = font_size;
    cmd.text.color = color;
    cmd.text.content = content;
}

void DrawQueue::queue_text(TransformHandle transform, const std::string& content, int font_size,
                            const DrawColor& color, FontHandle font, uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::TEXT, layer, z);
    cmd.text.transform = transform;
    cmd.text.font_size = font_size;
    cmd.text.color = color;
    cmd.text.font = font;
    cmd.text.content = content;
}

void DrawQueue::queue_texture_quad(TextureHandle texture, float src_x, float src_y, float src_w, float src_h,
                                    float world_x, float world_y, float size, float rotation,
                                    const DrawColor& color, uint8_t layer, float z) {
    auto& cmd = create_command(DrawCommand::Type::TEXTURE_QUAD, layer, z);
    cmd.texture_quad.texture = texture;
    cmd.texture_quad.src_x = src_x;
    cmd.texture_quad.src_y = src_y;
    cmd.texture_quad.src_w = src_w;
    cmd.texture_quad.src_h = src_h;
    cmd.texture_quad.x = world_x;
    cmd.texture_quad.y = world_y;
    cmd.texture_quad.size = size;
    cmd.texture_quad.rotation = rotation;
    cmd.texture_quad.color = color;
}

// ==================== Flush and Clear ====================

void DrawQueue::flush() {
    if (commands_.empty()) return;

    // Sort by draw_order ONLY
    // The layer system is causing camera transforms to break
    // For now, maintain backward compatibility by sorting everything by draw_order
    // TODO: Implement proper layer-aware camera handling
    std::stable_sort(commands_.begin(), commands_.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            // Temporarily sort by draw_order only to maintain backward compatibility
            // This ensures camera BEGIN/END pairs work correctly
            return a.draw_order < b.draw_order;
        });

    // Pass 1: Calculate bounds, effective scale, AND camera handle for surface-mode shader groups
    // Surface mode is used for shaders that require unified UV space (wave, CRT, etc.)
    // We must pre-scan for camera because SHADER_BEGIN comes before CAMERA_BEGIN in the queue
    struct ShaderGroupInfo {
        SurfaceBounds bounds;
        float effective_scale;
        CameraHandle camera_hint;
    };
    std::unordered_map<size_t, ShaderGroupInfo> shader_group_info;
    for (size_t i = 0; i < commands_.size(); ++i) {
        if (commands_[i].type == DrawCommand::Type::SHADER_BEGIN) {
            if (ShaderManager::instance().requires_surface_continuity(commands_[i].shader_handle)) {
                size_t end_idx = find_shader_end(i);
                SurfaceBounds bounds = calculate_group_bounds(i, end_idx);
                if (bounds.width > 0 && bounds.height > 0) {
                    CameraHandle cam_handle = get_shader_group_camera(i, end_idx);
                    float scale = (cam_handle != INVALID_CAMERA_HANDLE)
                        ? CameraManager::instance().get(cam_handle)->get_effective_scale()
                        : ASSET_PPU;
                    shader_group_info[i] = {bounds, scale, cam_handle};
                }
            }
        }
    }

    // Pass 2: Execute draw commands with surface mode routing
    for (size_t i = 0; i < commands_.size(); ++i) {
        const auto& cmd = commands_[i];
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
            case DrawCommand::Type::TEXTURE_QUAD:
                draw_texture_quad(cmd);
                break;
            case DrawCommand::Type::SHADER_BEGIN: {
                // Check if this shader needs surface mode
                auto it = shader_group_info.find(i);
                if (it != shader_group_info.end()) {
                    // Use surface mode for unified UV space with pre-scanned effective scale and camera
                    begin_surface_mode(cmd.shader_handle, it->second.bounds, it->second.effective_scale, it->second.camera_hint);
                } else {
                    // Regular shader mode
                    apply_shader_begin(cmd.shader_handle);
                }
                break;
            }
            case DrawCommand::Type::SHADER_END:
                if (in_surface_mode()) {
                    end_surface_mode();
                } else {
                    apply_shader_end();
                }
                break;
        }
    }

    // Safety: close any unclosed surface modes
    while (in_surface_mode()) {
        end_surface_mode();
    }

    // Safety: close any unclosed shaders
    apply_shader_end();

    // Safety: close any unclosed cameras
    while (active_camera_ != INVALID_CAMERA_HANDLE) {
        apply_camera_end();
    }

    // Clear for next frame
    commands_.clear();
    camera_stack_.clear();

    // End-of-frame surface pool maintenance
    SurfacePool::instance().end_frame();
}

void DrawQueue::clear() {
    commands_.clear();
    next_draw_order_ = 0;
    camera_stack_.clear();
    active_camera_ = INVALID_CAMERA_HANDLE;
    // Clear surface stack (should already be empty after flush, but ensure cleanup on error)
    while (!surface_stack_.empty()) {
        SurfacePool::instance().release(surface_stack_.top().handle);
        surface_stack_.pop();
    }
}

} // namespace gmr
