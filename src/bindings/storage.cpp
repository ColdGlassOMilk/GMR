#include "gmr/bindings/storage.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "storage_impl.hpp"

#include <mruby/string.h>
#include <mruby/hash.h>

namespace gmr {
namespace bindings {

// ============================================================================
// GMR::Storage Module Functions
// ============================================================================

/// @module GMR::Storage
/// @description Simple key-value storage for game settings, high scores, and unlockables.
///   Uses raylib's LoadStorageValue/SaveStorageValue for persistent integer storage.
///   Perfect for settings, preferences, achievements, and simple game state.
///
/// Storage is automatically persisted and survives game restarts on all platforms.

/// @function get
/// @param key [String, Symbol] Storage key (e.g., "high_score", :volume)
/// @param default [Integer] Default value if key doesn't exist (default: 0)
/// @return [Integer] Stored value or default
/// @example # Get high score (default 0)
///   high_score = GMR::Storage.get("high_score")
/// @example # Get volume with default
///   volume = GMR::Storage.get(:volume, 80)
static mrb_value mrb_storage_get(mrb_state* mrb, mrb_value self) {
    mrb_value key_val;
    mrb_int default_val = 0;

    mrb_get_args(mrb, "o|i", &key_val, &default_val);

    // Convert key to C string (support both string and symbol)
    const char* key_cstr = nullptr;
    if (mrb_string_p(key_val)) {
        key_cstr = RSTRING_PTR(key_val);
    } else if (mrb_symbol_p(key_val)) {
        mrb_sym sym = mrb_symbol(key_val);
        key_cstr = mrb_sym_name(mrb, sym);
    } else {
        mrb_raise(mrb, E_TYPE_ERROR,
            "[Storage.get] key must be a String or Symbol");
    }

    // Load value from storage (returns 0 if key doesn't exist)
    int stored_value = LoadStorageValue(0);  // Position 0 (will use key-based lookup via SetStorageValue)

    // For proper key-based storage, we need to use a hash of the key as the position
    // Raylib's storage is position-based, so we'll use a simple hash
    unsigned int position = 0;
    for (const char* p = key_cstr; *p; p++) {
        position = position * 31 + *p;
    }
    position %= 1000;  // Limit to 1000 positions

    stored_value = LoadStorageValue(position);

    // If value is 0, it might be the default or an actual stored 0
    // We'll use the default parameter to distinguish
    return mrb_fixnum_value(stored_value != 0 ? stored_value : default_val);
}

/// @function set
/// @param key [String, Symbol] Storage key (e.g., "high_score", :volume)
/// @param value [Integer] Value to store
/// @return [Integer] The stored value
/// @example # Save high score
///   GMR::Storage.set("high_score", 1000)
/// @example # Save volume setting
///   GMR::Storage.set(:volume, 80)
static mrb_value mrb_storage_set(mrb_state* mrb, mrb_value self) {
    mrb_value key_val;
    mrb_int value;

    mrb_get_args(mrb, "oi", &key_val, &value);

    // Convert key to C string
    const char* key_cstr = nullptr;
    if (mrb_string_p(key_val)) {
        key_cstr = RSTRING_PTR(key_val);
    } else if (mrb_symbol_p(key_val)) {
        mrb_sym sym = mrb_symbol(key_val);
        key_cstr = mrb_sym_name(mrb, sym);
    } else {
        mrb_raise(mrb, E_TYPE_ERROR,
            "[Storage.set] key must be a String or Symbol");
    }

    // Hash key to position
    unsigned int position = 0;
    for (const char* p = key_cstr; *p; p++) {
        position = position * 31 + *p;
    }
    position %= 1000;

    // Save value
    bool success = SaveStorageValue(position, static_cast<int>(value));
    if (!success) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[Storage.set] Failed to save value for key: %s",
            key_cstr);
    }

    return mrb_fixnum_value(value);
}

/// @function has_key?
/// @param key [String, Symbol] Storage key to check
/// @return [Boolean] true if key exists with non-zero value
/// @example # Check if high score exists
///   if GMR::Storage.has_key?("high_score")
///     score = GMR::Storage.get("high_score")
///   end
static mrb_value mrb_storage_has_key(mrb_state* mrb, mrb_value self) {
    mrb_value key_val;

    mrb_get_args(mrb, "o", &key_val);

    // Convert key to C string
    const char* key_cstr = nullptr;
    if (mrb_string_p(key_val)) {
        key_cstr = RSTRING_PTR(key_val);
    } else if (mrb_symbol_p(key_val)) {
        mrb_sym sym = mrb_symbol(key_val);
        key_cstr = mrb_sym_name(mrb, sym);
    } else {
        mrb_raise(mrb, E_TYPE_ERROR,
            "[Storage.has_key?] key must be a String or Symbol");
    }

    // Hash key to position
    unsigned int position = 0;
    for (const char* p = key_cstr; *p; p++) {
        position = position * 31 + *p;
    }
    position %= 1000;

    // Load and check for non-zero value
    int value = LoadStorageValue(position);
    return mrb_bool_value(value != 0);
}

/// @function delete
/// @param key [String, Symbol] Storage key to delete
/// @return [nil]
/// @example # Delete high score
///   GMR::Storage.delete("high_score")
static mrb_value mrb_storage_delete(mrb_state* mrb, mrb_value self) {
    mrb_value key_val;

    mrb_get_args(mrb, "o", &key_val);

    // Convert key to C string
    const char* key_cstr = nullptr;
    if (mrb_string_p(key_val)) {
        key_cstr = RSTRING_PTR(key_val);
    } else if (mrb_symbol_p(key_val)) {
        mrb_sym sym = mrb_symbol(key_val);
        key_cstr = mrb_sym_name(mrb, sym);
    } else {
        mrb_raise(mrb, E_TYPE_ERROR,
            "[Storage.delete] key must be a String or Symbol");
    }

    // Hash key to position
    unsigned int position = 0;
    for (const char* p = key_cstr; *p; p++) {
        position = position * 31 + *p;
    }
    position %= 1000;

    // Save 0 to delete
    SaveStorageValue(position, 0);

    return mrb_nil_value();
}

/// @function increment
/// @param key [String, Symbol] Storage key to increment
/// @param amount [Integer] Amount to add (default: 1)
/// @return [Integer] New value after increment
/// @example # Increment play count
///   GMR::Storage.increment(:play_count)
/// @example # Add to score
///   GMR::Storage.increment("total_score", 100)
static mrb_value mrb_storage_increment(mrb_state* mrb, mrb_value self) {
    mrb_value key_val;
    mrb_int amount = 1;

    mrb_get_args(mrb, "o|i", &key_val, &amount);

    // Convert key to C string
    const char* key_cstr = nullptr;
    if (mrb_string_p(key_val)) {
        key_cstr = RSTRING_PTR(key_val);
    } else if (mrb_symbol_p(key_val)) {
        mrb_sym sym = mrb_symbol(key_val);
        key_cstr = mrb_sym_name(mrb, sym);
    } else {
        mrb_raise(mrb, E_TYPE_ERROR,
            "[Storage.increment] key must be a String or Symbol");
    }

    // Hash key to position
    unsigned int position = 0;
    for (const char* p = key_cstr; *p; p++) {
        position = position * 31 + *p;
    }
    position %= 1000;

    // Load, increment, save
    int current = LoadStorageValue(position);
    int new_value = current + static_cast<int>(amount);
    SaveStorageValue(position, new_value);

    return mrb_fixnum_value(new_value);
}

/// @function decrement
/// @param key [String, Symbol] Storage key to decrement
/// @param amount [Integer] Amount to subtract (default: 1)
/// @return [Integer] New value after decrement
/// @example # Decrement lives
///   GMR::Storage.decrement(:lives_remaining)
/// @example # Subtract from currency
///   GMR::Storage.decrement("gold", 50)
static mrb_value mrb_storage_decrement(mrb_state* mrb, mrb_value self) {
    mrb_value key_val;
    mrb_int amount = 1;

    mrb_get_args(mrb, "o|i", &key_val, &amount);

    // Convert key to C string
    const char* key_cstr = nullptr;
    if (mrb_string_p(key_val)) {
        key_cstr = RSTRING_PTR(key_val);
    } else if (mrb_symbol_p(key_val)) {
        mrb_sym sym = mrb_symbol(key_val);
        key_cstr = mrb_sym_name(mrb, sym);
    } else {
        mrb_raise(mrb, E_TYPE_ERROR,
            "[Storage.decrement] key must be a String or Symbol");
    }

    // Hash key to position
    unsigned int position = 0;
    for (const char* p = key_cstr; *p; p++) {
        position = position * 31 + *p;
    }
    position %= 1000;

    // Load, decrement, save
    int current = LoadStorageValue(position);
    int new_value = current - static_cast<int>(amount);
    SaveStorageValue(position, new_value);

    return mrb_fixnum_value(new_value);
}

// ============================================================================
// Module Registration
// ============================================================================

void register_storage(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* storage = mrb_define_module_under(mrb, gmr, "Storage");

    // Core operations
    mrb_define_module_function(mrb, storage, "get", mrb_storage_get, MRB_ARGS_ARG(1, 1));
    mrb_define_module_function(mrb, storage, "set", mrb_storage_set, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, storage, "has_key?", mrb_storage_has_key, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, storage, "delete", mrb_storage_delete, MRB_ARGS_REQ(1));

    // Convenience operations
    mrb_define_module_function(mrb, storage, "increment", mrb_storage_increment, MRB_ARGS_ARG(1, 1));
    mrb_define_module_function(mrb, storage, "decrement", mrb_storage_decrement, MRB_ARGS_ARG(1, 1));

    // Aliases
    mrb_define_module_function(mrb, storage, "[]", mrb_storage_get, MRB_ARGS_ARG(1, 1));
    mrb_define_module_function(mrb, storage, "[]=", mrb_storage_set, MRB_ARGS_REQ(2));
}

}  // namespace bindings
}  // namespace gmr
