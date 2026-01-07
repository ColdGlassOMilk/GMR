#include "gmr/bindings/collision.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <cmath>
#include <algorithm>

namespace gmr {
namespace bindings {

// ============================================================================
// CollisionResult Class (Ruby object instead of hash - per CONTRIBUTING.md)
// ============================================================================

/// @class GMR::CollisionResult
/// @description Result from tilemap collision resolution containing resolved position,
///   velocity, and collision flags.
/// @example
///   result = GMR::Collision.tilemap_resolve(@tilemap, x, y, w, h, vx, vy)
///   @sprite.x = result.x
///   @sprite.y = result.y
///   @vx = result.vx
///   @vy = result.vy
///   @on_ground = result.grounded?  # or result.bottom?

// Internal data structure for CollisionResult
struct CollisionResultData {
    float x, y;
    float vx, vy;
    bool hit_left, hit_right, hit_top, hit_bottom;
};

static void collision_result_free(mrb_state*, void* ptr) {
    delete static_cast<CollisionResultData*>(ptr);
}

static const mrb_data_type collision_result_data_type = {
    "CollisionResult", collision_result_free
};

// Cache class pointer for efficiency
static RClass* collision_result_class_ptr = nullptr;

static CollisionResultData* get_collision_result_data(mrb_state* mrb, mrb_value self) {
    return static_cast<CollisionResultData*>(mrb_data_get_ptr(mrb, self, &collision_result_data_type));
}

// Create a CollisionResult Ruby object from C++ CollisionResult
static mrb_value create_collision_result(mrb_state* mrb, const CollisionResult& result) {
    if (!collision_result_class_ptr) {
        RClass* gmr = get_gmr_module(mrb);
        collision_result_class_ptr = mrb_class_get_under(mrb, gmr, "CollisionResult");
    }

    auto* data = new CollisionResultData{
        result.x, result.y,
        result.vx, result.vy,
        result.hit_left, result.hit_right, result.hit_top, result.hit_bottom
    };

    mrb_value obj = mrb_obj_value(mrb_data_object_alloc(mrb, collision_result_class_ptr, data, &collision_result_data_type));
    return obj;
}

// Create a default CollisionResult (no collision, original position)
static mrb_value create_default_collision_result(mrb_state* mrb, float x, float y, float vx, float vy) {
    if (!collision_result_class_ptr) {
        RClass* gmr = get_gmr_module(mrb);
        collision_result_class_ptr = mrb_class_get_under(mrb, gmr, "CollisionResult");
    }

    auto* data = new CollisionResultData{
        x, y, vx, vy,
        false, false, false, false
    };

    mrb_value obj = mrb_obj_value(mrb_data_object_alloc(mrb, collision_result_class_ptr, data, &collision_result_data_type));
    return obj;
}

/// @method x
/// @description Get the resolved X position after collision.
/// @returns [Float] Resolved X position
static mrb_value mrb_collision_result_x(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_float_value(mrb, data->x);
}

/// @method y
/// @description Get the resolved Y position after collision.
/// @returns [Float] Resolved Y position
static mrb_value mrb_collision_result_y(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_float_value(mrb, data->y);
}

/// @method vx
/// @description Get the resolved X velocity after collision (zeroed if hit wall).
/// @returns [Float] Resolved X velocity
static mrb_value mrb_collision_result_vx(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_float_value(mrb, data->vx);
}

/// @method vy
/// @description Get the resolved Y velocity after collision (zeroed if hit floor/ceiling).
/// @returns [Float] Resolved Y velocity
static mrb_value mrb_collision_result_vy(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_float_value(mrb, data->vy);
}

/// @method left?
/// @description Check if collided with a wall on the left.
/// @returns [Boolean] true if hit left wall
static mrb_value mrb_collision_result_left(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_bool_value(data->hit_left);
}

/// @method right?
/// @description Check if collided with a wall on the right.
/// @returns [Boolean] true if hit right wall
static mrb_value mrb_collision_result_right(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_bool_value(data->hit_right);
}

/// @method top?
/// @description Check if collided with a ceiling.
/// @returns [Boolean] true if hit ceiling
static mrb_value mrb_collision_result_top(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_bool_value(data->hit_top);
}

/// @method bottom?
/// @description Check if collided with the ground (landed).
/// @returns [Boolean] true if hit ground
static mrb_value mrb_collision_result_bottom(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_bool_value(data->hit_bottom);
}

/// @method grounded?
/// @description Alias for bottom? - Check if on the ground.
/// @returns [Boolean] true if on ground
static mrb_value mrb_collision_result_grounded(mrb_state* mrb, mrb_value self) {
    return mrb_collision_result_bottom(mrb, self);
}

/// @method hit_horizontal?
/// @description Check if collided horizontally (left or right wall).
/// @returns [Boolean] true if hit any horizontal surface
static mrb_value mrb_collision_result_hit_horizontal(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_bool_value(data->hit_left || data->hit_right);
}

/// @method hit_vertical?
/// @description Check if collided vertically (floor or ceiling).
/// @returns [Boolean] true if hit any vertical surface
static mrb_value mrb_collision_result_hit_vertical(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_bool_value(data->hit_top || data->hit_bottom);
}

/// @method any?
/// @description Check if any collision occurred.
/// @returns [Boolean] true if any collision happened
static mrb_value mrb_collision_result_any(mrb_state* mrb, mrb_value self) {
    CollisionResultData* data = get_collision_result_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid CollisionResult");
        return mrb_nil_value();
    }
    return mrb_bool_value(data->hit_left || data->hit_right || data->hit_top || data->hit_bottom);
}

/// @module GMR::Collision
/// @description Geometry collision detection utilities. Provides fast tests for
///   point, rectangle, and circle collisions, as well as tile-based collision helpers.
/// @example # Complete collision system for platformer
///   class CollisionSystem
///     def check_player_enemies(player, enemies)
///       player_bounds = player.bounds
///       enemies.each do |enemy|
///         next unless enemy.alive?
///         if GMR::Collision.rect_rect?(player_bounds, enemy.bounds)
///           if player.attacking? && player.attack_bounds
///             enemy.take_damage(player.attack_power)
///           elsif !player.invincible?
///             player.take_damage(enemy.damage)
///           end
///         end
///       end
///     end
///
///     def check_collectibles(player, items)
///       items.reject! do |item|
///         if GMR::Collision.rect_rect?(player.bounds, item.bounds)
///           item.collect(player)
///           true
///         else
///           false
///         end
///       end
///     end
///   end
/// @example # Projectile collision with circle hitbox
///   class Projectile
///     def update(dt)
///       @x += @vx * dt
///       @y += @vy * dt
///
///       @enemies.each do |enemy|
///         if GMR::Collision.circle_rect?(@x, @y, @radius, enemy.bounds)
///           enemy.take_damage(@damage)
///           @alive = false
///           spawn_hit_effect(@x, @y)
///           break
///         end
///       end
///     end
///   end
/// @example # Mouse hover detection for UI buttons
///   class Button
///     def update(dt)
///       mx = GMR::Input.mouse_x
///       my = GMR::Input.mouse_y
///       @hovered = GMR::Collision.point_in_rect?(mx, my, @x, @y, @width, @height)
///
///       if @hovered && GMR::Input.mouse_pressed?(:left)
///         @callback.call
///         GMR::Audio::Sound.play("assets/click.wav")
///       end
///     end
///
///     def draw
///       color = @hovered ? [100, 150, 255] : [80, 80, 120]
///       GMR::Graphics.draw_rect(@x, @y, @width, @height, color)
///       GMR::Graphics.draw_text(@label, @x + 10, @y + 8, 20, [255, 255, 255])
///     end
///   end

// ============================================================================
// Point Tests
// ============================================================================

/// @function point_in_rect?
/// @description Check if a point is inside a rectangle.
/// @param px [Float] Point X coordinate
/// @param py [Float] Point Y coordinate
/// @param rx [Float] Rectangle X position (top-left)
/// @param ry [Float] Rectangle Y position (top-left)
/// @param rw [Float] Rectangle width
/// @param rh [Float] Rectangle height
/// @returns [Boolean] true if the point is inside the rectangle
/// @example if GMR::Collision.point_in_rect?(mouse_x, mouse_y, btn.x, btn.y, btn.w, btn.h)
///   button_hovered = true
/// end
static mrb_value mrb_collision_point_in_rect(mrb_state* mrb, mrb_value) {
    mrb_float px, py, rx, ry, rw, rh;
    mrb_get_args(mrb, "ffffff", &px, &py, &rx, &ry, &rw, &rh);
    bool result = px >= rx && px < rx + rw && py >= ry && py < ry + rh;
    return to_mrb_bool(mrb, result);
}

/// @function point_in_circle?
/// @description Check if a point is inside a circle.
/// @param px [Float] Point X coordinate
/// @param py [Float] Point Y coordinate
/// @param cx [Float] Circle center X coordinate
/// @param cy [Float] Circle center Y coordinate
/// @param radius [Float] Circle radius
/// @returns [Boolean] true if the point is inside the circle
/// @example if GMR::Collision.point_in_circle?(x, y, orb.x, orb.y, orb.radius)
///   orb.collect
/// end
static mrb_value mrb_collision_point_in_circle(mrb_state* mrb, mrb_value) {
    mrb_float px, py, cx, cy, radius;
    mrb_get_args(mrb, "fffff", &px, &py, &cx, &cy, &radius);
    float dx = static_cast<float>(px - cx);
    float dy = static_cast<float>(py - cy);
    bool result = dx * dx + dy * dy <= radius * radius;
    return to_mrb_bool(mrb, result);
}

// ============================================================================
// Rectangle Tests
// ============================================================================

/// @function rect_overlap?
/// @description Check if two rectangles overlap (AABB collision).
/// @param x1 [Float] First rectangle X position
/// @param y1 [Float] First rectangle Y position
/// @param w1 [Float] First rectangle width
/// @param h1 [Float] First rectangle height
/// @param x2 [Float] Second rectangle X position
/// @param y2 [Float] Second rectangle Y position
/// @param w2 [Float] Second rectangle width
/// @param h2 [Float] Second rectangle height
/// @returns [Boolean] true if the rectangles overlap
/// @example if GMR::Collision.rect_overlap?(player.x, player.y, 32, 48,
///                                          platform.x, platform.y, 64, 16)
///   player.on_ground = true
/// end
static mrb_value mrb_collision_rect_overlap(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, w1, h1, x2, y2, w2, h2;
    mrb_get_args(mrb, "ffffffff", &x1, &y1, &w1, &h1, &x2, &y2, &w2, &h2);
    bool result = x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2;
    return to_mrb_bool(mrb, result);
}

/// @function rect_contains?
/// @description Check if the outer rectangle fully contains the inner rectangle.
/// @param outer_x [Float] Outer rectangle X position
/// @param outer_y [Float] Outer rectangle Y position
/// @param outer_w [Float] Outer rectangle width
/// @param outer_h [Float] Outer rectangle height
/// @param inner_x [Float] Inner rectangle X position
/// @param inner_y [Float] Inner rectangle Y position
/// @param inner_w [Float] Inner rectangle width
/// @param inner_h [Float] Inner rectangle height
/// @returns [Boolean] true if the inner rectangle is fully inside the outer rectangle
/// @example if GMR::Collision.rect_contains?(screen_x, screen_y, screen_w, screen_h,
///                                           entity.x, entity.y, entity.w, entity.h)
///   entity.draw  # Only draw if fully on screen
/// end
static mrb_value mrb_collision_rect_contains(mrb_state* mrb, mrb_value) {
    mrb_float ox, oy, ow, oh, ix, iy, iw, ih;
    mrb_get_args(mrb, "ffffffff", &ox, &oy, &ow, &oh, &ix, &iy, &iw, &ih);
    bool result = ix >= ox && iy >= oy && ix + iw <= ox + ow && iy + ih <= oy + oh;
    return to_mrb_bool(mrb, result);
}

// ============================================================================
// Circle Tests
// ============================================================================

/// @function circle_overlap?
/// @description Check if two circles overlap.
/// @param x1 [Float] First circle center X
/// @param y1 [Float] First circle center Y
/// @param r1 [Float] First circle radius
/// @param x2 [Float] Second circle center X
/// @param y2 [Float] Second circle center Y
/// @param r2 [Float] Second circle radius
/// @returns [Boolean] true if the circles overlap
/// @example if GMR::Collision.circle_overlap?(ball1.x, ball1.y, ball1.r,
///                                            ball2.x, ball2.y, ball2.r)
///   bounce_balls(ball1, ball2)
/// end
static mrb_value mrb_collision_circle_overlap(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, r1, x2, y2, r2;
    mrb_get_args(mrb, "ffffff", &x1, &y1, &r1, &x2, &y2, &r2);
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    float radius_sum = static_cast<float>(r1 + r2);
    bool result = dx * dx + dy * dy <= radius_sum * radius_sum;
    return to_mrb_bool(mrb, result);
}

/// @function circle_rect_overlap?
/// @description Check if a circle overlaps with a rectangle.
/// @param cx [Float] Circle center X
/// @param cy [Float] Circle center Y
/// @param cr [Float] Circle radius
/// @param rx [Float] Rectangle X position
/// @param ry [Float] Rectangle Y position
/// @param rw [Float] Rectangle width
/// @param rh [Float] Rectangle height
/// @returns [Boolean] true if the circle and rectangle overlap
/// @example if GMR::Collision.circle_rect_overlap?(ball.x, ball.y, ball.r,
///                                                  wall.x, wall.y, wall.w, wall.h)
///   ball.bounce
/// end
static mrb_value mrb_collision_circle_rect_overlap(mrb_state* mrb, mrb_value) {
    mrb_float cx, cy, cr, rx, ry, rw, rh;
    mrb_get_args(mrb, "fffffff", &cx, &cy, &cr, &rx, &ry, &rw, &rh);

    // Find closest point on rect to circle center
    float closest_x = std::max(static_cast<float>(rx),
                               std::min(static_cast<float>(cx), static_cast<float>(rx + rw)));
    float closest_y = std::max(static_cast<float>(ry),
                               std::min(static_cast<float>(cy), static_cast<float>(ry + rh)));

    float dx = static_cast<float>(cx) - closest_x;
    float dy = static_cast<float>(cy) - closest_y;
    bool result = dx * dx + dy * dy <= cr * cr;
    return to_mrb_bool(mrb, result);
}

// ============================================================================
// Tile Helpers
// ============================================================================

/// @function rect_tiles
/// @description Get all tile coordinates that a rectangle overlaps. Useful for
///   tile-based collision detection.
/// @param x [Float] Rectangle X position
/// @param y [Float] Rectangle Y position
/// @param w [Float] Rectangle width
/// @param h [Float] Rectangle height
/// @param tile_size [Integer] Size of each tile in pixels
/// @returns [Array<Array<Integer>>] Array of [tx, ty] tile coordinate pairs
/// @example tiles = GMR::Collision.rect_tiles(player.x, player.y, 32, 48, 16)
///   tiles.each do |tx, ty|
///     if tilemap.solid?(tx, ty)
///       # Handle collision with this tile
///     end
///   end
static mrb_value mrb_collision_rect_tiles(mrb_state* mrb, mrb_value) {
    mrb_float x, y, w, h;
    mrb_int tile_size;
    mrb_get_args(mrb, "ffffi", &x, &y, &w, &h, &tile_size);

    if (tile_size <= 0) {
        return mrb_ary_new(mrb);
    }

    int start_tx = static_cast<int>(std::floor(x / tile_size));
    int start_ty = static_cast<int>(std::floor(y / tile_size));
    int end_tx = static_cast<int>(std::floor((x + w - 1) / tile_size));
    int end_ty = static_cast<int>(std::floor((y + h - 1) / tile_size));

    mrb_value result = mrb_ary_new(mrb);
    for (int ty = start_ty; ty <= end_ty; ty++) {
        for (int tx = start_tx; tx <= end_tx; tx++) {
            mrb_value pair = mrb_ary_new_capa(mrb, 2);
            mrb_ary_push(mrb, pair, mrb_fixnum_value(tx));
            mrb_ary_push(mrb, pair, mrb_fixnum_value(ty));
            mrb_ary_push(mrb, result, pair);
        }
    }
    return result;
}

/// @function tile_rect
/// @description Convert tile coordinates to a world-space rectangle.
/// @param tx [Integer] Tile X coordinate
/// @param ty [Integer] Tile Y coordinate
/// @param tile_size [Integer] Size of each tile in pixels
/// @returns [Array<Integer>] Rectangle as [x, y, width, height]
/// @example x, y, w, h = GMR::Collision.tile_rect(5, 3, 16)
///   # Returns [80, 48, 16, 16]
static mrb_value mrb_collision_tile_rect(mrb_state* mrb, mrb_value) {
    mrb_int tx, ty, tile_size;
    mrb_get_args(mrb, "iii", &tx, &ty, &tile_size);

    mrb_value result = mrb_ary_new_capa(mrb, 4);
    mrb_ary_push(mrb, result, mrb_fixnum_value(tx * tile_size));
    mrb_ary_push(mrb, result, mrb_fixnum_value(ty * tile_size));
    mrb_ary_push(mrb, result, mrb_fixnum_value(tile_size));
    mrb_ary_push(mrb, result, mrb_fixnum_value(tile_size));
    return result;
}

// ============================================================================
// Tilemap Collision Resolution
// ============================================================================

/// @function tilemap_resolve
/// @description Resolve collision between a hitbox rectangle and a tilemap's solid tiles.
///   Returns a CollisionResult object with resolved position and collision flags.
///   This is the recommended way to handle character-tilemap collisions in platformers.
/// @param tilemap [Tilemap] The tilemap to check collision against
/// @param x [Float] Hitbox X position in tilemap local coordinates
/// @param y [Float] Hitbox Y position in tilemap local coordinates
/// @param w [Float] Hitbox width
/// @param h [Float] Hitbox height
/// @param vx [Float] Current X velocity (for directional checks)
/// @param vy [Float] Current Y velocity (for directional checks)
/// @returns [CollisionResult] Collision result with position, velocity, and collision flags
/// @example # In update loop:
///   local_x = @sprite.x + HITBOX_OFFSET_X - MAP_OFFSET_X
///   local_y = @sprite.y + HITBOX_OFFSET_Y - MAP_OFFSET_Y
///   result = Collision.tilemap_resolve(@tilemap, local_x, local_y, HITBOX_W, HITBOX_H, @vx, @vy)
///   @sprite.x = result.x + MAP_OFFSET_X - HITBOX_OFFSET_X
///   @sprite.y = result.y + MAP_OFFSET_Y - HITBOX_OFFSET_Y
///   @on_ground = result.grounded?
static mrb_value mrb_collision_tilemap_resolve(mrb_state* mrb, mrb_value) {
    mrb_value tilemap_obj;
    mrb_float x, y, w, h, vx, vy;
    mrb_get_args(mrb, "offffff", &tilemap_obj, &x, &y, &w, &h, &vx, &vy);

    // Get tilemap data
    TilemapData* tilemap = get_tilemap_from_value(mrb, tilemap_obj);
    if (!tilemap) {
        // Return default result (no collision, original position)
        return create_default_collision_result(mrb,
            static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(vx), static_cast<float>(vy));
    }

    // Perform collision resolution
    CollisionResult collision = gmr::collision::tilemap_resolve(
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(w), static_cast<float>(h),
        static_cast<float>(vx), static_cast<float>(vy),
        *tilemap
    );

    // Return proper CollisionResult Ruby object (per CONTRIBUTING.md - no hash returns)
    return create_collision_result(mrb, collision);
}

// ============================================================================
// Distance Helpers
// ============================================================================

/// @function distance
/// @description Calculate the Euclidean distance between two points.
/// @param x1 [Float] First point X
/// @param y1 [Float] First point Y
/// @param x2 [Float] Second point X
/// @param y2 [Float] Second point Y
/// @returns [Float] Distance between the points
/// @example dist = GMR::Collision.distance(player.x, player.y, enemy.x, enemy.y)
///   if dist < attack_range
///     attack_enemy(enemy)
///   end
static mrb_value mrb_collision_distance(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2;
    mrb_get_args(mrb, "ffff", &x1, &y1, &x2, &y2);
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    return mrb_float_value(mrb, std::sqrt(dx * dx + dy * dy));
}

/// @function distance_squared
/// @description Calculate the squared distance between two points. Faster than
///   distance() since it avoids the square root. Use for comparisons.
/// @param x1 [Float] First point X
/// @param y1 [Float] First point Y
/// @param x2 [Float] Second point X
/// @param y2 [Float] Second point Y
/// @returns [Float] Squared distance between the points
/// @example # More efficient for distance comparisons
///   dist_sq = GMR::Collision.distance_squared(a.x, a.y, b.x, b.y)
///   if dist_sq < range * range
///     in_range = true
///   end
static mrb_value mrb_collision_distance_squared(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2;
    mrb_get_args(mrb, "ffff", &x1, &y1, &x2, &y2);
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    return mrb_float_value(mrb, dx * dx + dy * dy);
}

// ============================================================================
// Registration
// ============================================================================

void register_collision(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* collision = get_gmr_submodule(mrb, "Collision");

    // CollisionResult class (per CONTRIBUTING.md - proper Ruby object instead of hash)
    collision_result_class_ptr = mrb_define_class_under(mrb, gmr, "CollisionResult", mrb->object_class);
    MRB_SET_INSTANCE_TT(collision_result_class_ptr, MRB_TT_CDATA);

    // Position getters
    mrb_define_method(mrb, collision_result_class_ptr, "x", mrb_collision_result_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "y", mrb_collision_result_y, MRB_ARGS_NONE());

    // Velocity getters
    mrb_define_method(mrb, collision_result_class_ptr, "vx", mrb_collision_result_vx, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "vy", mrb_collision_result_vy, MRB_ARGS_NONE());

    // Collision flag query methods
    mrb_define_method(mrb, collision_result_class_ptr, "left?", mrb_collision_result_left, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "right?", mrb_collision_result_right, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "top?", mrb_collision_result_top, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "bottom?", mrb_collision_result_bottom, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "grounded?", mrb_collision_result_grounded, MRB_ARGS_NONE());

    // Convenience methods
    mrb_define_method(mrb, collision_result_class_ptr, "hit_horizontal?", mrb_collision_result_hit_horizontal, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "hit_vertical?", mrb_collision_result_hit_vertical, MRB_ARGS_NONE());
    mrb_define_method(mrb, collision_result_class_ptr, "any?", mrb_collision_result_any, MRB_ARGS_NONE());

    // Point tests
    mrb_define_module_function(mrb, collision, "point_in_rect?", mrb_collision_point_in_rect, MRB_ARGS_REQ(6));
    mrb_define_module_function(mrb, collision, "point_in_circle?", mrb_collision_point_in_circle, MRB_ARGS_REQ(5));

    // Rectangle tests
    mrb_define_module_function(mrb, collision, "rect_overlap?", mrb_collision_rect_overlap, MRB_ARGS_REQ(8));
    mrb_define_module_function(mrb, collision, "rect_contains?", mrb_collision_rect_contains, MRB_ARGS_REQ(8));

    // Circle tests
    mrb_define_module_function(mrb, collision, "circle_overlap?", mrb_collision_circle_overlap, MRB_ARGS_REQ(6));
    mrb_define_module_function(mrb, collision, "circle_rect_overlap?", mrb_collision_circle_rect_overlap, MRB_ARGS_REQ(7));

    // Tile helpers
    mrb_define_module_function(mrb, collision, "rect_tiles", mrb_collision_rect_tiles, MRB_ARGS_REQ(5));
    mrb_define_module_function(mrb, collision, "tile_rect", mrb_collision_tile_rect, MRB_ARGS_REQ(3));

    // Tilemap collision resolution
    mrb_define_module_function(mrb, collision, "tilemap_resolve", mrb_collision_tilemap_resolve, MRB_ARGS_REQ(7));

    // Distance helpers
    mrb_define_module_function(mrb, collision, "distance", mrb_collision_distance, MRB_ARGS_REQ(4));
    mrb_define_module_function(mrb, collision, "distance_squared", mrb_collision_distance_squared, MRB_ARGS_REQ(4));
}

} // namespace bindings
} // namespace gmr
