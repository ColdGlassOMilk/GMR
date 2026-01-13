#include "gmr/bindings/signal.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/array.h>
#include <mruby/hash.h>

namespace gmr {
namespace bindings {

// ============================================================================
// Signal Module
// ============================================================================

/// @module GMR::Signal
/// @description Mixin module for objects that can emit and receive signals.
///   Provides a decoupled communication pattern between game objects.
/// @example class Player
///     include Signal
///
///     def initialize
///       @health = 100
///     end
///
///     def take_damage(amount)
///       @health -= amount
///       emit(:health_changed, @health)
///       emit(:died) if @health <= 0
///     end
///   end
///
/// @example # Connect to signals
///   @player.on(:health_changed) { |hp| @hud.update_health(hp) }
///   @player.on(:died) { @music.play(:game_over) }
///   @player.once(:died) { show_death_screen }  # Only fires once

// Instance variable for signal handlers
static mrb_sym sym_signal_handlers;

// Helper to get or create the handlers hash
static mrb_value get_handlers(mrb_state* mrb, mrb_value self) {
    mrb_value handlers = mrb_iv_get(mrb, self, sym_signal_handlers);
    if (mrb_nil_p(handlers)) {
        handlers = mrb_hash_new(mrb);
        mrb_iv_set(mrb, self, sym_signal_handlers, handlers);
    }
    return handlers;
}

// Helper to get handlers array for a signal
static mrb_value get_signal_handlers(mrb_state* mrb, mrb_value self, mrb_value signal_name) {
    mrb_value handlers = get_handlers(mrb, self);
    mrb_value arr = mrb_hash_get(mrb, handlers, signal_name);
    if (mrb_nil_p(arr)) {
        arr = mrb_ary_new(mrb);
        mrb_hash_set(mrb, handlers, signal_name, arr);
    }
    return arr;
}

/// @method on
/// @description Connect a handler to a signal. Handler is called each time signal is emitted.
/// @param signal [Symbol] The signal name to listen for
/// @param block [Block] The handler to call when signal is emitted
/// @returns [Integer] A connection ID that can be used with off()
/// @example id = @player.on(:died) { game_over }
static mrb_value mrb_signal_on(mrb_state* mrb, mrb_value self) {
    mrb_sym signal;
    mrb_value block;
    mrb_get_args(mrb, "n&!", &signal, &block);

    mrb_value signal_name = mrb_symbol_value(signal);
    mrb_value arr = get_signal_handlers(mrb, self, signal_name);

    // Create handler entry: { callback: block, once: false, id: unique_id }
    static mrb_int next_id = 1;
    mrb_int id = next_id++;

    mrb_value entry = mrb_hash_new(mrb);
    mrb_hash_set(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "callback")), block);
    mrb_hash_set(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "once")), mrb_false_value());
    mrb_hash_set(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "id")), mrb_fixnum_value(id));

    mrb_ary_push(mrb, arr, entry);

    return mrb_fixnum_value(id);
}

/// @method once
/// @description Connect a handler that only fires once, then auto-disconnects.
/// @param signal [Symbol] The signal name to listen for
/// @param block [Block] The handler to call (once) when signal is emitted
/// @returns [Integer] A connection ID that can be used with off()
/// @example @player.once(:died) { show_death_cutscene }
static mrb_value mrb_signal_once(mrb_state* mrb, mrb_value self) {
    mrb_sym signal;
    mrb_value block;
    mrb_get_args(mrb, "n&!", &signal, &block);

    mrb_value signal_name = mrb_symbol_value(signal);
    mrb_value arr = get_signal_handlers(mrb, self, signal_name);

    static mrb_int next_id = 1;
    mrb_int id = next_id++;

    mrb_value entry = mrb_hash_new(mrb);
    mrb_hash_set(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "callback")), block);
    mrb_hash_set(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "once")), mrb_true_value());
    mrb_hash_set(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "id")), mrb_fixnum_value(id));

    mrb_ary_push(mrb, arr, entry);

    return mrb_fixnum_value(id);
}

/// @method off
/// @description Disconnect a handler by its connection ID or signal name.
/// @param signal [Symbol] The signal name
/// @param id [Integer] (optional) The connection ID. If omitted, all handlers for the signal are removed.
/// @returns [self]
/// @example @player.off(:died, connection_id)  # Remove specific handler
/// @example @player.off(:died)                  # Remove all handlers for :died
static mrb_value mrb_signal_off(mrb_state* mrb, mrb_value self) {
    mrb_sym signal;
    mrb_int id = -1;
    mrb_bool has_id = false;
    mrb_get_args(mrb, "n|i?", &signal, &id, &has_id);

    mrb_value signal_name = mrb_symbol_value(signal);
    mrb_value handlers = get_handlers(mrb, self);
    mrb_value arr = mrb_hash_get(mrb, handlers, signal_name);

    if (mrb_nil_p(arr)) {
        return self;
    }

    if (!has_id || id < 0) {
        // Remove all handlers for this signal
        mrb_hash_delete_key(mrb, handlers, signal_name);
    } else {
        // Remove specific handler by ID
        mrb_int len = RARRAY_LEN(arr);
        for (mrb_int i = len - 1; i >= 0; --i) {
            mrb_value entry = mrb_ary_ref(mrb, arr, i);
            mrb_value entry_id = mrb_hash_get(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "id")));
            if (mrb_fixnum_p(entry_id) && mrb_fixnum(entry_id) == id) {
                // Remove this entry by shifting
                for (mrb_int j = i; j < len - 1; ++j) {
                    mrb_ary_set(mrb, arr, j, mrb_ary_ref(mrb, arr, j + 1));
                }
                mrb_ary_pop(mrb, arr);
                break;
            }
        }
    }

    return self;
}

/// @method emit
/// @description Emit a signal, calling all connected handlers with optional arguments.
/// @param signal [Symbol] The signal name to emit
/// @param args [Object...] Optional arguments to pass to handlers
/// @returns [self]
/// @example emit(:died)
/// @example emit(:health_changed, @health)
/// @example emit(:damage_taken, amount, source)
static mrb_value mrb_signal_emit(mrb_state* mrb, mrb_value self) {
    mrb_sym signal;
    mrb_value* args;
    mrb_int argc;
    mrb_get_args(mrb, "n*", &signal, &args, &argc);

    mrb_value signal_name = mrb_symbol_value(signal);
    mrb_value handlers = get_handlers(mrb, self);
    mrb_value arr = mrb_hash_get(mrb, handlers, signal_name);

    if (mrb_nil_p(arr) || RARRAY_LEN(arr) == 0) {
        return self;
    }

    // Build args array for yield
    std::vector<mrb_value> args_vec(args, args + argc);

    // Track which entries to remove (once handlers)
    std::vector<mrb_int> to_remove;

    mrb_int len = RARRAY_LEN(arr);
    for (mrb_int i = 0; i < len; ++i) {
        mrb_value entry = mrb_ary_ref(mrb, arr, i);
        mrb_value callback = mrb_hash_get(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "callback")));
        mrb_value once = mrb_hash_get(mrb, entry, mrb_symbol_value(mrb_intern_cstr(mrb, "once")));

        // Call the handler
        scripting::safe_yield(mrb, callback, args_vec);

        // Mark for removal if once
        if (mrb_test(once)) {
            to_remove.push_back(i);
        }
    }

    // Remove once handlers (in reverse order to preserve indices)
    for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
        mrb_int idx = *it;
        mrb_int current_len = RARRAY_LEN(arr);
        for (mrb_int j = idx; j < current_len - 1; ++j) {
            mrb_ary_set(mrb, arr, j, mrb_ary_ref(mrb, arr, j + 1));
        }
        mrb_ary_pop(mrb, arr);
    }

    return self;
}

/// @method has_signal?
/// @description Check if any handlers are connected to a signal.
/// @param signal [Symbol] The signal name
/// @returns [Boolean] true if at least one handler is connected
/// @example if @player.has_signal?(:died) then puts "death handlers exist" end
static mrb_value mrb_signal_has(mrb_state* mrb, mrb_value self) {
    mrb_sym signal;
    mrb_get_args(mrb, "n", &signal);

    mrb_value signal_name = mrb_symbol_value(signal);
    mrb_value handlers = get_handlers(mrb, self);
    mrb_value arr = mrb_hash_get(mrb, handlers, signal_name);

    if (mrb_nil_p(arr) || RARRAY_LEN(arr) == 0) {
        return mrb_false_value();
    }
    return mrb_true_value();
}

/// @method clear_signals
/// @description Remove all signal handlers from this object.
/// @returns [self]
/// @example @player.clear_signals  # On cleanup
static mrb_value mrb_signal_clear(mrb_state* mrb, mrb_value self) {
    mrb_iv_set(mrb, self, sym_signal_handlers, mrb_nil_value());
    return self;
}

// ============================================================================
// Registration
// ============================================================================

void register_signal(mrb_state* mrb) {
    // Cache symbol
    sym_signal_handlers = mrb_intern_cstr(mrb, "@_signal_handlers");

    RClass* gmr = get_gmr_module(mrb);

    // Define Signal module
    RClass* signal = mrb_define_module_under(mrb, gmr, "Signal");

    mrb_define_method(mrb, signal, "on", mrb_signal_on, MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
    mrb_define_method(mrb, signal, "once", mrb_signal_once, MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());
    mrb_define_method(mrb, signal, "off", mrb_signal_off, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_method(mrb, signal, "emit", mrb_signal_emit, MRB_ARGS_REQ(1) | MRB_ARGS_REST());
    mrb_define_method(mrb, signal, "has_signal?", mrb_signal_has, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, signal, "clear_signals", mrb_signal_clear, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
