#include "gmr/camera.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace gmr {

// ============================================================================
// Camera2DState Implementation
// ============================================================================

Camera2DState::Camera2DState()
    : target{0.0f, 0.0f}
    , offset{0.0f, 0.0f}
    , zoom{1.0f}
    , rotation{0.0f}
    , view_width{0.0f}                  // 0 = Unity-style (derive from aspect ratio)
    , view_height{7.5f}                 // Default: 7.5 world units tall (180px / 24 ASSET_PPU)
    , viewport_size{320.0f, 180.0f}     // Default: retro 16:9 resolution
    , pixels_per_unit{24.0f}            // Derived: 180 / 7.5 = 24
    , follow_target{mrb_nil_value()}
    , smoothing{0.0f}
    , deadzone{0.0f, 0.0f, 0.0f, 0.0f}
    , has_deadzone{false}
    , bounds{0.0f, 0.0f, 0.0f, 0.0f}
    , has_bounds{false}
    , shake_strength{0.0f}
    , shake_duration{0.0f}
    , shake_time_remaining{0.0f}
    , shake_frequency{30.0f}
    , shake_offset{0.0f, 0.0f}
    , dirty{true}
{
}

void Camera2DState::update_pixels_per_unit() {
    // PPU is derived from viewport_size and view_height
    // This ensures the same world view regardless of resolution
    if (view_height > 0.0f) {
        pixels_per_unit = viewport_size.y / view_height;
    }
}

// ============================================================================
// World-Space Projection Methods
// ============================================================================

float Camera2DState::get_effective_scale() const {
    return pixels_per_unit * zoom;
}

Vec2 Camera2DState::world_to_screen(const Vec2& world) const {
    // Transform: World -> View -> Screen
    // 1. Translate so camera target is at origin (view space)
    // 2. Apply rotation (if any)
    // 3. Scale by pixels_per_unit * zoom
    // 4. Add screen offset (where target appears on screen)

    float scale = get_effective_scale();

    // Translate to view space (relative to camera target)
    float vx = world.x - target.x;
    float vy = world.y - target.y;

    // Apply rotation if present
    if (rotation != 0.0f) {
        float rad = rotation * static_cast<float>(M_PI) / 180.0f;
        float cos_r = cosf(rad);
        float sin_r = sinf(rad);
        float rx = vx * cos_r - vy * sin_r;
        float ry = vx * sin_r + vy * cos_r;
        vx = rx;
        vy = ry;
    }

    // Scale to screen space and add offset
    Vec2 screen;
    screen.x = vx * scale + offset.x;
    screen.y = vy * scale + offset.y;

    return screen;
}

Vec2 Camera2DState::screen_to_world(const Vec2& screen) const {
    // Inverse transform: Screen -> View -> World
    float scale = get_effective_scale();

    // Remove offset and scale
    float vx = (screen.x - offset.x) / scale;
    float vy = (screen.y - offset.y) / scale;

    // Apply inverse rotation if present
    if (rotation != 0.0f) {
        float rad = -rotation * static_cast<float>(M_PI) / 180.0f;  // Negative for inverse
        float cos_r = cosf(rad);
        float sin_r = sinf(rad);
        float rx = vx * cos_r - vy * sin_r;
        float ry = vx * sin_r + vy * cos_r;
        vx = rx;
        vy = ry;
    }

    // Translate back to world space
    Vec2 world;
    world.x = vx + target.x;
    world.y = vy + target.y;

    return world;
}

Rect Camera2DState::get_visible_bounds() const {
    float half_width = get_visible_width() / 2.0f;
    float half_height = get_visible_height() / 2.0f;

    // Note: This doesn't account for rotation. For rotated cameras,
    // you'd need to compute the axis-aligned bounding box of the rotated viewport.
    return Rect{
        target.x - half_width,
        target.y - half_height,
        half_width * 2.0f,
        half_height * 2.0f
    };
}

float Camera2DState::get_visible_width() const {
    // In fixed view mode (retro), return the fixed view_width
    // In Unity mode, derive from viewport aspect ratio
    if (view_width > 0.0f) {
        return view_width / zoom;  // Fixed width, affected by zoom
    }
    return viewport_size.x / get_effective_scale();
}

float Camera2DState::get_visible_height() const {
    // Always based on view_height (the primary control)
    return view_height / zoom;
}

// ============================================================================
// CameraManager Implementation
// ============================================================================

CameraManager& CameraManager::instance() {
    static CameraManager instance;
    return instance;
}

CameraHandle CameraManager::create() {
    CameraHandle handle = next_id_++;
    cameras_[handle] = Camera2DState();
    return handle;
}

void CameraManager::destroy(mrb_state* mrb, CameraHandle handle) {
    auto it = cameras_.find(handle);
    if (it != cameras_.end()) {
        // Unregister follow_target from GC if it was set
        if (!mrb_nil_p(it->second.follow_target)) {
            mrb_gc_unregister(mrb, it->second.follow_target);
        }
        cameras_.erase(it);
        if (current_camera_ == handle) {
            current_camera_ = INVALID_CAMERA_HANDLE;
        }
    }
}

Camera2DState* CameraManager::get(CameraHandle handle) {
    auto it = cameras_.find(handle);
    if (it != cameras_.end()) {
        return &it->second;
    }
    return nullptr;
}

void CameraManager::set_current(CameraHandle handle) {
    current_camera_ = handle;
}

CameraHandle CameraManager::current() const {
    return current_camera_;
}

Camera2DState* CameraManager::get_current() {
    return get(current_camera_);
}

void CameraManager::clear(mrb_state* mrb) {
    // Unregister all follow_targets from GC before clearing
    for (auto& [id, cam] : cameras_) {
        if (!mrb_nil_p(cam.follow_target)) {
            mrb_gc_unregister(mrb, cam.follow_target);
        }
    }
    cameras_.clear();
    current_camera_ = INVALID_CAMERA_HANDLE;
    next_id_ = 0;
}

// ============================================================================
// Per-frame Update Logic
// ============================================================================

void CameraManager::update(mrb_state* mrb, float dt) {
    for (auto& [id, cam] : cameras_) {
        // 1. Resolve follow target position (if following)
        if (!mrb_nil_p(cam.follow_target)) {
            Vec2 target_pos = get_target_position(mrb, cam.follow_target);

            // 2. Apply deadzone logic (screen-space)
            if (cam.has_deadzone) {
                target_pos = apply_deadzone(cam, target_pos);
            }

            // 3. Apply smoothing (lerp towards target)
            if (cam.smoothing > 0.0f) {
                // smoothing factor: lower = faster, higher = slower
                // We use (1 - smoothing) as the interpolation factor per frame
                // For frame-rate independence, we'd need dt-based smoothing,
                // but for simplicity we use the direct factor approach
                float factor = 1.0f - cam.smoothing;
                cam.target.x = cam.target.x + (target_pos.x - cam.target.x) * factor;
                cam.target.y = cam.target.y + (target_pos.y - cam.target.y) * factor;
            } else {
                // No smoothing: instant follow
                cam.target = target_pos;
            }
        }

        // 4. Apply world bounds clamping
        if (cam.has_bounds) {
            clamp_to_bounds(cam);
        }

        // 5. Update screen shake
        if (cam.shake_time_remaining > 0.0f) {
            update_shake(cam, dt);
        } else {
            cam.shake_offset = {0.0f, 0.0f};
        }

        cam.dirty = false;
    }
}

Vec2 CameraManager::get_target_position(mrb_state* mrb, mrb_value target) {
    // Duck typing: try position method first, then fall back to x/y

    // Try position method
    mrb_sym pos_sym = mrb_intern_cstr(mrb, "position");
    if (mrb_respond_to(mrb, target, pos_sym)) {
        mrb_value pos = scripting::safe_method_call(mrb, target, "position");

        // Try to extract Vec2 from returned value
        // Check if it's a Vec2 by looking for x and y methods
        mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
        mrb_sym y_sym = mrb_intern_cstr(mrb, "y");
        if (mrb_respond_to(mrb, pos, x_sym) && mrb_respond_to(mrb, pos, y_sym)) {
            mrb_value x = scripting::safe_method_call(mrb, pos, "x");
            mrb_value y = scripting::safe_method_call(mrb, pos, "y");
            return {static_cast<float>(mrb_as_float(mrb, x)),
                    static_cast<float>(mrb_as_float(mrb, y))};
        }
    }

    // Fall back to direct x/y methods
    mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
    mrb_sym y_sym = mrb_intern_cstr(mrb, "y");
    if (mrb_respond_to(mrb, target, x_sym) && mrb_respond_to(mrb, target, y_sym)) {
        mrb_value x = scripting::safe_method_call(mrb, target, "x");
        mrb_value y = scripting::safe_method_call(mrb, target, "y");
        return {static_cast<float>(mrb_as_float(mrb, x)),
                static_cast<float>(mrb_as_float(mrb, y))};
    }

    // Target doesn't respond to position or x/y - return current target
    return {0.0f, 0.0f};
}

Vec2 CameraManager::apply_deadzone(Camera2DState& cam, const Vec2& target_pos) {
    // Deadzone is now in world units, centered on the camera target
    // Only move the camera when the target exits the deadzone

    // Calculate position of target relative to camera in world space
    float dx = target_pos.x - cam.target.x;
    float dy = target_pos.y - cam.target.y;

    // Deadzone half-sizes in world units
    float dz_half_width = cam.deadzone.width / 2.0f;
    float dz_half_height = cam.deadzone.height / 2.0f;

    // Calculate how much we need to adjust the target
    Vec2 adjusted_target = cam.target;

    // If target is outside deadzone horizontally, adjust
    if (dx < -dz_half_width) {
        adjusted_target.x = target_pos.x + dz_half_width;
    } else if (dx > dz_half_width) {
        adjusted_target.x = target_pos.x - dz_half_width;
    }

    // If target is outside deadzone vertically, adjust
    if (dy < -dz_half_height) {
        adjusted_target.y = target_pos.y + dz_half_height;
    } else if (dy > dz_half_height) {
        adjusted_target.y = target_pos.y - dz_half_height;
    }

    return adjusted_target;
}

void CameraManager::clamp_to_bounds(Camera2DState& cam) {
    // Calculate visible area in world space using projection methods
    float visible_width = cam.get_visible_width();
    float visible_height = cam.get_visible_height();

    // First, clamp zoom to prevent viewport from exceeding world bounds
    // Calculate minimum zoom needed to keep viewport within bounds
    float base_visible_width = cam.viewport_size.x / cam.pixels_per_unit;
    float base_visible_height = cam.viewport_size.y / cam.pixels_per_unit;
    float min_zoom_x = base_visible_width / cam.bounds.width;
    float min_zoom_y = base_visible_height / cam.bounds.height;
    float min_zoom = std::max(min_zoom_x, min_zoom_y);

    if (cam.zoom < min_zoom) {
        cam.zoom = min_zoom;
        // Recalculate visible area with clamped zoom
        visible_width = cam.get_visible_width();
        visible_height = cam.get_visible_height();
    }

    // Calculate min/max target positions to keep camera within bounds
    float min_x = cam.bounds.x + visible_width / 2.0f;
    float max_x = cam.bounds.x + cam.bounds.width - visible_width / 2.0f;
    float min_y = cam.bounds.y + visible_height / 2.0f;
    float max_y = cam.bounds.y + cam.bounds.height - visible_height / 2.0f;

    // Clamp target position
    if (min_x > max_x) {
        // Viewport exactly matches or exceeds world width - center it
        cam.target.x = cam.bounds.x + cam.bounds.width / 2.0f;
    } else {
        if (cam.target.x < min_x) cam.target.x = min_x;
        if (cam.target.x > max_x) cam.target.x = max_x;
    }

    if (min_y > max_y) {
        // Viewport exactly matches or exceeds world height - center it
        cam.target.y = cam.bounds.y + cam.bounds.height / 2.0f;
    } else {
        if (cam.target.y < min_y) cam.target.y = min_y;
        if (cam.target.y > max_y) cam.target.y = max_y;
    }
}

void CameraManager::update_shake(Camera2DState& cam, float dt) {
    cam.shake_time_remaining -= dt;

    if (cam.shake_time_remaining <= 0.0f) {
        cam.shake_time_remaining = 0.0f;
        cam.shake_offset = {0.0f, 0.0f};
        return;
    }

    // Decay factor (linear decay)
    float decay = cam.shake_time_remaining / cam.shake_duration;

    // Procedural noise using sine waves with different frequencies
    // This creates a more interesting shake pattern than pure random
    float time = cam.shake_duration - cam.shake_time_remaining;
    float freq = cam.shake_frequency;

    // Use multiple sine waves with different frequencies for pseudo-random feel
    float noise_x = sinf(time * freq * 2.0f * static_cast<float>(M_PI)) *
                    cosf(time * freq * 1.3f * static_cast<float>(M_PI));
    float noise_y = cosf(time * freq * 1.7f * static_cast<float>(M_PI)) *
                    sinf(time * freq * 2.3f * static_cast<float>(M_PI));

    // Apply strength and decay
    cam.shake_offset.x = noise_x * cam.shake_strength * decay;
    cam.shake_offset.y = noise_y * cam.shake_strength * decay;
}

} // namespace gmr
