#include "gmr/bindings/input.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/input/input_manager.hpp"
#include "gmr/state.hpp"
#include "raylib.h"
#include <vector>

// Namespace alias to avoid confusion with gmr::bindings::input
namespace gmr_input = gmr::input;

namespace gmr {
namespace bindings {

// ============================================================================
// Helper: Check if any key in array/single value matches a condition
// ============================================================================

template<typename CheckFn>
static bool check_keys_any(mrb_state* mrb, mrb_value arg, CheckFn check_fn) {
    if (mrb_array_p(arg)) {
        mrb_int len = RARRAY_LEN(arg);
        for (mrb_int i = 0; i < len; i++) {
            mrb_value item = mrb_ary_ref(mrb, arg, i);
            int key = parse_key_arg(mrb, item);
            if (check_fn(key)) return true;
        }
        return false;
    }
    return check_fn(parse_key_arg(mrb, arg));
}

// ============================================================================
// GMR::Input Module Functions - Mouse
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

// ============================================================================
// GMR::Input Module Functions - Keyboard
// ============================================================================

// GMR::Input.key_down?(key) - accepts integer, symbol, or array of either
static mrb_value mrb_input_key_down(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    bool result = check_keys_any(mrb, arg, [](int key) { return IsKeyDown(key); });
    return to_mrb_bool(mrb, result);
}

// GMR::Input.key_pressed?(key) - accepts integer, symbol, or array of either
static mrb_value mrb_input_key_pressed(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    bool result = check_keys_any(mrb, arg, [](int key) { return IsKeyPressed(key); });
    return to_mrb_bool(mrb, result);
}

// GMR::Input.key_released?(key) - accepts integer, symbol, or array of either
static mrb_value mrb_input_key_released(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    bool result = check_keys_any(mrb, arg, [](int key) { return IsKeyReleased(key); });
    return to_mrb_bool(mrb, result);
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
// Action Mapping System - Helpers
// ============================================================================

// Parse keys from array/single value into InputBinding vector
static std::vector<gmr_input::InputBinding> parse_bindings_from_keys(mrb_state* mrb, mrb_value keys) {
    std::vector<gmr_input::InputBinding> bindings;

    if (mrb_array_p(keys)) {
        mrb_int len = RARRAY_LEN(keys);
        for (mrb_int i = 0; i < len; i++) {
            mrb_value item = mrb_ary_ref(mrb, keys, i);
            gmr_input::InputBinding binding;
            binding.source = gmr_input::InputSource::Keyboard;
            binding.code = parse_key_arg(mrb, item);
            bindings.push_back(binding);
        }
    } else if (!mrb_nil_p(keys)) {
        // Single key
        gmr_input::InputBinding binding;
        binding.source = gmr_input::InputSource::Keyboard;
        binding.code = parse_key_arg(mrb, keys);
        bindings.push_back(binding);
    }

    return bindings;
}

// Parse mouse button(s) into InputBinding vector
static std::vector<gmr_input::InputBinding> parse_bindings_from_mouse(mrb_state* mrb, mrb_value mouse) {
    std::vector<gmr_input::InputBinding> bindings;

    if (mrb_array_p(mouse)) {
        mrb_int len = RARRAY_LEN(mouse);
        for (mrb_int i = 0; i < len; i++) {
            mrb_value item = mrb_ary_ref(mrb, mouse, i);
            gmr_input::InputBinding binding;
            binding.source = gmr_input::InputSource::Mouse;
            binding.code = parse_mouse_button_arg(mrb, item);
            bindings.push_back(binding);
        }
    } else if (!mrb_nil_p(mouse)) {
        // Single button
        gmr_input::InputBinding binding;
        binding.source = gmr_input::InputSource::Mouse;
        binding.code = parse_mouse_button_arg(mrb, mouse);
        bindings.push_back(binding);
    }

    return bindings;
}

// ============================================================================
// Action Mapping System - Traditional API
// ============================================================================

// GMR::Input.map(action_name, keys) - Map an action to one or more keys
// This is the backward-compatible API
static mrb_value mrb_input_map_simple(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_value keys;
    mrb_get_args(mrb, "no", &action_sym, &keys);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    std::vector<gmr_input::InputBinding> bindings = parse_bindings_from_keys(mrb, keys);

    gmr_input::InputManager::instance().define_action(action_name, bindings);

    return mrb_nil_value();
}

// ============================================================================
// Action Mapping System - Block DSL
// ============================================================================

// Builder data for block DSL
struct InputMapBuilderData {
    bool active;
};

static void input_builder_free(mrb_state* mrb, void* ptr) {
    if (ptr) {
        mrb_free(mrb, ptr);
    }
}

static const mrb_data_type input_builder_data_type = {
    "Input::MapBuilder", input_builder_free
};

static RClass* input_builder_class = nullptr;

// GMR::Input::MapBuilder#action - Define an action with bindings
// action :name, keys: [...], key: :x, mouse: :left
static mrb_value mrb_input_builder_action(mrb_state* mrb, mrb_value self) {
    mrb_sym action_sym;
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "n|H", &action_sym, &kwargs);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    std::vector<gmr_input::InputBinding> bindings;

    if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        // Parse :keys (array of keys)
        mrb_value keys_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "keys")));
        if (!mrb_nil_p(keys_val)) {
            auto key_bindings = parse_bindings_from_keys(mrb, keys_val);
            bindings.insert(bindings.end(), key_bindings.begin(), key_bindings.end());
        }

        // Parse :key (single key)
        mrb_value key_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "key")));
        if (!mrb_nil_p(key_val)) {
            auto key_bindings = parse_bindings_from_keys(mrb, key_val);
            bindings.insert(bindings.end(), key_bindings.begin(), key_bindings.end());
        }

        // Parse :mouse (single or array of mouse buttons)
        mrb_value mouse_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "mouse")));
        if (!mrb_nil_p(mouse_val)) {
            auto mouse_bindings = parse_bindings_from_mouse(mrb, mouse_val);
            bindings.insert(bindings.end(), mouse_bindings.begin(), mouse_bindings.end());
        }
    }

    gmr_input::InputManager::instance().define_action(action_name, bindings);

    return self;
}

// GMR::Input.map - supports both traditional and block DSL
// Traditional: GMR::Input.map(:action, [:a, :left])
// Block DSL: GMR::Input.map { action :move, keys: [:a, :left] }
static mrb_value mrb_input_map(mrb_state* mrb, mrb_value self) {
    mrb_value block = mrb_nil_value();
    mrb_sym action_sym = 0;
    mrb_value keys = mrb_nil_value();

    // Parse optional symbol, optional keys, optional block
    mrb_get_args(mrb, "|no&", &action_sym, &keys, &block);

    if (!mrb_nil_p(block) && action_sym == 0) {
        // Block DSL form: GMR::Input.map { ... }
        mrb_value builder = mrb_obj_new(mrb, input_builder_class, 0, nullptr);

        InputMapBuilderData* data = static_cast<InputMapBuilderData*>(
            mrb_malloc(mrb, sizeof(InputMapBuilderData)));
        data->active = true;
        mrb_data_init(builder, data, &input_builder_data_type);

        mrb_yield(mrb, block, builder);

        return mrb_nil_value();
    }

    if (action_sym != 0) {
        // Traditional form: GMR::Input.map(:action, keys)
        const char* action_name = mrb_sym_name(mrb, action_sym);
        std::vector<gmr_input::InputBinding> bindings = parse_bindings_from_keys(mrb, keys);
        gmr_input::InputManager::instance().define_action(action_name, bindings);
    }

    return mrb_nil_value();
}

// GMR::Input.unmap(action_name) - Remove an action mapping
static mrb_value mrb_input_unmap(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_get_args(mrb, "n", &action_sym);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    gmr_input::InputManager::instance().remove_action(action_name);

    return mrb_nil_value();
}

// GMR::Input.clear_mappings - Remove all action mappings
static mrb_value mrb_input_clear_mappings(mrb_state* mrb, mrb_value) {
    gmr_input::InputManager::instance().clear_actions();
    return mrb_nil_value();
}

// ============================================================================
// Action Query Functions (using InputManager)
// ============================================================================

// GMR::Input.action_down?(action_name)
static mrb_value mrb_input_action_down(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_get_args(mrb, "n", &action_sym);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    bool result = gmr_input::InputManager::instance().action_down(action_name);
    return to_mrb_bool(mrb, result);
}

// GMR::Input.action_pressed?(action_name)
static mrb_value mrb_input_action_pressed(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_get_args(mrb, "n", &action_sym);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    bool result = gmr_input::InputManager::instance().action_pressed(action_name);
    return to_mrb_bool(mrb, result);
}

// GMR::Input.action_released?(action_name)
static mrb_value mrb_input_action_released(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_get_args(mrb, "n", &action_sym);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    bool result = gmr_input::InputManager::instance().action_released(action_name);
    return to_mrb_bool(mrb, result);
}

// ============================================================================
// Standalone Callback System
// ============================================================================

// GMR::Input.on(action, when: :pressed, context: nil) { ... }
// Returns callback ID for later removal
static mrb_value mrb_input_on(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_value kwargs = mrb_nil_value();
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "n|H&", &action_sym, &kwargs, &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "GMR::Input.on requires a block");
        return mrb_nil_value();
    }

    const char* action_name = mrb_sym_name(mrb, action_sym);
    gmr_input::InputPhase phase = gmr_input::InputPhase::Pressed;
    mrb_value context = mrb_nil_value();

    if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        // Parse :when (phase)
        mrb_value when_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "when")));
        phase = parse_input_phase(mrb, when_val);

        // Parse :context (object for instance_exec)
        mrb_value context_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "context")));
        if (!mrb_nil_p(context_val)) {
            context = context_val;
        }
    }

    int callback_id = gmr_input::InputManager::instance().on(mrb, action_name, phase, block, context);

    return mrb_fixnum_value(callback_id);
}

// GMR::Input.off(id_or_action) - Remove callback by ID or all callbacks for action
static mrb_value mrb_input_off(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);

    if (mrb_fixnum_p(arg)) {
        // Remove by ID
        int callback_id = static_cast<int>(mrb_fixnum(arg));
        gmr_input::InputManager::instance().off(mrb, callback_id);
    } else if (mrb_symbol_p(arg)) {
        // Remove all callbacks for action
        const char* action_name = mrb_sym_name(mrb, mrb_symbol(arg));
        gmr_input::InputManager::instance().off_all(mrb, action_name);
    } else {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected callback ID (integer) or action name (symbol)");
    }

    return mrb_nil_value();
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

    // Mouse functions
    mrb_define_module_function(mrb, input, "mouse_x", mrb_input_mouse_x, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "mouse_y", mrb_input_mouse_y, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "mouse_down?", mrb_input_mouse_down, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "mouse_pressed?", mrb_input_mouse_pressed, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "mouse_released?", mrb_input_mouse_released, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "mouse_wheel", mrb_input_mouse_wheel, MRB_ARGS_NONE());

    // Keyboard functions
    mrb_define_module_function(mrb, input, "key_down?", mrb_input_key_down, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "key_pressed?", mrb_input_key_pressed, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "key_released?", mrb_input_key_released, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "key_pressed", mrb_input_get_key_pressed, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "char_pressed", mrb_input_get_char_pressed, MRB_ARGS_NONE());

    // Action mapping - supports both traditional and block DSL
    mrb_define_module_function(mrb, input, "map", mrb_input_map,
        MRB_ARGS_OPT(2) | MRB_ARGS_BLOCK());
    mrb_define_module_function(mrb, input, "unmap", mrb_input_unmap, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "clear_mappings", mrb_input_clear_mappings, MRB_ARGS_NONE());

    // Action query functions
    mrb_define_module_function(mrb, input, "action_down?", mrb_input_action_down, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "action_pressed?", mrb_input_action_pressed, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "action_released?", mrb_input_action_released, MRB_ARGS_REQ(1));

    // Callback system
    mrb_define_module_function(mrb, input, "on", mrb_input_on,
        MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_module_function(mrb, input, "off", mrb_input_off, MRB_ARGS_REQ(1));

    // Register MapBuilder internal class
    input_builder_class = mrb_define_class_under(mrb, input, "MapBuilder", mrb->object_class);
    MRB_SET_INSTANCE_TT(input_builder_class, MRB_TT_CDATA);
    mrb_define_method(mrb, input_builder_class, "action", mrb_input_builder_action,
        MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));

    // Register key constants
    register_key_constants(mrb, input);
}

} // namespace bindings
} // namespace gmr
