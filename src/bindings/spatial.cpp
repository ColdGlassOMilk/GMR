#include "gmr/bindings/spatial.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/spatial/spatial_hash.hpp"
#include <mruby/class.h>
#include <mruby/hash.h>

namespace gmr {
namespace bindings {

// ============================================================================
// SpatialHash Module
// ============================================================================

/// @module GMR::SpatialHash
/// @description Spatial hash for efficient entity queries by position.
///   Add entities with bounds, then query by rect, circle, or point.
/// @example # Register entities
///   SpatialHash.add(@enemy, bounds: @enemy.hitbox)
///   SpatialHash.add(@player, bounds: Rect.new(@player.x, @player.y, 16, 16))
///
/// @example # Query for nearby entities
///   nearby = SpatialHash.query_circle(@player.x, @player.y, 100)
///   nearby.each { |e| e.take_damage(10) }
///
/// @example # Find nearest entity
///   target = SpatialHash.nearest(@turret.x, @turret.y, max_distance: 200)

/// @function cell_size=
/// @description Set the cell size for the spatial hash grid.
///   Larger cells mean fewer cells but more entities per cell.
///   Default is 64. Set based on typical entity size.
/// @param size [Float] The cell size in world units
/// @returns [Float] The new cell size
/// @example SpatialHash.cell_size = 32  # Smaller cells for dense areas
static mrb_value mrb_spatial_set_cell_size(mrb_state* mrb, mrb_value) {
    mrb_float size;
    mrb_get_args(mrb, "f", &size);

    if (size <= 0.0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Cell size must be positive");
        return mrb_nil_value();
    }

    spatial::SpatialHash::instance().set_cell_size(static_cast<float>(size));
    return mrb_float_value(mrb, size);
}

/// @function cell_size
/// @description Get the current cell size.
/// @returns [Float] The cell size in world units
static mrb_value mrb_spatial_get_cell_size(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, spatial::SpatialHash::instance().cell_size());
}

// Helper to parse bounds from args
static bool parse_bounds(mrb_state* mrb, mrb_value opts, float& x, float& y, float& w, float& h) {
    if (mrb_nil_p(opts)) return false;

    mrb_value bounds = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "bounds")));

    if (mrb_nil_p(bounds)) {
        // Try individual x, y, w, h
        mrb_value x_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "x")));
        mrb_value y_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "y")));
        mrb_value w_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "w")));
        mrb_value h_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "h")));

        if (mrb_nil_p(x_val) || mrb_nil_p(y_val) || mrb_nil_p(w_val) || mrb_nil_p(h_val)) {
            return false;
        }

        x = static_cast<float>(mrb_as_float(mrb, x_val));
        y = static_cast<float>(mrb_as_float(mrb, y_val));
        w = static_cast<float>(mrb_as_float(mrb, w_val));
        h = static_cast<float>(mrb_as_float(mrb, h_val));
        return true;
    }

    // bounds: Rect object or array [x, y, w, h]
    if (mrb_array_p(bounds)) {
        if (RARRAY_LEN(bounds) < 4) return false;
        x = static_cast<float>(mrb_as_float(mrb, mrb_ary_ref(mrb, bounds, 0)));
        y = static_cast<float>(mrb_as_float(mrb, mrb_ary_ref(mrb, bounds, 1)));
        w = static_cast<float>(mrb_as_float(mrb, mrb_ary_ref(mrb, bounds, 2)));
        h = static_cast<float>(mrb_as_float(mrb, mrb_ary_ref(mrb, bounds, 3)));
        return true;
    }

    // Try to call .x, .y, .width/.w, .height/.h on the bounds object
    if (mrb_respond_to(mrb, bounds, mrb_intern_cstr(mrb, "x")) &&
        mrb_respond_to(mrb, bounds, mrb_intern_cstr(mrb, "y"))) {

        x = static_cast<float>(mrb_as_float(mrb, mrb_funcall(mrb, bounds, "x", 0)));
        y = static_cast<float>(mrb_as_float(mrb, mrb_funcall(mrb, bounds, "y", 0)));

        if (mrb_respond_to(mrb, bounds, mrb_intern_cstr(mrb, "width"))) {
            w = static_cast<float>(mrb_as_float(mrb, mrb_funcall(mrb, bounds, "width", 0)));
        } else if (mrb_respond_to(mrb, bounds, mrb_intern_cstr(mrb, "w"))) {
            w = static_cast<float>(mrb_as_float(mrb, mrb_funcall(mrb, bounds, "w", 0)));
        } else {
            return false;
        }

        if (mrb_respond_to(mrb, bounds, mrb_intern_cstr(mrb, "height"))) {
            h = static_cast<float>(mrb_as_float(mrb, mrb_funcall(mrb, bounds, "height", 0)));
        } else if (mrb_respond_to(mrb, bounds, mrb_intern_cstr(mrb, "h"))) {
            h = static_cast<float>(mrb_as_float(mrb, mrb_funcall(mrb, bounds, "h", 0)));
        } else {
            return false;
        }

        return true;
    }

    return false;
}

/// @function add
/// @description Add an entity to the spatial hash with its bounds.
/// @param entity [Object] The entity to add
/// @param bounds [Rect, Array, Hash] The bounding box (x, y, w, h)
/// @returns [Object] The entity
/// @example SpatialHash.add(@enemy, bounds: @enemy.hitbox)
/// @example SpatialHash.add(@player, bounds: [x, y, 16, 16])
/// @example SpatialHash.add(@item, x: 100, y: 200, w: 8, h: 8)
static mrb_value mrb_spatial_add(mrb_state* mrb, mrb_value) {
    mrb_value entity;
    mrb_value opts;
    mrb_get_args(mrb, "oH", &entity, &opts);

    float x, y, w, h;
    if (!parse_bounds(mrb, opts, x, y, w, h)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "SpatialHash.add requires bounds (Rect, [x,y,w,h], or x:,y:,w:,h:)");
        return mrb_nil_value();
    }

    spatial::SpatialHash::instance().add(mrb, entity, x, y, w, h);
    return entity;
}

/// @function update
/// @description Update an entity's position in the spatial hash.
///   Call this when the entity moves.
/// @param entity [Object] The entity to update
/// @param bounds [Rect, Array, Hash] The new bounding box
/// @returns [Object] The entity
/// @example SpatialHash.update(@enemy, bounds: @enemy.hitbox)
static mrb_value mrb_spatial_update(mrb_state* mrb, mrb_value) {
    mrb_value entity;
    mrb_value opts;
    mrb_get_args(mrb, "oH", &entity, &opts);

    float x, y, w, h;
    if (!parse_bounds(mrb, opts, x, y, w, h)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "SpatialHash.update requires bounds");
        return mrb_nil_value();
    }

    spatial::SpatialHash::instance().update(mrb, entity, x, y, w, h);
    return entity;
}

/// @function remove
/// @description Remove an entity from the spatial hash.
/// @param entity [Object] The entity to remove
/// @returns [nil]
/// @example SpatialHash.remove(@enemy)
static mrb_value mrb_spatial_remove(mrb_state* mrb, mrb_value) {
    mrb_value entity;
    mrb_get_args(mrb, "o", &entity);

    spatial::SpatialHash::instance().remove(mrb, entity);
    return mrb_nil_value();
}

/// @function query_rect
/// @description Find all entities overlapping a rectangle.
/// @param x [Float] Rectangle left edge
/// @param y [Float] Rectangle top edge
/// @param w [Float] Rectangle width
/// @param h [Float] Rectangle height
/// @returns [Array] Array of entities in the rectangle
/// @example entities = SpatialHash.query_rect(100, 100, 50, 50)
static mrb_value mrb_spatial_query_rect(mrb_state* mrb, mrb_value) {
    mrb_float x, y, w, h;
    mrb_get_args(mrb, "ffff", &x, &y, &w, &h);

    return spatial::SpatialHash::instance().query_rect(mrb,
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(w), static_cast<float>(h));
}

/// @function query_circle
/// @description Find all entities overlapping a circle.
/// @param x [Float] Circle center X
/// @param y [Float] Circle center Y
/// @param radius [Float] Circle radius
/// @returns [Array] Array of entities in the circle
/// @example nearby = SpatialHash.query_circle(@player.x, @player.y, 100)
static mrb_value mrb_spatial_query_circle(mrb_state* mrb, mrb_value) {
    mrb_float x, y, radius;
    mrb_get_args(mrb, "fff", &x, &y, &radius);

    return spatial::SpatialHash::instance().query_circle(mrb,
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(radius));
}

/// @function query_point
/// @description Find all entities containing a point.
/// @param x [Float] Point X
/// @param y [Float] Point Y
/// @returns [Array] Array of entities at the point
/// @example clicked = SpatialHash.query_point(mouse_x, mouse_y)
static mrb_value mrb_spatial_query_point(mrb_state* mrb, mrb_value) {
    mrb_float x, y;
    mrb_get_args(mrb, "ff", &x, &y);

    return spatial::SpatialHash::instance().query_point(mrb,
        static_cast<float>(x), static_cast<float>(y));
}

/// @function nearest
/// @description Find the nearest entity to a point.
/// @param x [Float] Point X
/// @param y [Float] Point Y
/// @param max_distance [Float] Maximum search distance (default: 1000)
/// @returns [Object, nil] The nearest entity, or nil if none in range
/// @example target = SpatialHash.nearest(@turret.x, @turret.y, max_distance: 200)
static mrb_value mrb_spatial_nearest(mrb_state* mrb, mrb_value) {
    mrb_float x, y;
    mrb_value opts = mrb_nil_value();
    mrb_get_args(mrb, "ff|H", &x, &y, &opts);

    float max_distance = 1000.0f;
    if (!mrb_nil_p(opts)) {
        mrb_value dist_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "max_distance")));
        if (!mrb_nil_p(dist_val)) {
            max_distance = static_cast<float>(mrb_as_float(mrb, dist_val));
        }
    }

    return spatial::SpatialHash::instance().query_nearest(mrb,
        static_cast<float>(x), static_cast<float>(y), max_distance);
}

/// @function clear
/// @description Remove all entities from the spatial hash.
/// @returns [nil]
/// @example SpatialHash.clear  # On scene change
static mrb_value mrb_spatial_clear(mrb_state* mrb, mrb_value) {
    spatial::SpatialHash::instance().clear(mrb);
    return mrb_nil_value();
}

/// @function count
/// @description Get the number of entities in the spatial hash.
/// @returns [Integer] Number of entities
/// @example puts "Entities: #{SpatialHash.count}"
static mrb_value mrb_spatial_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(static_cast<mrb_int>(spatial::SpatialHash::instance().entity_count()));
}

// ============================================================================
// Registration
// ============================================================================

void register_spatial(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // Define SpatialHash module
    RClass* spatial = mrb_define_module_under(mrb, gmr, "SpatialHash");

    // Configuration
    mrb_define_module_function(mrb, spatial, "cell_size", mrb_spatial_get_cell_size, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, spatial, "cell_size=", mrb_spatial_set_cell_size, MRB_ARGS_REQ(1));

    // Entity management
    mrb_define_module_function(mrb, spatial, "add", mrb_spatial_add, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, spatial, "update", mrb_spatial_update, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, spatial, "remove", mrb_spatial_remove, MRB_ARGS_REQ(1));

    // Queries
    mrb_define_module_function(mrb, spatial, "query_rect", mrb_spatial_query_rect, MRB_ARGS_REQ(4));
    mrb_define_module_function(mrb, spatial, "query_circle", mrb_spatial_query_circle, MRB_ARGS_REQ(3));
    mrb_define_module_function(mrb, spatial, "query_point", mrb_spatial_query_point, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, spatial, "nearest", mrb_spatial_nearest, MRB_ARGS_REQ(2) | MRB_ARGS_OPT(1));

    // Utility
    mrb_define_module_function(mrb, spatial, "clear", mrb_spatial_clear, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, spatial, "count", mrb_spatial_count, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
