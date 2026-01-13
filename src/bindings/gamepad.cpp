#include "gmr/bindings/gamepad.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include "raylib.h"

namespace gmr {
namespace bindings {

// ============================================================================
// GMR::Gamepad Module
// ============================================================================

/// @module GMR::Gamepad
/// @description Direct gamepad access for controller input. Supports up to 4 gamepads.
///   Provides button/axis polling, dead zone handling, and rumble support.
/// @example # Check if gamepad is connected
///   if GMR::Gamepad.connected?(0)
///     # Read analog stick with dead zone applied
///     move_x = GMR::Gamepad.axis(0, :left_x)
///     move_y = GMR::Gamepad.axis(0, :left_y)
///
///     # Check face buttons
///     player.jump if GMR::Gamepad.pressed?(0, :a)
///     player.attack if GMR::Gamepad.pressed?(0, :x)
///
///     # Rumble on hit
///     GMR::Gamepad.vibrate(0, left: 0.5, right: 0.3, duration: 0.2)
///   end

// Global dead zone settings
static float g_inner_dead_zone = 0.15f;
static float g_outer_dead_zone = 0.95f;

// Apply dead zone to axis value
static float apply_dead_zone(float value) {
    float abs_val = value < 0 ? -value : value;

    if (abs_val < g_inner_dead_zone) {
        return 0.0f;
    }

    if (abs_val > g_outer_dead_zone) {
        return value < 0 ? -1.0f : 1.0f;
    }

    // Linear interpolation between dead zones
    float sign = value < 0 ? -1.0f : 1.0f;
    float range = g_outer_dead_zone - g_inner_dead_zone;
    return sign * (abs_val - g_inner_dead_zone) / range;
}

// ============================================================================
// Connection Status
// ============================================================================

/// @function count
/// @description Get the number of connected gamepads (0-4).
/// @returns [Integer] Number of connected gamepads
/// @example num_players = GMR::Gamepad.count
static mrb_value mrb_gamepad_count(mrb_state* mrb, mrb_value) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (IsGamepadAvailable(i)) count++;
    }
    return mrb_fixnum_value(count);
}

/// @function connected?
/// @description Check if a specific gamepad is connected.
/// @param gamepad [Integer] Gamepad index (0-3)
/// @returns [Boolean] true if the gamepad is connected
/// @example if GMR::Gamepad.connected?(0) then ... end
static mrb_value mrb_gamepad_connected(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_get_args(mrb, "i", &gamepad);
    return to_mrb_bool(mrb, IsGamepadAvailable(static_cast<int>(gamepad)));
}

/// @function name
/// @description Get the internal name of a connected gamepad.
/// @param gamepad [Integer] Gamepad index (0-3)
/// @returns [String, nil] Gamepad name, or nil if not connected
/// @example puts GMR::Gamepad.name(0)  # => "Xbox Controller"
static mrb_value mrb_gamepad_name(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_get_args(mrb, "i", &gamepad);

    if (!IsGamepadAvailable(static_cast<int>(gamepad))) {
        return mrb_nil_value();
    }

    const char* name = GetGamepadName(static_cast<int>(gamepad));
    return mrb_str_new_cstr(mrb, name);
}

// ============================================================================
// Button State
// ============================================================================

/// @function down?
/// @description Check if a gamepad button is currently held down.
/// @param gamepad [Integer] Gamepad index (0-3)
/// @param button [Symbol, Integer] Button to check (:a, :b, :x, :y, :lb, :rb, etc.)
/// @returns [Boolean] true if the button is held
/// @example if GMR::Gamepad.down?(0, :a) then player.accelerate end
static mrb_value mrb_gamepad_down(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_value button_arg;
    mrb_get_args(mrb, "io", &gamepad, &button_arg);

    int button = parse_gamepad_button_arg(mrb, button_arg);
    return to_mrb_bool(mrb, IsGamepadButtonDown(static_cast<int>(gamepad), button));
}

/// @function pressed?
/// @description Check if a gamepad button was just pressed this frame.
/// @param gamepad [Integer] Gamepad index (0-3)
/// @param button [Symbol, Integer] Button to check
/// @returns [Boolean] true if the button was just pressed
/// @example if GMR::Gamepad.pressed?(0, :a) then player.jump end
static mrb_value mrb_gamepad_pressed(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_value button_arg;
    mrb_get_args(mrb, "io", &gamepad, &button_arg);

    int button = parse_gamepad_button_arg(mrb, button_arg);
    return to_mrb_bool(mrb, IsGamepadButtonPressed(static_cast<int>(gamepad), button));
}

/// @function released?
/// @description Check if a gamepad button was just released this frame.
/// @param gamepad [Integer] Gamepad index (0-3)
/// @param button [Symbol, Integer] Button to check
/// @returns [Boolean] true if the button was just released
/// @example if GMR::Gamepad.released?(0, :rt) then player.fire_charged_shot end
static mrb_value mrb_gamepad_released(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_value button_arg;
    mrb_get_args(mrb, "io", &gamepad, &button_arg);

    int button = parse_gamepad_button_arg(mrb, button_arg);
    return to_mrb_bool(mrb, IsGamepadButtonReleased(static_cast<int>(gamepad), button));
}

// ============================================================================
// Axis Input
// ============================================================================

/// @function axis
/// @description Get the value of a gamepad axis with dead zone applied.
///   Returns a float between -1.0 and 1.0, with dead zone filtering.
/// @param gamepad [Integer] Gamepad index (0-3)
/// @param axis [Symbol, Integer] Axis to read (:left_x, :left_y, :right_x, :right_y, etc.)
/// @returns [Float] Axis value (-1.0 to 1.0)
/// @example move_x = GMR::Gamepad.axis(0, :left_x)
///   move_y = GMR::Gamepad.axis(0, :left_y)
///   player.move(move_x, move_y)
static mrb_value mrb_gamepad_axis(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_value axis_arg;
    mrb_get_args(mrb, "io", &gamepad, &axis_arg);

    int axis = parse_gamepad_axis_arg(mrb, axis_arg);
    float value = GetGamepadAxisMovement(static_cast<int>(gamepad), axis);

    // Apply dead zone
    value = apply_dead_zone(value);

    return mrb_float_value(mrb, value);
}

/// @function axis_raw
/// @description Get the raw value of a gamepad axis without dead zone filtering.
/// @param gamepad [Integer] Gamepad index (0-3)
/// @param axis [Symbol, Integer] Axis to read
/// @returns [Float] Raw axis value (-1.0 to 1.0)
/// @example raw_x = GMR::Gamepad.axis_raw(0, :left_x)
static mrb_value mrb_gamepad_axis_raw(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_value axis_arg;
    mrb_get_args(mrb, "io", &gamepad, &axis_arg);

    int axis = parse_gamepad_axis_arg(mrb, axis_arg);
    float value = GetGamepadAxisMovement(static_cast<int>(gamepad), axis);

    return mrb_float_value(mrb, value);
}

// ============================================================================
// Dead Zone Configuration
// ============================================================================

/// @function dead_zone
/// @description Get the current inner dead zone threshold.
/// @returns [Float] Inner dead zone (0.0 to 1.0)
/// @example puts GMR::Gamepad.dead_zone  # => 0.15
static mrb_value mrb_gamepad_get_dead_zone(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, g_inner_dead_zone);
}

/// @function dead_zone=
/// @description Set the inner dead zone threshold. Axis values below this
///   threshold are treated as zero.
/// @param value [Float] Inner dead zone (0.0 to 1.0)
/// @returns [Float] The new dead zone value
/// @example GMR::Gamepad.dead_zone = 0.2
static mrb_value mrb_gamepad_set_dead_zone(mrb_state* mrb, mrb_value) {
    mrb_float value;
    mrb_get_args(mrb, "f", &value);

    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    g_inner_dead_zone = static_cast<float>(value);
    return mrb_float_value(mrb, g_inner_dead_zone);
}

/// @function outer_dead_zone
/// @description Get the current outer dead zone threshold.
/// @returns [Float] Outer dead zone (0.0 to 1.0)
/// @example puts GMR::Gamepad.outer_dead_zone  # => 0.95
static mrb_value mrb_gamepad_get_outer_dead_zone(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, g_outer_dead_zone);
}

/// @function outer_dead_zone=
/// @description Set the outer dead zone threshold. Axis values above this
///   threshold are treated as maximum (1.0 or -1.0).
/// @param value [Float] Outer dead zone (0.0 to 1.0)
/// @returns [Float] The new outer dead zone value
/// @example GMR::Gamepad.outer_dead_zone = 0.98
static mrb_value mrb_gamepad_set_outer_dead_zone(mrb_state* mrb, mrb_value) {
    mrb_float value;
    mrb_get_args(mrb, "f", &value);

    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    g_outer_dead_zone = static_cast<float>(value);
    return mrb_float_value(mrb, g_outer_dead_zone);
}

// ============================================================================
// Vibration / Rumble
// ============================================================================

/// @function vibrate
/// @description Trigger gamepad vibration (rumble).
/// @param gamepad [Integer] Gamepad index (0-3)
/// @param left [Float] Left motor intensity (0.0 to 1.0)
/// @param right [Float] Right motor intensity (0.0 to 1.0)
/// @param duration [Float] Duration in seconds
/// @returns [nil]
/// @example GMR::Gamepad.vibrate(0, left: 0.5, right: 0.3, duration: 0.2)
/// @example GMR::Gamepad.vibrate(0, 0.5, 0.5, 0.1)  # Positional args
static mrb_value mrb_gamepad_vibrate(mrb_state* mrb, mrb_value) {
    mrb_int gamepad;
    mrb_float left = 0.0f;
    mrb_float right = 0.0f;
    mrb_float duration = 0.0f;
    mrb_value kwargs = mrb_nil_value();

    // Try to parse as keyword args first, then positional
    mrb_int argc = mrb_get_argc(mrb);
    if (argc >= 1) {
        mrb_get_args(mrb, "i|fffH", &gamepad, &left, &right, &duration, &kwargs);

        // Check for keyword args
        if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
            mrb_value left_val = mrb_hash_get(mrb, kwargs,
                mrb_symbol_value(mrb_intern_lit(mrb, "left")));
            if (!mrb_nil_p(left_val)) {
                left = mrb_as_float(mrb, left_val);
            }

            mrb_value right_val = mrb_hash_get(mrb, kwargs,
                mrb_symbol_value(mrb_intern_lit(mrb, "right")));
            if (!mrb_nil_p(right_val)) {
                right = mrb_as_float(mrb, right_val);
            }

            mrb_value duration_val = mrb_hash_get(mrb, kwargs,
                mrb_symbol_value(mrb_intern_lit(mrb, "duration")));
            if (!mrb_nil_p(duration_val)) {
                duration = mrb_as_float(mrb, duration_val);
            }
        }
    }

    // Clamp values
    if (left < 0.0f) left = 0.0f;
    if (left > 1.0f) left = 1.0f;
    if (right < 0.0f) right = 0.0f;
    if (right > 1.0f) right = 1.0f;
    if (duration < 0.0f) duration = 0.0f;

    SetGamepadVibration(static_cast<int>(gamepad),
                        static_cast<float>(left),
                        static_cast<float>(right),
                        static_cast<float>(duration));

    return mrb_nil_value();
}

// ============================================================================
// Any Gamepad Helpers
// ============================================================================

/// @function any_pressed?
/// @description Check if a button was pressed on any connected gamepad.
/// @param button [Symbol, Integer] Button to check
/// @returns [Boolean] true if the button was pressed on any gamepad
/// @example if GMR::Gamepad.any_pressed?(:start) then toggle_pause end
static mrb_value mrb_gamepad_any_pressed(mrb_state* mrb, mrb_value) {
    mrb_value button_arg;
    mrb_get_args(mrb, "o", &button_arg);

    int button = parse_gamepad_button_arg(mrb, button_arg);

    for (int i = 0; i < 4; i++) {
        if (IsGamepadAvailable(i) && IsGamepadButtonPressed(i, button)) {
            return mrb_true_value();
        }
    }

    return mrb_false_value();
}

/// @function any_down?
/// @description Check if a button is held down on any connected gamepad.
/// @param button [Symbol, Integer] Button to check
/// @returns [Boolean] true if the button is held on any gamepad
/// @example if GMR::Gamepad.any_down?(:a) then player.accelerate end
static mrb_value mrb_gamepad_any_down(mrb_state* mrb, mrb_value) {
    mrb_value button_arg;
    mrb_get_args(mrb, "o", &button_arg);

    int button = parse_gamepad_button_arg(mrb, button_arg);

    for (int i = 0; i < 4; i++) {
        if (IsGamepadAvailable(i) && IsGamepadButtonDown(i, button)) {
            return mrb_true_value();
        }
    }

    return mrb_false_value();
}

// ============================================================================
// Constants Registration
// ============================================================================

static void register_gamepad_constants(mrb_state* mrb, RClass* gamepad) {
    // Face buttons
    mrb_define_const(mrb, gamepad, "BUTTON_A", mrb_fixnum_value(GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    mrb_define_const(mrb, gamepad, "BUTTON_B", mrb_fixnum_value(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
    mrb_define_const(mrb, gamepad, "BUTTON_X", mrb_fixnum_value(GAMEPAD_BUTTON_RIGHT_FACE_LEFT));
    mrb_define_const(mrb, gamepad, "BUTTON_Y", mrb_fixnum_value(GAMEPAD_BUTTON_RIGHT_FACE_UP));

    // D-pad
    mrb_define_const(mrb, gamepad, "BUTTON_DPAD_UP", mrb_fixnum_value(GAMEPAD_BUTTON_LEFT_FACE_UP));
    mrb_define_const(mrb, gamepad, "BUTTON_DPAD_DOWN", mrb_fixnum_value(GAMEPAD_BUTTON_LEFT_FACE_DOWN));
    mrb_define_const(mrb, gamepad, "BUTTON_DPAD_LEFT", mrb_fixnum_value(GAMEPAD_BUTTON_LEFT_FACE_LEFT));
    mrb_define_const(mrb, gamepad, "BUTTON_DPAD_RIGHT", mrb_fixnum_value(GAMEPAD_BUTTON_LEFT_FACE_RIGHT));

    // Shoulders
    mrb_define_const(mrb, gamepad, "BUTTON_LB", mrb_fixnum_value(GAMEPAD_BUTTON_LEFT_TRIGGER_1));
    mrb_define_const(mrb, gamepad, "BUTTON_RB", mrb_fixnum_value(GAMEPAD_BUTTON_RIGHT_TRIGGER_1));
    mrb_define_const(mrb, gamepad, "BUTTON_LT", mrb_fixnum_value(GAMEPAD_BUTTON_LEFT_TRIGGER_2));
    mrb_define_const(mrb, gamepad, "BUTTON_RT", mrb_fixnum_value(GAMEPAD_BUTTON_RIGHT_TRIGGER_2));

    // Thumbsticks
    mrb_define_const(mrb, gamepad, "BUTTON_L3", mrb_fixnum_value(GAMEPAD_BUTTON_LEFT_THUMB));
    mrb_define_const(mrb, gamepad, "BUTTON_R3", mrb_fixnum_value(GAMEPAD_BUTTON_RIGHT_THUMB));

    // Center buttons
    mrb_define_const(mrb, gamepad, "BUTTON_START", mrb_fixnum_value(GAMEPAD_BUTTON_MIDDLE_RIGHT));
    mrb_define_const(mrb, gamepad, "BUTTON_SELECT", mrb_fixnum_value(GAMEPAD_BUTTON_MIDDLE_LEFT));
    mrb_define_const(mrb, gamepad, "BUTTON_GUIDE", mrb_fixnum_value(GAMEPAD_BUTTON_MIDDLE));

    // Axes
    mrb_define_const(mrb, gamepad, "AXIS_LEFT_X", mrb_fixnum_value(GAMEPAD_AXIS_LEFT_X));
    mrb_define_const(mrb, gamepad, "AXIS_LEFT_Y", mrb_fixnum_value(GAMEPAD_AXIS_LEFT_Y));
    mrb_define_const(mrb, gamepad, "AXIS_RIGHT_X", mrb_fixnum_value(GAMEPAD_AXIS_RIGHT_X));
    mrb_define_const(mrb, gamepad, "AXIS_RIGHT_Y", mrb_fixnum_value(GAMEPAD_AXIS_RIGHT_Y));
    mrb_define_const(mrb, gamepad, "AXIS_LEFT_TRIGGER", mrb_fixnum_value(GAMEPAD_AXIS_LEFT_TRIGGER));
    mrb_define_const(mrb, gamepad, "AXIS_RIGHT_TRIGGER", mrb_fixnum_value(GAMEPAD_AXIS_RIGHT_TRIGGER));
}

// ============================================================================
// Registration
// ============================================================================

void register_gamepad(mrb_state* mrb) {
    // Get GMR::Input::Gamepad module
    RClass* input = get_gmr_submodule(mrb, "Input");
    RClass* gamepad = mrb_module_get_under(mrb, input, "Gamepad");

    // Connection status
    mrb_define_module_function(mrb, gamepad, "count", mrb_gamepad_count, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, gamepad, "connected?", mrb_gamepad_connected, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, gamepad, "name", mrb_gamepad_name, MRB_ARGS_REQ(1));

    // Button state
    mrb_define_module_function(mrb, gamepad, "down?", mrb_gamepad_down, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, gamepad, "pressed?", mrb_gamepad_pressed, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, gamepad, "released?", mrb_gamepad_released, MRB_ARGS_REQ(2));

    // Axis input
    mrb_define_module_function(mrb, gamepad, "axis", mrb_gamepad_axis, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, gamepad, "axis_raw", mrb_gamepad_axis_raw, MRB_ARGS_REQ(2));

    // Dead zone configuration
    mrb_define_module_function(mrb, gamepad, "dead_zone", mrb_gamepad_get_dead_zone, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, gamepad, "dead_zone=", mrb_gamepad_set_dead_zone, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, gamepad, "outer_dead_zone", mrb_gamepad_get_outer_dead_zone, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, gamepad, "outer_dead_zone=", mrb_gamepad_set_outer_dead_zone, MRB_ARGS_REQ(1));

    // Vibration
    mrb_define_module_function(mrb, gamepad, "vibrate", mrb_gamepad_vibrate, MRB_ARGS_ARG(1, 4));

    // Any gamepad helpers
    mrb_define_module_function(mrb, gamepad, "any_pressed?", mrb_gamepad_any_pressed, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, gamepad, "any_down?", mrb_gamepad_any_down, MRB_ARGS_REQ(1));

    // Register constants
    register_gamepad_constants(mrb, gamepad);
}

} // namespace bindings
} // namespace gmr
