#include "gmr/bindings/state_machine.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state_machine/state_machine_manager.hpp"
#include "gmr/input/input_manager.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/variable.h>
#include <mruby/proc.h>

namespace gmr {
namespace bindings {

/// @class GMR::StateMachine
/// @description Lightweight state machine for gameplay logic with automatic animation binding.
/// @example # Define a state machine on any object
///   player.state_machine do
///     state :idle do
///       animate :idle
///       on :move, :run
///       on :jump, :air, if: -> { @stamina > 0 }
///       enter { puts "Now idle" }
///     end
///
///     state :run do
///       animate :run
///       on :stop, :idle
///     end
///   end
///
///   # Trigger transitions
///   player.state_machine.trigger(:move)
///   player.state_machine.state  # => :run

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
    mrb_value obj = mrb_obj_new(mrb, state_machine_class, 0, nullptr);

    StateMachineData* data = static_cast<StateMachineData*>(
        mrb_malloc(mrb, sizeof(StateMachineData)));
    data->handle = handle;
    mrb_data_init(obj, data, &state_machine_data_type);

    return obj;
}

static mrb_value create_builder_object(mrb_state* mrb, StateMachineHandle handle) {
    mrb_value obj = mrb_obj_new(mrb, builder_class, 0, nullptr);

    BuilderData* data = static_cast<BuilderData*>(
        mrb_malloc(mrb, sizeof(BuilderData)));
    data->machine_handle = handle;
    data->current_state = 0;
    mrb_data_init(obj, data, &builder_data_type);

    return obj;
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

    machine->owner = owner;

    // Auto-detect sprite from @sprite
    manager.detect_sprite(mrb, handle);

    // Create Ruby wrapper object
    mrb_value machine_obj = create_machine_object(mrb, handle);
    machine->ruby_machine_obj = machine_obj;
    mrb_gc_register(mrb, machine_obj);

    // Store owner reference to prevent GC
    mrb_iv_set(mrb, machine_obj, mrb_intern_cstr(mrb, "@owner"), owner);

    // Create DSL builder and call block
    mrb_value builder = create_builder_object(mrb, handle);

    // Store builder reference on machine temporarily for nested access
    mrb_iv_set(mrb, machine_obj, mrb_intern_cstr(mrb, "@_builder"), builder);

    if (!mrb_nil_p(block)) {
        mrb_yield(mrb, block, builder);
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
    if (!data) return self;

    state_machine::StateMachineState* machine =
        state_machine::StateMachineManager::instance().get(data->machine_handle);
    if (!machine) return self;

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
        mrb_yield(mrb, block, self);
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
        }
    }

    state_def->transitions.push_back(trans);
    return self;
}

/// @method enter
/// @description Set a callback to run when entering this state.
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
    if (state_def) {
        state_def->enter_callback = block;
    }

    return self;
}

/// @method exit
/// @description Set a callback to run when exiting this state.
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
    if (state_def) {
        state_def->exit_callback = block;
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
    RClass* gmr = get_gmr_module(mrb);
    RClass* sm_class = mrb_class_get_under(mrb, gmr, "StateMachine");

    mrb_value args[1] = { self };
    return mrb_funcall_with_block(mrb, mrb_obj_value(sm_class),
        mrb_intern_lit(mrb, "attach"), 1, args, block);
}

// ============================================================================
// Registration
// ============================================================================

void register_state_machine(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // GMR::StateMachine class
    state_machine_class = mrb_define_class_under(mrb, gmr, "StateMachine", mrb->object_class);
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
