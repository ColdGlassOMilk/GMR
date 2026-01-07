#ifndef GMR_ANIMATION_ANIMATOR_HPP
#define GMR_ANIMATION_ANIMATOR_HPP

#include "gmr/types.hpp"
#include <mruby.h>
#include <unordered_map>
#include <set>
#include <string>

namespace gmr {
namespace animation {

// Transition mode when switching animations
enum class AnimationTransition {
    Immediate,      // Stop current, start new immediately
    FinishCurrent   // Queue new animation, play after current finishes
};

// Entry for a named animation within an Animator
struct AnimationEntry {
    SpriteAnimationHandle animation{INVALID_HANDLE};
    mrb_value ruby_animation;  // GC protection for Ruby SpriteAnimation object

    AnimationEntry() : ruby_animation(mrb_nil_value()) {}
};

// Animator state - manages multiple named animations for a sprite
struct AnimatorState {
    // Target sprite
    SpriteHandle sprite{INVALID_HANDLE};
    mrb_value sprite_ref;  // GC protection

    // Named animations: name -> animation entry
    std::unordered_map<std::string, AnimationEntry> animations;

    // Transition rules: from_animation -> set of allowed destination animations
    std::unordered_map<std::string, std::set<std::string>> transitions;

    // Global transitions: animations that can interrupt any other animation
    std::set<std::string> global_transitions;

    // Current playback state
    std::string current_animation;   // Name of currently playing animation
    std::string queued_animation;    // Name of animation to play next (for FinishCurrent mode)

    // Default transition mode
    AnimationTransition default_transition{AnimationTransition::Immediate};

    // Shared spritesheet parameters (used when creating animations)
    int frame_width{0};
    int frame_height{0};
    int columns{1};

    // State
    bool active{true};

    // Callbacks
    mrb_value on_complete;  // Called when any animation completes (legacy)
    std::unordered_map<std::string, mrb_value> on_complete_callbacks;  // Per-animation callbacks

    // Ruby object reference (GC protection)
    mrb_value ruby_animator_obj;

    AnimatorState()
        : sprite_ref(mrb_nil_value())
        , on_complete(mrb_nil_value())
        , ruby_animator_obj(mrb_nil_value()) {}

    // Get current animation handle
    SpriteAnimationHandle current_handle() const {
        auto it = animations.find(current_animation);
        return it != animations.end() ? it->second.animation : INVALID_HANDLE;
    }

    // Check if animation exists
    bool has_animation(const std::string& name) const {
        return animations.find(name) != animations.end();
    }

    // Check if transition from current animation to target is allowed
    bool can_transition(const std::string& to) const {
        // No current animation = can go anywhere
        if (current_animation.empty()) return true;

        // Global transitions can always be played
        if (global_transitions.count(to)) return true;

        // Check explicit rules
        auto it = transitions.find(current_animation);
        if (it != transitions.end()) {
            return it->second.count(to) > 0;
        }

        // No rules defined = allow all (backwards compatible)
        return transitions.empty();
    }
};

} // namespace animation
} // namespace gmr

#endif
