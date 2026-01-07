#ifndef GMR_BINDING_HELPERS_HPP
#define GMR_BINDING_HELPERS_HPP

#include "gmr/types.hpp"
#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/class.h>
#include <mruby/data.h>

namespace gmr {

// Forward declarations
struct TilemapData;

namespace bindings {

// Parse color from mruby args - supports [r,g,b], [r,g,b,a], or r,g,b / r,g,b,a
Color parse_color(mrb_state* mrb, mrb_value* argv, mrb_int argc, const Color& default_color);

// Parse a single color value (array [r,g,b] or [r,g,b,a])
Color parse_color_value(mrb_state* mrb, mrb_value val, const Color& default_color);

// Helper to return boolean
inline mrb_value to_mrb_bool(mrb_state*, bool value) {
    return value ? mrb_true_value() : mrb_false_value();
}

// ============================================================================
// Module Hierarchy Helpers
// ============================================================================

// Get or create the top-level GMR module
RClass* get_gmr_module(mrb_state* mrb);

// Get a submodule under GMR (e.g., "Graphics" -> GMR::Graphics)
RClass* get_gmr_submodule(mrb_state* mrb, const char* name);

// Initialize the full GMR module hierarchy - call once at startup
void init_gmr_modules(mrb_state* mrb);

// ============================================================================
// Symbol/Key Mapping Helpers (for Input module)
// ============================================================================

// Convert a symbol to a Raylib key code, returns -1 if unknown
int symbol_to_key(mrb_state* mrb, mrb_sym sym);

// Convert a symbol to a Raylib mouse button, returns -1 if unknown
int symbol_to_mouse_button(mrb_state* mrb, mrb_sym sym);

// Parse key argument - accepts integer or symbol
int parse_key_arg(mrb_state* mrb, mrb_value arg);

// Parse mouse button argument - accepts integer or symbol
int parse_mouse_button_arg(mrb_state* mrb, mrb_value arg);

} // namespace bindings

// Forward declaration of InputPhase from gmr::input module
namespace input {
    enum class InputPhase;
}

namespace bindings {

// ============================================================================
// Input Phase Parsing (for event-driven input system)
// ============================================================================

// Parse input phase from symbol (:pressed, :released, :held)
// Returns Pressed if symbol is unknown or nil
gmr::input::InputPhase parse_input_phase(mrb_state* mrb, mrb_value arg);

// ============================================================================
// Tilemap Helpers (for Collision module)
// ============================================================================

// Get TilemapData pointer from a Ruby Tilemap object
// Returns nullptr if the object is not a valid Tilemap
TilemapData* get_tilemap_from_value(mrb_state* mrb, mrb_value tilemap_obj);

} // namespace bindings
} // namespace gmr

#endif
