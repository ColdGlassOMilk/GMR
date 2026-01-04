#include "gmr/camera.hpp"
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

void CameraManager::destroy(CameraHandle handle) {
    auto it = cameras_.find(handle);
    if (it != cameras_.end()) {
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

void CameraManager::clear() {
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
        mrb_value pos = mrb_funcall(mrb, target, "position", 0);

        // Check if exception was raised
        if (mrb->exc) {
            mrb->exc = nullptr;
            return {0.0f, 0.0f};
        }

        // Try to extract Vec2 from returned value
        // Check if it's a Vec2 by looking for x and y methods
        mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
        mrb_sym y_sym = mrb_intern_cstr(mrb, "y");
        if (mrb_respond_to(mrb, pos, x_sym) && mrb_respond_to(mrb, pos, y_sym)) {
            mrb_value x = mrb_funcall(mrb, pos, "x", 0);
            mrb_value y = mrb_funcall(mrb, pos, "y", 0);
            if (!mrb->exc) {
                return {static_cast<float>(mrb_as_float(mrb,x)),
                        static_cast<float>(mrb_as_float(mrb,y))};
            }
            mrb->exc = nullptr;
        }
    }

    // Fall back to direct x/y methods
    mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
    mrb_sym y_sym = mrb_intern_cstr(mrb, "y");
    if (mrb_respond_to(mrb, target, x_sym) && mrb_respond_to(mrb, target, y_sym)) {
        mrb_value x = mrb_funcall(mrb, target, "x", 0);
        mrb_value y = mrb_funcall(mrb, target, "y", 0);
        if (!mrb->exc) {
            return {static_cast<float>(mrb_as_float(mrb,x)),
                    static_cast<float>(mrb_as_float(mrb,y))};
        }
        mrb->exc = nullptr;
    }

    // Target doesn't respond to position or x/y - return current target
    return {0.0f, 0.0f};
}

Vec2 CameraManager::apply_deadzone(Camera2DState& cam, const Vec2& target_pos) {
    // Deadzone is in screen-space, centered on the camera offset
    // Only move the camera when the target exits the deadzone

    // Calculate screen-space position of target relative to camera
    Vec2 screen_pos;
    screen_pos.x = (target_pos.x - cam.target.x) * cam.zoom + cam.offset.x;
    screen_pos.y = (target_pos.y - cam.target.y) * cam.zoom + cam.offset.y;

    // Deadzone bounds (centered on camera offset)
    float dz_left = cam.offset.x - cam.deadzone.width / 2.0f;
    float dz_right = cam.offset.x + cam.deadzone.width / 2.0f;
    float dz_top = cam.offset.y - cam.deadzone.height / 2.0f;
    float dz_bottom = cam.offset.y + cam.deadzone.height / 2.0f;

    // Calculate how much we need to adjust the target
    Vec2 adjusted_target = cam.target;

    // If target is outside deadzone horizontally, adjust
    if (screen_pos.x < dz_left) {
        adjusted_target.x = target_pos.x - (dz_left - cam.offset.x) / cam.zoom;
    } else if (screen_pos.x > dz_right) {
        adjusted_target.x = target_pos.x - (dz_right - cam.offset.x) / cam.zoom;
    }

    // If target is outside deadzone vertically, adjust
    if (screen_pos.y < dz_top) {
        adjusted_target.y = target_pos.y - (dz_top - cam.offset.y) / cam.zoom;
    } else if (screen_pos.y > dz_bottom) {
        adjusted_target.y = target_pos.y - (dz_bottom - cam.offset.y) / cam.zoom;
    }

    return adjusted_target;
}

void CameraManager::clamp_to_bounds(Camera2DState& cam) {
    // Calculate visible area in world space
    float visible_width = (cam.offset.x * 2.0f) / cam.zoom;
    float visible_height = (cam.offset.y * 2.0f) / cam.zoom;

    // Calculate min/max target positions to keep camera within bounds
    float min_x = cam.bounds.x + visible_width / 2.0f;
    float max_x = cam.bounds.x + cam.bounds.width - visible_width / 2.0f;
    float min_y = cam.bounds.y + visible_height / 2.0f;
    float max_y = cam.bounds.y + cam.bounds.height - visible_height / 2.0f;

    // Handle case where bounds are smaller than visible area
    if (min_x > max_x) {
        cam.target.x = cam.bounds.x + cam.bounds.width / 2.0f;
    } else {
        if (cam.target.x < min_x) cam.target.x = min_x;
        if (cam.target.x > max_x) cam.target.x = max_x;
    }

    if (min_y > max_y) {
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
