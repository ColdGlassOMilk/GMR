#include "gmr/animation/animation_manager.hpp"
#include "gmr/sprite.hpp"
#include <mruby/string.h>
#include <algorithm>
#include <cmath>

namespace gmr {
namespace animation {

AnimationManager& AnimationManager::instance() {
    static AnimationManager instance;
    return instance;
}

// ============================================================================
// Tween Management
// ============================================================================

TweenHandle AnimationManager::create_tween() {
    TweenHandle handle = next_tween_id_++;
    tweens_[handle] = TweenState();
    return handle;
}

void AnimationManager::destroy_tween(TweenHandle handle) {
    auto it = tweens_.find(handle);
    if (it != tweens_.end()) {
        tweens_.erase(it);
    }
}

TweenState* AnimationManager::get_tween(TweenHandle handle) {
    auto it = tweens_.find(handle);
    if (it != tweens_.end()) {
        return &it->second;
    }
    return nullptr;
}

void AnimationManager::cancel_tween(TweenHandle handle) {
    auto* tween = get_tween(handle);
    if (tween) {
        tween->cancelled = true;
        tween->active = false;
    }
}

void AnimationManager::pause_tween(TweenHandle handle) {
    auto* tween = get_tween(handle);
    if (tween) {
        tween->paused = true;
    }
}

void AnimationManager::resume_tween(TweenHandle handle) {
    auto* tween = get_tween(handle);
    if (tween) {
        tween->paused = false;
    }
}

bool AnimationManager::same_target(mrb_state* mrb, mrb_value a, mrb_value b) {
    // Compare object identity using mrb_obj_equal
    return mrb_obj_equal(mrb, a, b);
}

void AnimationManager::cancel_tweens_for(mrb_state* mrb, mrb_value target) {
    for (auto& [handle, tween] : tweens_) {
        if (tween.active && same_target(mrb, tween.target, target)) {
            tween.cancelled = true;
            tween.active = false;
        }
    }
}

void AnimationManager::cancel_tweens_for_property(mrb_state* mrb, mrb_value target,
                                                   const std::string& property) {
    for (auto& [handle, tween] : tweens_) {
        if (tween.active &&
            tween.property == property &&
            same_target(mrb, tween.target, target)) {
            tween.cancelled = true;
            tween.active = false;
        }
    }
}

// ============================================================================
// Sprite Animation Management
// ============================================================================

SpriteAnimationHandle AnimationManager::create_animation() {
    SpriteAnimationHandle handle = next_anim_id_++;
    animations_[handle] = SpriteAnimationState();
    return handle;
}

void AnimationManager::destroy_animation(SpriteAnimationHandle handle) {
    auto it = animations_.find(handle);
    if (it != animations_.end()) {
        animations_.erase(it);
    }
}

SpriteAnimationState* AnimationManager::get_animation(SpriteAnimationHandle handle) {
    auto it = animations_.find(handle);
    if (it != animations_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// Property Access via Ruby Duck Typing
// ============================================================================

float AnimationManager::get_property_value(mrb_state* mrb, mrb_value target,
                                            const std::string& property) {
    // Call the getter method (e.g., "x", "alpha", "rotation")
    mrb_value result = mrb_funcall(mrb, target, property.c_str(), 0);

    if (mrb->exc) {
        // Clear exception and return 0
        mrb->exc = nullptr;
        return 0.0f;
    }

    return static_cast<float>(mrb_as_float(mrb, result));
}

void AnimationManager::set_property_value(mrb_state* mrb, mrb_value target,
                                          const std::string& property, float value) {
    // Build setter method name (e.g., "x=", "alpha=", "rotation=")
    std::string setter = property + "=";

    // Call the setter
    mrb_value val = mrb_float_value(mrb, value);
    mrb_funcall(mrb, target, setter.c_str(), 1, val);

    if (mrb->exc) {
        // Clear exception silently
        mrb->exc = nullptr;
    }
}

// ============================================================================
// Callback Invocation
// ============================================================================

void AnimationManager::invoke_callback(mrb_state* mrb, mrb_value callback) {
    if (mrb_nil_p(callback)) return;

    mrb_funcall(mrb, callback, "call", 0);

    if (mrb->exc) {
        // Log error and clear exception
        mrb->exc = nullptr;
    }
}

void AnimationManager::invoke_callback_with_args(mrb_state* mrb, mrb_value callback,
                                                  mrb_value* args, int argc) {
    if (mrb_nil_p(callback)) return;

    // Use mrb_funcall_argv for variable args
    mrb_funcall_argv(mrb, callback, mrb_intern_lit(mrb, "call"), argc, args);

    if (mrb->exc) {
        // Log error and clear exception
        mrb->exc = nullptr;
    }
}

// ============================================================================
// Update Loop
// ============================================================================

void AnimationManager::update(mrb_state* mrb, float dt) {
    tweens_to_complete_.clear();
    animations_to_complete_.clear();

    // Update all active tweens
    for (auto& [handle, tween] : tweens_) {
        if (tween.should_update()) {
            update_tween(mrb, tween, dt);
            if (tween.completed) {
                tweens_to_complete_.push_back(handle);
            }
        }
    }

    // Invoke tween completion callbacks (after iteration to avoid mutation issues)
    for (TweenHandle handle : tweens_to_complete_) {
        TweenState* tween = get_tween(handle);
        if (tween && !mrb_nil_p(tween->on_complete)) {
            invoke_callback(mrb, tween->on_complete);
        }
    }

    // Update all active sprite animations
    for (auto& [handle, anim] : animations_) {
        if (anim.should_update()) {
            update_animation(mrb, anim, dt);
            if (anim.completed) {
                animations_to_complete_.push_back(handle);
            }
        }
    }

    // Invoke animation completion callbacks
    for (SpriteAnimationHandle handle : animations_to_complete_) {
        SpriteAnimationState* anim = get_animation(handle);
        if (anim && !mrb_nil_p(anim->on_complete)) {
            invoke_callback(mrb, anim->on_complete);
        }
    }

    // Cleanup finished tweens/animations
    cleanup_finished(mrb);
}

void AnimationManager::update_tween(mrb_state* mrb, TweenState& tween, float dt) {
    // Handle delay
    if (tween.delay_remaining > 0.0f) {
        tween.delay_remaining -= dt;
        return;
    }

    // Update elapsed time
    tween.elapsed += dt;

    // Calculate normalized time [0, 1]
    float t = tween.duration > 0.0f
        ? std::min(tween.elapsed / tween.duration, 1.0f)
        : 1.0f;

    // Apply easing
    float eased_t = apply_easing(tween.easing, t);

    // Interpolate value
    float value = tween.start_value + (tween.end_value - tween.start_value) * eased_t;

    // Set property on target
    set_property_value(mrb, tween.target, tween.property, value);

    // Call on_update callback if set
    if (!mrb_nil_p(tween.on_update)) {
        mrb_value args[2] = {
            mrb_float_value(mrb, t),
            mrb_float_value(mrb, value)
        };
        invoke_callback_with_args(mrb, tween.on_update, args, 2);
    }

    // Check for completion
    if (t >= 1.0f) {
        tween.completed = true;
        tween.active = false;
    }
}

void AnimationManager::update_animation(mrb_state* mrb, SpriteAnimationState& anim, float dt) {
    anim.elapsed += dt;

    // Check if we need to advance frames
    while (anim.elapsed >= anim.frame_duration && anim.frame_duration > 0.0f) {
        anim.elapsed -= anim.frame_duration;

        int prev_frame = anim.current_frame_index;
        anim.current_frame_index++;

        // Check if we've gone past the last frame
        if (anim.current_frame_index >= static_cast<int>(anim.frames.size())) {
            if (anim.loop) {
                anim.current_frame_index = 0;
            } else {
                // Stay on last frame, mark as completed
                anim.current_frame_index = static_cast<int>(anim.frames.size()) - 1;
                anim.completed = true;
                anim.playing = false;
                break;
            }
        }

        // Call on_frame_change callback if frame actually changed
        if (anim.current_frame_index != prev_frame && !mrb_nil_p(anim.on_frame_change)) {
            mrb_value frame_arg = mrb_fixnum_value(anim.current_frame());
            invoke_callback_with_args(mrb, anim.on_frame_change, &frame_arg, 1);
        }
    }

    // Update sprite's source_rect based on current frame
    SpriteState* sprite = SpriteManager::instance().get(anim.sprite);
    if (sprite && anim.frame_width > 0 && anim.frame_height > 0) {
        int frame = anim.current_frame();
        int col = frame % anim.columns;
        int row = frame / anim.columns;

        sprite->source_rect.x = static_cast<float>(col * anim.frame_width);
        sprite->source_rect.y = static_cast<float>(row * anim.frame_height);
        sprite->source_rect.width = static_cast<float>(anim.frame_width);
        sprite->source_rect.height = static_cast<float>(anim.frame_height);
        sprite->use_source_rect = true;
    }
}

void AnimationManager::cleanup_finished(mrb_state* mrb) {
    // Collect handles to remove
    tweens_to_remove_.clear();
    animations_to_remove_.clear();

    for (auto& [handle, tween] : tweens_) {
        if (tween.completed || tween.cancelled) {
            // Unregister from GC before removal
            if (!mrb_nil_p(tween.ruby_tween_obj)) {
                mrb_gc_unregister(mrb, tween.ruby_tween_obj);
            }
            tweens_to_remove_.push_back(handle);
        }
    }

    for (auto& [handle, anim] : animations_) {
        // Only remove non-looping animations that completed
        // Looping animations stay until explicitly destroyed
        if (anim.completed && !anim.loop) {
            if (!mrb_nil_p(anim.ruby_anim_obj)) {
                mrb_gc_unregister(mrb, anim.ruby_anim_obj);
            }
            animations_to_remove_.push_back(handle);
        }
    }

    // Remove collected handles
    for (TweenHandle handle : tweens_to_remove_) {
        tweens_.erase(handle);
    }

    for (SpriteAnimationHandle handle : animations_to_remove_) {
        animations_.erase(handle);
    }
}

// ============================================================================
// Cleanup
// ============================================================================

void AnimationManager::clear(mrb_state* mrb) {
    // Unregister all Ruby objects from GC
    for (auto& [handle, tween] : tweens_) {
        if (!mrb_nil_p(tween.ruby_tween_obj)) {
            mrb_gc_unregister(mrb, tween.ruby_tween_obj);
        }
    }

    for (auto& [handle, anim] : animations_) {
        if (!mrb_nil_p(anim.ruby_anim_obj)) {
            mrb_gc_unregister(mrb, anim.ruby_anim_obj);
        }
    }

    tweens_.clear();
    animations_.clear();

    // Don't reset IDs - keeps handles unique across reloads
}

} // namespace animation
} // namespace gmr
