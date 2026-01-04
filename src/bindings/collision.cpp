#include "gmr/bindings/collision.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include <cmath>
#include <algorithm>

namespace gmr {
namespace bindings {

// ============================================================================
// Point Tests
// ============================================================================

// GMR::Collision.point_in_rect?(px, py, rx, ry, rw, rh)
static mrb_value mrb_collision_point_in_rect(mrb_state* mrb, mrb_value) {
    mrb_float px, py, rx, ry, rw, rh;
    mrb_get_args(mrb, "ffffff", &px, &py, &rx, &ry, &rw, &rh);
    bool result = px >= rx && px < rx + rw && py >= ry && py < ry + rh;
    return to_mrb_bool(mrb, result);
}

// GMR::Collision.point_in_circle?(px, py, cx, cy, radius)
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

// GMR::Collision.rect_overlap?(x1, y1, w1, h1, x2, y2, w2, h2)
static mrb_value mrb_collision_rect_overlap(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, w1, h1, x2, y2, w2, h2;
    mrb_get_args(mrb, "ffffffff", &x1, &y1, &w1, &h1, &x2, &y2, &w2, &h2);
    bool result = x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2;
    return to_mrb_bool(mrb, result);
}

// GMR::Collision.rect_contains?(outer_x, outer_y, outer_w, outer_h, inner_x, inner_y, inner_w, inner_h)
static mrb_value mrb_collision_rect_contains(mrb_state* mrb, mrb_value) {
    mrb_float ox, oy, ow, oh, ix, iy, iw, ih;
    mrb_get_args(mrb, "ffffffff", &ox, &oy, &ow, &oh, &ix, &iy, &iw, &ih);
    bool result = ix >= ox && iy >= oy && ix + iw <= ox + ow && iy + ih <= oy + oh;
    return to_mrb_bool(mrb, result);
}

// ============================================================================
// Circle Tests
// ============================================================================

// GMR::Collision.circle_overlap?(x1, y1, r1, x2, y2, r2)
static mrb_value mrb_collision_circle_overlap(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, r1, x2, y2, r2;
    mrb_get_args(mrb, "ffffff", &x1, &y1, &r1, &x2, &y2, &r2);
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    float radius_sum = static_cast<float>(r1 + r2);
    bool result = dx * dx + dy * dy <= radius_sum * radius_sum;
    return to_mrb_bool(mrb, result);
}

// GMR::Collision.circle_rect_overlap?(cx, cy, cr, rx, ry, rw, rh)
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

// GMR::Collision.rect_tiles(x, y, w, h, tile_size) - Returns array of [tx, ty] pairs
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

// GMR::Collision.tile_rect(tx, ty, tile_size) - Returns [x, y, w, h]
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

// GMR::Collision.distance(x1, y1, x2, y2)
static mrb_value mrb_collision_distance(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2;
    mrb_get_args(mrb, "ffff", &x1, &y1, &x2, &y2);
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    return mrb_float_value(mrb, std::sqrt(dx * dx + dy * dy));
}

// GMR::Collision.distance_squared(x1, y1, x2, y2)
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
