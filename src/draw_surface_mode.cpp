#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/camera.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/resources/tilemap_manager.hpp"
#include "gmr/resources/shader_manager.hpp"
#include "gmr/rendering/surface_pool.hpp"
#include "raylib.h"
#include <cmath>
#include <limits>

namespace gmr {

// ==================== Camera and Shader Apply Functions ====================

void DrawQueue::apply_camera_begin(CameraHandle handle) {
    // Track that this camera would be active (even in surface mode)
    active_camera_ = handle;

    // In surface mode, the surface camera handles all transforms
    // We just record the camera handle but don't start a new Mode2D
    if (in_surface_mode()) {
        return;
    }

    auto* cam = CameraManager::instance().get(handle);
    if (cam) {
        ::Camera2D raylib_cam = {};

        // World-space projection:
        // The effective scale (pixels_per_unit * zoom) becomes the raylib zoom factor.
        // This means all world coordinates are automatically multiplied by effective_scale
        // when rendered, converting world units to screen pixels.
        float effective_scale = cam->get_effective_scale();

        // Keep smooth camera movement - pixel snapping happens in tile rendering
        raylib_cam.target = {cam->target.x, cam->target.y};

        // Shake offset is in world units, convert to screen offset contribution
        float shake_screen_x = cam->shake_offset.x * effective_scale;
        float shake_screen_y = cam->shake_offset.y * effective_scale;
        raylib_cam.offset = {cam->offset.x + shake_screen_x,
                             cam->offset.y + shake_screen_y};
        raylib_cam.rotation = cam->rotation;
        raylib_cam.zoom = effective_scale;  // Uses effective scale as raylib zoom

        BeginMode2D(raylib_cam);
    }
}

void DrawQueue::apply_camera_end() {
    // In surface mode, we didn't start a Mode2D, so don't end one
    if (in_surface_mode()) {
        active_camera_ = INVALID_CAMERA_HANDLE;
        return;
    }

    if (active_camera_ != INVALID_CAMERA_HANDLE) {
        EndMode2D();
        active_camera_ = INVALID_CAMERA_HANDLE;
    }
}

void DrawQueue::apply_shader_begin(ShaderHandle handle) {
    auto* shader = ShaderManager::instance().get(handle);
    if (shader) {
        if (IsShaderValid(shader->raylib_shader)) {
            BeginShaderMode(shader->raylib_shader);
        } else {
            TraceLog(LOG_WARNING, "SHADER: Attempted to use invalid raylib shader [ID %d]", handle);
        }
    } else {
        TraceLog(LOG_WARNING, "SHADER: Attempted to use null shader [ID %d]", handle);
    }
}

void DrawQueue::apply_shader_end() {
    EndShaderMode();
}

// ==================== Surface Mode Helpers ====================

size_t DrawQueue::find_shader_end(size_t start_idx) const {
    int depth = 1;
    for (size_t i = start_idx + 1; i < commands_.size(); ++i) {
        if (commands_[i].type == DrawCommand::Type::SHADER_BEGIN) {
            depth++;
        }
        if (commands_[i].type == DrawCommand::Type::SHADER_END) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return commands_.size() - 1;  // Fallback to end if no matching end found
}

DrawQueue::SurfaceBounds DrawQueue::get_drawable_bounds(const DrawCommand& cmd) const {
    switch (cmd.type) {
        case DrawCommand::Type::SPRITE: {
            auto* sprite = SpriteManager::instance().get(cmd.sprite_handle);
            if (!sprite) return {};

            auto* texture = TextureManager::instance().get(sprite->texture);
            if (!texture) return {};

            Vec2 pos = TransformManager::instance().get_world_position(sprite->transform);
            Vec2 scale = TransformManager::instance().get_world_scale(sprite->transform);

            float tex_w = sprite->use_source_rect ? sprite->source_rect.width :
                          static_cast<float>(texture->width);
            float tex_h = sprite->use_source_rect ? sprite->source_rect.height :
                          static_cast<float>(texture->height);

            float w = std::abs(tex_w * scale.x / ASSET_PPU);
            float h = std::abs(tex_h * scale.y / ASSET_PPU);

            return {pos.x, pos.y, w, h};
        }
        case DrawCommand::Type::TILEMAP: {
            auto* tm = TilemapManager::instance().get(cmd.tilemap.handle);
            if (!tm) return {};

            float tile_size = static_cast<float>(tm->tile_width) / ASSET_PPU;
            float w, h;
            if (cmd.tilemap.use_region) {
                w = cmd.tilemap.region_w * tile_size;
                h = cmd.tilemap.region_h * tile_size;
            } else {
                w = tm->width * tile_size;
                h = tm->height * tile_size;
            }

            return {cmd.tilemap.offset_x, cmd.tilemap.offset_y, w, h};
        }
        case DrawCommand::Type::RECT: {
            if (cmd.rect.transform != INVALID_HANDLE) {
                Vec2 pos = TransformManager::instance().get_world_position(cmd.rect.transform);
                Vec2 scale = TransformManager::instance().get_world_scale(cmd.rect.transform);
                return {pos.x, pos.y, cmd.rect.width * std::abs(scale.x), cmd.rect.height * std::abs(scale.y)};
            }
            return {cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h};
        }
        case DrawCommand::Type::CIRCLE: {
            if (cmd.circle.transform != INVALID_HANDLE) {
                Vec2 pos = TransformManager::instance().get_world_position(cmd.circle.transform);
                Vec2 scale = TransformManager::instance().get_world_scale(cmd.circle.transform);
                float avg_scale = (std::abs(scale.x) + std::abs(scale.y)) * 0.5f;
                float r = cmd.circle.radius * avg_scale;
                return {pos.x - r, pos.y - r, r * 2, r * 2};
            }
            return {cmd.circle.x - cmd.circle.radius, cmd.circle.y - cmd.circle.radius,
                    cmd.circle.radius * 2, cmd.circle.radius * 2};
        }
        default:
            return {};
    }
}

DrawQueue::SurfaceBounds DrawQueue::calculate_group_bounds(size_t start, size_t end) const {
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    bool found_any = false;

    for (size_t i = start; i <= end; ++i) {
        SurfaceBounds drawable = get_drawable_bounds(commands_[i]);
        if (drawable.width > 0 && drawable.height > 0) {
            min_x = std::min(min_x, drawable.x);
            min_y = std::min(min_y, drawable.y);
            max_x = std::max(max_x, drawable.x + drawable.width);
            max_y = std::max(max_y, drawable.y + drawable.height);
            found_any = true;
        }
    }

    if (!found_any) {
        return {0, 0, 0, 0};
    }

    return {min_x, min_y, max_x - min_x, max_y - min_y};
}

float DrawQueue::get_effective_scale() const {
    if (active_camera_ != INVALID_CAMERA_HANDLE) {
        auto* cam = CameraManager::instance().get(active_camera_);
        if (cam) {
            return cam->get_effective_scale();
        }
    }
    // Default scale when no camera active
    return ASSET_PPU;
}

float DrawQueue::get_shader_group_effective_scale(size_t start_idx, size_t end_idx) const {
    // Pre-scan the command range to find any CAMERA_BEGIN and use its effective scale
    // This solves the timing problem where SHADER_BEGIN is processed before CAMERA_BEGIN
    CameraHandle cam_handle = get_shader_group_camera(start_idx, end_idx);
    if (cam_handle != INVALID_CAMERA_HANDLE) {
        auto* cam = CameraManager::instance().get(cam_handle);
        if (cam) {
            return cam->get_effective_scale();
        }
    }
    // No camera found in this shader group - use default
    return ASSET_PPU;
}

CameraHandle DrawQueue::get_shader_group_camera(size_t start_idx, size_t end_idx) const {
    // Pre-scan the command range to find any CAMERA_BEGIN
    for (size_t i = start_idx; i <= end_idx; ++i) {
        if (commands_[i].type == DrawCommand::Type::CAMERA_BEGIN) {
            return commands_[i].camera_handle;
        }
    }
    return INVALID_CAMERA_HANDLE;
}

DrawQueue::SurfaceBounds DrawQueue::get_surface_offset() const {
    if (surface_stack_.empty()) {
        return {0, 0, 0, 0};
    }
    return surface_stack_.top().bounds;
}

void DrawQueue::begin_surface_mode(ShaderHandle shader, const SurfaceBounds& bounds, float effective_scale, CameraHandle camera_hint) {
    // Create a surface that matches the viewport size.
    // We use the same camera transform as normal rendering, so the surface
    // just needs to be big enough to hold what the camera sees.

    int pixel_width = 0;
    int pixel_height = 0;

    if (camera_hint != INVALID_CAMERA_HANDLE) {
        auto* cam = CameraManager::instance().get(camera_hint);
        if (cam) {
            // Use viewport size directly - that's exactly what we need
            pixel_width = static_cast<int>(cam->viewport_size.x);
            pixel_height = static_cast<int>(cam->viewport_size.y);
        }
    }

    // Fallback if no camera
    if (pixel_width == 0 || pixel_height == 0) {
        pixel_width = static_cast<int>(std::ceil(bounds.width * effective_scale));
        pixel_height = static_cast<int>(std::ceil(bounds.height * effective_scale));
    }

    // Clamp to reasonable size (should rarely hit this now since we use visible bounds)
    pixel_width = std::max(1, std::min(pixel_width, 4096));
    pixel_height = std::max(1, std::min(pixel_height, 4096));

    // Acquire surface with EXACT dimensions (no power-of-two rounding)
    // This is critical for correct UV coordinates in shaders
    SurfaceHandle surface = SurfacePool::instance().acquire_exact(pixel_width, pixel_height);
    if (surface == INVALID_SURFACE_HANDLE) {
        TraceLog(LOG_ERROR, "DRAW_QUEUE: Failed to acquire surface for shader surface mode");
        // Fall back to direct shader mode
        apply_shader_begin(shader);
        return;
    }

    RenderTexture2D* rt = SurfacePool::instance().get(surface);

    // Begin rendering to surface
    BeginTextureMode(*rt);
    ClearBackground({0, 0, 0, 0});  // Transparent clear

    // Set up surface camera to EXACTLY match the original camera
    ::Camera2D surface_cam = {};

    if (camera_hint != INVALID_CAMERA_HANDLE) {
        auto* cam = CameraManager::instance().get(camera_hint);
        if (cam) {
            // Use the EXACT same camera settings - no modifications needed
            // since texture dimensions now match viewport exactly
            surface_cam.target = {cam->target.x, cam->target.y};
            surface_cam.offset = {cam->offset.x, cam->offset.y};
            surface_cam.rotation = cam->rotation;
            surface_cam.zoom = effective_scale;

            // Apply shake
            float shake_screen_x = cam->shake_offset.x * effective_scale;
            float shake_screen_y = cam->shake_offset.y * effective_scale;
            surface_cam.offset.x += shake_screen_x;
            surface_cam.offset.y += shake_screen_y;
        }
    }

    if (surface_cam.zoom == 0.0f) {
        // Fallback if no camera - center on bounds
        surface_cam.target = {
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        };
        surface_cam.offset = {
            static_cast<float>(pixel_width) * 0.5f,
            static_cast<float>(pixel_height) * 0.5f
        };
        surface_cam.rotation = 0.0f;
        surface_cam.zoom = effective_scale;
    }

    BeginMode2D(surface_cam);

    // Push surface state
    SurfaceState state;
    state.handle = surface;
    state.bounds = bounds;  // Store original bounds (not used for blitting anymore)
    state.shader = shader;
    state.pixel_width = pixel_width;
    state.pixel_height = pixel_height;
    state.effective_scale = effective_scale;
    state.camera_hint = camera_hint;
    surface_stack_.push(state);

    TraceLog(LOG_DEBUG, "DRAW_QUEUE: Begin surface mode [%d] pixels=%dx%d scale=%.1f",
             surface, pixel_width, pixel_height, effective_scale);
}

void DrawQueue::end_surface_mode() {
    if (surface_stack_.empty()) {
        TraceLog(LOG_WARNING, "DRAW_QUEUE: end_surface_mode called with empty stack");
        return;
    }

    SurfaceState state = surface_stack_.top();
    surface_stack_.pop();

    // End the surface-local camera
    EndMode2D();

    // End rendering to surface
    EndTextureMode();

    // Get the rendered surface
    RenderTexture2D* rt = SurfacePool::instance().get(state.handle);
    if (!rt) {
        TraceLog(LOG_ERROR, "DRAW_QUEUE: Invalid surface handle in end_surface_mode");
        return;
    }

    // Simple blit - texture dimensions match viewport exactly now
    // Negative height flips Y for OpenGL render texture
    Rectangle source = {
        0,
        0,
        static_cast<float>(state.pixel_width),
        -static_cast<float>(state.pixel_height)  // Negative height flips Y
    };

    // Destination: screen (0,0), same size as viewport
    Rectangle dest = {
        0,
        0,
        static_cast<float>(state.pixel_width),
        static_cast<float>(state.pixel_height)
    };

    // Apply shader when blitting surface to main target
    apply_shader_begin(state.shader);

    DrawTexturePro(rt->texture, source, dest, {0, 0}, 0, ::Color{255, 255, 255, 255});

    apply_shader_end();

    // Release surface back to pool
    SurfacePool::instance().release(state.handle);

    TraceLog(LOG_DEBUG, "DRAW_QUEUE: End surface mode [%d] dest=(%.0f,%.0f,%.0f,%.0f)",
             state.handle, dest.x, dest.y, dest.width, dest.height);
}

void DrawQueue::suspend_surface_camera() {
    if (!in_surface_mode()) return;
    EndMode2D();
}

void DrawQueue::restore_surface_camera() {
    if (!in_surface_mode()) return;

    const SurfaceState& state = surface_stack_.top();
    if (state.camera_hint != INVALID_CAMERA_HANDLE) {
        auto* cam = CameraManager::instance().get(state.camera_hint);
        if (cam) {
            ::Camera2D surface_cam = {};
            surface_cam.target = {cam->target.x, cam->target.y};
            surface_cam.offset = {cam->offset.x, cam->offset.y};
            surface_cam.rotation = cam->rotation;
            surface_cam.zoom = state.effective_scale;
            float shake_screen_x = cam->shake_offset.x * state.effective_scale;
            float shake_screen_y = cam->shake_offset.y * state.effective_scale;
            surface_cam.offset.x += shake_screen_x;
            surface_cam.offset.y += shake_screen_y;
            BeginMode2D(surface_cam);
        }
    }
}

} // namespace gmr
