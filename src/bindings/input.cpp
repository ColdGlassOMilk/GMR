#include "gmr/bindings/input.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/input/input_manager.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/state.hpp"
#include "raylib.h"
#include <vector>

// Namespace alias to avoid confusion with gmr::bindings::input
namespace gmr_input = gmr::input;

namespace gmr {
namespace bindings {

/// @module GMR::Input
/// @description Handles keyboard, mouse, and action-based input. Supports both raw input
///   polling and an action mapping system for game-agnostic input handling.
///   Actions can be queried, have callbacks attached, and be organized into contexts.
/// @example # Complete game input setup with contexts for gameplay and menus
///   class GameScene < GMR::Scene
///     def init
///       # Define global gameplay actions
///       input do |i|
///         i.move_left [:a, :left]
///         i.move_right [:d, :right]
///         i.move_up [:w, :up]
///         i.move_down [:s, :down]
///         i.jump :space
///         i.attack :z, mouse: :left
///         i.interact :e
///         i.pause :escape
///       end
///
///       # Define menu-specific context
///       input_context :menu do |i|
///         i.confirm :enter
///         i.cancel :escape
///         i.nav_up :up
///         i.nav_down :down
///       end
///
///       # Register callbacks
///       GMR::Input.on(:pause) { toggle_pause }
///       GMR::Input.on(:interact) { check_nearby_objects }
///
///       @player = Player.new
///     end
///
///     def toggle_pause
///       if @paused
///         GMR::Input.pop_context
///         @paused = false
///       else
///         GMR::Input.push_context(:menu)
///         SceneManager.add_overlay(PauseOverlay.new)
///         @paused = true
///       end
///     end
///
///     def update(dt)
///       return if @paused
///
///       # Movement using action queries
///       dx = 0
///       dy = 0
///       dx -= 1 if GMR::Input.action_down?(:move_left)
///       dx += 1 if GMR::Input.action_down?(:move_right)
///       dy -= 1 if GMR::Input.action_down?(:move_up)
///       dy += 1 if GMR::Input.action_down?(:move_down)
///       @player.move(dx, dy, dt)
///
///       # Discrete actions
///       @player.jump if GMR::Input.action_pressed?(:jump)
///       @player.attack if GMR::Input.action_pressed?(:attack)
///     end
///   end
/// @example # Mouse-based aiming and shooting
///   class Turret
///     def update(dt)
///       # Aim at mouse position
///       mx = GMR::Input.mouse_x
///       my = GMR::Input.mouse_y
///       @rotation = Math.atan2(my - @y, mx - @x)
///
///       # Fire on click
///       if GMR::Input.mouse_pressed?(:left)
///         fire_bullet(@rotation)
///       end
///
///       # Zoom with scroll wheel
///       zoom_delta = GMR::Input.mouse_wheel
///       @camera.zoom += zoom_delta * 0.1 if zoom_delta != 0
///     end
///   end
/// @example # Text input handling
///   class TextInput
///     def initialize
///       @text = ""
///       @cursor = 0
///     end
///
///     def update(dt)
///       # Handle character input
///       char = GMR::Input.char_pressed
///       if char
///         @text.insert(@cursor, char.chr(Encoding::UTF_8))
///         @cursor += 1
///       end
///
///       # Handle special keys
///       if GMR::Input.key_pressed?(:backspace) && @cursor > 0
///         @text.slice!(@cursor - 1)
///         @cursor -= 1
///       end
///       @cursor -= 1 if GMR::Input.key_pressed?(:left) && @cursor > 0
///       @cursor += 1 if GMR::Input.key_pressed?(:right) && @cursor < @text.length
///     end
///   end

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

/// @function mouse_x
/// @description Get the mouse X position in virtual resolution coordinates.
///   Automatically accounts for letterboxing when using virtual resolution.
/// @returns [Integer] Mouse X position
/// @example x = GMR::Input.mouse_x
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

/// @function mouse_y
/// @description Get the mouse Y position in virtual resolution coordinates.
///   Automatically accounts for letterboxing when using virtual resolution.
/// @returns [Integer] Mouse Y position
/// @example y = GMR::Input.mouse_y
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

/// @function mouse_down?
/// @description Check if a mouse button is currently held down.
/// @param button [Symbol, Integer] The button to check (:left, :right, :middle, or constant)
/// @returns [Boolean] true if the button is held
/// @example if GMR::Input.mouse_down?(:left)
///   player.aim
/// end
static mrb_value mrb_input_mouse_down(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int button = parse_mouse_button_arg(mrb, arg);
    return to_mrb_bool(mrb, IsMouseButtonDown(button));
}

/// @function mouse_pressed?
/// @description Check if a mouse button was just pressed this frame.
/// @param button [Symbol, Integer] The button to check (:left, :right, :middle, or constant)
/// @returns [Boolean] true if the button was just pressed
/// @example if GMR::Input.mouse_pressed?(:left)
///   player.shoot
/// end
static mrb_value mrb_input_mouse_pressed(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int button = parse_mouse_button_arg(mrb, arg);
    return to_mrb_bool(mrb, IsMouseButtonPressed(button));
}

/// @function mouse_released?
/// @description Check if a mouse button was just released this frame.
/// @param button [Symbol, Integer] The button to check (:left, :right, :middle, or constant)
/// @returns [Boolean] true if the button was just released
/// @example if GMR::Input.mouse_released?(:left)
///   bow.release_arrow
/// end
static mrb_value mrb_input_mouse_released(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    int button = parse_mouse_button_arg(mrb, arg);
    return to_mrb_bool(mrb, IsMouseButtonReleased(button));
}

/// @function mouse_wheel
/// @description Get the mouse wheel movement this frame. Positive values indicate
///   scrolling up/forward, negative values indicate scrolling down/backward.
/// @returns [Float] Wheel movement amount
/// @example zoom += GMR::Input.mouse_wheel * 0.1
static mrb_value mrb_input_mouse_wheel(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetMouseWheelMove());
}

// ============================================================================
// GMR::Input Module Functions - Keyboard
// ============================================================================

/// @function key_down?
/// @description Check if a key is currently held down. Accepts a single key or
///   an array of keys (returns true if any key in the array is held).
/// @param key [Symbol, Integer, Array] The key(s) to check (:space, :a, :left, etc.)
/// @returns [Boolean] true if the key (or any key in array) is held
/// @example if GMR::Input.key_down?(:left)
///   player.x -= speed
/// end
/// @example if GMR::Input.key_down?([:a, :left])  # Either key works
///   player.move_left
/// end
static mrb_value mrb_input_key_down(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    bool result = check_keys_any(mrb, arg, [](int key) { return IsKeyDown(key); });
    return to_mrb_bool(mrb, result);
}

/// @function key_pressed?
/// @description Check if a key was just pressed this frame. Accepts a single key or
///   an array of keys (returns true if any key in the array was just pressed).
/// @param key [Symbol, Integer, Array] The key(s) to check
/// @returns [Boolean] true if the key (or any key in array) was just pressed
/// @example if GMR::Input.key_pressed?(:space)
///   player.jump
/// end
static mrb_value mrb_input_key_pressed(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    bool result = check_keys_any(mrb, arg, [](int key) { return IsKeyPressed(key); });
    return to_mrb_bool(mrb, result);
}

/// @function key_released?
/// @description Check if a key was just released this frame. Accepts a single key or
///   an array of keys (returns true if any key in the array was just released).
/// @param key [Symbol, Integer, Array] The key(s) to check
/// @returns [Boolean] true if the key (or any key in array) was just released
/// @example if GMR::Input.key_released?(:shift)
///   player.stop_running
/// end
static mrb_value mrb_input_key_released(mrb_state* mrb, mrb_value) {
    mrb_value arg;
    mrb_get_args(mrb, "o", &arg);
    bool result = check_keys_any(mrb, arg, [](int key) { return IsKeyReleased(key); });
    return to_mrb_bool(mrb, result);
}

/// @function key_pressed
/// @description Get the key code of the last key pressed this frame.
///   Useful for text input or detecting any key press.
/// @returns [Integer, nil] Key code, or nil if no key was pressed
/// @example key = GMR::Input.key_pressed
///   if key
///     puts "Key code: #{key}"
///   end
static mrb_value mrb_input_get_key_pressed(mrb_state* mrb, mrb_value) {
    int key = GetKeyPressed();
    if (key == 0) return mrb_nil_value();
    return mrb_fixnum_value(key);
}

/// @function char_pressed
/// @description Get the Unicode character code of the last character pressed this frame.
///   Useful for text input fields. Returns the character, not the key code.
/// @returns [Integer, nil] Unicode character code, or nil if no character was pressed
/// @example char = GMR::Input.char_pressed
///   if char
///     @text += char.chr(Encoding::UTF_8)
///   end
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
static mrb_value mrb_input_map_simple(mrb_state* mrb, mrb_value self) {
    mrb_sym action_sym;
    mrb_value keys;
    mrb_get_args(mrb, "no", &action_sym, &keys);

    // Save action name immediately - mrb_sym_name pointer can be invalidated
    std::string action_name = mrb_sym_name(mrb, action_sym);
    std::vector<gmr_input::InputBinding> bindings = parse_bindings_from_keys(mrb, keys);

    gmr_input::InputManager::instance().define_action(action_name, bindings);

    return self;
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

    // Save action name immediately - mrb_sym_name pointer can be invalidated
    std::string action_name = mrb_sym_name(mrb, action_sym);
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

/// @function map
/// @description Map an action name to input bindings. Supports two forms:
///   Traditional form maps a single action to keys directly.
///   Block form allows defining multiple actions with a DSL.
/// @param action [Symbol] (optional) The action name for traditional form
/// @param keys [Symbol, Array] (optional) Key(s) to bind for traditional form
/// @returns [Module] self for chaining
/// @example # Traditional form with chaining
///   GMR::Input.map(:jump, :space).map(:move_left, [:a, :left])
/// @example # Block DSL form
///   GMR::Input.map do |i|
///     i.action :jump, key: :space
///     i.action :attack, keys: [:z, :x], mouse: :left
///   end
static mrb_value mrb_input_map(mrb_state* mrb, mrb_value self) {
    mrb_value block = mrb_nil_value();
    mrb_sym action_sym = 0;
    mrb_value keys = mrb_nil_value();

    // Parse optional symbol, optional keys, optional block
    mrb_get_args(mrb, "|no&", &action_sym, &keys, &block);

    // IMMEDIATELY save the action name before any other mruby calls
    // mrb_sym_name returns a pointer to internal mruby string table
    // which can be invalidated by subsequent mruby operations
    std::string action_name;
    if (action_sym != 0) {
        action_name = mrb_sym_name(mrb, action_sym);
    }

    if (!mrb_nil_p(block) && action_sym == 0) {
        // Block DSL form: GMR::Input.map { ... }
        mrb_value builder = mrb_obj_new(mrb, input_builder_class, 0, nullptr);

        InputMapBuilderData* data = static_cast<InputMapBuilderData*>(
            mrb_malloc(mrb, sizeof(InputMapBuilderData)));
        data->active = true;
        mrb_data_init(builder, data, &input_builder_data_type);

        // Use safe_yield to catch exceptions in input mapping DSL
        scripting::safe_yield(mrb, block, builder);

        return self;
    }

    if (action_sym != 0) {
        // Traditional form: GMR::Input.map(:action, keys)
        std::vector<gmr_input::InputBinding> bindings = parse_bindings_from_keys(mrb, keys);
        gmr_input::InputManager::instance().define_action(action_name, bindings);
    }

    return self;
}

/// @function unmap
/// @description Remove an action mapping by name.
/// @param action [Symbol] The action name to remove
/// @returns [Module] self for chaining
/// @example GMR::Input.unmap(:jump).unmap(:attack)
static mrb_value mrb_input_unmap(mrb_state* mrb, mrb_value self) {
    mrb_sym action_sym;
    mrb_get_args(mrb, "n", &action_sym);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    gmr_input::InputManager::instance().remove_action(action_name);

    return self;
}

/// @function clear_mappings
/// @description Remove all action mappings.
/// @returns [Module] self for chaining
/// @example GMR::Input.clear_mappings.map(:new_action, :space)
static mrb_value mrb_input_clear_mappings(mrb_state* mrb, mrb_value self) {
    gmr_input::InputManager::instance().clear_actions();
    return self;
}

// ============================================================================
// Action Query Functions (using InputManager)
// ============================================================================

/// @function action_down?
/// @description Check if a mapped action is currently active (any bound input is held).
/// @param action [Symbol] The action name to check
/// @returns [Boolean] true if the action is active
/// @example if GMR::Input.action_down?(:move_left)
///   player.x -= speed
/// end
static mrb_value mrb_input_action_down(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_get_args(mrb, "n", &action_sym);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    bool result = gmr_input::InputManager::instance().action_down(action_name);
    return to_mrb_bool(mrb, result);
}

/// @function action_pressed?
/// @description Check if a mapped action was just triggered this frame.
/// @param action [Symbol] The action name to check
/// @returns [Boolean] true if the action was just triggered
/// @example if GMR::Input.action_pressed?(:jump)
///   player.jump
/// end
static mrb_value mrb_input_action_pressed(mrb_state* mrb, mrb_value) {
    mrb_sym action_sym;
    mrb_get_args(mrb, "n", &action_sym);

    const char* action_name = mrb_sym_name(mrb, action_sym);
    bool result = gmr_input::InputManager::instance().action_pressed(action_name);
    return to_mrb_bool(mrb, result);
}

/// @function action_released?
/// @description Check if a mapped action was just released this frame.
/// @param action [Symbol] The action name to check
/// @returns [Boolean] true if the action was just released
/// @example if GMR::Input.action_released?(:charge_attack)
///   player.release_charge
/// end
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

/// @function on
/// @description Register a callback for an action. The callback fires when the action
///   reaches the specified phase. Returns an ID for later removal with `off`.
/// @param action [Symbol] The action name to listen for
/// @param when [Symbol] (optional, default: :pressed) Phase: :pressed, :down, or :released
/// @param context [Object] (optional) Object to use as self in the callback block
/// @returns [Integer] Callback ID for later removal
/// @example # Simple callback
///   GMR::Input.on(:jump) { player.jump }
/// @example # With phase and context
///   id = GMR::Input.on(:attack, when: :released, context: player) do
///     release_charge_attack
///   end
///   GMR::Input.off(id)  # Remove later
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

/// @function off
/// @description Remove input callback(s). Pass an ID to remove a specific callback,
///   or an action name to remove all callbacks for that action.
/// @param id_or_action [Integer, Symbol] Callback ID or action name
/// @returns [nil]
/// @example GMR::Input.off(callback_id)  # Remove specific callback
/// @example GMR::Input.off(:jump)        # Remove all :jump callbacks
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
// Input Context Builder (Verb-Style DSL)
// ============================================================================

// Builder data for verb-style context DSL
struct InputContextBuilderData {
    std::string context_name;
    bool is_global;
};

static void context_builder_free(mrb_state* mrb, void* ptr) {
    if (ptr) {
        InputContextBuilderData* data = static_cast<InputContextBuilderData*>(ptr);
        data->~InputContextBuilderData();
        mrb_free(mrb, ptr);
    }
}

static const mrb_data_type context_builder_data_type = {
    "Input::ContextBuilder", context_builder_free
};

static RClass* context_builder_class = nullptr;

static InputContextBuilderData* get_context_builder_data(mrb_state* mrb, mrb_value self) {
    return static_cast<InputContextBuilderData*>(
        mrb_data_get_ptr(mrb, self, &context_builder_data_type));
}

// Helper: Parse bindings from mixed arguments (array of keys, single key, hash with mouse:)
static std::vector<gmr_input::InputBinding> parse_bindings_from_args(
    mrb_state* mrb, mrb_value* argv, mrb_int argc)
{
    std::vector<gmr_input::InputBinding> bindings;

    for (mrb_int i = 0; i < argc; i++) {
        if (mrb_array_p(argv[i])) {
            // Array of keys: [:a, :left]
            auto key_bindings = parse_bindings_from_keys(mrb, argv[i]);
            bindings.insert(bindings.end(), key_bindings.begin(), key_bindings.end());
        } else if (mrb_hash_p(argv[i])) {
            // Hash with mouse: or gamepad:
            mrb_value mouse_val = mrb_hash_get(mrb, argv[i],
                mrb_symbol_value(mrb_intern_lit(mrb, "mouse")));
            if (!mrb_nil_p(mouse_val)) {
                auto mouse_bindings = parse_bindings_from_mouse(mrb, mouse_val);
                bindings.insert(bindings.end(), mouse_bindings.begin(), mouse_bindings.end());
            }
        } else if (mrb_symbol_p(argv[i]) || mrb_fixnum_p(argv[i])) {
            // Single key
            auto key_bindings = parse_bindings_from_keys(mrb, argv[i]);
            bindings.insert(bindings.end(), key_bindings.begin(), key_bindings.end());
        }
    }

    return bindings;
}

// Input::ContextBuilder#method_missing - Verb-style action definition
// Usage: move_left [:a, :left]
//        jump :space
//        attack mouse: :left
static mrb_value mrb_context_builder_method_missing(mrb_state* mrb, mrb_value self) {
    mrb_sym method_sym;
    mrb_value* argv;
    mrb_int argc;
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "n*&", &method_sym, &argv, &argc, &block);

    // Save action name immediately - mrb_sym_name pointer can be invalidated
    std::string action_name = mrb_sym_name(mrb, method_sym);

    // Parse bindings from arguments
    std::vector<gmr_input::InputBinding> bindings = parse_bindings_from_args(mrb, argv, argc);

    // Get builder data
    InputContextBuilderData* data = get_context_builder_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid context builder");
        return mrb_nil_value();
    }

    // Register action in appropriate context
    if (data->is_global) {
        gmr_input::InputManager::instance().define_action(action_name, bindings);
    } else {
        gmr_input::InputManager::instance().define_action_in_context(
            data->context_name, action_name, bindings);
    }

    return self;
}

// Input::ContextBuilder#respond_to_missing? - For Ruby introspection
static mrb_value mrb_context_builder_respond_to_missing(mrb_state* mrb, mrb_value self) {
    // We respond to any method (they become action names)
    return mrb_true_value();
}

// Create a context builder object
static mrb_value create_context_builder(mrb_state* mrb, const std::string& context_name, bool is_global) {
    mrb_value obj = mrb_obj_new(mrb, context_builder_class, 0, nullptr);

    // Allocate and initialize data using placement new
    void* ptr = mrb_malloc(mrb, sizeof(InputContextBuilderData));
    InputContextBuilderData* data = new (ptr) InputContextBuilderData();
    data->context_name = context_name;
    data->is_global = is_global;

    mrb_data_init(obj, data, &context_builder_data_type);
    return obj;
}

// ============================================================================
// Context Management (GMR::Input module functions)
// ============================================================================

/// @function push_context
/// @description Push a named input context onto the stack. Actions defined in
///   this context become active. Previous contexts remain on the stack.
/// @param name [Symbol] The context name to push
/// @param blocks_global [Boolean] (optional) If true, global actions are blocked while this context is active
/// @returns [Module] self for chaining
/// @example GMR::Input.push_context(:menu)
///   # :menu actions are now active, global actions still fire
/// @example GMR::Input.push_context(:pause, blocks_global: true)
///   # :pause actions active, global game actions blocked
static mrb_value mrb_input_push_context(mrb_state* mrb, mrb_value self) {
    mrb_sym name_sym;
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "n|H", &name_sym, &kwargs);

    const char* name = mrb_sym_name(mrb, name_sym);
    auto& ctx_stack = gmr_input::ContextStack::instance();
    ctx_stack.push(name);

    // Check for blocks_global option
    if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        mrb_value blocks_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "blocks_global")));
        if (mrb_test(blocks_val)) {
            if (auto* ctx = ctx_stack.current()) {
                ctx->blocks_global = true;
            }
        }
    }

    return self;
}

/// @function pop_context
/// @description Pop the current input context from the stack, returning to
///   the previous context.
/// @returns [Module] self for chaining
/// @example GMR::Input.pop_context  # Return to previous context
static mrb_value mrb_input_pop_context(mrb_state* mrb, mrb_value self) {
    gmr_input::ContextStack::instance().pop();
    return self;
}

/// @function set_context
/// @description Replace the entire context stack with a single context.
///   Clears the stack and sets the named context as the only active context.
/// @param name [Symbol] The context name to set
/// @returns [Module] self for chaining
/// @example GMR::Input.set_context(:gameplay)
static mrb_value mrb_input_set_context(mrb_state* mrb, mrb_value self) {
    mrb_sym name_sym;
    mrb_get_args(mrb, "n", &name_sym);

    const char* name = mrb_sym_name(mrb, name_sym);
    gmr_input::ContextStack::instance().set(name);

    return self;
}

/// @function current_context
/// @description Get the name of the current active input context.
/// @returns [Symbol, nil] Current context name, or nil if no context is active
/// @example context = GMR::Input.current_context
static mrb_value mrb_input_current_context(mrb_state* mrb, mrb_value) {
    std::string name = gmr_input::ContextStack::instance().current_name();
    if (name.empty()) {
        return mrb_nil_value();
    }
    return mrb_symbol_value(mrb_intern_cstr(mrb, name.c_str()));
}

/// @function has_context?
/// @description Check if a named input context exists (has been defined).
/// @param name [Symbol] The context name to check
/// @returns [Boolean] true if the context exists
/// @example if GMR::Input.has_context?(:menu)
///   GMR::Input.push_context(:menu)
/// end
static mrb_value mrb_input_has_context(mrb_state* mrb, mrb_value) {
    mrb_sym name_sym;
    mrb_get_args(mrb, "n", &name_sym);

    const char* name = mrb_sym_name(mrb, name_sym);
    bool result = gmr_input::ContextStack::instance().has(name);

    return to_mrb_bool(mrb, result);
}

// ============================================================================
// Top-Level DSL Functions (input { } and input_context :name { })
// ============================================================================

/// @function input
/// @parent Kernel
/// @description Define global input actions using a verb-style DSL. Actions defined
///   here are always available regardless of context.
/// @returns [nil]
/// @example input do |i|
///   i.jump :space
///   i.move_left [:a, :left]
///   i.attack :z, mouse: :left
/// end
static mrb_value mrb_input_dsl_block(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "input requires a block");
        return mrb_nil_value();
    }

    // Create builder for global context
    mrb_value builder = create_context_builder(mrb, "", true);
    // Use safe_yield to catch exceptions in input DSL
    scripting::safe_yield(mrb, block, builder);

    return mrb_nil_value();
}

/// @function input_context
/// @parent Kernel
/// @description Define input actions for a named context. Context-specific actions
///   are only active when that context is pushed onto the stack.
/// @param name [Symbol] The context name
/// @returns [nil]
/// @example input_context :menu do |i|
///   i.confirm :enter
///   i.cancel :escape
///   i.navigate_up :up
///   i.navigate_down :down
/// end
static mrb_value mrb_input_context_dsl_block(mrb_state* mrb, mrb_value self) {
    mrb_sym name_sym;
    mrb_value block;
    mrb_get_args(mrb, "n&", &name_sym, &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "input_context requires a block");
        return mrb_nil_value();
    }

    const char* context_name = mrb_sym_name(mrb, name_sym);

    // Create builder for named context
    mrb_value builder = create_context_builder(mrb, context_name, false);
    // Use safe_yield to catch exceptions in input context DSL
    scripting::safe_yield(mrb, block, builder);

    return mrb_nil_value();
}

// ============================================================================
// Key Constants Registration
// ============================================================================

/// @constant MOUSE_LEFT
/// @description Left mouse button constant.
/// @constant MOUSE_RIGHT
/// @description Right mouse button constant.
/// @constant MOUSE_MIDDLE
/// @description Middle mouse button (scroll wheel click) constant.
/// @constant MOUSE_SIDE
/// @description Side mouse button constant.
/// @constant MOUSE_EXTRA
/// @description Extra mouse button constant.
/// @constant MOUSE_FORWARD
/// @description Forward navigation mouse button constant.
/// @constant MOUSE_BACK
/// @description Back navigation mouse button constant.
/// @constant KEY_SPACE
/// @description Spacebar key constant.
/// @constant KEY_ESCAPE
/// @description Escape key constant.
/// @constant KEY_ENTER
/// @description Enter/Return key constant.
/// @constant KEY_TAB
/// @description Tab key constant.
/// @constant KEY_BACKSPACE
/// @description Backspace key constant.
/// @constant KEY_DELETE
/// @description Delete key constant.
/// @constant KEY_INSERT
/// @description Insert key constant.
/// @constant KEY_UP
/// @description Up arrow key constant.
/// @constant KEY_DOWN
/// @description Down arrow key constant.
/// @constant KEY_LEFT
/// @description Left arrow key constant.
/// @constant KEY_RIGHT
/// @description Right arrow key constant.
/// @constant KEY_HOME
/// @description Home key constant.
/// @constant KEY_END
/// @description End key constant.
/// @constant KEY_PAGE_UP
/// @description Page Up key constant.
/// @constant KEY_PAGE_DOWN
/// @description Page Down key constant.
/// @constant KEY_LEFT_SHIFT
/// @description Left Shift key constant.
/// @constant KEY_RIGHT_SHIFT
/// @description Right Shift key constant.
/// @constant KEY_LEFT_CONTROL
/// @description Left Control key constant.
/// @constant KEY_RIGHT_CONTROL
/// @description Right Control key constant.
/// @constant KEY_LEFT_ALT
/// @description Left Alt key constant.
/// @constant KEY_RIGHT_ALT
/// @description Right Alt key constant.
/// @constant KEY_F1
/// @description F1 function key constant.
/// @constant KEY_F2
/// @description F2 function key constant.
/// @constant KEY_F3
/// @description F3 function key constant.
/// @constant KEY_F4
/// @description F4 function key constant.
/// @constant KEY_F5
/// @description F5 function key constant.
/// @constant KEY_F6
/// @description F6 function key constant.
/// @constant KEY_F7
/// @description F7 function key constant.
/// @constant KEY_F8
/// @description F8 function key constant.
/// @constant KEY_F9
/// @description F9 function key constant.
/// @constant KEY_F10
/// @description F10 function key constant.
/// @constant KEY_F11
/// @description F11 function key constant.
/// @constant KEY_F12
/// @description F12 function key constant.
/// @constant KEY_A
/// @description A key constant.
/// @constant KEY_B
/// @description B key constant.
/// @constant KEY_C
/// @description C key constant.
/// @constant KEY_D
/// @description D key constant.
/// @constant KEY_E
/// @description E key constant.
/// @constant KEY_F
/// @description F key constant.
/// @constant KEY_G
/// @description G key constant.
/// @constant KEY_H
/// @description H key constant.
/// @constant KEY_I
/// @description I key constant.
/// @constant KEY_J
/// @description J key constant.
/// @constant KEY_K
/// @description K key constant.
/// @constant KEY_L
/// @description L key constant.
/// @constant KEY_M
/// @description M key constant.
/// @constant KEY_N
/// @description N key constant.
/// @constant KEY_O
/// @description O key constant.
/// @constant KEY_P
/// @description P key constant.
/// @constant KEY_Q
/// @description Q key constant.
/// @constant KEY_R
/// @description R key constant.
/// @constant KEY_S
/// @description S key constant.
/// @constant KEY_T
/// @description T key constant.
/// @constant KEY_U
/// @description U key constant.
/// @constant KEY_V
/// @description V key constant.
/// @constant KEY_W
/// @description W key constant.
/// @constant KEY_X
/// @description X key constant.
/// @constant KEY_Y
/// @description Y key constant.
/// @constant KEY_Z
/// @description Z key constant.
/// @constant KEY_0
/// @description 0 number key constant.
/// @constant KEY_1
/// @description 1 number key constant.
/// @constant KEY_2
/// @description 2 number key constant.
/// @constant KEY_3
/// @description 3 number key constant.
/// @constant KEY_4
/// @description 4 number key constant.
/// @constant KEY_5
/// @description 5 number key constant.
/// @constant KEY_6
/// @description 6 number key constant.
/// @constant KEY_7
/// @description 7 number key constant.
/// @constant KEY_8
/// @description 8 number key constant.
/// @constant KEY_9
/// @description 9 number key constant.

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

    // Context management
    mrb_define_module_function(mrb, input, "push_context", mrb_input_push_context, MRB_ARGS_ARG(1, 1));
    mrb_define_module_function(mrb, input, "pop_context", mrb_input_pop_context, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "set_context", mrb_input_set_context, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, input, "current_context", mrb_input_current_context, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, input, "has_context?", mrb_input_has_context, MRB_ARGS_REQ(1));

    // Register MapBuilder internal class (legacy block DSL)
    input_builder_class = mrb_define_class_under(mrb, input, "MapBuilder", mrb->object_class);
    MRB_SET_INSTANCE_TT(input_builder_class, MRB_TT_CDATA);
    mrb_define_method(mrb, input_builder_class, "action", mrb_input_builder_action,
        MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));

    // Register ContextBuilder class (verb-style DSL)
    context_builder_class = mrb_define_class_under(mrb, input, "ContextBuilder", mrb->object_class);
    MRB_SET_INSTANCE_TT(context_builder_class, MRB_TT_CDATA);
    mrb_define_method(mrb, context_builder_class, "method_missing",
        mrb_context_builder_method_missing, MRB_ARGS_ANY());
    mrb_define_method(mrb, context_builder_class, "respond_to_missing?",
        mrb_context_builder_respond_to_missing, MRB_ARGS_ANY());

    // Register key constants
    register_key_constants(mrb, input);

    // Register top-level DSL functions on Kernel
    mrb_define_method(mrb, mrb->kernel_module, "input",
        mrb_input_dsl_block, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, mrb->kernel_module, "input_context",
        mrb_input_context_dsl_block, MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
}

} // namespace bindings
} // namespace gmr
