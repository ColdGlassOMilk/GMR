#include "gmr/bindings/binding_helpers.hpp"
#include "raylib.h"
#include <cstring>

namespace gmr {
namespace bindings {

Color parse_color(mrb_state* mrb, mrb_value* argv, mrb_int argc, const Color& default_color) {
    if (argc == 0) {
        return default_color;
    }

    if (argc == 1 && mrb_array_p(argv[0])) {
        return parse_color_value(mrb, argv[0], default_color);
    }

    if (argc >= 3) {
        uint8_t r = static_cast<uint8_t>(mrb_fixnum(argv[0]));
        uint8_t g = static_cast<uint8_t>(mrb_fixnum(argv[1]));
        uint8_t b = static_cast<uint8_t>(mrb_fixnum(argv[2]));
        uint8_t a = argc > 3 ? static_cast<uint8_t>(mrb_fixnum(argv[3])) : 255;

        return Color{r, g, b, a};
    }

    mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected [r,g,b,a] array or 3-4 integers");
    return default_color;
}

Color parse_color_value(mrb_state* mrb, mrb_value val, const Color& default_color) {
    if (mrb_nil_p(val)) {
        return default_color;
    }

    if (!mrb_array_p(val)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected color array [r,g,b] or [r,g,b,a]");
        return default_color;
    }

    mrb_int len = RARRAY_LEN(val);

    if (len < 3) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected array of 3-4 elements [r,g,b] or [r,g,b,a]");
        return default_color;
    }

    uint8_t r = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, val, 0)));
    uint8_t g = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, val, 1)));
    uint8_t b = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, val, 2)));
    uint8_t a = len > 3 ? static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, val, 3))) : 255;

    return Color{r, g, b, a};
}

// ============================================================================
// Module Hierarchy
// ============================================================================

RClass* get_gmr_module(mrb_state* mrb) {
    return mrb_module_get(mrb, "GMR");
}

RClass* get_gmr_submodule(mrb_state* mrb, const char* name) {
    RClass* gmr = get_gmr_module(mrb);
    return mrb_module_get_under(mrb, gmr, name);
}

void init_gmr_modules(mrb_state* mrb) {
    // Create top-level GMR module
    RClass* gmr = mrb_define_module(mrb, "GMR");

    // Create all submodules
    mrb_define_module_under(mrb, gmr, "Graphics");
    mrb_define_module_under(mrb, gmr, "Audio");
    mrb_define_module_under(mrb, gmr, "Input");
    mrb_define_module_under(mrb, gmr, "Window");
    mrb_define_module_under(mrb, gmr, "Time");
    mrb_define_module_under(mrb, gmr, "System");
}

// ============================================================================
// Symbol/Key Mapping
// ============================================================================

int symbol_to_key(mrb_state* mrb, mrb_sym sym) {
    const char* name = mrb_sym_name(mrb, sym);

    // Common keys
    if (strcmp(name, "space") == 0) return KEY_SPACE;
    if (strcmp(name, "escape") == 0) return KEY_ESCAPE;
    if (strcmp(name, "enter") == 0) return KEY_ENTER;
    if (strcmp(name, "return") == 0) return KEY_ENTER;
    if (strcmp(name, "tab") == 0) return KEY_TAB;
    if (strcmp(name, "backspace") == 0) return KEY_BACKSPACE;
    if (strcmp(name, "delete") == 0) return KEY_DELETE;
    if (strcmp(name, "insert") == 0) return KEY_INSERT;

    // Arrow keys
    if (strcmp(name, "up") == 0) return KEY_UP;
    if (strcmp(name, "down") == 0) return KEY_DOWN;
    if (strcmp(name, "left") == 0) return KEY_LEFT;
    if (strcmp(name, "right") == 0) return KEY_RIGHT;

    // Navigation
    if (strcmp(name, "home") == 0) return KEY_HOME;
    if (strcmp(name, "end") == 0) return KEY_END;
    if (strcmp(name, "page_up") == 0) return KEY_PAGE_UP;
    if (strcmp(name, "page_down") == 0) return KEY_PAGE_DOWN;

    // Modifiers
    if (strcmp(name, "left_shift") == 0) return KEY_LEFT_SHIFT;
    if (strcmp(name, "right_shift") == 0) return KEY_RIGHT_SHIFT;
    if (strcmp(name, "left_control") == 0) return KEY_LEFT_CONTROL;
    if (strcmp(name, "right_control") == 0) return KEY_RIGHT_CONTROL;
    if (strcmp(name, "left_alt") == 0) return KEY_LEFT_ALT;
    if (strcmp(name, "right_alt") == 0) return KEY_RIGHT_ALT;

    // Function keys
    if (strcmp(name, "f1") == 0) return KEY_F1;
    if (strcmp(name, "f2") == 0) return KEY_F2;
    if (strcmp(name, "f3") == 0) return KEY_F3;
    if (strcmp(name, "f4") == 0) return KEY_F4;
    if (strcmp(name, "f5") == 0) return KEY_F5;
    if (strcmp(name, "f6") == 0) return KEY_F6;
    if (strcmp(name, "f7") == 0) return KEY_F7;
    if (strcmp(name, "f8") == 0) return KEY_F8;
    if (strcmp(name, "f9") == 0) return KEY_F9;
    if (strcmp(name, "f10") == 0) return KEY_F10;
    if (strcmp(name, "f11") == 0) return KEY_F11;
    if (strcmp(name, "f12") == 0) return KEY_F12;

    // Single letter keys (a-z)
    if (strlen(name) == 1 && name[0] >= 'a' && name[0] <= 'z') {
        return KEY_A + (name[0] - 'a');
    }

    // Single digit keys (0-9)
    if (strlen(name) == 1 && name[0] >= '0' && name[0] <= '9') {
        return KEY_ZERO + (name[0] - '0');
    }

    return -1; // Unknown symbol
}

int symbol_to_mouse_button(mrb_state* mrb, mrb_sym sym) {
    const char* name = mrb_sym_name(mrb, sym);

    if (strcmp(name, "left") == 0) return MOUSE_BUTTON_LEFT;
    if (strcmp(name, "right") == 0) return MOUSE_BUTTON_RIGHT;
    if (strcmp(name, "middle") == 0) return MOUSE_BUTTON_MIDDLE;
    if (strcmp(name, "side") == 0) return MOUSE_BUTTON_SIDE;
    if (strcmp(name, "extra") == 0) return MOUSE_BUTTON_EXTRA;
    if (strcmp(name, "forward") == 0) return MOUSE_BUTTON_FORWARD;
    if (strcmp(name, "back") == 0) return MOUSE_BUTTON_BACK;

    return -1; // Unknown symbol
}

int parse_key_arg(mrb_state* mrb, mrb_value arg) {
    if (mrb_fixnum_p(arg)) {
        return static_cast<int>(mrb_fixnum(arg));
    }

    if (mrb_symbol_p(arg)) {
        int key = symbol_to_key(mrb, mrb_symbol(arg));
        if (key < 0) {
            mrb_raisef(mrb, E_ARGUMENT_ERROR, "Unknown key symbol: %s",
                       mrb_sym_name(mrb, mrb_symbol(arg)));
        }
        return key;
    }

    mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected key code (integer) or symbol");
    return -1;
}

int parse_mouse_button_arg(mrb_state* mrb, mrb_value arg) {
    if (mrb_fixnum_p(arg)) {
        return static_cast<int>(mrb_fixnum(arg));
    }

    if (mrb_symbol_p(arg)) {
        int button = symbol_to_mouse_button(mrb, mrb_symbol(arg));
        if (button < 0) {
            mrb_raisef(mrb, E_ARGUMENT_ERROR, "Unknown mouse button symbol: %s",
                       mrb_sym_name(mrb, mrb_symbol(arg)));
        }
        return button;
    }

    mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected mouse button (integer) or symbol");
    return -1;
}

} // namespace bindings
} // namespace gmr
