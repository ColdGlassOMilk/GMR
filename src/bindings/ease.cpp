#include "gmr/bindings/ease.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/animation/easing.hpp"
#include <mruby/class.h>

namespace gmr {
namespace bindings {

/// @module GMR::Ease
/// @description Provides easing functions for smooth animations.
///   Easing functions control the rate of change during an animation,
///   making movements feel more natural and polished.
/// @example # Common easing patterns for different UI/game scenarios
///   class UIAnimations
///     # Menu item slide in - fast start, gentle stop
///     def slide_in_menu_item(item, delay)
///       item.x = -200  # Start off-screen
///       GMR::Tween.to(item, :x, 50, duration: 0.3, ease: :out_cubic, delay: delay)
///     end
///
///     # Popup appear - overshoot then settle (feels bouncy)
///     def popup_appear(popup)
///       popup.scale_x = 0
///       popup.scale_y = 0
///       GMR::Tween.to(popup, :scale_x, 1.0, duration: 0.4, ease: :out_back)
///       GMR::Tween.to(popup, :scale_y, 1.0, duration: 0.4, ease: :out_back)
///     end
///
///     # Button press - quick squash then restore
///     def button_press(button)
///       GMR::Tween.to(button, :scale_y, 0.9, duration: 0.05, ease: :out_quad)
///         .on_complete do
///           GMR::Tween.to(button, :scale_y, 1.0, duration: 0.15, ease: :out_elastic)
///         end
///     end
///
///     # Notification bounce in then auto-dismiss
///     def show_notification(notif)
///       notif.y = -50
///       GMR::Tween.to(notif, :y, 20, duration: 0.5, ease: :out_bounce)
///         .on_complete do
///           GMR::Tween.to(notif, :y, -50, duration: 0.3, ease: :in_quad, delay: 2.0)
///         end
///     end
///   end
/// @example # Custom interpolation without tweens
///   class Projectile
///     def initialize(start_x, start_y, target_x, target_y)
///       @start_x, @start_y = start_x, start_y
///       @target_x, @target_y = target_x, target_y
///       @elapsed = 0
///       @duration = 0.5
///     end
///
///     def update(dt)
///       @elapsed += dt
///       t = [@elapsed / @duration, 1.0].min  # Clamp to 0-1
///
///       # Use easing for arc trajectory
///       # Horizontal: linear movement
///       eased_x = GMR::Ease.apply(:linear, t)
///       # Vertical: parabolic arc using in_out_quad
///       eased_y = GMR::Ease.apply(:in_out_quad, t)
///
///       @x = @start_x + (@target_x - @start_x) * eased_x
///       @y = @start_y + (@target_y - @start_y) * eased_y - Math.sin(t * Math::PI) * 100  # Arc height
///     end
///   end
/// @example # Easing comparison visualization
///   class EasingDemo
///     EASINGS = [:linear, :in_quad, :out_quad, :in_out_quad,
///                :in_cubic, :out_cubic, :out_back, :out_elastic, :out_bounce]
///
///     def initialize
///       @time = 0
///       @duration = 2.0
///     end
///
///     def update(dt)
///       @time += dt
///       @time = 0 if @time > @duration  # Loop
///     end
///
///     def draw
///       t = @time / @duration
///
///       EASINGS.each_with_index do |ease_name, i|
///         y = 50 + i * 50
///         eased = GMR::Ease.apply(ease_name, t)
///         x = 150 + eased * 400
///
///         GMR::Graphics.draw_text(ease_name.to_s, 10, y, 14, [200, 200, 200])
///         GMR::Graphics.draw_circle(x, y + 8, 8, [100, 200, 255])
///       end
///     end
///   end

// ============================================================================
// Easing Symbol Getters
// Each returns a symbol that can be passed to Tween.to
// ============================================================================

/// @method linear
/// @description Linear interpolation (no easing). Constant speed throughout.
/// @returns [Symbol] :linear
static mrb_value mrb_ease_linear(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "linear"));
}

// Quadratic

/// @method in_quad
/// @description Quadratic ease-in. Starts slow, accelerates.
/// @returns [Symbol] :in_quad
static mrb_value mrb_ease_in_quad(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_quad"));
}

/// @method out_quad
/// @description Quadratic ease-out. Starts fast, decelerates.
/// @returns [Symbol] :out_quad
static mrb_value mrb_ease_out_quad(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_quad"));
}

/// @method in_out_quad
/// @description Quadratic ease-in-out. Slow start and end, fast middle.
/// @returns [Symbol] :in_out_quad
static mrb_value mrb_ease_in_out_quad(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_quad"));
}

// Cubic

/// @method in_cubic
/// @description Cubic ease-in. Starts slow, accelerates more than quadratic.
/// @returns [Symbol] :in_cubic
static mrb_value mrb_ease_in_cubic(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_cubic"));
}

/// @method out_cubic
/// @description Cubic ease-out. Starts fast, decelerates smoothly. Most common.
/// @returns [Symbol] :out_cubic
static mrb_value mrb_ease_out_cubic(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_cubic"));
}

/// @method in_out_cubic
/// @description Cubic ease-in-out. Smooth acceleration and deceleration.
/// @returns [Symbol] :in_out_cubic
static mrb_value mrb_ease_in_out_cubic(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_cubic"));
}

// Quartic

/// @method in_quart
/// @description Quartic ease-in. Dramatic slow start.
/// @returns [Symbol] :in_quart
static mrb_value mrb_ease_in_quart(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_quart"));
}

/// @method out_quart
/// @description Quartic ease-out. Dramatic fast start.
/// @returns [Symbol] :out_quart
static mrb_value mrb_ease_out_quart(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_quart"));
}

/// @method in_out_quart
/// @description Quartic ease-in-out.
/// @returns [Symbol] :in_out_quart
static mrb_value mrb_ease_in_out_quart(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_quart"));
}

// Sine

/// @method in_sine
/// @description Sinusoidal ease-in. Gentle acceleration.
/// @returns [Symbol] :in_sine
static mrb_value mrb_ease_in_sine(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_sine"));
}

/// @method out_sine
/// @description Sinusoidal ease-out. Gentle deceleration.
/// @returns [Symbol] :out_sine
static mrb_value mrb_ease_out_sine(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_sine"));
}

/// @method in_out_sine
/// @description Sinusoidal ease-in-out. Very smooth, subtle effect.
/// @returns [Symbol] :in_out_sine
static mrb_value mrb_ease_in_out_sine(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_sine"));
}

// Exponential

/// @method in_expo
/// @description Exponential ease-in. Extreme slow start.
/// @returns [Symbol] :in_expo
static mrb_value mrb_ease_in_expo(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_expo"));
}

/// @method out_expo
/// @description Exponential ease-out. Extreme fast start.
/// @returns [Symbol] :out_expo
static mrb_value mrb_ease_out_expo(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_expo"));
}

/// @method in_out_expo
/// @description Exponential ease-in-out.
/// @returns [Symbol] :in_out_expo
static mrb_value mrb_ease_in_out_expo(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_expo"));
}

// Circular

/// @method in_circ
/// @description Circular ease-in.
/// @returns [Symbol] :in_circ
static mrb_value mrb_ease_in_circ(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_circ"));
}

/// @method out_circ
/// @description Circular ease-out.
/// @returns [Symbol] :out_circ
static mrb_value mrb_ease_out_circ(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_circ"));
}

/// @method in_out_circ
/// @description Circular ease-in-out.
/// @returns [Symbol] :in_out_circ
static mrb_value mrb_ease_in_out_circ(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_circ"));
}

// Back (overshoot)

/// @method in_back
/// @description Back ease-in. Pulls back before moving forward.
/// @returns [Symbol] :in_back
static mrb_value mrb_ease_in_back(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_back"));
}

/// @method out_back
/// @description Back ease-out. Overshoots target, then settles. Great for UI.
/// @returns [Symbol] :out_back
static mrb_value mrb_ease_out_back(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_back"));
}

/// @method in_out_back
/// @description Back ease-in-out. Overshoots on both ends.
/// @returns [Symbol] :in_out_back
static mrb_value mrb_ease_in_out_back(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_back"));
}

// Elastic

/// @method in_elastic
/// @description Elastic ease-in. Spring-like wind-up.
/// @returns [Symbol] :in_elastic
static mrb_value mrb_ease_in_elastic(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_elastic"));
}

/// @method out_elastic
/// @description Elastic ease-out. Spring-like bounce at end. Great for attention.
/// @returns [Symbol] :out_elastic
static mrb_value mrb_ease_out_elastic(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_elastic"));
}

/// @method in_out_elastic
/// @description Elastic ease-in-out. Spring on both ends.
/// @returns [Symbol] :in_out_elastic
static mrb_value mrb_ease_in_out_elastic(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_elastic"));
}

// Bounce

/// @method in_bounce
/// @description Bounce ease-in. Bouncing at start.
/// @returns [Symbol] :in_bounce
static mrb_value mrb_ease_in_bounce(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_bounce"));
}

/// @method out_bounce
/// @description Bounce ease-out. Bouncing at end, like a ball.
/// @returns [Symbol] :out_bounce
static mrb_value mrb_ease_out_bounce(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "out_bounce"));
}

/// @method in_out_bounce
/// @description Bounce ease-in-out. Bouncing on both ends.
/// @returns [Symbol] :in_out_bounce
static mrb_value mrb_ease_in_out_bounce(mrb_state* mrb, mrb_value) {
    return mrb_symbol_value(mrb_intern_lit(mrb, "in_out_bounce"));
}

// ============================================================================
// Manual Easing Application
// ============================================================================

/// @method apply
/// @description Apply an easing function to a normalized time value.
///   Useful for custom interpolation outside of tweens.
/// @param ease [Symbol] The easing function to apply
/// @param t [Float] Normalized time (0.0 to 1.0)
/// @returns [Float] Eased value (usually 0.0 to 1.0, but may overshoot for elastic/back)
/// @example progress = GMR::Ease.apply(:out_cubic, 0.5)  # => 0.875
/// @example # Custom interpolation
///   t = elapsed / duration
///   eased_t = GMR::Ease.apply(:out_back, t)
///   current_x = start_x + (end_x - start_x) * eased_t
static mrb_value mrb_ease_apply(mrb_state* mrb, mrb_value) {
    mrb_sym ease_sym;
    mrb_float t;
    mrb_get_args(mrb, "nf", &ease_sym, &t);

    animation::EasingType easing = animation::symbol_to_easing(mrb, ease_sym);
    float result = animation::apply_easing(easing, static_cast<float>(t));

    return mrb_float_value(mrb, result);
}

// ============================================================================
// Registration
// ============================================================================

void register_ease(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* ease = mrb_define_module_under(mrb, gmr, "Ease");

    // Linear
    mrb_define_module_function(mrb, ease, "linear", mrb_ease_linear, MRB_ARGS_NONE());

    // Quadratic
    mrb_define_module_function(mrb, ease, "in_quad", mrb_ease_in_quad, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_quad", mrb_ease_out_quad, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_quad", mrb_ease_in_out_quad, MRB_ARGS_NONE());

    // Cubic
    mrb_define_module_function(mrb, ease, "in_cubic", mrb_ease_in_cubic, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_cubic", mrb_ease_out_cubic, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_cubic", mrb_ease_in_out_cubic, MRB_ARGS_NONE());

    // Quartic
    mrb_define_module_function(mrb, ease, "in_quart", mrb_ease_in_quart, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_quart", mrb_ease_out_quart, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_quart", mrb_ease_in_out_quart, MRB_ARGS_NONE());

    // Sine
    mrb_define_module_function(mrb, ease, "in_sine", mrb_ease_in_sine, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_sine", mrb_ease_out_sine, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_sine", mrb_ease_in_out_sine, MRB_ARGS_NONE());

    // Exponential
    mrb_define_module_function(mrb, ease, "in_expo", mrb_ease_in_expo, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_expo", mrb_ease_out_expo, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_expo", mrb_ease_in_out_expo, MRB_ARGS_NONE());

    // Circular
    mrb_define_module_function(mrb, ease, "in_circ", mrb_ease_in_circ, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_circ", mrb_ease_out_circ, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_circ", mrb_ease_in_out_circ, MRB_ARGS_NONE());

    // Back
    mrb_define_module_function(mrb, ease, "in_back", mrb_ease_in_back, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_back", mrb_ease_out_back, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_back", mrb_ease_in_out_back, MRB_ARGS_NONE());

    // Elastic
    mrb_define_module_function(mrb, ease, "in_elastic", mrb_ease_in_elastic, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_elastic", mrb_ease_out_elastic, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_elastic", mrb_ease_in_out_elastic, MRB_ARGS_NONE());

    // Bounce
    mrb_define_module_function(mrb, ease, "in_bounce", mrb_ease_in_bounce, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "out_bounce", mrb_ease_out_bounce, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, ease, "in_out_bounce", mrb_ease_in_out_bounce, MRB_ARGS_NONE());

    // Manual application
    mrb_define_module_function(mrb, ease, "apply", mrb_ease_apply, MRB_ARGS_REQ(2));
}

} // namespace bindings
} // namespace gmr
