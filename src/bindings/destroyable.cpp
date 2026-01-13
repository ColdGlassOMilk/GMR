#include "gmr/bindings/destroyable.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/array.h>

namespace gmr {
namespace bindings {

// ============================================================================
// Destroyable Module
// ============================================================================

/// @module GMR::Destroyable
/// @description Mixin module for game objects that can be destroyed.
///   Provides a standard lifecycle pattern with destruction marking and cleanup hooks.
/// @example class Enemy
///     include Destroyable
///
///     def initialize
///       @health = 100
///     end
///
///     def on_destroy
///       # Called when destroy is invoked
///       play_death_sound
///       spawn_particles
///     end
///
///     def take_damage(amount)
///       @health -= amount
///       destroy if @health <= 0
///     end
///   end
///
/// @example # Using with GameArray for automatic cleanup
///   @enemies = GameArray.new
///   @enemies << Enemy.new
///   # ... later, destroyed enemies are automatically removed

// Instance variable names
static mrb_sym sym_destroyed;

/// @method destroy
/// @description Mark this object for destruction. Calls on_destroy hook if defined.
///   Safe to call multiple times (only destroys once).
/// @returns [self]
/// @example enemy.destroy
static mrb_value mrb_destroyable_destroy(mrb_state* mrb, mrb_value self) {
    // Check if already destroyed
    mrb_value destroyed = mrb_iv_get(mrb, self, sym_destroyed);
    if (mrb_test(destroyed)) {
        return self;  // Already destroyed, no-op
    }

    // Mark as destroyed
    mrb_iv_set(mrb, self, sym_destroyed, mrb_true_value());

    // Call on_destroy hook if it exists
    if (mrb_respond_to(mrb, self, mrb_intern_cstr(mrb, "on_destroy"))) {
        scripting::safe_method_call(mrb, self, "on_destroy");
    }

    return self;
}

/// @method destroyed?
/// @description Check if this object has been destroyed.
/// @returns [Boolean] true if destroy has been called
/// @example if enemy.destroyed? then skip end
static mrb_value mrb_destroyable_destroyed(mrb_state* mrb, mrb_value self) {
    mrb_value destroyed = mrb_iv_get(mrb, self, sym_destroyed);
    return mrb_test(destroyed) ? mrb_true_value() : mrb_false_value();
}

/// @method alive?
/// @description Check if this object is still alive (not destroyed).
/// @returns [Boolean] true if destroy has NOT been called
/// @example if enemy.alive? then enemy.update(dt) end
static mrb_value mrb_destroyable_alive(mrb_state* mrb, mrb_value self) {
    mrb_value destroyed = mrb_iv_get(mrb, self, sym_destroyed);
    return mrb_test(destroyed) ? mrb_false_value() : mrb_true_value();
}

// ============================================================================
// GameArray Class
// ============================================================================

/// @class GMR::GameArray
/// @description An Array subclass that automatically removes destroyed objects.
///   Destroyed objects are removed during iteration (each, select, etc.)
///   or when calling compact_destroyed!.
/// @example @enemies = GameArray.new
///   @enemies << Enemy.new
///   @enemies << Enemy.new
///
///   # During iteration, destroyed enemies are skipped and removed
///   @enemies.each_alive do |enemy|
///     enemy.update(dt)
///   end

/// @method each_alive
/// @description Iterate over all alive (non-destroyed) objects.
///   Automatically removes destroyed objects from the array.
/// @param block [Block] Block to call with each alive object
/// @returns [self]
/// @example @enemies.each_alive { |e| e.update(dt) }
static mrb_value mrb_game_array_each_alive(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&!", &block);

    mrb_int len = RARRAY_LEN(self);
    mrb_int write_idx = 0;

    for (mrb_int read_idx = 0; read_idx < len; ++read_idx) {
        mrb_value obj = mrb_ary_ref(mrb, self, read_idx);

        // Check if object responds to destroyed? and is destroyed
        bool is_destroyed = false;
        if (mrb_respond_to(mrb, obj, mrb_intern_cstr(mrb, "destroyed?"))) {
            mrb_value destroyed = scripting::safe_method_call(mrb, obj, "destroyed?");
            is_destroyed = mrb_test(destroyed);
        }

        if (!is_destroyed) {
            // Keep this object
            if (write_idx != read_idx) {
                mrb_ary_set(mrb, self, write_idx, obj);
            }
            ++write_idx;

            // Yield to block
            scripting::safe_yield(mrb, block, obj);
        }
    }

    // Truncate array to remove dead objects
    if (write_idx < len) {
        // Pop elements from the end
        for (mrb_int i = len - 1; i >= write_idx; --i) {
            mrb_ary_pop(mrb, self);
        }
    }

    return self;
}

/// @method compact_destroyed!
/// @description Remove all destroyed objects from the array.
///   Called automatically by each_alive, but can be called manually.
/// @returns [self]
/// @example @enemies.compact_destroyed!
static mrb_value mrb_game_array_compact_destroyed(mrb_state* mrb, mrb_value self) {
    mrb_int len = RARRAY_LEN(self);
    mrb_int write_idx = 0;

    for (mrb_int read_idx = 0; read_idx < len; ++read_idx) {
        mrb_value obj = mrb_ary_ref(mrb, self, read_idx);

        bool is_destroyed = false;
        if (mrb_respond_to(mrb, obj, mrb_intern_cstr(mrb, "destroyed?"))) {
            mrb_value destroyed = scripting::safe_method_call(mrb, obj, "destroyed?");
            is_destroyed = mrb_test(destroyed);
        }

        if (!is_destroyed) {
            if (write_idx != read_idx) {
                mrb_ary_set(mrb, self, write_idx, obj);
            }
            ++write_idx;
        }
    }

    // Truncate array
    if (write_idx < len) {
        for (mrb_int i = len - 1; i >= write_idx; --i) {
            mrb_ary_pop(mrb, self);
        }
    }

    return self;
}

/// @method alive_count
/// @description Count the number of alive (non-destroyed) objects.
/// @returns [Integer] Number of alive objects
/// @example puts "Enemies remaining: #{@enemies.alive_count}"
static mrb_value mrb_game_array_alive_count(mrb_state* mrb, mrb_value self) {
    mrb_int len = RARRAY_LEN(self);
    mrb_int count = 0;

    for (mrb_int i = 0; i < len; ++i) {
        mrb_value obj = mrb_ary_ref(mrb, self, i);

        bool is_destroyed = false;
        if (mrb_respond_to(mrb, obj, mrb_intern_cstr(mrb, "destroyed?"))) {
            mrb_value destroyed = scripting::safe_method_call(mrb, obj, "destroyed?");
            is_destroyed = mrb_test(destroyed);
        }

        if (!is_destroyed) {
            ++count;
        }
    }

    return mrb_fixnum_value(count);
}

/// @method all_destroyed?
/// @description Check if all objects in the array are destroyed.
/// @returns [Boolean] true if array is empty or all objects are destroyed
/// @example if @wave_enemies.all_destroyed? then next_wave end
static mrb_value mrb_game_array_all_destroyed(mrb_state* mrb, mrb_value self) {
    mrb_int len = RARRAY_LEN(self);

    for (mrb_int i = 0; i < len; ++i) {
        mrb_value obj = mrb_ary_ref(mrb, self, i);

        if (mrb_respond_to(mrb, obj, mrb_intern_cstr(mrb, "destroyed?"))) {
            mrb_value destroyed = scripting::safe_method_call(mrb, obj, "destroyed?");
            if (!mrb_test(destroyed)) {
                return mrb_false_value();  // Found an alive object
            }
        } else {
            return mrb_false_value();  // Object doesn't support destroyed? - assume alive
        }
    }

    return mrb_true_value();  // All destroyed (or empty)
}

// ============================================================================
// Registration
// ============================================================================

void register_destroyable(mrb_state* mrb) {
    // Cache symbols
    sym_destroyed = mrb_intern_cstr(mrb, "@_destroyed");

    RClass* gmr = get_gmr_module(mrb);

    // Define Destroyable module
    RClass* destroyable = mrb_define_module_under(mrb, gmr, "Destroyable");

    mrb_define_method(mrb, destroyable, "destroy", mrb_destroyable_destroy, MRB_ARGS_NONE());
    mrb_define_method(mrb, destroyable, "destroyed?", mrb_destroyable_destroyed, MRB_ARGS_NONE());
    mrb_define_method(mrb, destroyable, "alive?", mrb_destroyable_alive, MRB_ARGS_NONE());

    // Define GameArray class (subclass of Array)
    RClass* game_array = mrb_define_class_under(mrb, gmr, "GameArray", mrb->array_class);

    mrb_define_method(mrb, game_array, "each_alive", mrb_game_array_each_alive, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, game_array, "compact_destroyed!", mrb_game_array_compact_destroyed, MRB_ARGS_NONE());
    mrb_define_method(mrb, game_array, "alive_count", mrb_game_array_alive_count, MRB_ARGS_NONE());
    mrb_define_method(mrb, game_array, "all_destroyed?", mrb_game_array_all_destroyed, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
