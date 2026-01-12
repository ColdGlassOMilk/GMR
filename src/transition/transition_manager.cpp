#include "gmr/transition/transition_manager.hpp"
#include "gmr/state.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/gc.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace gmr {
namespace transition {

TransitionManager& TransitionManager::instance() {
    static TransitionManager inst;
    return inst;
}

bool TransitionManager::start(mrb_state* mrb, const TransitionConfig& config) {
    // Don't start if already transitioning
    if (state_ != TransitionState::NONE) {
        return false;
    }

    // Store config and initialize
    config_ = config;
    mrb_ = mrb;
    state_ = TransitionState::OUT;
    elapsed_ = 0.0f;
    progress_ = 0.0f;

    // Register callback with GC if present
    if (config_.has_callback && mrb_ && !mrb_nil_p(config_.callback)) {
        mrb_gc_register(mrb_, config_.callback);
    }

    return true;
}

void TransitionManager::update(float dt) {
    if (state_ == TransitionState::NONE) {
        return;
    }

    // Half duration for each phase
    float phase_duration = config_.duration / 2.0f;
    if (phase_duration <= 0.0f) {
        phase_duration = 0.001f; // Prevent division by zero
    }

    elapsed_ += dt;

    // Calculate normalized time (0 to 1) for current phase
    float t = std::min(elapsed_ / phase_duration, 1.0f);

    // Apply easing
    progress_ = animation::apply_easing(config_.easing, t);

    // Check for phase completion
    if (elapsed_ >= phase_duration) {
        if (state_ == TransitionState::OUT) {
            // OUT phase complete - scene switch happens now
            // SceneManager will call scene_switched() after the switch
            progress_ = 1.0f;
        } else if (state_ == TransitionState::IN) {
            // IN phase complete - transition finished
            progress_ = 1.0f;

            // Fire callback if present
            if (config_.has_callback && mrb_ && !mrb_nil_p(config_.callback)) {
                std::vector<mrb_value> empty_args;
                scripting::safe_yield(mrb_, config_.callback, empty_args);
                mrb_gc_unregister(mrb_, config_.callback);
            }

            // Cleanup
            release_surface();
            state_ = TransitionState::NONE;
            config_ = TransitionConfig{};
            mrb_ = nullptr;
        }
    }
}

void TransitionManager::scene_switched() {
    if (state_ != TransitionState::OUT) {
        return;
    }

    // Transition to IN phase
    state_ = TransitionState::IN;
    elapsed_ = 0.0f;
    progress_ = 0.0f;
}

void TransitionManager::draw() {
    if (state_ == TransitionState::NONE) {
        return;
    }

    switch (config_.type) {
        case TransitionType::FADE:
            draw_fade();
            break;
        case TransitionType::WIPE_LEFT:
        case TransitionType::WIPE_RIGHT:
        case TransitionType::WIPE_UP:
        case TransitionType::WIPE_DOWN:
            draw_wipe();
            break;
        default:
            break;
    }
}

void TransitionManager::draw_fade() {
    auto& gmr_state = State::instance();

    // Calculate alpha based on state and progress
    float alpha;
    if (state_ == TransitionState::OUT) {
        // Fading out: 0 -> 255
        alpha = progress_ * 255.0f;
    } else {
        // Fading in: 255 -> 0
        alpha = (1.0f - progress_) * 255.0f;
    }

    // Draw fullscreen rectangle with transition color
    // Use ::Color for raylib's Color type (not gmr::Color)
    ::Color overlay = {
        config_.color.r,
        config_.color.g,
        config_.color.b,
        static_cast<unsigned char>(std::clamp(alpha, 0.0f, 255.0f))
    };

    // Use screen dimensions (works with both virtual and native resolution)
    int width = gmr_state.use_virtual_resolution ? gmr_state.virtual_width : gmr_state.screen_width;
    int height = gmr_state.use_virtual_resolution ? gmr_state.virtual_height : gmr_state.screen_height;

    DrawRectangle(0, 0, width, height, overlay);
}

void TransitionManager::draw_wipe() {
    auto& gmr_state = State::instance();

    int width = gmr_state.use_virtual_resolution ? gmr_state.virtual_width : gmr_state.screen_width;
    int height = gmr_state.use_virtual_resolution ? gmr_state.virtual_height : gmr_state.screen_height;

    // For wipe transitions, we draw a solid rectangle that reveals/covers the scene
    float wipe_progress;
    if (state_ == TransitionState::OUT) {
        // Covering the scene: 0% -> 100% covered
        wipe_progress = progress_;
    } else {
        // Revealing the scene: 100% -> 0% covered
        wipe_progress = 1.0f - progress_;
    }

    int rect_x = 0, rect_y = 0, rect_w = width, rect_h = height;

    switch (config_.type) {
        case TransitionType::WIPE_LEFT:
            // Rectangle comes from right
            rect_x = static_cast<int>((1.0f - wipe_progress) * width);
            rect_w = static_cast<int>(wipe_progress * width);
            break;
        case TransitionType::WIPE_RIGHT:
            // Rectangle comes from left
            rect_x = 0;
            rect_w = static_cast<int>(wipe_progress * width);
            break;
        case TransitionType::WIPE_UP:
            // Rectangle comes from bottom
            rect_y = static_cast<int>((1.0f - wipe_progress) * height);
            rect_h = static_cast<int>(wipe_progress * height);
            break;
        case TransitionType::WIPE_DOWN:
            // Rectangle comes from top
            rect_y = 0;
            rect_h = static_cast<int>(wipe_progress * height);
            break;
        default:
            break;
    }

    // Use ::Color for raylib's Color type (not gmr::Color)
    ::Color wipe_color = {
        config_.color.r,
        config_.color.g,
        config_.color.b,
        config_.color.a
    };
    DrawRectangle(rect_x, rect_y, rect_w, rect_h, wipe_color);
}

void TransitionManager::cancel() {
    if (state_ == TransitionState::NONE) {
        return;
    }

    // Unregister callback if present
    if (config_.has_callback && mrb_ && !mrb_nil_p(config_.callback)) {
        mrb_gc_unregister(mrb_, config_.callback);
    }

    release_surface();
    state_ = TransitionState::NONE;
    config_ = TransitionConfig{};
    mrb_ = nullptr;
    elapsed_ = 0.0f;
    progress_ = 0.0f;
}

void TransitionManager::clear() {
    cancel();
}

void TransitionManager::release_surface() {
    if (captured_surface_ != INVALID_SURFACE_HANDLE) {
        SurfacePool::instance().release(captured_surface_);
        captured_surface_ = INVALID_SURFACE_HANDLE;
        capture_width_ = 0;
        capture_height_ = 0;
    }
}

TransitionType symbol_to_transition_type(mrb_state* mrb, mrb_sym sym) {
    const char* name = mrb_sym_name(mrb, sym);

    if (strcmp(name, "fade") == 0) return TransitionType::FADE;
    if (strcmp(name, "wipe_left") == 0) return TransitionType::WIPE_LEFT;
    if (strcmp(name, "wipe_right") == 0) return TransitionType::WIPE_RIGHT;
    if (strcmp(name, "wipe_up") == 0) return TransitionType::WIPE_UP;
    if (strcmp(name, "wipe_down") == 0) return TransitionType::WIPE_DOWN;

    // Default to none
    return TransitionType::NONE;
}

} // namespace transition
} // namespace gmr
