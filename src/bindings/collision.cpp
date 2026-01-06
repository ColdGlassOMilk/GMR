#include "gmr/bindings/collision.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include <cmath>
#include <algorithm>

namespace gmr {
namespace bindings {

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
    RClass* collision = get_gmr_submodule(mrb, "Collision");

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

    // Distance helpers
    mrb_define_module_function(mrb, collision, "distance", mrb_collision_distance, MRB_ARGS_REQ(4));
    mrb_define_module_function(mrb, collision, "distance_squared", mrb_collision_distance_squared, MRB_ARGS_REQ(4));
}

} // namespace bindings
} // namespace gmr
