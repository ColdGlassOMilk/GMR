#ifndef GMR_ANIMATION_MANAGER_HPP
#define GMR_ANIMATION_MANAGER_HPP

#include "gmr/animation/tween.hpp"
#include "gmr/animation/sprite_animation.hpp"
#include <mruby.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace gmr {
namespace animation {

// Singleton manager for all animations (tweens and sprite animations)
// Updated automatically in the main loop before Ruby update()
class AnimationManager {
public:
    static AnimationManager& instance();

    // Per-frame update (called from main.cpp before Ruby update)
    void update(mrb_state* mrb, float dt);

    // === Tween Management ===

    // Create a new tween, returns handle
    TweenHandle create_tween();

    // Destroy a tween by handle
    void destroy_tween(TweenHandle handle);

    // Get tween state by handle (returns nullptr if not found)
    TweenState* get_tween(TweenHandle handle);

    // Cancel a specific tween
    void cancel_tween(TweenHandle handle);

    // Pause a tween
    void pause_tween(TweenHandle handle);

    // Resume a paused tween
    void resume_tween(TweenHandle handle);

    // Cancel all tweens for a specific target object
    void cancel_tweens_for(mrb_state* mrb, mrb_value target);

    // Cancel tweens for a specific target+property combination
    // Called automatically when creating a new tween for the same target+property
    void cancel_tweens_for_property(mrb_state* mrb, mrb_value target, const std::string& property);

    // === Sprite Animation Management ===

    // Create a new sprite animation, returns handle
    SpriteAnimationHandle create_animation();

    // Destroy a sprite animation by handle
    void destroy_animation(SpriteAnimationHandle handle);

    // Get sprite animation state by handle (returns nullptr if not found)
    SpriteAnimationState* get_animation(SpriteAnimationHandle handle);

    // === Cleanup ===

    // Clear all tweens and animations (for hot reload/cleanup)
    void clear(mrb_state* mrb);

    // === Debug Info ===
    size_t tween_count() const { return tweens_.size(); }
    size_t animation_count() const { return animations_.size(); }

    // === Property Access (public for bindings) ===
    float get_property_value(mrb_state* mrb, mrb_value target, const std::string& property);
    void set_property_value(mrb_state* mrb, mrb_value target,
                           const std::string& property, float value);

private:
    AnimationManager() = default;
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    // Update helpers
    void update_tween(mrb_state* mrb, TweenState& tween, float dt);
    void update_animation(mrb_state* mrb, SpriteAnimationState& anim, float dt);

    // Callback invocation with exception handling
    void invoke_callback(mrb_state* mrb, mrb_value callback);
    void invoke_callback_with_args(mrb_state* mrb, mrb_value callback,
                                   mrb_value* args, int argc);

    // Check if two Ruby values refer to the same object
    bool same_target(mrb_state* mrb, mrb_value a, mrb_value b);

    // Cleanup completed/cancelled tweens and animations
    void cleanup_finished(mrb_state* mrb);

    // Storage
    std::unordered_map<TweenHandle, TweenState> tweens_;
    std::unordered_map<SpriteAnimationHandle, SpriteAnimationState> animations_;

    // ID generators
    TweenHandle next_tween_id_{0};
    SpriteAnimationHandle next_anim_id_{0};

    // Deferred operations (to avoid mutation during iteration)
    std::vector<TweenHandle> tweens_to_complete_;
    std::vector<SpriteAnimationHandle> animations_to_complete_;
    std::vector<TweenHandle> tweens_to_remove_;
    std::vector<SpriteAnimationHandle> animations_to_remove_;
};

} // namespace animation
} // namespace gmr

#endif
