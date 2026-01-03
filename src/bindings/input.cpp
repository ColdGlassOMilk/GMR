#include "gmr/bindings/input.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include "raylib.h"

namespace gmr {
namespace bindings {

// ============================================================================
// GMR::Input Module Functions
// ============================================================================

// GMR::Input.mouse_x
static mrb_value mrb_input_mouse_x(mrb_state* mrb, mrb_value) {
    auto& state = State::instance();

    if (state.use_virtual_resolution) {
        float scale_x = static_cast<float>(GetScreenWidth()) / state.virtual_width;
        float scale_y = static_cast<float>(GetScreenHeight()) / state.virtual_height;
        float scale = (scale_x < scale_y) ? scale_x : scale_y;

        int scaled_width = static_cast<int>(state.virtual_width * scale);
        int offset_x = (GetScreenWidth() - scaled_width) / 2;

        return mrb_fixnum_value(static_cast<int>((GetMouseX() - offset_x) / scale));
    }
    return mrb_fixnum_value(GetMouseX());
}

// GMR::Input.mouse_y
static mrb_value mrb_input_mouse_y(mrb_state* mrb, mrb_value) {
    auto& state = State::instance();

    if (state.use_virtual_resolution) {
        float scale_x = static_cast<float>(GetScreenWidth()) / state.virtual_width;
        float scale_y = static_cast<float>(GetScreenHeight()) / state.virtual_height;
        float scale = (scale_x < scale_y) ? scale_x : scale_y;

        int scaled_height = static_cast<int>(state.virtual_height * scale);
        int offset_y = (GetScreenHeight() - scaled_height) / 2;

        return mrb_fixnum_value(static_cast<int>((GetMouseY() - offset_y) / scale));
    }
    return mrb_fixnum_value(GetMouseY());
}

// GMR::Input.mouse_down?(button) - accepts integer or symbol
static mrb_value mrb_input_mouse_down(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int button = parse_mouse_button_arg(mrb, arg);
    return to_mrb_bool(mrb, IsMouseButtonDown(button));
}

// GMR::Input.mouse_pressed?(button)
static mrb_value mrb_input_mouse_pressed(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int button = parse_mouse_button_arg(mrb, arg);
    return to_mrb_bool(mrb, IsMouseButtonPressed(button));
}

// GMR::Input.mouse_released?(button)
static mrb_value mrb_input_mouse_released(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int button = parse_mouse_button_arg(mrb, arg);
    return to_mrb_bool(mrb, IsMouseButtonReleased(button));
}

// GMR::Input.mouse_wheel
static mrb_value mrb_input_mouse_wheel(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetMouseWheelMove());
}

// GMR::Input.key_down?(key) - accepts integer or symbol
static mrb_value mrb_input_key_down(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int key = parse_key_arg(mrb, arg);
    return to_mrb_bool(mrb, IsKeyDown(key));
}

// GMR::Input.key_pressed?(key)
static mrb_value mrb_input_key_pressed(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int key = parse_key_arg(mrb, arg);
    return to_mrb_bool(mrb, IsKeyPressed(key));
}

// GMR::Input.key_released?(key)
static mrb_value mrb_input_key_released(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int key = parse_key_arg(mrb, arg);
    return to_mrb_bool(mrb, IsKeyReleased(key));
}

// GMR::Input.key_pressed (returns the key code of the last key pressed)
static mrb_value mrb_input_get_key_pressed(mrb_state* mrb, mrb_value) {
    int key = GetKeyPressed();
    if (key == 0) return mrb_nil_value();
    return mrb_fixnum_value(key);
}

// GMR::Input.char_pressed (returns the character code of the last char pressed)
static mrb_value mrb_input_get_char_pressed(mrb_state* mrb, mrb_value) {
    int ch = GetCharPressed();
    if (ch == 0) return mrb_nil_value();
    return mrb_fixnum_value(ch);
}

// ============================================================================
// Key Constants Registration
// ============================================================================

static void register_key_constants(mrb_state* mrb, RClass* input) {
    // Mouse buttons
    mrb_define_const(mrb, input, "MOUSE_LEFT", mrb_fixnum_value(MOUSE_BUTTON_LEFT));
    mrb_define_const(mrb, input, "MOUSE_RIGHT", mrb_fixnum_value(MOUSE_BUTTON_RIGHT));
    mrb_define_const(mrb, input, "MOUSE_MIDDLE", mrb_fixnum_value(MOUSE_BUTTON_MIDDLE));
    mrb_define_const(mrb, input, "MOUSE_SIDE", mrb_fixnum_value(MOUSE_BUTTON_SIDE));
    mrb_define_const(mrb, input, "MOUSE_EXTRA", mrb_fixnum_value(MOUSE_BUTTON_EXTRA));
    mrb_define_const(mrb, input, "MOUSE_FORWARD", mrb_fixnum_value(MOUSE_BUTTON_FORWARD));
    mrb_define_const(mrb, input, "MOUSE_BACK", mrb_fixnum_value(MOUSE_BUTTON_BACK));

    // Common keys
    mrb_define_const(mrb, input, "KEY_SPACE", mrb_fixnum_value(KEY_SPACE));
    mrb_define_const(mrb, input, "KEY_ESCAPE", mrb_fixnum_value(KEY_ESCAPE));
    mrb_define_const(mrb, input, "KEY_ENTER", mrb_fixnum_value(KEY_ENTER));
    mrb_define_const(mrb, input, "KEY_TAB", mrb_fixnum_value(KEY_TAB));
    mrb_define_const(mrb, input, "KEY_BACKSPACE", mrb_fixnum_value(KEY_BACKSPACE));
    mrb_define_const(mrb, input, "KEY_DELETE", mrb_fixnum_value(KEY_DELETE));
    mrb_define_const(mrb, input, "KEY_INSERT", mrb_fixnum_value(KEY_INSERT));

    // Arrow keys
    mrb_define_const(mrb, input, "KEY_UP", mrb_fixnum_value(KEY_UP));
    mrb_define_const(mrb, input, "KEY_DOWN", mrb_fixnum_value(KEY_DOWN));
    mrb_define_const(mrb, input, "KEY_LEFT", mrb_fixnum_value(KEY_LEFT));
    mrb_define_const(mrb, input, "KEY_RIGHT", mrb_fixnum_value(KEY_RIGHT));

    // Navigation
    mrb_define_const(mrb, input, "KEY_HOME", mrb_fixnum_value(KEY_HOME));
    mrb_define_const(mrb, input, "KEY_END", mrb_fixnum_value(KEY_END));
    mrb_define_const(mrb, input, "KEY_PAGE_UP", mrb_fixnum_value(KEY_PAGE_UP));
    mrb_define_const(mrb, input, "KEY_PAGE_DOWN", mrb_fixnum_value(KEY_PAGE_DOWN));

    // Modifiers
    mrb_define_const(mrb, input, "KEY_LEFT_SHIFT", mrb_fixnum_value(KEY_LEFT_SHIFT));
    mrb_define_const(mrb, input, "KEY_RIGHT_SHIFT", mrb_fixnum_value(KEY_RIGHT_SHIFT));
    mrb_define_const(mrb, input, "KEY_LEFT_CONTROL", mrb_fixnum_value(KEY_LEFT_CONTROL));
    mrb_define_const(mrb, input, "KEY_RIGHT_CONTROL", mrb_fixnum_value(KEY_RIGHT_CONTROL));
    mrb_define_const(mrb, input, "KEY_LEFT_ALT", mrb_fixnum_value(KEY_LEFT_ALT));
    mrb_define_const(mrb, input, "KEY_RIGHT_ALT", mrb_fixnum_value(KEY_RIGHT_ALT));

    // Function keys
    mrb_define_const(mrb, input, "KEY_F1", mrb_fixnum_value(KEY_F1));
    mrb_define_const(mrb, input, "KEY_F2", mrb_fixnum_value(KEY_F2));
    mrb_define_const(mrb, input, "KEY_F3", mrb_fixnum_value(KEY_F3));
    mrb_define_const(mrb, input, "KEY_F4", mrb_fixnum_value(KEY_F4));
    mrb_define_const(mrb, input, "KEY_F5", mrb_fixnum_value(KEY_F5));
    mrb_define_const(mrb, input, "KEY_F6", mrb_fixnum_value(KEY_F6));
    mrb_define_const(mrb, input, "KEY_F7", mrb_fixnum_value(KEY_F7));
    mrb_define_const(mrb, input, "KEY_F8", mrb_fixnum_value(KEY_F8));
    mrb_define_const(mrb, input, "KEY_F9", mrb_fixnum_value(KEY_F9));
    mrb_define_const(mrb, input, "KEY_F10", mrb_fixnum_value(KEY_F10));
    mrb_define_const(mrb, input, "KEY_F11", mrb_fixnum_value(KEY_F11));
    mrb_define_const(mrb, input, "KEY_F12", mrb_fixnum_value(KEY_F12));

    // Letter keys
    mrb_define_const(mrb, input, "KEY_A", mrb_fixnum_value(KEY_A));
    mrb_define_const(mrb, input, "KEY_B", mrb_fixnum_value(KEY_B));
    mrb_define_const(mrb, input, "KEY_C", mrb_fixnum_value(KEY_C));
    mrb_define_const(mrb, input, "KEY_D", mrb_fixnum_value(KEY_D));
    mrb_define_const(mrb, input, "KEY_E", mrb_fixnum_value(KEY_E));
    mrb_define_const(mrb, input, "KEY_F", mrb_fixnum_value(KEY_F));
    mrb_define_const(mrb, input, "KEY_G", mrb_fixnum_value(KEY_G));
    mrb_define_const(mrb, input, "KEY_H", mrb_fixnum_value(KEY_H));
    mrb_define_const(mrb, input, "KEY_I", mrb_fixnum_value(KEY_I));
    mrb_define_const(mrb, input, "KEY_J", mrb_fixnum_value(KEY_J));
    mrb_define_const(mrb, input, "KEY_K", mrb_fixnum_value(KEY_K));
    mrb_define_const(mrb, input, "KEY_L", mrb_fixnum_value(KEY_L));
    mrb_define_const(mrb, input, "KEY_M", mrb_fixnum_value(KEY_M));
    mrb_define_const(mrb, input, "KEY_N", mrb_fixnum_value(KEY_N));
    mrb_define_const(mrb, input, "KEY_O", mrb_fixnum_value(KEY_O));
    mrb_define_const(mrb, input, "KEY_P", mrb_fixnum_value(KEY_P));
    mrb_define_const(mrb, input, "KEY_Q", mrb_fixnum_value(KEY_Q));
    mrb_define_const(mrb, input, "KEY_R", mrb_fixnum_value(KEY_R));
    mrb_define_const(mrb, input, "KEY_S", mrb_fixnum_value(KEY_S));
    mrb_define_const(mrb, input, "KEY_T", mrb_fixnum_value(KEY_T));
    mrb_define_const(mrb, input, "KEY_U", mrb_fixnum_value(KEY_U));
    mrb_define_const(mrb, input, "KEY_V", mrb_fixnum_value(KEY_V));
    mrb_define_const(mrb, input, "KEY_W", mrb_fixnum_value(KEY_W));
    mrb_define_const(mrb, input, "KEY_X", mrb_fixnum_value(KEY_X));
    mrb_define_const(mrb, input, "KEY_Y", mrb_fixnum_value(KEY_Y));
    mrb_define_const(mrb, input, "KEY_Z", mrb_fixnum_value(KEY_Z));

    // Number keys
    mrb_define_const(mrb, input, "KEY_0", mrb_fixnum_value(KEY_ZERO));
    mrb_define_const(mrb, input, "KEY_1", mrb_fixnum_value(KEY_ONE));
    mrb_define_const(mrb, input, "KEY_2", mrb_fixnum_value(KEY_TWO));
    mrb_define_const(mrb, input, "KEY_3", mrb_fixnum_value(KEY_THREE));
    mrb_define_const(mrb, input, "KEY_4", mrb_fixnum_value(KEY_FOUR));
    mrb_define_const(mrb, input, "KEY_5", mrb_fixnum_value(KEY_FIVE));
    mrb_define_const(mrb, input, "KEY_6", mrb_fixnum_value(KEY_SIX));
    mrb_define_const(mrb, input, "KEY_7", mrb_fixnum_value(KEY_SEVEN));
    mrb_define_const(mrb, input, "KEY_8", mrb_fixnum_value(KEY_EIGHT));
    mrb_define_const(mrb, input, "KEY_9", mrb_fixnum_value(KEY_NINE));
}

// ============================================================================
// Registration
// ============================================================================

void register_input(mrb_state* mrb) {
    RClass* input = get_gmr_submodule(mrb, "Input");

    // Module functions
    mrb_define_module_function(mrb, input, "mouse_x", mrb_input_mouse_x, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "mouse_y", mrb_input_mouse_y, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "mouse_down?", mrb_input_mouse_down, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "mouse_pressed?", mrb_input_mouse_pressed, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "mouse_released?", mrb_input_mouse_released, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "mouse_wheel", mrb_input_mouse_wheel, MRB_ARGS_NONE());

    mrb_define_module_function(mrb, input, "key_down?", mrb_input_key_down, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "key_pressed?", mrb_input_key_pressed, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "key_released?", mrb_input_key_released, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "key_pressed", mrb_input_get_key_pressed, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "char_pressed", mrb_input_get_char_pressed, MRB_ARGS_NONE());

    // Register key constants
    register_key_constants(mrb, input);
}

} // namespace bindings
} // namespace gmr
