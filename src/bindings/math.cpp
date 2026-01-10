#include "gmr/bindings/math.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/types.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <cmath>
#include <cstdlib>
#include "raylib.h"

namespace gmr {
namespace bindings {

// ============================================================================
// Vec2 Class
// ============================================================================

/// @class Vec2
/// @description A 2D vector with x and y components. Used for positions, velocities,
///   and directions. Supports arithmetic operations (+, -, *, /).
/// @example # Player movement with velocity
///   class Player
///     def initialize(x, y)
///       @position = Vec2.new(x, y)
///       @velocity = Vec2.new(0, 0)
///       @acceleration = 500  # Pixels per second squared
///       @max_speed = 200
///       @friction = 0.9
///     end
///
///     def update(dt)
///       # Input direction as vector
///       input = Vec2.new(0, 0)
///       input = input + Vec2.new(-1, 0) if GMR::Input.key_down?(:left)
///       input = input + Vec2.new(1, 0) if GMR::Input.key_down?(:right)
///       input = input + Vec2.new(0, -1) if GMR::Input.key_down?(:up)
///       input = input + Vec2.new(0, 1) if GMR::Input.key_down?(:down)
///
///       # Accelerate in input direction
///       @velocity = @velocity + input * (@acceleration * dt)
///
///       # Apply friction
///       @velocity = @velocity * @friction
///
///       # Clamp to max speed
///       speed = Math.sqrt(@velocity.x ** 2 + @velocity.y ** 2)
///       if speed > @max_speed
///         @velocity = @velocity * (@max_speed / speed)
///       end
///
///       # Update position
///       @position = @position + @velocity * dt
///     end
///
///     def draw
///       @sprite.x = @position.x
///       @sprite.y = @position.y
///       @sprite.draw
///     end
///   end
/// @example # Direction and distance calculations
///   class Enemy
///     def update_ai(player_pos, dt)
///       # Vector from enemy to player
///       to_player = player_pos - @position
///       distance = Math.sqrt(to_player.x ** 2 + to_player.y ** 2)
///
///       if distance < 200 && distance > 0
///         # Move toward player
///         direction = to_player / distance  # Normalize
///         @position = @position + direction * (@speed * dt)
///       end
///     end
///   end
/// @example # Camera smoothing with lerp
///   class SmoothCamera
///     def follow(target_pos, dt)
///       # Lerp toward target position
///       lerp_factor = 5.0 * dt  # Smoothing speed
///       diff = target_pos - @position
///       @position = @position + diff * lerp_factor
///     end
///   end

struct Vec2Data {
    float x;
    float y;
};

static void vec2_free(mrb_state* mrb, void* ptr) {
    mrb_free(mrb, ptr);
}

static const mrb_data_type vec2_data_type = {
    "Vec2", vec2_free
};

static Vec2Data* get_vec2_data(mrb_state* mrb, mrb_value self) {
    return static_cast<Vec2Data*>(mrb_data_get_ptr(mrb, self, &vec2_data_type));
}

/// @method initialize
/// @description Create a new Vec2 with optional x and y values.
/// @param x [Float] The x component (default: 0)
/// @param y [Float] The y component (default: 0)
/// @returns [Vec2] The new vector
/// @example Vec2.new              # (0, 0)
/// @example Vec2.new(100, 200)    # (100, 200)
static mrb_value mrb_vec2_initialize(mrb_state* mrb, mrb_value self) {
    mrb_float x = 0.0f, y = 0.0f;
    mrb_get_args(mrb, "|ff", &x, &y);

    Vec2Data* data = static_cast<Vec2Data*>(mrb_malloc(mrb, sizeof(Vec2Data)));
    data->x = static_cast<float>(x);
    data->y = static_cast<float>(y);
    mrb_data_init(self, data, &vec2_data_type);

    return self;
}

/// @method x
/// @description Get the x component.
/// @returns [Float] The x value
/// @example x = vec.x
static mrb_value mrb_vec2_x(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);
    return mrb_float_value(mrb, data->x);
}

/// @method y
/// @description Get the y component.
/// @returns [Float] The y value
/// @example y = vec.y
static mrb_value mrb_vec2_y(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);
    return mrb_float_value(mrb, data->y);
}

// Helper to create a new Vec2 Ruby object
static mrb_value create_vec2(mrb_state* mrb, float x, float y) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* mathf = mrb_module_get_under(mrb, gmr, "Mathf");
    RClass* vec2_class = mrb_class_get_under(mrb, mathf, "Vec2");
    mrb_value obj = mrb_obj_new(mrb, vec2_class, 0, nullptr);

    Vec2Data* data = static_cast<Vec2Data*>(mrb_malloc(mrb, sizeof(Vec2Data)));
    data->x = x;
    data->y = y;
    mrb_data_init(obj, data, &vec2_data_type);

    return obj;
}

/// @method +
/// @description Add two vectors, returning a new Vec2.
/// @param other [Vec2] The vector to add
/// @returns [Vec2] A new vector with the sum
/// @example result = Vec2.new(1, 2) + Vec2.new(3, 4)  # Vec2(4, 6)
static mrb_value mrb_vec2_add(mrb_state* mrb, mrb_value self) {
    mrb_value other;
    mrb_get_args(mrb, "o", &other);

    Vec2Data* a = get_vec2_data(mrb, self);
    Vec2Data* b = static_cast<Vec2Data*>(mrb_data_get_ptr(mrb, other, &vec2_data_type));
    if (!b) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec2");
        return mrb_nil_value();
    }

    return create_vec2(mrb, a->x + b->x, a->y + b->y);
}

/// @method -
/// @description Subtract two vectors, returning a new Vec2.
/// @param other [Vec2] The vector to subtract
/// @returns [Vec2] A new vector with the difference
/// @example result = Vec2.new(5, 5) - Vec2.new(2, 1)  # Vec2(3, 4)
static mrb_value mrb_vec2_sub(mrb_state* mrb, mrb_value self) {
    mrb_value other;
    mrb_get_args(mrb, "o", &other);

    Vec2Data* a = get_vec2_data(mrb, self);
    Vec2Data* b = static_cast<Vec2Data*>(mrb_data_get_ptr(mrb, other, &vec2_data_type));
    if (!b) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec2");
        return mrb_nil_value();
    }

    return create_vec2(mrb, a->x - b->x, a->y - b->y);
}

/// @method *
/// @description Multiply vector by a scalar, returning a new Vec2.
/// @param scalar [Float] The scalar value
/// @returns [Vec2] A new scaled vector
/// @example result = Vec2.new(2, 3) * 2.0  # Vec2(4, 6)
static mrb_value mrb_vec2_mul(mrb_state* mrb, mrb_value self) {
    mrb_float scalar;
    mrb_get_args(mrb, "f", &scalar);

    Vec2Data* data = get_vec2_data(mrb, self);
    return create_vec2(mrb, data->x * static_cast<float>(scalar),
                           data->y * static_cast<float>(scalar));
}

/// @method /
/// @description Divide vector by a scalar, returning a new Vec2.
/// @param scalar [Float] The scalar value (must not be zero)
/// @returns [Vec2] A new scaled vector
/// @example result = Vec2.new(10, 20) / 2.0  # Vec2(5, 10)
static mrb_value mrb_vec2_div(mrb_state* mrb, mrb_value self) {
    mrb_float scalar;
    mrb_get_args(mrb, "f", &scalar);

    if (scalar == 0.0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Division by zero");
        return mrb_nil_value();
    }

    Vec2Data* data = get_vec2_data(mrb, self);
    return create_vec2(mrb, data->x / static_cast<float>(scalar),
                           data->y / static_cast<float>(scalar));
}

/// @method to_s
/// @description Convert to a string representation.
/// @returns [String] String in format "Vec2(x, y)"
/// @example puts Vec2.new(1, 2).to_s  # "Vec2(1.00, 2.00)"
static mrb_value mrb_vec2_to_s(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);
    char buf[64];
    snprintf(buf, sizeof(buf), "Vec2(%.2f, %.2f)", data->x, data->y);
    return mrb_str_new_cstr(mrb, buf);
}

/// @method to_a
/// @description Convert to an array [x, y].
/// @returns [Array<Float>] Array containing [x, y]
/// @example pos = Vec2.new(100, 200)
///   x, y = pos.to_a  # x=100, y=200
/// @example Player.new(*spawn_point.to_a)  # Splat into arguments
static mrb_value mrb_vec2_to_a(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);
    mrb_value arr = mrb_ary_new_capa(mrb, 2);
    mrb_ary_push(mrb, arr, mrb_float_value(mrb, data->x));
    mrb_ary_push(mrb, arr, mrb_float_value(mrb, data->y));
    return arr;
}

// ============================================================================
// Vec3 Class
// ============================================================================

/// @class Vec3
/// @description A 3D vector with x, y, and z components. Used for 3D positions,
///   colors (RGB), and other 3-component values. Supports arithmetic operations.
/// @example # Color manipulation for visual effects
///   class ColorEffects
///     def initialize
///       @base_color = Vec3.new(255, 100, 50)  # Orange
///       @target_color = Vec3.new(50, 100, 255)  # Blue
///       @flash_color = Vec3.new(255, 255, 255)  # White
///     end
///
///     # Lerp between two colors
///     def lerp_color(from, to, t)
///       diff = to - from
///       from + diff * t
///     end
///
///     # Flash white and return to base color
///     def damage_flash(sprite, duration)
///       # Start white, fade back to original
///       @current_color = @flash_color
///       GMR::Tween.to(self, :current_color, @base_color, duration: duration)
///     end
///
///     def apply_color(sprite)
///       sprite.color = [@current_color.x, @current_color.y, @current_color.z]
///     end
///   end
/// @example # 3D position for parallax layers
///   class ParallaxLayer
///     def initialize(texture, depth)
///       @texture = texture
///       @position = Vec3.new(0, 0, depth)  # Z is depth for parallax factor
///     end
///
///     def update(camera_x, camera_y)
///       # Deeper layers move slower (lower Z = further back)
///       parallax_factor = 1.0 / (@position.z + 1)
///       @draw_x = -camera_x * parallax_factor
///       @draw_y = -camera_y * parallax_factor
///     end
///   end

struct Vec3Data {
    float x;
    float y;
    float z;
};

static void vec3_free(mrb_state* mrb, void* ptr) {
    mrb_free(mrb, ptr);
}

static const mrb_data_type vec3_data_type = {
    "Vec3", vec3_free
};

static Vec3Data* get_vec3_data(mrb_state* mrb, mrb_value self) {
    return static_cast<Vec3Data*>(mrb_data_get_ptr(mrb, self, &vec3_data_type));
}

/// @method initialize
/// @description Create a new Vec3 with optional x, y, and z values.
/// @param x [Float] The x component (default: 0)
/// @param y [Float] The y component (default: 0)
/// @param z [Float] The z component (default: 0)
/// @returns [Vec3] The new vector
/// @example Vec3.new              # (0, 0, 0)
/// @example Vec3.new(1, 2, 3)     # (1, 2, 3)
static mrb_value mrb_vec3_initialize(mrb_state* mrb, mrb_value self) {
    mrb_float x = 0.0f, y = 0.0f, z = 0.0f;
    mrb_get_args(mrb, "|fff", &x, &y, &z);

    Vec3Data* data = static_cast<Vec3Data*>(mrb_malloc(mrb, sizeof(Vec3Data)));
    data->x = static_cast<float>(x);
    data->y = static_cast<float>(y);
    data->z = static_cast<float>(z);
    mrb_data_init(self, data, &vec3_data_type);

    return self;
}

/// @method x
/// @description Get the x component.
/// @returns [Float] The x value
/// @example x = vec.x
static mrb_value mrb_vec3_x(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    return mrb_float_value(mrb, data->x);
}

/// @method y
/// @description Get the y component.
/// @returns [Float] The y value
/// @example y = vec.y
static mrb_value mrb_vec3_y(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    return mrb_float_value(mrb, data->y);
}

/// @method z
/// @description Get the z component.
/// @returns [Float] The z value
/// @example z = vec.z
static mrb_value mrb_vec3_z(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    return mrb_float_value(mrb, data->z);
}

// Helper to create a new Vec3 Ruby object
static mrb_value create_vec3(mrb_state* mrb, float x, float y, float z) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* mathf = mrb_module_get_under(mrb, gmr, "Mathf");
    RClass* vec3_class = mrb_class_get_under(mrb, mathf, "Vec3");
    mrb_value obj = mrb_obj_new(mrb, vec3_class, 0, nullptr);

    Vec3Data* data = static_cast<Vec3Data*>(mrb_malloc(mrb, sizeof(Vec3Data)));
    data->x = x;
    data->y = y;
    data->z = z;
    mrb_data_init(obj, data, &vec3_data_type);

    return obj;
}

/// @method +
/// @description Add two vectors, returning a new Vec3.
/// @param other [Vec3] The vector to add
/// @returns [Vec3] A new vector with the sum
/// @example result = Vec3.new(1, 2, 3) + Vec3.new(1, 1, 1)  # Vec3(2, 3, 4)
static mrb_value mrb_vec3_add(mrb_state* mrb, mrb_value self) {
    mrb_value other;
    mrb_get_args(mrb, "o", &other);

    Vec3Data* a = get_vec3_data(mrb, self);
    Vec3Data* b = static_cast<Vec3Data*>(mrb_data_get_ptr(mrb, other, &vec3_data_type));
    if (!b) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec3");
        return mrb_nil_value();
    }

    return create_vec3(mrb, a->x + b->x, a->y + b->y, a->z + b->z);
}

/// @method -
/// @description Subtract two vectors, returning a new Vec3.
/// @param other [Vec3] The vector to subtract
/// @returns [Vec3] A new vector with the difference
/// @example result = Vec3.new(5, 5, 5) - Vec3.new(1, 2, 3)  # Vec3(4, 3, 2)
static mrb_value mrb_vec3_sub(mrb_state* mrb, mrb_value self) {
    mrb_value other;
    mrb_get_args(mrb, "o", &other);

    Vec3Data* a = get_vec3_data(mrb, self);
    Vec3Data* b = static_cast<Vec3Data*>(mrb_data_get_ptr(mrb, other, &vec3_data_type));
    if (!b) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec3");
        return mrb_nil_value();
    }

    return create_vec3(mrb, a->x - b->x, a->y - b->y, a->z - b->z);
}

/// @method *
/// @description Multiply vector by a scalar, returning a new Vec3.
/// @param scalar [Float] The scalar value
/// @returns [Vec3] A new scaled vector
/// @example result = Vec3.new(1, 2, 3) * 2.0  # Vec3(2, 4, 6)
static mrb_value mrb_vec3_mul(mrb_state* mrb, mrb_value self) {
    mrb_float scalar;
    mrb_get_args(mrb, "f", &scalar);

    Vec3Data* data = get_vec3_data(mrb, self);
    return create_vec3(mrb, data->x * static_cast<float>(scalar),
                           data->y * static_cast<float>(scalar),
                           data->z * static_cast<float>(scalar));
}

/// @method /
/// @description Divide vector by a scalar, returning a new Vec3.
/// @param scalar [Float] The scalar value (must not be zero)
/// @returns [Vec3] A new scaled vector
/// @example result = Vec3.new(10, 20, 30) / 10.0  # Vec3(1, 2, 3)
static mrb_value mrb_vec3_div(mrb_state* mrb, mrb_value self) {
    mrb_float scalar;
    mrb_get_args(mrb, "f", &scalar);

    if (scalar == 0.0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Division by zero");
        return mrb_nil_value();
    }

    Vec3Data* data = get_vec3_data(mrb, self);
    return create_vec3(mrb, data->x / static_cast<float>(scalar),
                           data->y / static_cast<float>(scalar),
                           data->z / static_cast<float>(scalar));
}

/// @method to_s
/// @description Convert to a string representation.
/// @returns [String] String in format "Vec3(x, y, z)"
/// @example puts Vec3.new(1, 2, 3).to_s  # "Vec3(1.00, 2.00, 3.00)"
static mrb_value mrb_vec3_to_s(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    char buf[96];
    snprintf(buf, sizeof(buf), "Vec3(%.2f, %.2f, %.2f)", data->x, data->y, data->z);
    return mrb_str_new_cstr(mrb, buf);
}

// ============================================================================
// Rect Class
// ============================================================================

/// @class Rect
/// @description A rectangle with position (x, y) and dimensions (w, h).
///   Used for bounds, source rectangles, collision areas, and UI layout.
/// @example # Entity bounds for collision
///   class Entity
///     def initialize(x, y, width, height)
///       @x, @y = x, y
///       @width, @height = width, height
///     end
///
///     def bounds
///       Rect.new(@x, @y, @width, @height)
///     end
///
///     def collides_with?(other)
///       a = bounds
///       b = other.bounds
///       # AABB collision
///       a.x < b.x + b.w && a.x + a.w > b.x &&
///       a.y < b.y + b.h && a.y + a.h > b.y
///     end
///   end
/// @example # Sprite sheet source rectangles
///   class SpriteSheet
///     def initialize(texture, tile_width, tile_height)
///       @texture = texture
///       @tile_w = tile_width
///       @tile_h = tile_height
///       @columns = (@texture.width / tile_width).to_i
///     end
///
///     # Get source rect for a specific tile index
///     def tile_rect(index)
///       col = index % @columns
///       row = index / @columns
///       Rect.new(col * @tile_w, row * @tile_h, @tile_w, @tile_h)
///     end
///
///     def draw_tile(index, x, y)
///       src = tile_rect(index)
///       GMR::Graphics.draw_texture_rec(@texture, src, x, y, [255, 255, 255])
///     end
///   end
/// @example # UI layout helper
///   class UILayoutHelper
///     def initialize(container)
///       @container = container  # Rect defining available space
///       @padding = 10
///     end
///
///     # Divide container into grid cells
///     def grid_cell(row, col, rows, cols)
///       cell_w = (@container.w - @padding * (cols + 1)) / cols
///       cell_h = (@container.h - @padding * (rows + 1)) / rows
///       x = @container.x + @padding + col * (cell_w + @padding)
///       y = @container.y + @padding + row * (cell_h + @padding)
///       Rect.new(x, y, cell_w, cell_h)
///     end
///
///     # Get rect for a row within container
///     def row_rect(row_index, row_height)
///       Rect.new(
///         @container.x + @padding,
///         @container.y + @padding + row_index * (row_height + @padding),
///         @container.w - @padding * 2,
///         row_height
///       )
///     end
///   end

struct RectData {
    float x;
    float y;
    float w;
    float h;
};

static void rect_free(mrb_state* mrb, void* ptr) {
    mrb_free(mrb, ptr);
}

static const mrb_data_type rect_data_type = {
    "Rect", rect_free
};

static RectData* get_rect_data(mrb_state* mrb, mrb_value self) {
    return static_cast<RectData*>(mrb_data_get_ptr(mrb, self, &rect_data_type));
}

/// @method initialize
/// @description Create a new Rect with optional position and dimensions.
/// @param x [Float] The x position (default: 0)
/// @param y [Float] The y position (default: 0)
/// @param w [Float] The width (default: 0)
/// @param h [Float] The height (default: 0)
/// @returns [Rect] The new rectangle
/// @example Rect.new                    # (0, 0, 0, 0)
/// @example Rect.new(10, 20, 100, 50)   # x=10, y=20, w=100, h=50
static mrb_value mrb_rect_initialize(mrb_state* mrb, mrb_value self) {
    mrb_float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    mrb_get_args(mrb, "|ffff", &x, &y, &w, &h);

    RectData* data = static_cast<RectData*>(mrb_malloc(mrb, sizeof(RectData)));
    data->x = static_cast<float>(x);
    data->y = static_cast<float>(y);
    data->w = static_cast<float>(w);
    data->h = static_cast<float>(h);
    mrb_data_init(self, data, &rect_data_type);

    return self;
}

/// @method x
/// @description Get the x position (left edge).
/// @returns [Float] The x position
/// @example x = rect.x
static mrb_value mrb_rect_x(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->x);
}

/// @method y
/// @description Get the y position (top edge).
/// @returns [Float] The y position
/// @example y = rect.y
static mrb_value mrb_rect_y(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->y);
}

/// @method w
/// @description Get the width.
/// @returns [Float] The width
/// @example width = rect.w
static mrb_value mrb_rect_w(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->w);
}

/// @method h
/// @description Get the height.
/// @returns [Float] The height
/// @example height = rect.h
static mrb_value mrb_rect_h(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->h);
}

/// @method to_s
/// @description Convert to a string representation.
/// @returns [String] String in format "Rect(x, y, w, h)"
/// @example puts Rect.new(0, 0, 32, 32).to_s  # "Rect(0.00, 0.00, 32.00, 32.00)"
static mrb_value mrb_rect_to_s(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    char buf[96];
    snprintf(buf, sizeof(buf), "Rect(%.2f, %.2f, %.2f, %.2f)",
             data->x, data->y, data->w, data->h);
    return mrb_str_new_cstr(mrb, buf);
}

// ============================================================================
// GMR::Mathf Module - Common math utilities
// ============================================================================

/// @module GMR::Mathf
/// @description Common math utility functions for game development.
///   Provides lerp, clamp, smoothstep, and other interpolation helpers.
///   Named "Mathf" to avoid conflict with Ruby's built-in Math module.
/// @example # Smooth camera following
///   target_x = player.x
///   @camera_x = Mathf.lerp(@camera_x, target_x, 0.1)
/// @example # Clamping values
///   health = Mathf.clamp(health, 0, max_health)
///   zoom = Mathf.clamp(zoom, 0.5, 4.0)
/// @example # Smooth transitions
///   t = elapsed / duration
///   alpha = Mathf.smoothstep(0, 1, t) * 255

/// @method lerp
/// @description Linear interpolation between two values.
/// @param a [Float] Start value
/// @param b [Float] End value
/// @param t [Float] Interpolation factor (0.0 to 1.0)
/// @returns [Float] Interpolated value
/// @example Mathf.lerp(0, 100, 0.5)  # => 50.0
/// @example Mathf.lerp(10, 20, 0.25) # => 12.5
static mrb_value mrb_math_lerp(mrb_state* mrb, mrb_value) {
    mrb_float a, b, t;
    mrb_get_args(mrb, "fff", &a, &b, &t);
    return mrb_float_value(mrb, a + (b - a) * t);
}

/// @method clamp
/// @description Clamp a value between minimum and maximum bounds.
/// @param value [Float] The value to clamp
/// @param min [Float] Minimum bound
/// @param max [Float] Maximum bound
/// @returns [Float] Clamped value
/// @example Mathf.clamp(150, 0, 100)  # => 100.0
/// @example Mathf.clamp(-5, 0, 100)   # => 0.0
/// @example Mathf.clamp(50, 0, 100)   # => 50.0
static mrb_value mrb_math_clamp(mrb_state* mrb, mrb_value) {
    mrb_float value, min_val, max_val;
    mrb_get_args(mrb, "fff", &value, &min_val, &max_val);
    if (value < min_val) return mrb_float_value(mrb, min_val);
    if (value > max_val) return mrb_float_value(mrb, max_val);
    return mrb_float_value(mrb, value);
}

/// @method smoothstep
/// @description Smooth Hermite interpolation between two values.
///   Produces an S-curve that eases in and out.
/// @param edge0 [Float] Lower edge
/// @param edge1 [Float] Upper edge
/// @param x [Float] Input value
/// @returns [Float] Smoothed value (0.0 to 1.0 when x is between edges)
/// @example Mathf.smoothstep(0, 1, 0.5)  # => 0.5
/// @example Mathf.smoothstep(0, 1, 0.25) # => ~0.156
static mrb_value mrb_math_smoothstep(mrb_state* mrb, mrb_value) {
    mrb_float edge0, edge1, x;
    mrb_get_args(mrb, "fff", &edge0, &edge1, &x);

    // Clamp t to 0-1 range
    double t = (x - edge0) / (edge1 - edge0);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    // Hermite interpolation
    return mrb_float_value(mrb, t * t * (3.0 - 2.0 * t));
}

/// @method inverse_lerp
/// @description Find the interpolation factor for a value between two bounds.
///   The inverse of lerp - given a value, find what t would produce it.
/// @param a [Float] Start value
/// @param b [Float] End value
/// @param value [Float] The value to find the factor for
/// @returns [Float] Interpolation factor (can be outside 0-1 if value is outside a-b)
/// @example Mathf.inverse_lerp(0, 100, 50)  # => 0.5
/// @example Mathf.inverse_lerp(10, 20, 15)  # => 0.5
static mrb_value mrb_math_inverse_lerp(mrb_state* mrb, mrb_value) {
    mrb_float a, b, value;
    mrb_get_args(mrb, "fff", &a, &b, &value);

    if (b - a == 0.0) {
        return mrb_float_value(mrb, 0.0);
    }
    return mrb_float_value(mrb, (value - a) / (b - a));
}

/// @method remap
/// @description Remap a value from one range to another.
/// @param value [Float] Input value
/// @param in_min [Float] Input range minimum
/// @param in_max [Float] Input range maximum
/// @param out_min [Float] Output range minimum
/// @param out_max [Float] Output range maximum
/// @returns [Float] Remapped value
/// @example Mathf.remap(50, 0, 100, 0, 1)    # => 0.5
/// @example Mathf.remap(0.5, 0, 1, 0, 255)   # => 127.5
static mrb_value mrb_math_remap(mrb_state* mrb, mrb_value) {
    mrb_float value, in_min, in_max, out_min, out_max;
    mrb_get_args(mrb, "fffff", &value, &in_min, &in_max, &out_min, &out_max);

    double t = (in_max - in_min == 0.0) ? 0.0 : (value - in_min) / (in_max - in_min);
    return mrb_float_value(mrb, out_min + (out_max - out_min) * t);
}

/// @method distance
/// @description Calculate Euclidean distance between two 2D points.
/// @param x1 [Float] First point X
/// @param y1 [Float] First point Y
/// @param x2 [Float] Second point X
/// @param y2 [Float] Second point Y
/// @returns [Float] Distance between points
/// @example Mathf.distance(0, 0, 3, 4)  # => 5.0
static mrb_value mrb_math_distance(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2;
    mrb_get_args(mrb, "ffff", &x1, &y1, &x2, &y2);
    double dx = x2 - x1;
    double dy = y2 - y1;
    return mrb_float_value(mrb, sqrt(dx * dx + dy * dy));
}

/// @method distance_squared
/// @description Calculate squared distance between two 2D points.
///   Faster than distance() when you only need to compare distances.
/// @param x1 [Float] First point X
/// @param y1 [Float] First point Y
/// @param x2 [Float] Second point X
/// @param y2 [Float] Second point Y
/// @returns [Float] Squared distance between points
/// @example Mathf.distance_squared(0, 0, 3, 4)  # => 25.0
static mrb_value mrb_math_distance_squared(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2;
    mrb_get_args(mrb, "ffff", &x1, &y1, &x2, &y2);
    double dx = x2 - x1;
    double dy = y2 - y1;
    return mrb_float_value(mrb, dx * dx + dy * dy);
}

/// @method sign
/// @description Get the sign of a number (-1, 0, or 1).
/// @param value [Float] The value to check
/// @returns [Integer] -1 if negative, 0 if zero, 1 if positive
/// @example Mathf.sign(-5)  # => -1
/// @example Mathf.sign(0)   # => 0
/// @example Mathf.sign(10)  # => 1
static mrb_value mrb_math_sign(mrb_state* mrb, mrb_value) {
    mrb_float value;
    mrb_get_args(mrb, "f", &value);
    if (value < 0) return mrb_fixnum_value(-1);
    if (value > 0) return mrb_fixnum_value(1);
    return mrb_fixnum_value(0);
}

/// @method move_toward
/// @description Move a value toward a target by a maximum delta.
/// @param current [Float] Current value
/// @param target [Float] Target value
/// @param max_delta [Float] Maximum amount to move
/// @returns [Float] New value moved toward target
/// @example Mathf.move_toward(0, 100, 10)   # => 10.0
/// @example Mathf.move_toward(95, 100, 10)  # => 100.0
static mrb_value mrb_math_move_toward(mrb_state* mrb, mrb_value) {
    mrb_float current, target, max_delta;
    mrb_get_args(mrb, "fff", &current, &target, &max_delta);

    double diff = target - current;
    if (fabs(diff) <= max_delta) {
        return mrb_float_value(mrb, target);
    }
    return mrb_float_value(mrb, current + (diff > 0 ? max_delta : -max_delta));
}

/// @method wrap
/// @description Wrap a value within a range (like modulo but works with floats and negatives).
/// @param value [Float] The value to wrap
/// @param min [Float] Range minimum
/// @param max [Float] Range maximum
/// @returns [Float] Wrapped value
/// @example Mathf.wrap(370, 0, 360)  # => 10.0
/// @example Mathf.wrap(-10, 0, 360)  # => 350.0
static mrb_value mrb_math_wrap(mrb_state* mrb, mrb_value) {
    mrb_float value, min_val, max_val;
    mrb_get_args(mrb, "fff", &value, &min_val, &max_val);

    double range = max_val - min_val;
    if (range == 0.0) return mrb_float_value(mrb, min_val);

    double result = fmod(value - min_val, range);
    if (result < 0) result += range;
    return mrb_float_value(mrb, result + min_val);
}

/// @method deg_to_rad
/// @description Convert degrees to radians.
/// @param degrees [Float] Angle in degrees
/// @returns [Float] Angle in radians
/// @example Mathf.deg_to_rad(180)  # => ~3.14159
static mrb_value mrb_math_deg_to_rad(mrb_state* mrb, mrb_value) {
    mrb_float degrees;
    mrb_get_args(mrb, "f", &degrees);
    return mrb_float_value(mrb, degrees * M_PI / 180.0);
}

/// @method rad_to_deg
/// @description Convert radians to degrees.
/// @param radians [Float] Angle in radians
/// @returns [Float] Angle in degrees
/// @example Mathf.rad_to_deg(Math::PI)  # => 180.0
static mrb_value mrb_math_rad_to_deg(mrb_state* mrb, mrb_value) {
    mrb_float radians;
    mrb_get_args(mrb, "f", &radians);
    return mrb_float_value(mrb, radians * 180.0 / M_PI);
}

/// @method random_int
/// @description Generate a random integer within an inclusive range.
/// @param min [Integer] Minimum value (inclusive)
/// @param max [Integer] Maximum value (inclusive)
/// @returns [Integer] Random integer between min and max
/// @example dice = Mathf.random_int(1, 6)
/// @example spawn_x = Mathf.random_int(0, 800)
static mrb_value mrb_math_random_int(mrb_state* mrb, mrb_value) {
    mrb_int min, max;
    mrb_get_args(mrb, "ii", &min, &max);
    return mrb_fixnum_value(GetRandomValue(static_cast<int>(min), static_cast<int>(max)));
}

/// @method random_float
/// @description Generate a random float between 0.0 and 1.0.
/// @returns [Float] Random float in range [0.0, 1.0]
/// @example chance = Mathf.random_float
///   critical_hit = chance > 0.9
/// @example # Random value in custom range
///   speed = 50 + Mathf.random_float * 100  # 50-150
static mrb_value mrb_math_random_float(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, static_cast<double>(GetRandomValue(0, RAND_MAX)) / RAND_MAX);
}

// ============================================================================
// Serialization Support (to_h methods)
// ============================================================================

/// @method to_h
/// @description Convert Vec2 to a hash for JSON serialization.
///   Includes a "_type" field for automatic deserialization.
/// @return [Hash] { "_type" => "Vec2", "x" => x, "y" => y }
/// @example vec = Vec2.new(100, 200)
///   data = vec.to_h  # { "_type" => "Vec2", "x" => 100.0, "y" => 200.0 }
///   json = GMR::JSON.stringify(data)
static mrb_value mrb_vec2_to_h(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);

    mrb_value hash = mrb_hash_new(mrb);
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "_type"), mrb_str_new_lit(mrb, "Vec2"));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "x"), mrb_float_value(mrb, data->x));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "y"), mrb_float_value(mrb, data->y));

    return hash;
}

/// @method to_h
/// @description Convert Vec3 to a hash for JSON serialization.
///   Includes a "_type" field for automatic deserialization.
/// @return [Hash] { "_type" => "Vec3", "x" => x, "y" => y, "z" => z }
/// @example vec = Vec3.new(1, 2, 3)
///   data = vec.to_h  # { "_type" => "Vec3", "x" => 1.0, "y" => 2.0, "z" => 3.0 }
static mrb_value mrb_vec3_to_h(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);

    mrb_value hash = mrb_hash_new(mrb);
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "_type"), mrb_str_new_lit(mrb, "Vec3"));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "x"), mrb_float_value(mrb, data->x));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "y"), mrb_float_value(mrb, data->y));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "z"), mrb_float_value(mrb, data->z));

    return hash;
}

/// @method to_h
/// @description Convert Rect to a hash for JSON serialization.
///   Includes a "_type" field for automatic deserialization.
/// @return [Hash] { "_type" => "Rect", "x" => x, "y" => y, "w" => w, "h" => h }
/// @example rect = Rect.new(10, 20, 100, 50)
///   data = rect.to_h  # { "_type" => "Rect", "x" => 10.0, "y" => 20.0, "w" => 100.0, "h" => 50.0 }
static mrb_value mrb_rect_to_h(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);

    mrb_value hash = mrb_hash_new(mrb);
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "_type"), mrb_str_new_lit(mrb, "Rect"));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "x"), mrb_float_value(mrb, data->x));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "y"), mrb_float_value(mrb, data->y));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "w"), mrb_float_value(mrb, data->w));
    mrb_hash_set(mrb, hash, mrb_str_new_lit(mrb, "h"), mrb_float_value(mrb, data->h));

    return hash;
}

// ============================================================================
// Registration
// ============================================================================

void register_math(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* mathf = mrb_module_get_under(mrb, gmr, "Mathf");

    // Vec2 class under GMR::Mathf module
    RClass* vec2_class = mrb_define_class_under(mrb, mathf, "Vec2", mrb->object_class);
    MRB_SET_INSTANCE_TT(vec2_class, MRB_TT_CDATA);

    mrb_define_method(mrb, vec2_class, "initialize", mrb_vec2_initialize, MRB_ARGS_OPT(2));
    mrb_define_method(mrb, vec2_class, "x", mrb_vec2_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec2_class, "y", mrb_vec2_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec2_class, "+", mrb_vec2_add, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec2_class, "-", mrb_vec2_sub, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec2_class, "*", mrb_vec2_mul, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec2_class, "/", mrb_vec2_div, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec2_class, "to_s", mrb_vec2_to_s, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec2_class, "inspect", mrb_vec2_to_s, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec2_class, "to_a", mrb_vec2_to_a, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec2_class, "to_h", mrb_vec2_to_h, MRB_ARGS_NONE());

    // Vec3 class under GMR::Mathf module
    RClass* vec3_class = mrb_define_class_under(mrb, mathf, "Vec3", mrb->object_class);
    MRB_SET_INSTANCE_TT(vec3_class, MRB_TT_CDATA);

    mrb_define_method(mrb, vec3_class, "initialize", mrb_vec3_initialize, MRB_ARGS_OPT(3));
    mrb_define_method(mrb, vec3_class, "x", mrb_vec3_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec3_class, "y", mrb_vec3_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec3_class, "z", mrb_vec3_z, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec3_class, "+", mrb_vec3_add, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec3_class, "-", mrb_vec3_sub, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec3_class, "*", mrb_vec3_mul, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec3_class, "/", mrb_vec3_div, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, vec3_class, "to_s", mrb_vec3_to_s, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec3_class, "inspect", mrb_vec3_to_s, MRB_ARGS_NONE());
    mrb_define_method(mrb, vec3_class, "to_h", mrb_vec3_to_h, MRB_ARGS_NONE());

    // Rect class under GMR::Graphics module
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
    RClass* rect_class = mrb_define_class_under(mrb, graphics, "Rect", mrb->object_class);
    MRB_SET_INSTANCE_TT(rect_class, MRB_TT_CDATA);

    mrb_define_method(mrb, rect_class, "initialize", mrb_rect_initialize, MRB_ARGS_OPT(4));
    mrb_define_method(mrb, rect_class, "x", mrb_rect_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "y", mrb_rect_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "w", mrb_rect_w, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "h", mrb_rect_h, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "to_s", mrb_rect_to_s, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "inspect", mrb_rect_to_s, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "to_h", mrb_rect_to_h, MRB_ARGS_NONE());

    // GMR::Mathf module (named to avoid conflict with Ruby's Math)
    RClass* math_module = get_gmr_submodule(mrb, "Mathf");

    // Interpolation
    mrb_define_module_function(mrb, math_module, "lerp", mrb_math_lerp, MRB_ARGS_REQ(3));
    mrb_define_module_function(mrb, math_module, "inverse_lerp", mrb_math_inverse_lerp, MRB_ARGS_REQ(3));
    mrb_define_module_function(mrb, math_module, "smoothstep", mrb_math_smoothstep, MRB_ARGS_REQ(3));
    mrb_define_module_function(mrb, math_module, "remap", mrb_math_remap, MRB_ARGS_REQ(5));

    // Clamping and bounds
    mrb_define_module_function(mrb, math_module, "clamp", mrb_math_clamp, MRB_ARGS_REQ(3));
    mrb_define_module_function(mrb, math_module, "wrap", mrb_math_wrap, MRB_ARGS_REQ(3));

    // Distance
    mrb_define_module_function(mrb, math_module, "distance", mrb_math_distance, MRB_ARGS_REQ(4));
    mrb_define_module_function(mrb, math_module, "distance_squared", mrb_math_distance_squared, MRB_ARGS_REQ(4));

    // Movement and direction
    mrb_define_module_function(mrb, math_module, "sign", mrb_math_sign, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, math_module, "move_toward", mrb_math_move_toward, MRB_ARGS_REQ(3));

    // Angle conversion
    mrb_define_module_function(mrb, math_module, "deg_to_rad", mrb_math_deg_to_rad, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, math_module, "rad_to_deg", mrb_math_rad_to_deg, MRB_ARGS_REQ(1));

    // Random numbers
    mrb_define_module_function(mrb, math_module, "random_int", mrb_math_random_int, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, math_module, "random_float", mrb_math_random_float, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
