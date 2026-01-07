#ifndef GMR_ANIMATION_EASING_HPP
#define GMR_ANIMATION_EASING_HPP

#include <mruby.h>

namespace gmr {
namespace animation {

// Easing function types
// All easing functions take t in [0,1] and return a value (usually [0,1], but elastic/back overshoot)
enum class EasingType {
    LINEAR = 0,

    // Quadratic
    IN_QUAD,
    OUT_QUAD,
    IN_OUT_QUAD,

    // Cubic
    IN_CUBIC,
    OUT_CUBIC,
    IN_OUT_CUBIC,

    // Quartic
    IN_QUART,
    OUT_QUART,
    IN_OUT_QUART,

    // Quintic
    IN_QUINT,
    OUT_QUINT,
    IN_OUT_QUINT,

    // Sine
    IN_SINE,
    OUT_SINE,
    IN_OUT_SINE,

    // Exponential
    IN_EXPO,
    OUT_EXPO,
    IN_OUT_EXPO,

    // Circular
    IN_CIRC,
    OUT_CIRC,
    IN_OUT_CIRC,

    // Back (overshoot)
    IN_BACK,
    OUT_BACK,
    IN_OUT_BACK,

    // Elastic
    IN_ELASTIC,
    OUT_ELASTIC,
    IN_OUT_ELASTIC,

    // Bounce
    IN_BOUNCE,
    OUT_BOUNCE,
    IN_OUT_BOUNCE
};

// Apply easing function to normalized time t [0,1]
// Returns eased value (usually [0,1], but elastic/back may overshoot)
float apply_easing(EasingType type, float t);

// Convert mruby symbol to EasingType
// Returns LINEAR if symbol is not recognized
EasingType symbol_to_easing(mrb_state* mrb, mrb_sym sym);

// Convert EasingType to symbol name (for debugging)
const char* easing_to_name(EasingType type);

} // namespace animation
} // namespace gmr

#endif
