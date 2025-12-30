#include "gmr/bindings/input.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include "raylib.h"

namespace gmr {
namespace bindings {

static mrb_value mrb_mouse_x(mrb_state* mrb, mrb_value) {
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

static mrb_value mrb_mouse_y(mrb_state* mrb, mrb_value) {
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

static mrb_value mrb_mouse_down(mrb_state* mrb, mrb_value) {
    mrb_int button;
    mrb_get_args(mrb, "i", &button);
    return to_mrb_bool(mrb, IsMouseButtonDown(button));
}

static mrb_value mrb_mouse_pressed(mrb_state* mrb, mrb_value) {
    mrb_int button;
    mrb_get_args(mrb, "i", &button);
    return to_mrb_bool(mrb, IsMouseButtonPressed(button));
}

static mrb_value mrb_mouse_released(mrb_state* mrb, mrb_value) {
    mrb_int button;
    mrb_get_args(mrb, "i", &button);
    return to_mrb_bool(mrb, IsMouseButtonReleased(button));
}

static mrb_value mrb_mouse_wheel(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetMouseWheelMove());
}

static mrb_value mrb_key_down(mrb_state* mrb, mrb_value) {
    mrb_int key;
    mrb_get_args(mrb, "i", &key);
    return to_mrb_bool(mrb, IsKeyDown(key));
}

static mrb_value mrb_key_pressed(mrb_state* mrb, mrb_value) {
    mrb_int key;
    mrb_get_args(mrb, "i", &key);
    return to_mrb_bool(mrb, IsKeyPressed(key));
}

static mrb_value mrb_key_released(mrb_state* mrb, mrb_value) {
    mrb_int key;
    mrb_get_args(mrb, "i", &key);
    return to_mrb_bool(mrb, IsKeyReleased(key));
}

static mrb_value mrb_get_key_pressed(mrb_state* mrb, mrb_value) {
    int key = GetKeyPressed();
    if (key == 0) return mrb_nil_value();
    return mrb_fixnum_value(key);
}

static mrb_value mrb_get_char_pressed(mrb_state* mrb, mrb_value) {
    int ch = GetCharPressed();
    if (ch == 0) return mrb_nil_value();
    return mrb_fixnum_value(ch);
}

void register_input(mrb_state* mrb) {
    define_method(mrb, "mouse_x", mrb_mouse_x, MRB_ARGS_NONE());
    define_method(mrb, "mouse_y", mrb_mouse_y, MRB_ARGS_NONE());
    define_method(mrb, "mouse_down?", mrb_mouse_down, MRB_ARGS_REQ(1));
    define_method(mrb, "mouse_pressed?", mrb_mouse_pressed, MRB_ARGS_REQ(1));
    define_method(mrb, "mouse_released?", mrb_mouse_released, MRB_ARGS_REQ(1));
    define_method(mrb, "mouse_wheel", mrb_mouse_wheel, MRB_ARGS_NONE());
    
    define_method(mrb, "key_down?", mrb_key_down, MRB_ARGS_REQ(1));
    define_method(mrb, "key_pressed?", mrb_key_pressed, MRB_ARGS_REQ(1));
    define_method(mrb, "key_released?", mrb_key_released, MRB_ARGS_REQ(1));
    define_method(mrb, "get_key_pressed", mrb_get_key_pressed, MRB_ARGS_NONE());
    define_method(mrb, "get_char_pressed", mrb_get_char_pressed, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
