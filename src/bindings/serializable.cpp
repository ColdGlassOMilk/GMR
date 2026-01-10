#include "gmr/bindings/serializable.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/scripting/helpers.hpp"

#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/proc.h>

namespace gmr {
namespace bindings {

// ============================================================================
// GMR::Serializable Module
// ============================================================================

/// @module GMR::Serializable
/// @description Declarative serialization DSL for opt-in field persistence.
///   Include this module in a class and use the `serializable` block to declare
///   which fields should be saved/loaded.
/// @example # Basic usage
///   class Player
///     include GMR::Serializable
///
///     serializable do
///       field :id
///       field :name
///       field :health, default: 100
///       field :position   # Works with engine objects that have to_h
///     end
///
///     attr_accessor :id, :name, :health, :position
///   end
///
///   # Serialize to hash
///   player = Player.new
///   player.id = 1
///   player.name = "Hero"
///   player.health = 85
///   player.position = Vec2.new(100, 200)
///
///   data = player.serialize
///   # => { "id" => 1, "name" => "Hero", "health" => 85, "position" => { "_type" => "Vec2", "x" => 100.0, "y" => 200.0 } }
///
///   json = player.to_json
///   # => '{"id":1,"name":"Hero","health":85,"position":{"_type":"Vec2","x":100.0,"y":200.0}}'
///
///   # Deserialize
///   loaded = Player.deserialize(data)
///   loaded = Player.from_json(json)
/// @example # Nested serializable objects
///   class Inventory
///     include GMR::Serializable
///
///     serializable do
///       field :items, type: :array
///       field :gold, default: 0
///     end
///
///     attr_accessor :items, :gold
///   end
///
///   class GameState
///     include GMR::Serializable
///
///     serializable do
///       field :player, type: Player
///       field :inventory, type: Inventory
///       field :level_name
///     end
///
///     attr_accessor :player, :inventory, :level_name
///   end

// ============================================================================
// Field Builder Data
// ============================================================================

struct FieldBuilderData {
    RClass* target_class;
};

static void field_builder_free(mrb_state* mrb, void* ptr) {
    FieldBuilderData* data = static_cast<FieldBuilderData*>(ptr);
    if (data) {
        mrb_free(mrb, data);
    }
}

static const mrb_data_type field_builder_data_type = {
    "Serializable::FieldBuilder", field_builder_free
};

static FieldBuilderData* get_field_builder_data(mrb_state* mrb, mrb_value self) {
    return static_cast<FieldBuilderData*>(mrb_data_get_ptr(mrb, self, &field_builder_data_type));
}

// Cache class pointers
static RClass* field_builder_class = nullptr;

// ============================================================================
// Helper: Create Field Builder Object
// ============================================================================

static mrb_value create_field_builder(mrb_state* mrb, RClass* target_class) {
    if (!field_builder_class) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "FieldBuilder class not initialized");
        return mrb_nil_value();
    }

    mrb_value obj = mrb_obj_new(mrb, field_builder_class, 0, nullptr);
    FieldBuilderData* data = static_cast<FieldBuilderData*>(
        mrb_malloc(mrb, sizeof(FieldBuilderData)));
    data->target_class = target_class;
    mrb_data_init(obj, data, &field_builder_data_type);

    return obj;
}

// ============================================================================
// Helper: Serialize a single value
// ============================================================================

static mrb_value serialize_value(mrb_state* mrb, mrb_value value) {
    // Nil, true, false pass through
    if (mrb_nil_p(value) || mrb_true_p(value) || mrb_false_p(value)) {
        return value;
    }

    // Integers and floats pass through
    if (mrb_integer_p(value) || mrb_float_p(value)) {
        return value;
    }

    // Strings pass through
    if (mrb_string_p(value)) {
        return value;
    }

    // Symbols -> convert to string
    if (mrb_symbol_p(value)) {
        return mrb_sym_str(mrb, mrb_symbol(value));
    }

    // Arrays -> map each element recursively
    if (mrb_array_p(value)) {
        mrb_int len = RARRAY_LEN(value);
        mrb_value result = mrb_ary_new_capa(mrb, len);
        for (mrb_int i = 0; i < len; i++) {
            mrb_value elem = mrb_ary_ref(mrb, value, i);
            mrb_ary_push(mrb, result, serialize_value(mrb, elem));
        }
        return result;
    }

    // Hashes -> map values recursively
    if (mrb_hash_p(value)) {
        mrb_value result = mrb_hash_new(mrb);
        mrb_value keys = mrb_hash_keys(mrb, value);
        mrb_int len = RARRAY_LEN(keys);
        for (mrb_int i = 0; i < len; i++) {
            mrb_value key = mrb_ary_ref(mrb, keys, i);
            mrb_value val = mrb_hash_get(mrb, value, key);
            // Convert symbol keys to strings for JSON compatibility
            mrb_value key_str = mrb_symbol_p(key) ? mrb_sym_str(mrb, mrb_symbol(key)) : key;
            mrb_hash_set(mrb, result, key_str, serialize_value(mrb, val));
        }
        return result;
    }

    // Objects with to_h (engine objects like Vec2, Transform2D)
    if (mrb_respond_to(mrb, value, mrb_intern_lit(mrb, "to_h"))) {
        return mrb_funcall(mrb, value, "to_h", 0);
    }

    // Objects with serialize (nested serializable objects)
    if (mrb_respond_to(mrb, value, mrb_intern_lit(mrb, "serialize"))) {
        return mrb_funcall(mrb, value, "serialize", 0);
    }

    // Fallback: convert to string
    return mrb_funcall(mrb, value, "to_s", 0);
}

// ============================================================================
// Helper: Deserialize a value based on type
// ============================================================================

static mrb_value deserialize_value(mrb_state* mrb, mrb_value value, mrb_value type_val) {
    // Nil stays nil
    if (mrb_nil_p(value)) {
        return value;
    }

    // If type is specified and is a Class with deserialize, use it
    if (!mrb_nil_p(type_val) && mrb_class_p(type_val)) {
        RClass* type_class = mrb_class_ptr(type_val);
        if (mrb_respond_to(mrb, type_val, mrb_intern_lit(mrb, "deserialize"))) {
            return mrb_funcall(mrb, type_val, "deserialize", 1, value);
        }
    }

    // If type is :array, process each element
    if (mrb_symbol_p(type_val)) {
        mrb_sym type_sym = mrb_symbol(type_val);
        if (type_sym == mrb_intern_lit(mrb, "array") && mrb_array_p(value)) {
            mrb_int len = RARRAY_LEN(value);
            mrb_value result = mrb_ary_new_capa(mrb, len);
            for (mrb_int i = 0; i < len; i++) {
                mrb_value elem = mrb_ary_ref(mrb, value, i);
                // Auto-detect type from _type field
                mrb_ary_push(mrb, result, deserialize_value(mrb, elem, mrb_nil_value()));
            }
            return result;
        }
    }

    // Auto-detect engine types from _type field in hashes
    if (mrb_hash_p(value)) {
        mrb_value type_key = mrb_str_new_lit(mrb, "_type");
        mrb_value type_str = mrb_hash_get(mrb, value, type_key);

        if (mrb_string_p(type_str)) {
            const char* type_name = RSTRING_PTR(type_str);

            // Vec2
            if (strcmp(type_name, "Vec2") == 0) {
                RClass* gmr = get_gmr_module(mrb);
                RClass* mathf = mrb_module_get_under(mrb, gmr, "Mathf");
                RClass* vec2_class = mrb_class_get_under(mrb, mathf, "Vec2");

                mrb_value x = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "x"));
                mrb_value y = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "y"));
                mrb_value args[2] = { x, y };
                return mrb_obj_new(mrb, vec2_class, 2, args);
            }

            // Vec3
            if (strcmp(type_name, "Vec3") == 0) {
                RClass* gmr = get_gmr_module(mrb);
                RClass* mathf = mrb_module_get_under(mrb, gmr, "Mathf");
                RClass* vec3_class = mrb_class_get_under(mrb, mathf, "Vec3");

                mrb_value x = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "x"));
                mrb_value y = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "y"));
                mrb_value z = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "z"));
                mrb_value args[3] = { x, y, z };
                return mrb_obj_new(mrb, vec3_class, 3, args);
            }

            // Rect
            if (strcmp(type_name, "Rect") == 0) {
                RClass* gmr = get_gmr_module(mrb);
                RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
                RClass* rect_class = mrb_class_get_under(mrb, graphics, "Rect");

                mrb_value x = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "x"));
                mrb_value y = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "y"));
                mrb_value w = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "w"));
                mrb_value h = mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "h"));
                mrb_value args[4] = { x, y, w, h };
                return mrb_obj_new(mrb, rect_class, 4, args);
            }

            // Transform2D
            if (strcmp(type_name, "Transform2D") == 0) {
                RClass* gmr = get_gmr_module(mrb);
                RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
                RClass* transform_class = mrb_class_get_under(mrb, graphics, "Transform2D");

                // Build kwargs hash from data
                mrb_value kwargs = mrb_hash_new(mrb);
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "x")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "x")));
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "y")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "y")));
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "rotation")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "rotation")));
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "scale_x")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "scale_x")));
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "scale_y")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "scale_y")));
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "origin_x")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "origin_x")));
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "origin_y")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "origin_y")));
                mrb_hash_set(mrb, kwargs, mrb_symbol_value(mrb_intern_lit(mrb, "parallax")),
                    mrb_hash_get(mrb, value, mrb_str_new_lit(mrb, "parallax")));

                mrb_value args[1] = { kwargs };
                return mrb_obj_new(mrb, transform_class, 1, args);
            }
        }
    }

    // Return as-is for primitives
    return value;
}

// ============================================================================
// FieldBuilder Methods
// ============================================================================

/// @method field
/// @description Declare a serializable field.
/// @param name [Symbol] The field name (matches instance variable @name)
/// @param default [Object] Default value when deserializing if field is nil
/// @param type [Class, Symbol] Optional type hint for deserialization
///   - Use a Class that includes Serializable for nested objects
///   - Use :array for arrays of auto-detected types
/// @example serializable do
///   field :id
///   field :name
///   field :health, default: 100
///   field :position      # Engine objects with to_h work automatically
///   field :items, type: :array
///   field :stats, type: PlayerStats  # Nested serializable class
/// end
static mrb_value mrb_field_builder_field(mrb_state* mrb, mrb_value self) {
    mrb_sym name;
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "n|H", &name, &kwargs);

    FieldBuilderData* builder = get_field_builder_data(mrb, self);
    if (!builder || !builder->target_class) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "[Serializable] Invalid field builder");
        return mrb_nil_value();
    }

    // Get or create the @_serializable_fields hash on the class
    mrb_sym fields_ivar = mrb_intern_lit(mrb, "@_serializable_fields");
    mrb_value fields = mrb_iv_get(mrb, mrb_obj_value(builder->target_class), fields_ivar);

    if (mrb_nil_p(fields)) {
        fields = mrb_hash_new(mrb);
        mrb_iv_set(mrb, mrb_obj_value(builder->target_class), fields_ivar, fields);
    }

    // Extract options from kwargs
    mrb_value default_val = mrb_nil_value();
    mrb_value type_val = mrb_nil_value();

    if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        mrb_value default_key = mrb_symbol_value(mrb_intern_lit(mrb, "default"));
        mrb_value type_key = mrb_symbol_value(mrb_intern_lit(mrb, "type"));

        default_val = mrb_hash_get(mrb, kwargs, default_key);
        type_val = mrb_hash_get(mrb, kwargs, type_key);
    }

    // Store field metadata: { name => { default: ..., type: ... } }
    mrb_value field_opts = mrb_hash_new(mrb);
    if (!mrb_nil_p(default_val)) {
        mrb_hash_set(mrb, field_opts, mrb_symbol_value(mrb_intern_lit(mrb, "default")), default_val);
    }
    if (!mrb_nil_p(type_val)) {
        mrb_hash_set(mrb, field_opts, mrb_symbol_value(mrb_intern_lit(mrb, "type")), type_val);
    }

    mrb_hash_set(mrb, fields, mrb_symbol_value(name), field_opts);

    return mrb_nil_value();
}

// ============================================================================
// Serializable Module Methods
// ============================================================================

/// @method serializable
/// @description Define which fields should be serialized. Called on the class.
/// @param block [Block] A block containing field declarations
/// @example class Player
///   include GMR::Serializable
///   serializable do
///     field :name
///     field :health, default: 100
///   end
/// end
static mrb_value mrb_serializable_block(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "[Serializable] serializable requires a block");
        return mrb_nil_value();
    }

    // self is the class that called serializable
    if (!mrb_class_p(self) && !mrb_module_p(self)) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "[Serializable] serializable must be called on a class");
        return mrb_nil_value();
    }

    RClass* target_class = mrb_class_ptr(self);

    // Create field builder
    mrb_value builder = create_field_builder(mrb, target_class);

    // Execute block with builder as self
    scripting::safe_instance_exec(mrb, builder, block);

    return mrb_nil_value();
}

/// @method serialize
/// @description Serialize this object to a hash. Only declared fields are included.
/// @return [Hash] A hash containing all serialized field values
/// @example player.serialize  # => { "name" => "Hero", "health" => 100 }
static mrb_value mrb_serializable_serialize(mrb_state* mrb, mrb_value self) {
    // Get the class of this instance
    RClass* klass = mrb_obj_class(mrb, self);

    // Walk up the class hierarchy to find @_serializable_fields
    mrb_value fields = mrb_nil_value();
    RClass* check_class = klass;
    while (check_class && mrb_nil_p(fields)) {
        fields = mrb_iv_get(mrb, mrb_obj_value(check_class), mrb_intern_lit(mrb, "@_serializable_fields"));
        check_class = check_class->super;
    }

    if (mrb_nil_p(fields) || !mrb_hash_p(fields)) {
        // No fields declared - return empty hash
        return mrb_hash_new(mrb);
    }

    mrb_value result = mrb_hash_new(mrb);
    mrb_value keys = mrb_hash_keys(mrb, fields);
    mrb_int len = RARRAY_LEN(keys);

    for (mrb_int i = 0; i < len; i++) {
        mrb_value key = mrb_ary_ref(mrb, keys, i);  // Symbol key (field name)
        mrb_sym field_sym = mrb_symbol(key);

        // Get instance variable @field_name
        char ivar_name[64];
        snprintf(ivar_name, sizeof(ivar_name), "@%s", mrb_sym_name(mrb, field_sym));
        mrb_sym ivar_sym = mrb_intern_cstr(mrb, ivar_name);
        mrb_value value = mrb_iv_get(mrb, self, ivar_sym);

        // Serialize the value
        mrb_value serialized = serialize_value(mrb, value);

        // Store with string key for JSON compatibility
        mrb_value key_str = mrb_sym_str(mrb, field_sym);
        mrb_hash_set(mrb, result, key_str, serialized);
    }

    return result;
}

/// @method to_json
/// @description Serialize this object to a JSON string.
/// @param pretty [Boolean] If true, format with indentation (default: false)
/// @return [String] JSON representation of the object
/// @example player.to_json        # => '{"name":"Hero","health":100}'
///   player.to_json(true)   # => Pretty-printed JSON
static mrb_value mrb_serializable_to_json(mrb_state* mrb, mrb_value self) {
    mrb_bool pretty = FALSE;
    mrb_get_args(mrb, "|b", &pretty);

    // First serialize to hash
    mrb_value hash = mrb_funcall(mrb, self, "serialize", 0);

    // Then convert to JSON using the JSON module
    RClass* json_mod = mrb_module_get(mrb, "JSON");
    if (!json_mod) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "[Serializable] JSON module not available");
        return mrb_nil_value();
    }

    const char* method = pretty ? "pretty_generate" : "generate";
    return mrb_funcall(mrb, mrb_obj_value(json_mod), method, 1, hash);
}

/// @method deserialize
/// @description Create an instance from a hash of field values. Class method.
/// @param data [Hash] The serialized data
/// @return [Object] A new instance with fields populated
/// @example player = Player.deserialize({ "name" => "Hero", "health" => 85 })
static mrb_value mrb_serializable_deserialize(mrb_state* mrb, mrb_value self) {
    mrb_value data;
    mrb_get_args(mrb, "H", &data);

    if (!mrb_class_p(self)) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "[Serializable] deserialize must be called on a class");
        return mrb_nil_value();
    }

    RClass* klass = mrb_class_ptr(self);

    // Get field definitions
    mrb_value fields = mrb_nil_value();
    RClass* check_class = klass;
    while (check_class && mrb_nil_p(fields)) {
        fields = mrb_iv_get(mrb, mrb_obj_value(check_class), mrb_intern_lit(mrb, "@_serializable_fields"));
        check_class = check_class->super;
    }

    // Allocate new instance (call allocate, not new, to avoid running initialize)
    mrb_value instance = mrb_obj_new(mrb, klass, 0, nullptr);

    if (!mrb_nil_p(fields) && mrb_hash_p(fields)) {
        mrb_value keys = mrb_hash_keys(mrb, fields);
        mrb_int len = RARRAY_LEN(keys);

        for (mrb_int i = 0; i < len; i++) {
            mrb_value key = mrb_ary_ref(mrb, keys, i);  // Symbol key (field name)
            mrb_sym field_sym = mrb_symbol(key);
            mrb_value field_opts = mrb_hash_get(mrb, fields, key);

            // Try to get value from data hash (both string and symbol keys)
            mrb_value key_str = mrb_sym_str(mrb, field_sym);
            mrb_value value = mrb_hash_get(mrb, data, key_str);
            if (mrb_nil_p(value)) {
                value = mrb_hash_get(mrb, data, key);  // Try symbol key too
            }

            // If still nil, use default
            if (mrb_nil_p(value) && mrb_hash_p(field_opts)) {
                mrb_value default_key = mrb_symbol_value(mrb_intern_lit(mrb, "default"));
                value = mrb_hash_get(mrb, field_opts, default_key);

                // If default is a proc, call it
                if (mrb_proc_p(value)) {
                    value = mrb_funcall(mrb, value, "call", 0);
                }
            }

            // Get type hint for deserialization
            mrb_value type_val = mrb_nil_value();
            if (mrb_hash_p(field_opts)) {
                mrb_value type_key = mrb_symbol_value(mrb_intern_lit(mrb, "type"));
                type_val = mrb_hash_get(mrb, field_opts, type_key);
            }

            // Deserialize the value
            mrb_value deserialized = deserialize_value(mrb, value, type_val);

            // Set instance variable
            char ivar_name[64];
            snprintf(ivar_name, sizeof(ivar_name), "@%s", mrb_sym_name(mrb, field_sym));
            mrb_sym ivar_sym = mrb_intern_cstr(mrb, ivar_name);
            mrb_iv_set(mrb, instance, ivar_sym, deserialized);
        }
    }

    // Call _after_deserialize if defined
    if (mrb_respond_to(mrb, instance, mrb_intern_lit(mrb, "_after_deserialize"))) {
        mrb_funcall(mrb, instance, "_after_deserialize", 0);
    }

    return instance;
}

/// @method from_json
/// @description Create an instance from a JSON string. Class method.
/// @param json_string [String] The JSON representation
/// @return [Object] A new instance with fields populated
/// @example player = Player.from_json('{"name":"Hero","health":85}')
static mrb_value mrb_serializable_from_json(mrb_state* mrb, mrb_value self) {
    const char* json_str;
    mrb_get_args(mrb, "z", &json_str);

    // Parse JSON to hash
    RClass* json_mod = mrb_module_get(mrb, "JSON");
    if (!json_mod) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "[Serializable] JSON module not available");
        return mrb_nil_value();
    }

    mrb_value json_val = mrb_str_new_cstr(mrb, json_str);
    mrb_value data = mrb_funcall(mrb, mrb_obj_value(json_mod), "parse", 1, json_val);

    // Check for parse errors
    if (mrb->exc) {
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb->exc = nullptr;
        mrb_value exc_msg = mrb_funcall(mrb, exc, "to_s", 0);
        const char* exc_str = mrb_string_p(exc_msg) ? RSTRING_PTR(exc_msg) : "unknown error";
        mrb_raisef(mrb, E_RUNTIME_ERROR, "[Serializable.from_json] Invalid JSON: %s", exc_str);
    }

    // Call deserialize
    return mrb_funcall(mrb, self, "deserialize", 1, data);
}

// ============================================================================
// Module included hook
// ============================================================================

/// Called when GMR::Serializable is included in a class
/// Adds the class methods (serializable, deserialize, from_json)
static mrb_value mrb_serializable_included(mrb_state* mrb, mrb_value self) {
    mrb_value base;
    mrb_get_args(mrb, "o", &base);

    if (!mrb_class_p(base) && !mrb_module_p(base)) {
        return mrb_nil_value();
    }

    RClass* base_class = mrb_class_ptr(base);

    // Add class methods
    mrb_define_class_method(mrb, base_class, "serializable", mrb_serializable_block, MRB_ARGS_BLOCK());
    mrb_define_class_method(mrb, base_class, "deserialize", mrb_serializable_deserialize, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, base_class, "from_json", mrb_serializable_from_json, MRB_ARGS_REQ(1));

    return mrb_nil_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_serializable(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // Create GMR::Serializable module
    RClass* serializable = mrb_define_module_under(mrb, gmr, "Serializable");

    // Instance methods (added to including class)
    mrb_define_method(mrb, serializable, "serialize", mrb_serializable_serialize, MRB_ARGS_NONE());
    mrb_define_method(mrb, serializable, "to_json", mrb_serializable_to_json, MRB_ARGS_OPT(1));

    // Module callback for when included
    mrb_define_class_method(mrb, serializable, "included", mrb_serializable_included, MRB_ARGS_REQ(1));

    // Create internal FieldBuilder class for DSL
    field_builder_class = mrb_define_class_under(mrb, serializable, "FieldBuilder", mrb->object_class);
    MRB_SET_INSTANCE_TT(field_builder_class, MRB_TT_CDATA);

    mrb_define_method(mrb, field_builder_class, "field", mrb_field_builder_field, MRB_ARGS_ARG(1, 1));
}

} // namespace bindings
} // namespace gmr
