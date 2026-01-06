#include "gmr/bindings/state_machine.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state_machine/state_machine_manager.hpp"
#include "gmr/input/input_manager.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/variable.h>
#include <mruby/proc.h>

namespace gmr {
namespace bindings {

/// @class GMR::StateMachine
/// @description Lightweight state machine for gameplay logic with automatic animation binding.
/// @example # Complete platformer player with state machine controlling animations and behavior
///   class Player
///     attr_reader :sprite, :state_machine
///
///     def initialize
///       @sprite = Sprite.new(GMR::Graphics::Texture.load("assets/player.png"))
///       @sprite.source_rect = Rect.new(0, 0, 32, 48)
///       @velocity_y = 0
///       @on_ground = false
///       @stamina = 100
///
///       # Animation lookup table for state machine
///       @animations = {
///         idle: GMR::SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8),
///         run: GMR::SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
///         jump: GMR::SpriteAnimation.new(@sprite, frames: 16..18, fps: 8, loop: false, columns: 8),
///         fall: GMR::SpriteAnimation.new(@sprite, frames: 19..21, fps: 8, columns: 8),
///         attack: GMR::SpriteAnimation.new(@sprite, frames: 24..29, fps: 15, loop: false, columns: 8)
///       }
///
///       setup_state_machine
///       setup_input
///     end
///
///     def setup_input
///       input do |i|
///         i.move_left [:a, :left]
///         i.move_right [:d, :right]
///         i.jump :space
///         i.attack :z
///       end
///     end
///
///     def setup_state_machine
///       state_machine do
///         state :idle do
///           animate :idle
///           on :move, :run
///           on :jump, :jumping, if: -> { @on_ground && @stamina >= 10 }
///           on :attack, :attacking, if: -> { @stamina >= 20 }
///           on :fall, :falling
///         end
///
///         state :run do
///           animate :run
///           on :stop, :idle
///           on :jump, :jumping, if: -> { @on_ground && @stamina >= 10 }
///           on :attack, :attacking, if: -> { @stamina >= 20 }
///           on :fall, :falling
///         end
///
///         state :jumping do
///           animate :jump
///           enter do
///             @velocity_y = -12
///             @stamina -= 10
///             GMR::Audio::Sound.play("assets/jump.wav")
///           end
///           on :peak, :falling
///           on :land, :idle
///         end
///
///         state :falling do
///           animate :fall
///           on :land, :idle
///         end
///
///         state :attacking do
///           animate :attack
///           enter do
///             @stamina -= 20
///             @attack_hitbox_active = true
///           end
///           exit { @attack_hitbox_active = false }
///           on :attack_done, :idle
///         end
///       end
///     end
///
///     def update(dt)
///       # Movement triggers state transitions
///       if GMR::Input.action_down?(:move_left) || GMR::Input.action_down?(:move_right)
///         state_machine.trigger(:move)
///       else
///         state_machine.trigger(:stop)
///       end
///
///       # Jump and attack input
///       state_machine.trigger(:jump) if GMR::Input.action_pressed?(:jump)
///       state_machine.trigger(:attack) if GMR::Input.action_pressed?(:attack)
///
///       # Physics checks
///       state_machine.trigger(:fall) if !@on_ground && @velocity_y > 0
///       state_machine.trigger(:peak) if @velocity_y >= 0 && state_machine.state == :jumping
///       state_machine.trigger(:land) if @on_ground
///
///       # Attack animation complete check
///       if state_machine.state == :attacking && @animations[:attack].complete?
///         state_machine.trigger(:attack_done)
///       end
///     end
///   end
/// @example # Enemy AI with patrol and chase states
///   class Enemy
///     def initialize(x, y, patrol_left, patrol_right)
///       @sprite = Sprite.new(GMR::Graphics::Texture.load("assets/enemy.png"))
///       @sprite.x = x
///       @sprite.y = y
///       @patrol_left = patrol_left
///       @patrol_right = patrol_right
///       @direction = 1
///
///       @animations = {
///         idle: GMR::SpriteAnimation.new(@sprite, frames: 0..3, fps: 4, columns: 6),
///         walk: GMR::SpriteAnimation.new(@sprite, frames: 4..9, fps: 8, columns: 6),
///         chase: GMR::SpriteAnimation.new(@sprite, frames: 10..15, fps: 12, columns: 6),
///         attack: GMR::SpriteAnimation.new(@sprite, frames: 16..21, fps: 15, loop: false, columns: 6)
///       }
///
///       setup_ai_state_machine
///     end
///
///     def setup_ai_state_machine
///       state_machine do
///         state :patrol do
///           animate :walk
///           on :see_player, :chase
///           on :reach_edge, :idle
///         end
///
///         state :idle do
///           animate :idle
///           enter { @wait_timer = 1.5 }
///           on :wait_done, :patrol
///           on :see_player, :chase
///         end
///
///         state :chase do
///           animate :chase
///           enter { GMR::Audio::Sound.play("assets/alert.wav") }
///           on :lose_player, :patrol
///           on :in_range, :attack
///         end
///
///         state :attack do
///           animate :attack
///           enter { @attack_cooldown = 0.8 }
///           on :attack_done, :chase
///         end
///       end
///     end
///
///     def update(dt, player)
///       distance = (player.sprite.x - @sprite.x).abs
///       can_see = distance < 200
///       in_range = distance < 40
///
///       state_machine.trigger(:see_player) if can_see
///       state_machine.trigger(:lose_player) if !can_see && state_machine.state == :chase
///       state_machine.trigger(:in_range) if in_range
///
///       case state_machine.state
///       when :patrol
///         move_patrol(dt)
///       when :chase
///         chase_player(dt, player)
///       when :idle
///         @wait_timer -= dt
///         state_machine.trigger(:wait_done) if @wait_timer <= 0
///       when :attack
///         @attack_cooldown -= dt
///         state_machine.trigger(:attack_done) if @attack_cooldown <= 0
///       end
///     end
///   end
/// @example # Menu system with input-driven state machine
///   class MenuScreen
///     def initialize
///       @selected_index = 0
///       @options = ["New Game", "Continue", "Options", "Quit"]
///
///       input_context :menu do |i|
///         i.confirm :enter
///         i.cancel :escape
///         i.nav_up :up
///         i.nav_down :down
///       end
///
///       state_machine do
///         state :main do
///           on_input :confirm, :confirm_selection
///           on_input :nav_up, :main
///           on_input :nav_down, :main
///           enter { @selected_index = 0 }
///         end
///
///         state :confirm_selection do
///           enter { handle_selection }
///           on :back, :main
///         end
///       end
///
///       GMR::Input.push_context(:menu)
///     end
///
///     def handle_selection
///       case @selected_index
///       when 0 then SceneManager.load(GameScene.new)
///       when 3 then GMR::System.quit
///       else state_machine.trigger(:back)
///       end
///     end
///   end

// ============================================================================
// StateMachine Binding Data
// ============================================================================

struct StateMachineData {
    StateMachineHandle handle;
};

static void state_machine_free(mrb_state* mrb, void* ptr) {
    StateMachineData* data = static_cast<StateMachineData*>(ptr);
    if (data) {
        state_machine::StateMachineManager::instance().destroy(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type state_machine_data_type = {
    "StateMachine", state_machine_free
};

static StateMachineData* get_machine_data(mrb_state* mrb, mrb_value self) {
    return static_cast<StateMachineData*>(mrb_data_get_ptr(mrb, self, &state_machine_data_type));
}

// ============================================================================
// Builder Binding Data
// ============================================================================

struct BuilderData {
    StateMachineHandle machine_handle;
    mrb_sym current_state;  // State being defined during state {} block
};

static void builder_free(mrb_state* mrb, void* ptr) {
    BuilderData* data = static_cast<BuilderData*>(ptr);
    if (data) {
        mrb_free(mrb, data);
    }
}

static const mrb_data_type builder_data_type = {
    "StateMachine::Builder", builder_free
};

static BuilderData* get_builder_data(mrb_state* mrb, mrb_value self) {
    return static_cast<BuilderData*>(mrb_data_get_ptr(mrb, self, &builder_data_type));
}

// ============================================================================
// Helper: Create machine wrapper object
// ============================================================================

static RClass* state_machine_class = nullptr;
static RClass* builder_class = nullptr;

static mrb_value create_machine_object(mrb_state* mrb, StateMachineHandle handle) {
    if (!state_machine_class) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "StateMachine class not initialized");
        return mrb_nil_value();
    }
    mrb_value obj = mrb_obj_new(mrb, state_machine_class, 0, nullptr);

    StateMachineData* data = static_cast<StateMachineData*>(
        mrb_malloc(mrb, sizeof(StateMachineData)));
    data->handle = handle;
    mrb_data_init(obj, data, &state_machine_data_type);

    return obj;
}

static mrb_value create_builder_object(mrb_state* mrb, StateMachineHandle handle) {
    if (!builder_class) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Builder class not initialized");
        return mrb_nil_value();
    }
    mrb_value obj = mrb_obj_new(mrb, builder_class, 0, nullptr);

    BuilderData* data = static_cast<BuilderData*>(
        mrb_malloc(mrb, sizeof(BuilderData)));
    data->machine_handle = handle;
    data->current_state = 0;
    mrb_data_init(obj, data, &builder_data_type);

    return obj;
}

// ============================================================================
// Helper: GC-Safe Callback Storage
// ============================================================================

// Safely store a callback in the machine's @_callbacks array for GC protection.
// This function handles null checks and arena protection to prevent GC race conditions.
static void store_callback_for_gc_protection(mrb_state* mrb, StateMachineHandle handle, mrb_value callback) {
    if (mrb_nil_p(callback)) return;

    // Get machine fresh (not from stale pointer)
    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(handle);
    if (!machine) return;

    // Validate machine wrapper exists before any mruby operations
    if (mrb_nil_p(machine->ruby_machine_obj)) return;

    // Use arena to protect temporary objects during this sequence
    int arena_idx = mrb_gc_arena_save(mrb);

    // Get or create the callbacks array
    mrb_value callbacks_arr = mrb_iv_get(mrb, machine->ruby_machine_obj,
        mrb_intern_cstr(mrb, "@_callbacks"));

    if (mrb_nil_p(callbacks_arr)) {
        callbacks_arr = mrb_ary_new(mrb);
        // Re-validate machine after mrb_ary_new (could trigger GC)
        machine = state_machine::StateMachineManager::instance().get(handle);
        if (!machine || mrb_nil_p(machine->ruby_machine_obj)) {
            mrb_gc_arena_restore(mrb, arena_idx);
            return;
        }
        mrb_iv_set(mrb, machine->ruby_machine_obj,
            mrb_intern_cstr(mrb, "@_callbacks"), callbacks_arr);
    }

    mrb_ary_push(mrb, callbacks_arr, callback);

    mrb_gc_arena_restore(mrb, arena_idx);
}

// ============================================================================
// GMR::StateMachine Class Methods
// ============================================================================

/// @method attach
/// @description Attach a state machine to an object. Called internally by Object#state_machine.
/// @param owner [Object] The object to attach the state machine to
/// @returns [StateMachine] The created state machine
static mrb_value mrb_state_machine_attach(mrb_state* mrb, mrb_value) {
    mrb_value owner;
    mrb_value block;
    mrb_get_args(mrb, "o&", &owner, &block);

    auto& manager = state_machine::StateMachineManager::instance();
    StateMachineHandle handle = manager.create();

    state_machine::StateMachineState* machine = manager.get(handle);
    if (!machine) {
        return mrb_nil_value();
    }

    machine->owner = owner;

    // Create Ruby wrapper object FIRST (before any mruby calls that could trigger GC)
    mrb_value machine_obj = create_machine_object(mrb, handle);

    machine->ruby_machine_obj = machine_obj;
    mrb_gc_register(mrb, machine_obj);

    // Store owner reference to prevent GC (NOW safe - wrapper is registered)
    mrb_iv_set(mrb, machine_obj, mrb_intern_cstr(mrb, "@owner"), owner);

    // Auto-detect sprite from @sprite (AFTER GC protection is in place)
    manager.detect_sprite(mrb, handle);

    // Create DSL builder and call block
    mrb_value builder = create_builder_object(mrb, handle);

    // Store builder reference on machine temporarily for nested access
    mrb_iv_set(mrb, machine_obj, mrb_intern_cstr(mrb, "@_builder"), builder);

    if (!mrb_nil_p(block)) {
        // Execute block with builder as 'self' using instance_exec
        // This allows DSL methods like 'state' to be called on the builder
        // IMPORTANT: Must use safe_instance_exec, not safe_method_call with block as arg
        scripting::safe_instance_exec(mrb, builder, block);
    }

    // Re-get machine pointer after executing Ruby code (GC might have moved things)
    machine = manager.get(handle);
    if (!machine) {
        // Machine was destroyed during DSL execution - this is an error
        return mrb_nil_value();
    }

    // Set initial state to first defined state if not set
    if (machine->initial_state == 0 && !machine->states.empty()) {
        machine->initial_state = machine->states.begin()->first;
    }

    // Store machine on owner object for later access
    mrb_iv_set(mrb, owner, mrb_intern_cstr(mrb, "@_state_machine"), machine_obj);

    return machine_obj;
}

/// @method count
/// @description Get the number of active state machines.
/// @returns [Integer] Number of active machines
static mrb_value mrb_state_machine_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(
        static_cast<mrb_int>(state_machine::StateMachineManager::instance().count()));
}

// ============================================================================
// GMR::StateMachine Instance Methods
// ============================================================================

/// @method trigger
/// @description Trigger an event, causing a transition if one is defined.
/// @param event [Symbol] The event to trigger
/// @returns [Boolean] true if a transition occurred
/// @example # Trigger based on collision detection
///   def check_collectibles
///     @collectibles.each do |coin|
///       if GMR::Collision.rect_rect?(player_bounds, coin.bounds)
///         coin.state_machine.trigger(:collected)
///         @score += coin.value
///       end
///     end
///   end
/// @example # Chain triggers from animation callbacks
///   @animations[:death].on_complete do
///     state_machine.trigger(:death_anim_finished)
///   end
static mrb_value mrb_state_machine_trigger(mrb_state* mrb, mrb_value self) {
    mrb_sym event;
    mrb_get_args(mrb, "n", &event);

    StateMachineData* data = get_machine_data(mrb, self);
    if (!data) return mrb_false_value();

    bool transitioned = state_machine::StateMachineManager::instance()
        .trigger(mrb, data->handle, event);

    return mrb_bool_value(transitioned);
}

/// @method state
/// @description Get the current state.
/// @returns [Symbol] Current state symbol
static mrb_value mrb_state_machine_get_state(mrb_state* mrb, mrb_value self) {
    StateMachineData* data = get_machine_data(mrb, self);
    if (!data) return mrb_nil_value();

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->handle);
    if (!machine) return mrb_nil_value();

    if (machine->current_state == 0) return mrb_nil_value();
    return mrb_symbol_value(machine->current_state);
}

/// @method state=
/// @description Force a state change, bypassing transitions.
/// @param new_state [Symbol] The state to switch to
/// @returns [Symbol] The new state
static mrb_value mrb_state_machine_set_state(mrb_state* mrb, mrb_value self) {
    mrb_sym state;
    mrb_get_args(mrb, "n", &state);

    StateMachineData* data = get_machine_data(mrb, self);
    if (!data) return mrb_symbol_value(state);

    state_machine::StateMachineManager::instance()
        .set_state(mrb, data->handle, state);

    return mrb_symbol_value(state);
}

/// @method active?
/// @description Check if the state machine is active.
/// @returns [Boolean] true if active
static mrb_value mrb_state_machine_active_p(mrb_state* mrb, mrb_value self) {
    StateMachineData* data = get_machine_data(mrb, self);
    if (!data) return mrb_false_value();

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->handle);
    if (!machine) return mrb_false_value();

    return mrb_bool_value(machine->active);
}

// ============================================================================
// GMR::StateMachine::Builder Instance Methods (DSL)
// ============================================================================

/// @method state
/// @description Define a state with its behavior.
/// @param name [Symbol] The state name
static mrb_value mrb_builder_state(mrb_state* mrb, mrb_value self) {
    mrb_sym name;
    mrb_value block;
    mrb_get_args(mrb, "n&", &name, &block);

    BuilderData* data = get_builder_data(mrb, self);
    if (!data) {
        return self;
    }

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->machine_handle);
    if (!machine) {
        return self;
    }

    // Create state definition
    state_machine::StateDefinition state_def;
    state_def.name = name;
    machine->states[name] = state_def;

    // Set as initial state if first
    if (machine->initial_state == 0) {
        machine->initial_state = name;
    }

    // Set current state context for nested calls
    data->current_state = name;

    // Call block to define state behavior
    if (!mrb_nil_p(block)) {
        // Execute block with builder as 'self' using instance_exec
        // This allows DSL methods like 'on', 'enter', 'exit' to be called
        // IMPORTANT: Must use safe_instance_exec, not safe_method_call with block as arg
        scripting::safe_instance_exec(mrb, self, block);
    }

    data->current_state = 0;
    return self;
}

/// @method animate
/// @description Set the animation to play when entering this state.
/// @param name [Symbol] The animation name (looked up in @animations hash)
static mrb_value mrb_builder_animate(mrb_state* mrb, mrb_value self) {
    mrb_sym anim_name;
    mrb_get_args(mrb, "n", &anim_name);

    BuilderData* data = get_builder_data(mrb, self);
    if (!data || data->current_state == 0) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "animate must be called inside state block");
        return self;
    }

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->machine_handle);
    if (!machine) return self;

    state_machine::StateDefinition* state_def = machine->get_state_def(data->current_state);
    if (state_def) {
        state_def->animation_name = anim_name;
    }

    return self;
}

/// @method on
/// @description Define a transition from this state.
/// @param event [Symbol] The event that triggers the transition
/// @param target [Symbol] The target state
/// @param if: [Proc] Optional condition (must return truthy for transition to occur)
/// @example # Conditional transition based on game state
///   state :idle do
///     on :attack, :light_attack, if: -> { @stamina >= 10 }
///     on :attack, :exhausted, if: -> { @stamina < 10 }
///     on :special, :power_attack, if: -> { @power_meter >= 100 }
///   end
/// @example # Multiple transitions from same state
///   state :grounded do
///     on :jump, :jumping
///     on :crouch, :crouching
///     on :damage, :hurt
///     on :fall, :falling
///   end
static mrb_value mrb_builder_on(mrb_state* mrb, mrb_value self) {
    mrb_sym event, target;
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "nn|H", &event, &target, &kwargs);

    BuilderData* data = get_builder_data(mrb, self);
    if (!data || data->current_state == 0) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "on must be called inside state block");
        return self;
    }

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->machine_handle);
    if (!machine) return self;

    state_machine::StateDefinition* state_def = machine->get_state_def(data->current_state);
    if (!state_def) return self;

    state_machine::TransitionDefinition trans;
    trans.event = event;
    trans.target_state = target;

    // Parse :if condition
    if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        mrb_value if_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "if")));
        if (!mrb_nil_p(if_val)) {
            trans.condition = if_val;

            // GC protect condition using safe helper (handles arena and null checks)
            store_callback_for_gc_protection(mrb, data->machine_handle, if_val);
        }
    }

    state_def->transitions.push_back(trans);
    return self;
}

/// @method enter
/// @description Set a callback to run when entering this state.
/// @example # Initialize state-specific data on enter
///   state :charging do
///     enter do
///       @charge_start_time = GMR::Time.total
///       @charge_particles = ParticleEmitter.new(:charge)
///       GMR::Audio::Sound.play("assets/charge_start.wav")
///     end
///     # ...
///   end
static mrb_value mrb_builder_enter(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    BuilderData* data = get_builder_data(mrb, self);
    if (!data || data->current_state == 0) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "enter must be called inside state block");
        return self;
    }

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->machine_handle);
    if (!machine) return self;

    state_machine::StateDefinition* state_def = machine->get_state_def(data->current_state);
    if (state_def && !mrb_nil_p(block)) {
        state_def->enter_callback = block;

        // GC protect callback using safe helper (handles arena and null checks)
        store_callback_for_gc_protection(mrb, data->machine_handle, block);
    }

    return self;
}

/// @method exit
/// @description Set a callback to run when exiting this state.
/// @example # Clean up state resources on exit
///   state :charging do
///     exit do
///       @charge_particles&.stop
///       if @charge_level >= 100
///         GMR::Audio::Sound.play("assets/charge_full.wav")
///       end
///     end
///   end
/// @example # Cancel pending actions on state exit
///   state :aiming do
///     exit do
///       @aim_line.visible = false
///       @slowmo_active = false
///       GMR::Time.scale = 1.0
///     end
///   end
static mrb_value mrb_builder_exit(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    BuilderData* data = get_builder_data(mrb, self);
    if (!data || data->current_state == 0) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "exit must be called inside state block");
        return self;
    }

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->machine_handle);
    if (!machine) return self;

    state_machine::StateDefinition* state_def = machine->get_state_def(data->current_state);
    if (state_def && !mrb_nil_p(block)) {
        state_def->exit_callback = block;

        // GC protect callback using safe helper (handles arena and null checks)
        store_callback_for_gc_protection(mrb, data->machine_handle, block);
    }

    return self;
}

/// @method on_input
/// @description Define an input-driven transition from this state.
/// @param action [Symbol] The input action that triggers the transition
/// @param target [Symbol] The target state
/// @param when: [Symbol] Input phase (:pressed, :released, :held) - default :pressed
/// @param if: [Proc] Optional condition (must return truthy for transition to occur)
/// @example
///   state :idle do
///     on_input :jump, :air
///     on_input :attack, :attack, when: :pressed, if: -> { @stamina > 0 }
///   end
static mrb_value mrb_builder_on_input(mrb_state* mrb, mrb_value self) {
    mrb_sym action_sym, target_sym;
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "nn|H", &action_sym, &target_sym, &kwargs);

    BuilderData* data = get_builder_data(mrb, self);
    if (!data || data->current_state == 0) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "on_input must be called inside state block");
        return self;
    }

    // Build the input binding
    input::StateMachineInputBinding binding;
    binding.machine = data->machine_handle;
    binding.current_state = data->current_state;
    binding.action = mrb_sym_name(mrb, action_sym);
    binding.target_state = target_sym;
    binding.phase = input::InputPhase::Pressed;  // Default

    // Parse optional kwargs
    if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        // Parse :when (phase)
        mrb_value when_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "when")));
        if (!mrb_nil_p(when_val)) {
            binding.phase = parse_input_phase(mrb, when_val);
        }

        // Parse :if (condition)
        mrb_value if_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "if")));
        if (!mrb_nil_p(if_val)) {
            binding.condition = if_val;

            // GC protect condition using safe helper (handles arena and null checks)
            store_callback_for_gc_protection(mrb, data->machine_handle, if_val);
        }
    }

    // Register with InputManager
    input::InputManager::instance().register_state_machine_binding(mrb, binding);

    return self;
}

// ============================================================================
// PendingInputTransition (for fluent .hold/.release modifiers)
// ============================================================================

struct PendingTransitionData {
    std::string action;
    bool forced;
    StateMachineHandle machine;
    mrb_sym current_state;
};

static void pending_transition_free(mrb_state* mrb, void* ptr) {
    if (ptr) {
        PendingTransitionData* data = static_cast<PendingTransitionData*>(ptr);
        data->~PendingTransitionData();
        mrb_free(mrb, ptr);
    }
}

static const mrb_data_type pending_transition_data_type = {
    "StateMachine::PendingTransition", pending_transition_free
};

static RClass* pending_transition_class = nullptr;

static PendingTransitionData* get_pending_data(mrb_state* mrb, mrb_value self) {
    return static_cast<PendingTransitionData*>(
        mrb_data_get_ptr(mrb, self, &pending_transition_data_type));
}

// Helper: Register input binding with specified phase
static void register_input_binding_with_phase(
    mrb_state* mrb,
    const std::string& action,
    mrb_sym target_state,
    input::InputPhase phase,
    mrb_value condition,
    bool forced,
    StateMachineHandle machine,
    mrb_sym current_state
) {
    input::StateMachineInputBinding binding;
    binding.machine = machine;
    binding.current_state = current_state;
    binding.action = action;
    binding.target_state = target_state;
    binding.phase = phase;
    binding.condition = condition;
    binding.forced = forced;

    input::InputManager::instance().register_state_machine_binding(mrb, binding);
}

// Create a pending transition object
static mrb_value create_pending_transition(mrb_state* mrb, BuilderData* builder,
                                            const std::string& action, bool forced) {
    if (!pending_transition_class) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "PendingTransition class not initialized");
        return mrb_nil_value();
    }
    mrb_value obj = mrb_obj_new(mrb, pending_transition_class, 0, nullptr);

    void* ptr = mrb_malloc(mrb, sizeof(PendingTransitionData));
    PendingTransitionData* data = new (ptr) PendingTransitionData();
    data->action = action;
    data->forced = forced;
    data->machine = builder->machine_handle;
    data->current_state = builder->current_state;

    mrb_data_init(obj, data, &pending_transition_data_type);
    return obj;
}

// PendingTransition#hold - Register with :held phase
static mrb_value mrb_pending_transition_hold(mrb_state* mrb, mrb_value self) {
    mrb_sym target;
    mrb_value kwargs = mrb_nil_value();
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "n|H&", &target, &kwargs, &block);

    PendingTransitionData* data = get_pending_data(mrb, self);
    if (!data) return mrb_nil_value();

    // Parse condition from kwargs or block
    mrb_value condition = mrb_nil_value();
    if (!mrb_nil_p(block)) {
        condition = block;
    } else if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        mrb_value if_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "if")));
        if (!mrb_nil_p(if_val)) {
            condition = if_val;
        }
    }

    register_input_binding_with_phase(mrb, data->action, target,
        input::InputPhase::Held, condition, data->forced,
        data->machine, data->current_state);

    return mrb_nil_value();
}

// PendingTransition#release - Register with :released phase
static mrb_value mrb_pending_transition_release(mrb_state* mrb, mrb_value self) {
    mrb_sym target;
    mrb_value kwargs = mrb_nil_value();
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "n|H&", &target, &kwargs, &block);

    PendingTransitionData* data = get_pending_data(mrb, self);
    if (!data) return mrb_nil_value();

    // Parse condition from kwargs or block
    mrb_value condition = mrb_nil_value();
    if (!mrb_nil_p(block)) {
        condition = block;
    } else if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        mrb_value if_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "if")));
        if (!mrb_nil_p(if_val)) {
            condition = if_val;
        }
    }

    register_input_binding_with_phase(mrb, data->action, target,
        input::InputPhase::Released, condition, data->forced,
        data->machine, data->current_state);

    return mrb_nil_value();
}

// PendingTransition#press - Register with :pressed phase (explicit)
static mrb_value mrb_pending_transition_press(mrb_state* mrb, mrb_value self) {
    mrb_sym target;
    mrb_value kwargs = mrb_nil_value();
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "n|H&", &target, &kwargs, &block);

    PendingTransitionData* data = get_pending_data(mrb, self);
    if (!data) return mrb_nil_value();

    // Parse condition from kwargs or block
    mrb_value condition = mrb_nil_value();
    if (!mrb_nil_p(block)) {
        condition = block;
    } else if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        mrb_value if_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "if")));
        if (!mrb_nil_p(if_val)) {
            condition = if_val;
        }
    }

    register_input_binding_with_phase(mrb, data->action, target,
        input::InputPhase::Pressed, condition, data->forced,
        data->machine, data->current_state);

    return mrb_nil_value();
}

// ============================================================================
// Builder method_missing (Verb-Style Input Transitions)
// ============================================================================

// Reserved method names that should NOT be treated as input actions
static bool is_reserved_method(const char* name) {
    static const char* reserved[] = {
        "state", "on", "on_input", "animate", "enter", "exit",
        "initialize", "method_missing", "respond_to_missing?"
    };
    for (const char* r : reserved) {
        if (strcmp(name, r) == 0) return true;
    }
    return false;
}

/// Builder#method_missing - Verb-style input transition
/// Usage: jump :air
///        attack.hold :charge
///        die! :dead
static mrb_value mrb_builder_method_missing(mrb_state* mrb, mrb_value self) {
    mrb_sym method_sym;
    mrb_value* argv;
    mrb_int argc;
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "n*&", &method_sym, &argv, &argc, &block);

    const char* method_name = mrb_sym_name(mrb, method_sym);
    size_t name_len = strlen(method_name);

    // Check for reserved methods
    if (is_reserved_method(method_name)) {
        mrb_raisef(mrb, E_NOMETHOD_ERROR,
            "undefined method `%s' for StateMachine::Builder", method_name);
        return mrb_nil_value();
    }

    BuilderData* data = get_builder_data(mrb, self);
    if (!data || data->current_state == 0) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "Input action '%s' must be called inside a state block", method_name);
        return mrb_nil_value();
    }

    // Check for forced transition: action! :target
    bool forced = (name_len > 0 && method_name[name_len - 1] == '!');
    std::string action_name = forced
        ? std::string(method_name, name_len - 1)
        : method_name;

    // No arguments = return PendingTransition for fluent .hold/.release
    if (argc == 0) {
        return create_pending_transition(mrb, data, action_name, forced);
    }

    // First argument must be target state
    if (!mrb_symbol_p(argv[0])) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "Expected target state symbol for '%s'", method_name);
        return mrb_nil_value();
    }

    mrb_sym target_state = mrb_symbol(argv[0]);

    // Parse optional condition from kwargs or block
    mrb_value condition = mrb_nil_value();
    if (!mrb_nil_p(block)) {
        condition = block;
    } else if (argc >= 2 && mrb_hash_p(argv[1])) {
        mrb_value if_val = mrb_hash_get(mrb, argv[1],
            mrb_symbol_value(mrb_intern_lit(mrb, "if")));
        if (!mrb_nil_p(if_val)) {
            condition = if_val;
        }
    }

    // Register the input binding with default phase (:pressed)
    register_input_binding_with_phase(mrb, action_name, target_state,
        input::InputPhase::Pressed, condition, forced,
        data->machine_handle, data->current_state);

    return self;
}

/// Builder#respond_to_missing? - For Ruby introspection
static mrb_value mrb_builder_respond_to_missing(mrb_state* mrb, mrb_value self) {
    mrb_sym method_sym;
    mrb_get_args(mrb, "n|o", &method_sym);

    const char* method_name = mrb_sym_name(mrb, method_sym);

    // Return false for reserved methods, true for anything else
    return is_reserved_method(method_name) ? mrb_false_value() : mrb_true_value();
}

// ============================================================================
// Object#state_machine Mixin
// ============================================================================

/// @method state_machine
/// @description Define or get the state machine for this object.
/// @returns [StateMachine] The state machine
static mrb_value mrb_object_state_machine(mrb_state* mrb, mrb_value self) {
    mrb_value block = mrb_nil_value();
    mrb_get_args(mrb, "|&", &block);

    // If no block, return existing state machine
    if (mrb_nil_p(block)) {
        mrb_value existing = mrb_iv_get(mrb, self,
            mrb_intern_cstr(mrb, "@_state_machine"));
        return existing;
    }

    // Otherwise, create new state machine with block
    // Use the static class pointer directly (already initialized during registration)
    if (!state_machine_class) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "StateMachine class not initialized");
        return mrb_nil_value();
    }

    mrb_value args[1] = { self };
    mrb_value result = mrb_funcall_with_block(mrb, mrb_obj_value(state_machine_class),
        mrb_intern_lit(mrb, "attach"), 1, args, block);

    // Check for exception from attach call
    if (mrb->exc) {
        scripting::check_error(mrb, "state_machine");
        return mrb_nil_value();
    }

    return result;
}

// ============================================================================
// Registration
// ============================================================================

void register_state_machine(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* core = mrb_module_get_under(mrb, gmr, "Core");

    // GMR::Core::StateMachine class
    state_machine_class = mrb_define_class_under(mrb, core, "StateMachine", mrb->object_class);
    MRB_SET_INSTANCE_TT(state_machine_class, MRB_TT_CDATA);

    // Class methods
    mrb_define_class_method(mrb, state_machine_class, "attach", mrb_state_machine_attach,
        MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
    mrb_define_class_method(mrb, state_machine_class, "count", mrb_state_machine_count,
        MRB_ARGS_NONE());

    // Instance methods
    mrb_define_method(mrb, state_machine_class, "trigger", mrb_state_machine_trigger,
        MRB_ARGS_REQ(1));
    mrb_define_method(mrb, state_machine_class, "state", mrb_state_machine_get_state,
        MRB_ARGS_NONE());
    mrb_define_method(mrb, state_machine_class, "state=", mrb_state_machine_set_state,
        MRB_ARGS_REQ(1));
    mrb_define_method(mrb, state_machine_class, "active?", mrb_state_machine_active_p,
        MRB_ARGS_NONE());

    // GMR::StateMachine::Builder (internal)
    builder_class = mrb_define_class_under(mrb, state_machine_class, "Builder",
        mrb->object_class);
    MRB_SET_INSTANCE_TT(builder_class, MRB_TT_CDATA);

    mrb_define_method(mrb, builder_class, "state", mrb_builder_state,
        MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
    mrb_define_method(mrb, builder_class, "animate", mrb_builder_animate,
        MRB_ARGS_REQ(1));
    mrb_define_method(mrb, builder_class, "on", mrb_builder_on,
        MRB_ARGS_ARG(2, 1));
    mrb_define_method(mrb, builder_class, "on_input", mrb_builder_on_input,
        MRB_ARGS_ARG(2, 1));
    mrb_define_method(mrb, builder_class, "enter", mrb_builder_enter,
        MRB_ARGS_BLOCK());
    mrb_define_method(mrb, builder_class, "exit", mrb_builder_exit,
        MRB_ARGS_BLOCK());

    // Verb-style DSL via method_missing
    mrb_define_method(mrb, builder_class, "method_missing",
        mrb_builder_method_missing, MRB_ARGS_ANY());
    mrb_define_method(mrb, builder_class, "respond_to_missing?",
        mrb_builder_respond_to_missing, MRB_ARGS_ANY());

    // GMR::StateMachine::PendingTransition (for fluent .hold/.release)
    pending_transition_class = mrb_define_class_under(mrb, state_machine_class,
        "PendingTransition", mrb->object_class);
    MRB_SET_INSTANCE_TT(pending_transition_class, MRB_TT_CDATA);

    mrb_define_method(mrb, pending_transition_class, "hold",
        mrb_pending_transition_hold, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_method(mrb, pending_transition_class, "release",
        mrb_pending_transition_release, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_method(mrb, pending_transition_class, "press",
        mrb_pending_transition_press, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());

    // Add state_machine to Object for DSL sugar
    mrb_define_method(mrb, mrb->object_class, "state_machine",
        mrb_object_state_machine, MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
}

} // namespace bindings
} // namespace gmr
