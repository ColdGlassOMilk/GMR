#ifndef GMR_ANIMATION_TWEEN_HPP
#define GMR_ANIMATION_TWEEN_HPP

#include "gmr/types.hpp"
#include "gmr/animation/easing.hpp"
#include <mruby.h>
#include <string>

namespace gmr {
namespace animation {

// Internal state for a single tween
struct TweenState {
    // Target object and property
    mrb_value target;           // Ruby object being tweened
    std::string property;       // Property name (e.g., "x", "alpha", "rotation")

    // Tween values
    float start_value{0.0f};
    float end_value{0.0f};

    // Timing
    float duration{0.0f};       // Total duration in seconds
    float elapsed{0.0f};        // Time elapsed since start
    float delay{0.0f};          // Initial delay before tweening
    float delay_remaining{0.0f};// Remaining delay time

    // Easing
    EasingType easing{EasingType::LINEAR};

    // Callbacks (Ruby procs/blocks)
    mrb_value on_complete;      // Proc to call when tween completes
    mrb_value on_update;        // Proc to call each update (receives t, value)

    // The Ruby Tween object itself (for GC registration)
    mrb_value ruby_tween_obj;

    // State flags
    bool active{true};          // Is this tween currently running?
    bool paused{false};         // Is this tween paused?
    bool completed{false};      // Has this tween finished?
    bool cancelled{false};      // Was this tween cancelled?

    TweenState()
        : target(mrb_nil_value())
        , on_complete(mrb_nil_value())
        , on_update(mrb_nil_value())
        , ruby_tween_obj(mrb_nil_value()) {}

    // Check if the tween should be updated this frame
    bool should_update() const {
        return active && !paused && !completed && !cancelled;
    }
};

} // namespace animation
} // namespace gmr

#endif
