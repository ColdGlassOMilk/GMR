#include "gmr/bindings/timer.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/timer/timer_manager.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/string.h>

namespace gmr {
namespace bindings {

// ============================================================================
// Timer Class (Ruby object wrapping a timer handle)
// ============================================================================

/// @class GMR::Timer
/// @description A timer that executes a callback after a delay, optionally repeating.
///   Timers are automatically updated by the engine each frame.
/// @example # One-shot timer (fires once after 2 seconds)
///   Timer.after(2.0) { puts "2 seconds passed!" }
///
/// @example # Repeating timer (fires every 0.5 seconds)
///   @spawn_timer = Timer.every(0.5) { spawn_enemy }
///   # Later, to stop:
///   @spawn_timer.cancel
///
/// @example # Named timer for cancellation by name
///   Timer.after(5.0, name: :respawn) { respawn_player }
///   # Cancel by name from anywhere:
///   Timer.cancel(:respawn)
///
/// @example # Unscaled timer (ignores Time.scale, good for pause menus)
///   Timer.after(0.5, scaled: false) { show_pause_menu }

struct TimerData {
    timer::TimerHandle handle;
};

static void timer_data_free(mrb_state*, void* ptr) {
    delete static_cast<TimerData*>(ptr);
}

static const mrb_data_type timer_data_type = {
    "Timer", timer_data_free
};

static RClass* timer_class_ptr = nullptr;

static TimerData* get_timer_data(mrb_state* mrb, mrb_value self) {
    return static_cast<TimerData*>(mrb_data_get_ptr(mrb, self, &timer_data_type));
}

// Create a Ruby Timer object wrapping a handle
static mrb_value create_timer_object(mrb_state* mrb, timer::TimerHandle handle) {
    if (!timer_class_ptr) {
        RClass* gmr = get_gmr_module(mrb);
        timer_class_ptr = mrb_class_get_under(mrb, gmr, "Timer");
    }

    auto* data = new TimerData{handle};
    mrb_value obj = mrb_obj_value(mrb_data_object_alloc(mrb, timer_class_ptr, data, &timer_data_type));

    // Register with GC to keep alive
    mrb_gc_register(mrb, obj);

    // Store reference back to Ruby object in timer state
    timer::TimerState* state = timer::TimerManager::instance().get(handle);
    if (state) {
        state->ruby_timer_obj = obj;
    }

    return obj;
}

// ============================================================================
// Module Functions (Timer.after, Timer.every, Timer.cancel)
// ============================================================================

/// @function after
/// @description Create a one-shot timer that fires once after a delay.
/// @param delay [Float] Delay in seconds before the callback fires
/// @param name [Symbol] (optional) Name for cancellation via Timer.cancel(:name)
/// @param scaled [Boolean] (optional) Whether to respect Time.scale (default: true)
/// @param block [Block] The callback to execute when the timer fires
/// @returns [Timer] The timer object (can be used to cancel)
/// @example Timer.after(1.0) { @invincible = false }
/// @example Timer.after(3.0, name: :respawn) { respawn_player }
/// @example Timer.after(0.5, scaled: false) { show_menu }  # Ignores Time.scale
static mrb_value mrb_timer_after(mrb_state* mrb, mrb_value) {
    mrb_float delay;
    mrb_value block = mrb_nil_value();
    mrb_value opts = mrb_nil_value();

    mrb_get_args(mrb, "f|H&", &delay, &opts, &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Timer.after requires a block");
        return mrb_nil_value();
    }

    if (delay < 0.0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Timer delay cannot be negative");
        return mrb_nil_value();
    }

    // Parse options
    std::string name;
    bool scaled = true;

    if (!mrb_nil_p(opts)) {
        mrb_value name_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "name")));
        if (mrb_symbol_p(name_val)) {
            name = mrb_sym_name(mrb, mrb_symbol(name_val));
        }

        mrb_value scaled_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "scaled")));
        if (!mrb_nil_p(scaled_val)) {
            scaled = mrb_test(scaled_val);
        }
    }

    // Create timer
    timer::TimerHandle handle = timer::TimerManager::instance().create();
    timer::TimerState* state = timer::TimerManager::instance().get(handle);
    if (!state) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create timer");
        return mrb_nil_value();
    }

    state->delay = static_cast<float>(delay);
    state->interval = 0.0f;  // One-shot
    state->scaled = scaled;
    state->callback = block;
    state->name = name;

    return create_timer_object(mrb, handle);
}

/// @function every
/// @description Create a repeating timer that fires at regular intervals.
/// @param interval [Float] Time in seconds between each callback
/// @param name [Symbol] (optional) Name for cancellation via Timer.cancel(:name)
/// @param scaled [Boolean] (optional) Whether to respect Time.scale (default: true)
/// @param delay [Float] (optional) Initial delay before first fire (default: same as interval)
/// @param block [Block] The callback to execute on each interval
/// @returns [Timer] The timer object (can be used to cancel)
/// @example @spawn_timer = Timer.every(3.0) { spawn_wave }
/// @example Timer.every(0.1, name: :damage_tick) { apply_poison_damage }
/// @example Timer.every(1.0, delay: 0.0) { tick }  # Fire immediately, then every 1s
static mrb_value mrb_timer_every(mrb_state* mrb, mrb_value) {
    mrb_float interval;
    mrb_value block = mrb_nil_value();
    mrb_value opts = mrb_nil_value();

    mrb_get_args(mrb, "f|H&", &interval, &opts, &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Timer.every requires a block");
        return mrb_nil_value();
    }

    if (interval <= 0.0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Timer interval must be positive");
        return mrb_nil_value();
    }

    // Parse options
    std::string name;
    bool scaled = true;
    float delay = static_cast<float>(interval);  // Default: first fire after one interval

    if (!mrb_nil_p(opts)) {
        mrb_value name_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "name")));
        if (mrb_symbol_p(name_val)) {
            name = mrb_sym_name(mrb, mrb_symbol(name_val));
        }

        mrb_value scaled_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "scaled")));
        if (!mrb_nil_p(scaled_val)) {
            scaled = mrb_test(scaled_val);
        }

        mrb_value delay_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "delay")));
        if (!mrb_nil_p(delay_val)) {
            delay = static_cast<float>(mrb_as_float(mrb, delay_val));
            if (delay < 0.0f) {
                mrb_raise(mrb, E_ARGUMENT_ERROR, "Timer delay cannot be negative");
                return mrb_nil_value();
            }
        }
    }

    // Create timer
    timer::TimerHandle handle = timer::TimerManager::instance().create();
    timer::TimerState* state = timer::TimerManager::instance().get(handle);
    if (!state) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create timer");
        return mrb_nil_value();
    }

    state->delay = delay;
    state->interval = static_cast<float>(interval);
    state->scaled = scaled;
    state->callback = block;
    state->name = name;

    return create_timer_object(mrb, handle);
}

/// @function cancel
/// @description Cancel all timers with the given name.
/// @param name [Symbol] The name of the timer(s) to cancel
/// @returns [nil]
/// @example Timer.cancel(:respawn)
/// @example Timer.cancel(:damage_tick)
static mrb_value mrb_timer_cancel_by_name(mrb_state* mrb, mrb_value) {
    mrb_value name_arg;
    mrb_get_args(mrb, "o", &name_arg);

    if (!mrb_symbol_p(name_arg)) {
        mrb_raise(mrb, E_TYPE_ERROR, "Timer.cancel expects a Symbol");
        return mrb_nil_value();
    }

    std::string name = mrb_sym_name(mrb, mrb_symbol(name_arg));
    timer::TimerManager::instance().cancel_by_name(name);

    return mrb_nil_value();
}

// ============================================================================
// Instance Methods (timer.cancel, timer.pause, timer.resume, etc.)
// ============================================================================

/// @method cancel
/// @description Cancel this timer. It will not fire again.
/// @returns [self]
/// @example @my_timer.cancel
static mrb_value mrb_timer_instance_cancel(mrb_state* mrb, mrb_value self) {
    TimerData* data = get_timer_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Timer");
        return mrb_nil_value();
    }

    timer::TimerManager::instance().cancel(data->handle);
    return self;
}

/// @method pause
/// @description Pause this timer. Elapsed time is preserved.
/// @returns [self]
/// @example @my_timer.pause
static mrb_value mrb_timer_instance_pause(mrb_state* mrb, mrb_value self) {
    TimerData* data = get_timer_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Timer");
        return mrb_nil_value();
    }

    timer::TimerManager::instance().pause(data->handle);
    return self;
}

/// @method resume
/// @description Resume a paused timer.
/// @returns [self]
/// @example @my_timer.resume
static mrb_value mrb_timer_instance_resume(mrb_state* mrb, mrb_value self) {
    TimerData* data = get_timer_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Timer");
        return mrb_nil_value();
    }

    timer::TimerManager::instance().resume(data->handle);
    return self;
}

/// @method active?
/// @description Check if this timer is currently running (not paused or cancelled).
/// @returns [Boolean] true if the timer is active
/// @example if @my_timer.active?
static mrb_value mrb_timer_instance_active(mrb_state* mrb, mrb_value self) {
    TimerData* data = get_timer_data(mrb, self);
    if (!data) {
        return mrb_false_value();
    }

    timer::TimerState* state = timer::TimerManager::instance().get(data->handle);
    if (!state) {
        return mrb_false_value();
    }

    return to_mrb_bool(mrb, state->active && !state->cancelled);
}

/// @method cancelled?
/// @description Check if this timer has been cancelled.
/// @returns [Boolean] true if the timer was cancelled
/// @example if @my_timer.cancelled?
static mrb_value mrb_timer_instance_cancelled(mrb_state* mrb, mrb_value self) {
    TimerData* data = get_timer_data(mrb, self);
    if (!data) {
        return mrb_true_value();
    }

    timer::TimerState* state = timer::TimerManager::instance().get(data->handle);
    if (!state) {
        return mrb_true_value();
    }

    return to_mrb_bool(mrb, state->cancelled);
}

/// @method elapsed
/// @description Get the elapsed time since the timer started or last fired.
/// @returns [Float] Elapsed time in seconds
/// @example progress = @my_timer.elapsed / @my_timer.delay
static mrb_value mrb_timer_instance_elapsed(mrb_state* mrb, mrb_value self) {
    TimerData* data = get_timer_data(mrb, self);
    if (!data) {
        return mrb_float_value(mrb, 0.0);
    }

    timer::TimerState* state = timer::TimerManager::instance().get(data->handle);
    if (!state) {
        return mrb_float_value(mrb, 0.0);
    }

    return mrb_float_value(mrb, state->elapsed);
}

/// @method remaining
/// @description Get the time remaining until the timer fires.
/// @returns [Float] Remaining time in seconds (negative if overdue)
/// @example if @my_timer.remaining < 1.0 then show_warning end
static mrb_value mrb_timer_instance_remaining(mrb_state* mrb, mrb_value self) {
    TimerData* data = get_timer_data(mrb, self);
    if (!data) {
        return mrb_float_value(mrb, 0.0);
    }

    timer::TimerState* state = timer::TimerManager::instance().get(data->handle);
    if (!state) {
        return mrb_float_value(mrb, 0.0);
    }

    float threshold = state->fired ? state->interval : state->delay;
    return mrb_float_value(mrb, threshold - state->elapsed);
}

// ============================================================================
// Registration
// ============================================================================

void register_timer(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // Define Timer class under GMR module
    timer_class_ptr = mrb_define_class_under(mrb, gmr, "Timer", mrb->object_class);
    MRB_SET_INSTANCE_TT(timer_class_ptr, MRB_TT_CDATA);

    // Module functions (Timer.after, Timer.every, Timer.cancel)
    mrb_define_class_method(mrb, timer_class_ptr, "after", mrb_timer_after, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_class_method(mrb, timer_class_ptr, "every", mrb_timer_every, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_class_method(mrb, timer_class_ptr, "cancel", mrb_timer_cancel_by_name, MRB_ARGS_REQ(1));

    // Instance methods
    mrb_define_method(mrb, timer_class_ptr, "cancel", mrb_timer_instance_cancel, MRB_ARGS_NONE());
    mrb_define_method(mrb, timer_class_ptr, "pause", mrb_timer_instance_pause, MRB_ARGS_NONE());
    mrb_define_method(mrb, timer_class_ptr, "resume", mrb_timer_instance_resume, MRB_ARGS_NONE());
    mrb_define_method(mrb, timer_class_ptr, "active?", mrb_timer_instance_active, MRB_ARGS_NONE());
    mrb_define_method(mrb, timer_class_ptr, "cancelled?", mrb_timer_instance_cancelled, MRB_ARGS_NONE());
    mrb_define_method(mrb, timer_class_ptr, "elapsed", mrb_timer_instance_elapsed, MRB_ARGS_NONE());
    mrb_define_method(mrb, timer_class_ptr, "remaining", mrb_timer_instance_remaining, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
