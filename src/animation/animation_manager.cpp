#include "gmr/animation/animation_manager.hpp"
#include "gmr/sprite.hpp"
#include "gmr/scripting/helpers.hpp"
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
        if (!tween.active) continue;
        if (mrb_nil_p(tween.target)) continue;  // Skip invalid targets
        if (same_target(mrb, tween.target, target)) {
            tween.cancelled = true;
            tween.active = false;
        }
    }
}

void AnimationManager::cancel_tweens_for_property(mrb_state* mrb, mrb_value target,
                                                   const std::string& property) {
    for (auto& [handle, tween] : tweens_) {
        if (!tween.active) continue;
        if (mrb_nil_p(tween.target)) continue;  // Skip invalid targets
        if (tween.property == property && same_target(mrb, tween.target, target)) {
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
// Animator Management
// ============================================================================

AnimatorHandle AnimationManager::create_animator() {
    AnimatorHandle handle = next_animator_id_++;
    animators_[handle] = AnimatorState();
    return handle;
}

void AnimationManager::destroy_animator(AnimatorHandle handle) {
    auto it = animators_.find(handle);
    if (it != animators_.end()) {
        animators_.erase(it);
    }
}

AnimatorState* AnimationManager::get_animator(AnimatorHandle handle) {
    auto it = animators_.find(handle);
    if (it != animators_.end()) {
        return &it->second;
    }
    return nullptr;
}

void AnimationManager::update_animators(mrb_state* mrb) {
    for (auto& [handle, animator] : animators_) {
        if (!animator.active) continue;

        // Check if current animation finished
        SpriteAnimationHandle current = animator.current_handle();
        if (current == INVALID_HANDLE) continue;

        SpriteAnimationState* anim = get_animation(current);
        if (!anim || !anim->completed) continue;

        // Animation just completed - invoke per-animation callback if registered
        std::string completed_anim_name = animator.current_animation;
        auto callback_it = animator.on_complete_callbacks.find(completed_anim_name);
        if (callback_it != animator.on_complete_callbacks.end() && !mrb_nil_p(callback_it->second)) {
            invoke_callback(mrb, callback_it->second);
        }

        // Also invoke legacy global on_complete callback (backwards compatibility)
        if (!mrb_nil_p(animator.on_complete)) {
            invoke_callback(mrb, animator.on_complete);
        }

        // Handle queued animation transition (for FinishCurrent mode)
        if (!animator.queued_animation.empty()) {
            if (animator.can_transition(animator.queued_animation)) {
                // Stop current
                anim->playing = false;

                // Start queued animation
                auto queued_it = animator.animations.find(animator.queued_animation);
                if (queued_it != animator.animations.end()) {
                    SpriteAnimationState* queued_anim = get_animation(queued_it->second.animation);
                    if (queued_anim) {
                        queued_anim->current_frame_index = 0;
                        queued_anim->elapsed = 0.0f;
                        queued_anim->playing = true;
                        queued_anim->completed = false;
                    }
                }
                animator.current_animation = animator.queued_animation;
            }
            animator.queued_animation.clear();
        }

        // Reset the completed flag so we don't fire callback again next frame
        anim->completed = false;
    }
}

// ============================================================================
// Property Access via Ruby Duck Typing
// ============================================================================

float AnimationManager::get_property_value(mrb_state* mrb, mrb_value target,
                                            const std::string& property) {
    // Call the getter method (e.g., "x", "alpha", "rotation") - protected
    mrb_value result = scripting::safe_method_call(mrb, target, property.c_str());
    return static_cast<float>(mrb_as_float(mrb, result));
}

void AnimationManager::set_property_value(mrb_state* mrb, mrb_value target,
                                          const std::string& property, float value) {
    // Build setter method name (e.g., "x=", "alpha=", "rotation=")
    std::string setter = property + "=";

    // Call the setter - protected
    mrb_value val = mrb_float_value(mrb, value);
    scripting::safe_method_call(mrb, target, setter.c_str(), {val});
}

// ============================================================================
// Callback Invocation
// ============================================================================

void AnimationManager::invoke_callback(mrb_state* mrb, mrb_value callback) {
    if (mrb_nil_p(callback)) return;

    scripting::safe_method_call(mrb, callback, "call");
}

void AnimationManager::invoke_callback_with_args(mrb_state* mrb, mrb_value callback,
                                                  mrb_value* args, int argc) {
    if (mrb_nil_p(callback)) return;

    // Build args vector for safe call
    std::vector<mrb_value> args_vec(args, args + argc);
    scripting::safe_method_call(mrb, callback, "call", args_vec);
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

    // Update animators (check for queued transitions)
    update_animators(mrb);

    // Cleanup finished tweens/animations
    cleanup_finished(mrb);
}

void AnimationManager::update_tween(mrb_state* mrb, TweenState& tween, float dt) {
    // Validate target before any property access
    if (mrb_nil_p(tween.target)) {
        tween.cancelled = true;
        tween.active = false;
        return;
    }

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

    for (auto& [handle, animator] : animators_) {
        if (!mrb_nil_p(animator.ruby_animator_obj)) {
            mrb_gc_unregister(mrb, animator.ruby_animator_obj);
        }
    }

    tweens_.clear();
    animations_.clear();
    animators_.clear();

    // Don't reset IDs - keeps handles unique across reloads
}

} // namespace animation
} // namespace gmr
