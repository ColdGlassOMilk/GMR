#include "gmr/bindings/debug_draw.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/variable.h>
#include <vector>
#include <cmath>

namespace gmr {
namespace bindings {

// ============================================================================
// GMR::Debug Module
// ============================================================================

/// @module GMR::Debug
/// @description Debug drawing utilities for visualizing collision bounds, positions, and game state.
///   All debug draws are queued to the DEBUG_OVERLAY layer and rendered on top of everything.
///   Debug drawing is auto-cleared each frame.
/// @example # Visualize hitboxes during development
///   Debug.draw_rect(@hitbox.x, @hitbox.y, @hitbox.w, @hitbox.h, color: :green)
///   Debug.draw_circle(@x, @y, @radius, color: :red)
///   Debug.draw_line(@x, @y, @target_x, @target_y, color: :yellow)
///   Debug.draw_text("HP: #{@health}", @x, @y - 20)
/// @example # Conditional debug drawing
///   Debug.when_enabled do
///     draw_all_colliders
///   end

// Global debug state
static bool g_debug_enabled = true;

// ============================================================================
// Debug Toggle
// ============================================================================

/// @function enabled?
/// @description Check if debug drawing is enabled.
/// @returns [Boolean] true if debug drawing is enabled
/// @example if Debug.enabled? then draw_debug_info end
static mrb_value mrb_debug_enabled(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, g_debug_enabled);
}

/// @function enabled=
/// @description Enable or disable debug drawing globally.
/// @param value [Boolean] Whether debug drawing should be enabled
/// @returns [Boolean] The new value
/// @example Debug.enabled = false  # Disable all debug drawing
static mrb_value mrb_debug_set_enabled(mrb_state* mrb, mrb_value) {
    mrb_bool value;
    mrb_get_args(mrb, "b", &value);
    g_debug_enabled = value;
    return to_mrb_bool(mrb, g_debug_enabled);
}

/// @function when_enabled
/// @description Execute a block only when debug drawing is enabled.
///   Useful for conditionally drawing debug visualizations.
/// @param block [Block] The block to execute if debug is enabled
/// @returns [nil]
/// @example Debug.when_enabled do
///   @entities.each { |e| e.draw_debug }
/// end
static mrb_value mrb_debug_when_enabled(mrb_state* mrb, mrb_value) {
    mrb_value block;
    mrb_get_args(mrb, "&!", &block);

    if (g_debug_enabled) {
        std::vector<mrb_value> empty_args;
        scripting::safe_yield(mrb, block, empty_args);
    }

    return mrb_nil_value();
}

// ============================================================================
// Debug Drawing Functions
// ============================================================================

// Helper to parse color from args (symbol, array, or hash with color: key)
static DrawColor parse_debug_color(mrb_state* mrb, mrb_value arg, DrawColor default_color = {0, 255, 0, 255}) {
    if (mrb_nil_p(arg)) {
        return default_color;
    }

    // Hash with color: key
    if (mrb_hash_p(arg)) {
        mrb_value color_val = mrb_hash_get(mrb, arg,
            mrb_symbol_value(mrb_intern_lit(mrb, "color")));
        if (!mrb_nil_p(color_val)) {
            Color c = parse_color_value(mrb, color_val, {default_color.r, default_color.g, default_color.b, default_color.a});
            return {c.r, c.g, c.b, c.a};
        }
        return default_color;
    }

    // Direct color value (symbol or array)
    Color c = parse_color_value(mrb, arg, {default_color.r, default_color.g, default_color.b, default_color.a});
    return {c.r, c.g, c.b, c.a};
}

/// @function draw_rect
/// @description Draw a debug rectangle outline.
/// @param x [Float] X position (world units)
/// @param y [Float] Y position (world units)
/// @param w [Float] Width (world units)
/// @param h [Float] Height (world units)
/// @param color [Symbol, Array, Hash] Color (:green, [r,g,b], color: :red)
/// @returns [nil]
/// @example Debug.draw_rect(0, 0, 2, 1, color: :green)
/// @example Debug.draw_rect(@x, @y, @w, @h, :red)
static mrb_value mrb_debug_draw_rect(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x, y, w, h;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "ffff|o", &x, &y, &w, &h, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {0, 255, 0, 255});

    DrawQueue::instance().queue_rect(
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(w), static_cast<float>(h),
        color, false,  // false = outline only
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

/// @function draw_rect_filled
/// @description Draw a filled debug rectangle.
/// @param x [Float] X position (world units)
/// @param y [Float] Y position (world units)
/// @param w [Float] Width (world units)
/// @param h [Float] Height (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_rect_filled(0, 0, 2, 1, color: [255, 0, 0, 128])
static mrb_value mrb_debug_draw_rect_filled(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x, y, w, h;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "ffff|o", &x, &y, &w, &h, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {0, 255, 0, 128});

    DrawQueue::instance().queue_rect(
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(w), static_cast<float>(h),
        color, true,  // true = filled
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

/// @function draw_circle
/// @description Draw a debug circle outline.
/// @param x [Float] Center X position (world units)
/// @param y [Float] Center Y position (world units)
/// @param radius [Float] Radius (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_circle(@x, @y, @radius, :red)
static mrb_value mrb_debug_draw_circle(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x, y, radius;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "fff|o", &x, &y, &radius, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {255, 0, 0, 255});

    DrawQueue::instance().queue_circle(
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(radius),
        color, false,  // false = outline only
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

/// @function draw_circle_filled
/// @description Draw a filled debug circle.
/// @param x [Float] Center X position (world units)
/// @param y [Float] Center Y position (world units)
/// @param radius [Float] Radius (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_circle_filled(@x, @y, 0.5, color: [255, 0, 0, 128])
static mrb_value mrb_debug_draw_circle_filled(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x, y, radius;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "fff|o", &x, &y, &radius, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {255, 0, 0, 128});

    DrawQueue::instance().queue_circle(
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(radius),
        color, true,  // true = filled
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

/// @function draw_line
/// @description Draw a debug line.
/// @param x1 [Float] Start X position (world units)
/// @param y1 [Float] Start Y position (world units)
/// @param x2 [Float] End X position (world units)
/// @param y2 [Float] End Y position (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_line(@x, @y, @target_x, @target_y, :yellow)
static mrb_value mrb_debug_draw_line(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x1, y1, x2, y2;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "ffff|o", &x1, &y1, &x2, &y2, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {255, 255, 0, 255});

    DrawQueue::instance().queue_line(
        static_cast<float>(x1), static_cast<float>(y1),
        static_cast<float>(x2), static_cast<float>(y2),
        color, 1.0f,
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

/// @function draw_point
/// @description Draw a debug point (small cross or dot).
/// @param x [Float] X position (world units)
/// @param y [Float] Y position (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_point(@x, @y, :cyan)
static mrb_value mrb_debug_draw_point(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x, y;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "ff|o", &x, &y, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {255, 255, 255, 255});

    // Draw a small cross
    float size = 0.1f;  // Small size in world units
    DrawQueue::instance().queue_line(
        static_cast<float>(x) - size, static_cast<float>(y),
        static_cast<float>(x) + size, static_cast<float>(y),
        color, 1.0f,
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);
    DrawQueue::instance().queue_line(
        static_cast<float>(x), static_cast<float>(y) - size,
        static_cast<float>(x), static_cast<float>(y) + size,
        color, 1.0f,
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

/// @function draw_text
/// @description Draw debug text at world position.
/// @param text [String] Text to display
/// @param x [Float] X position (world units)
/// @param y [Float] Y position (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_text("HP: #{@health}", @x, @y - 1)
static mrb_value mrb_debug_draw_text(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    const char* text;
    mrb_float x, y;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "zff|o", &text, &x, &y, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {255, 255, 255, 255});

    DrawQueue::instance().queue_text(
        static_cast<float>(x), static_cast<float>(y),
        text, 16,  // Fixed small font size for debug
        color,
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

/// @function draw_arrow
/// @description Draw a debug arrow from start to end point.
/// @param x1 [Float] Start X position (world units)
/// @param y1 [Float] Start Y position (world units)
/// @param x2 [Float] End X position (world units)
/// @param y2 [Float] End Y position (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_arrow(@x, @y, @velocity_x, @velocity_y, :magenta)
static mrb_value mrb_debug_draw_arrow(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x1, y1, x2, y2;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "ffff|o", &x1, &y1, &x2, &y2, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {255, 0, 255, 255});

    // Main line
    DrawQueue::instance().queue_line(
        static_cast<float>(x1), static_cast<float>(y1),
        static_cast<float>(x2), static_cast<float>(y2),
        color, 1.0f,
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    // Arrow head
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    float len = sqrtf(dx * dx + dy * dy);

    if (len > 0.01f) {
        float head_size = len * 0.2f;  // 20% of line length
        if (head_size > 0.3f) head_size = 0.3f;  // Cap at 0.3 world units

        // Normalize direction
        dx /= len;
        dy /= len;

        // Perpendicular
        float px = -dy;
        float py = dx;

        // Arrow head points
        float hx1 = static_cast<float>(x2) - dx * head_size + px * head_size * 0.5f;
        float hy1 = static_cast<float>(y2) - dy * head_size + py * head_size * 0.5f;
        float hx2 = static_cast<float>(x2) - dx * head_size - px * head_size * 0.5f;
        float hy2 = static_cast<float>(y2) - dy * head_size - py * head_size * 0.5f;

        DrawQueue::instance().queue_line(
            static_cast<float>(x2), static_cast<float>(y2),
            hx1, hy1,
            color, 1.0f,
            static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);
        DrawQueue::instance().queue_line(
            static_cast<float>(x2), static_cast<float>(y2),
            hx2, hy2,
            color, 1.0f,
            static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);
    }

    return mrb_nil_value();
}

/// @function draw_cross
/// @description Draw a debug cross marker.
/// @param x [Float] Center X position (world units)
/// @param y [Float] Center Y position (world units)
/// @param size [Float] Size of the cross (world units)
/// @param color [Symbol, Array, Hash] Color
/// @returns [nil]
/// @example Debug.draw_cross(@spawn_x, @spawn_y, 0.5, :cyan)
static mrb_value mrb_debug_draw_cross(mrb_state* mrb, mrb_value) {
    if (!g_debug_enabled) return mrb_nil_value();

    mrb_float x, y, size;
    mrb_value color_arg = mrb_nil_value();
    mrb_get_args(mrb, "fff|o", &x, &y, &size, &color_arg);

    DrawColor color = parse_debug_color(mrb, color_arg, {0, 255, 255, 255});

    float half = static_cast<float>(size) * 0.5f;

    DrawQueue::instance().queue_line(
        static_cast<float>(x) - half, static_cast<float>(y),
        static_cast<float>(x) + half, static_cast<float>(y),
        color, 1.0f,
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);
    DrawQueue::instance().queue_line(
        static_cast<float>(x), static_cast<float>(y) - half,
        static_cast<float>(x), static_cast<float>(y) + half,
        color, 1.0f,
        static_cast<uint8_t>(RenderLayer::DEBUG_OVERLAY), 0.0f);

    return mrb_nil_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_debug_draw(mrb_state* mrb) {
    RClass* debug = get_gmr_submodule(mrb, "Debug");

    // Toggle
    mrb_define_module_function(mrb, debug, "enabled?", mrb_debug_enabled, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, debug, "enabled=", mrb_debug_set_enabled, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, debug, "when_enabled", mrb_debug_when_enabled, MRB_ARGS_BLOCK());

    // Drawing functions
    mrb_define_module_function(mrb, debug, "draw_rect", mrb_debug_draw_rect, MRB_ARGS_ARG(4, 1));
    mrb_define_module_function(mrb, debug, "draw_rect_filled", mrb_debug_draw_rect_filled, MRB_ARGS_ARG(4, 1));
    mrb_define_module_function(mrb, debug, "draw_circle", mrb_debug_draw_circle, MRB_ARGS_ARG(3, 1));
    mrb_define_module_function(mrb, debug, "draw_circle_filled", mrb_debug_draw_circle_filled, MRB_ARGS_ARG(3, 1));
    mrb_define_module_function(mrb, debug, "draw_line", mrb_debug_draw_line, MRB_ARGS_ARG(4, 1));
    mrb_define_module_function(mrb, debug, "draw_point", mrb_debug_draw_point, MRB_ARGS_ARG(2, 1));
    mrb_define_module_function(mrb, debug, "draw_text", mrb_debug_draw_text, MRB_ARGS_ARG(3, 1));
    mrb_define_module_function(mrb, debug, "draw_arrow", mrb_debug_draw_arrow, MRB_ARGS_ARG(4, 1));
    mrb_define_module_function(mrb, debug, "draw_cross", mrb_debug_draw_cross, MRB_ARGS_ARG(3, 1));
}

} // namespace bindings
} // namespace gmr
