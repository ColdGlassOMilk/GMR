#include "gmr/bindings/json.hpp"
#include "gmr/bindings/binding_helpers.hpp"

#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/array.h>
#include <mruby/class.h>

namespace gmr {
namespace bindings {

// ============================================================================
// GMR::JSON Module
// ============================================================================

/// @module GMR::JSON
/// @description JSON parsing and generation utilities for serialization.
///   Provides methods to convert between Ruby objects and JSON strings.
/// @example # Parse JSON string to Ruby object
///   data = GMR::JSON.parse('{"name": "Player", "health": 100}')
///   puts data["name"]   # "Player"
///   puts data["health"] # 100
/// @example # Convert Ruby object to JSON string
///   player = { name: "Hero", level: 5, items: ["sword", "shield"] }
///   json = GMR::JSON.stringify(player)
///   # '{"name":"Hero","level":5,"items":["sword","shield"]}'
/// @example # Pretty-print JSON
///   json = GMR::JSON.stringify(player, true)
///   # Formatted with indentation

/// @function parse
/// @description Parse a JSON string into a Ruby object (Hash or Array).
/// @param json_string [String] The JSON string to parse
/// @return [Hash, Array, String, Fixnum, Float, Boolean, nil] The parsed data
/// @raise [RuntimeError] if the JSON is malformed
/// @example data = GMR::JSON.parse('{"x": 100, "y": 200}')
///   puts data["x"]  # 100
/// @example items = GMR::JSON.parse('["sword", "shield", "potion"]')
///   puts items[0]   # "sword"
static mrb_value mrb_json_parse(mrb_state* mrb, mrb_value self) {
    const char* json_str;
    mrb_get_args(mrb, "z", &json_str);

    // Get the mruby-json JSON module
    RClass* json_mod = mrb_module_get(mrb, "JSON");
    if (!json_mod) {
        mrb_raise(mrb, E_RUNTIME_ERROR,
            "[JSON.parse] JSON module not available");
    }

    // Call JSON.parse
    mrb_value json_val = mrb_str_new_cstr(mrb, json_str);
    mrb_value parsed = mrb_funcall(mrb, mrb_obj_value(json_mod), "parse", 1, json_val);

    // Check for exceptions
    if (mrb->exc) {
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb->exc = nullptr;

        mrb_value exc_msg = mrb_funcall(mrb, exc, "to_s", 0);
        const char* exc_str = mrb_string_p(exc_msg) ? RSTRING_PTR(exc_msg) : "unknown error";

        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[JSON.parse] Invalid JSON: %s", exc_str);
    }

    return parsed;
}

/// @function stringify
/// @description Convert a Ruby object to a JSON string.
///   Supports Hash, Array, String, Fixnum, Float, true, false, and nil.
///   Objects with a to_h method will be converted via that method.
/// @param data [Object] The Ruby object to serialize
/// @param pretty [Boolean] If true, format with indentation (default: false)
/// @return [String] The JSON string
/// @raise [RuntimeError] if the object cannot be serialized
/// @example json = GMR::JSON.stringify({ level: 5, score: 1000 })
///   # '{"level":5,"score":1000}'
/// @example json = GMR::JSON.stringify(data, true)  # Pretty print
static mrb_value mrb_json_stringify(mrb_state* mrb, mrb_value self) {
    mrb_value data;
    mrb_bool pretty = FALSE;
    mrb_get_args(mrb, "o|b", &data, &pretty);

    // Get the mruby-json JSON module
    RClass* json_mod = mrb_module_get(mrb, "JSON");
    if (!json_mod) {
        mrb_raise(mrb, E_RUNTIME_ERROR,
            "[JSON.stringify] JSON module not available");
    }

    // Choose method based on pretty flag
    const char* method = pretty ? "pretty_generate" : "generate";
    mrb_value json_str = mrb_funcall(mrb, mrb_obj_value(json_mod), method, 1, data);

    // Check for exceptions
    if (mrb->exc) {
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb->exc = nullptr;

        mrb_value exc_msg = mrb_funcall(mrb, exc, "to_s", 0);
        const char* exc_str = mrb_string_p(exc_msg) ? RSTRING_PTR(exc_msg) : "unknown error";

        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[JSON.stringify] Failed to serialize: %s", exc_str);
    }

    return json_str;
}

// ============================================================================
// Registration
// ============================================================================

void register_json(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* json = mrb_define_module_under(mrb, gmr, "JSON");

    mrb_define_module_function(mrb, json, "parse", mrb_json_parse, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, json, "stringify", mrb_json_stringify, MRB_ARGS_ARG(1, 1));
}

} // namespace bindings
} // namespace gmr
