#include "gmr/bindings/transform.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/transform.hpp"
#include "gmr/types.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <cstring>
#include <cmath>
#include <raylib.h>

// Error handling macros for fail-loud philosophy (per CONTRIBUTING.md)
#define GMR_REQUIRE_TRANSFORM_DATA(data) \
    if (!data) { \
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Transform2D: internal data is null"); \
        return mrb_nil_value(); \
    }

#define GMR_REQUIRE_TRANSFORM_STATE(state, handle) \
    if (!state) { \
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Invalid Transform2D handle %d: transform may have been destroyed", handle); \
        return mrb_nil_value(); \
    }

namespace gmr {
namespace bindings {

/// @class Transform2D
/// @description A 2D transformation with position, rotation, scale, and origin (pivot point).
///   Transforms can be parented to other transforms to create hierarchies - when a parent
///   transforms, all children transform with it. Useful for turrets, attached weapons,
///   or any object that should move relative to another.
/// @example # Rotating collectible with bobbing animation
///   class Collectible
///     def initialize(x, y)
///       @transform = Transform2D.new(x: x, y: y, origin_x: 16, origin_y: 16)
///       @base_y = y
///       @time = 0
///       @texture = GMR::Graphics::Texture.load("assets/coin.png")
///       @sprite = Sprite.new(@texture, @transform)
///     end
///
///     def update(dt)
///       @time += dt
///       # Continuous rotation
///       @transform.rotation += 90 * dt  # 90 degrees per second
///       # Bobbing motion
///       @transform.y = @base_y + Math.sin(@time * 3) * 4
///     end
///
///     def draw
///       @sprite.draw
///     end
///   end
/// @example # Tank with rotating turret using parent hierarchy
///   class Tank
///     def initialize(x, y)
///       @body = Transform2D.new(x: x, y: y, origin_x: 32, origin_y: 32)
///
///       @turret = Transform2D.new(y: -8, origin_x: 8, origin_y: 16)
///       @turret.parent = @body  # Turret follows body
///
///       body_tex = GMR::Graphics::Texture.load("assets/tank_body.png")
///       turret_tex = GMR::Graphics::Texture.load("assets/tank_turret.png")
///       @body_sprite = Sprite.new(body_tex, @body)
///       @turret_sprite = Sprite.new(turret_tex, @turret)
///     end
///
///     def update(dt)
///       # Drive forward in direction facing
///       if GMR::Input.key_down?(:w)
///         angle_rad = @body.rotation * Math::PI / 180
///         @body.x += Math.cos(angle_rad) * 100 * dt
///         @body.y += Math.sin(angle_rad) * 100 * dt
///       end
///
///       # Rotate tank body
///       @body.rotation -= 60 * dt if GMR::Input.key_down?(:a)
///       @body.rotation += 60 * dt if GMR::Input.key_down?(:d)
///
///       # Aim turret at mouse
///       aim_turret_at_mouse
///     end
///
///     def aim_turret_at_mouse
///       mx, my = GMR::Input.mouse_x, GMR::Input.mouse_y
///       turret_world = @turret.world_position
///       dx = mx - turret_world.x
///       dy = my - turret_world.y
///       target_angle = Math.atan2(dy, dx) * 180 / Math::PI
///       # Turret rotation is relative to body, so subtract body rotation
///       @turret.rotation = target_angle - @body.rotation
///     end
///
///     def draw
///       @body_sprite.draw
///       @turret_sprite.draw  # Automatically uses world transform from hierarchy
///     end
///   end
/// @example # Scaling effects for damage feedback
///   class Enemy
///     def initialize(x, y)
///       @transform = Transform2D.new(x: x, y: y, origin_x: 16, origin_y: 16)
///       enemy_tex = GMR::Graphics::Texture.load("assets/enemy.png")
///       @sprite = Sprite.new(enemy_tex, @transform)
///     end
///
///     def take_damage(amount)
///       @health -= amount
///       # Scale up briefly then back to normal (squash and stretch)
///       GMR::Tween.to(@transform, :scale_x, 1.3, duration: 0.05, ease: :out_quad)
///         .on_complete do
///           GMR::Tween.to(@transform, :scale_x, 1.0, duration: 0.1, ease: :out_elastic)
///         end
///       GMR::Tween.to(@transform, :scale_y, 0.8, duration: 0.05, ease: :out_quad)
///         .on_complete do
///           GMR::Tween.to(@transform, :scale_y, 1.0, duration: 0.1, ease: :out_elastic)
///         end
///     end
///
///     def die
///       # Spin and shrink death animation
///       GMR::Tween.to(@transform, :rotation, @transform.rotation + 720, duration: 0.5)
///       GMR::Tween.to(@transform, :scale_x, 0, duration: 0.5, ease: :in_back)
///       GMR::Tween.to(@transform, :scale_y, 0, duration: 0.5, ease: :in_back)
///         .on_complete { destroy }
///     end
///
///     def draw
///       @sprite.x = @transform.x
///       @sprite.y = @transform.y
///       @sprite.rotation = @transform.rotation
///       @sprite.scale_x = @transform.scale_x
///       @sprite.scale_y = @transform.scale_y
///       @sprite.origin_x = @transform.origin_x
///       @sprite.origin_y = @transform.origin_y
///       @sprite.draw
///     end
///   end

// Degrees <-> Radians conversion
static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;

// Transform2D Binding Data
struct TransformData {
    TransformHandle handle;
};

// Non-static to allow transform_data_type to have external linkage
void transform_free(mrb_state* mrb, void* ptr) {
    TransformData* data = static_cast<TransformData*>(ptr);
    if (data) {
        TransformManager::instance().destroy(data->handle);
        mrb_free(mrb, data);
    }
}

// Exported for use by sprite.cpp (parent transform lookup)
// extern required because const objects have internal linkage by default in C++
extern const mrb_data_type transform_data_type;
const mrb_data_type transform_data_type = {
    "Transform2D", transform_free
};

static TransformData* get_transform_data(mrb_state* mrb, mrb_value self) {
    return static_cast<TransformData*>(mrb_data_get_ptr(mrb, self, &transform_data_type));
}

// ============================================================================
// Helper: Create Vec2 Ruby Object
// ============================================================================

static mrb_value create_vec2(mrb_state* mrb, float x, float y) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* mathf = mrb_module_get_under(mrb, gmr, "Mathf");
    RClass* vec2_class = mrb_class_get_under(mrb, mathf, "Vec2");
    mrb_value args[2] = {mrb_float_value(mrb, x), mrb_float_value(mrb, y)};
    return mrb_obj_new(mrb, vec2_class, 2, args);
}

// ============================================================================
// Helper: Extract Vec2 from Ruby Value
// ============================================================================

static Vec2 extract_vec2(mrb_state* mrb, mrb_value val) {
    mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
    mrb_sym y_sym = mrb_intern_cstr(mrb, "y");

    if (mrb_respond_to(mrb, val, x_sym) && mrb_respond_to(mrb, val, y_sym)) {
        mrb_value x = scripting::safe_method_call(mrb, val, "x");
        mrb_value y = scripting::safe_method_call(mrb, val, "y");
        return {static_cast<float>(mrb_as_float(mrb, x)),
                static_cast<float>(mrb_as_float(mrb, y))};
    }

    mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec2 or object with x/y methods");
    return {0.0f, 0.0f};
}

/// @method initialize
/// @description Create a new Transform2D with optional initial values.
/// @param x [Float] Initial X position (default: 0)
/// @param y [Float] Initial Y position (default: 0)
/// @param rotation [Float] Initial rotation in degrees (default: 0)
/// @param scale_x [Float] Initial X scale (default: 1.0)
/// @param scale_y [Float] Initial Y scale (default: 1.0)
/// @param origin_x [Float] Pivot point X (default: 0)
/// @param origin_y [Float] Pivot point Y (default: 0)
/// @returns [Transform2D] The new transform
/// @example t = Transform2D.new
/// @example t = Transform2D.new(x: 100, y: 50, rotation: 45)
static mrb_value mrb_transform_initialize(mrb_state* mrb, mrb_value self) {
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "|H", &kwargs);

    // Create transform
    TransformHandle handle = TransformManager::instance().create();
    Transform2DState* t = TransformManager::instance().get(handle);

    // Parse keyword arguments
    if (!mrb_nil_p(kwargs)) {
        mrb_value keys = mrb_hash_keys(mrb, kwargs);
        mrb_int len = RARRAY_LEN(keys);

        for (mrb_int i = 0; i < len; i++) {
            mrb_value key = mrb_ary_ref(mrb, keys, i);
            mrb_value val = mrb_hash_get(mrb, kwargs, key);

            const char* key_name = nullptr;
            if (mrb_symbol_p(key)) {
                key_name = mrb_sym_name(mrb, mrb_symbol(key));
            } else if (mrb_string_p(key)) {
                key_name = mrb_string_cstr(mrb, key);
            }

            if (key_name) {
                if (strcmp(key_name, "x") == 0) {
                    t->position.x = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "y") == 0) {
                    t->position.y = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "rotation") == 0) {
                    // Input in degrees, store in radians
                    t->rotation = static_cast<float>(mrb_as_float(mrb, val)) * DEG_TO_RAD;
                } else if (strcmp(key_name, "scale_x") == 0) {
                    t->scale.x = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "scale_y") == 0) {
                    t->scale.y = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "origin_x") == 0) {
                    t->origin.x = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "origin_y") == 0) {
                    t->origin.y = static_cast<float>(mrb_as_float(mrb, val));
                }
            }
        }
    }

    // Attach handle to Ruby object
    TransformData* data = static_cast<TransformData*>(mrb_malloc(mrb, sizeof(TransformData)));
    data->handle = handle;
    mrb_data_init(self, data, &transform_data_type);

    return self;
}

/// @method x
/// @description Get the X position of the transform.
/// @returns [Float] The X position
/// @example x_pos = transform.x
static mrb_value mrb_transform_x(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    return mrb_float_value(mrb, t->position.x);
}

/// @method y
/// @description Get the Y position of the transform.
/// @returns [Float] The Y position
/// @example y_pos = transform.y
static mrb_value mrb_transform_y(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    return mrb_float_value(mrb, t->position.y);
}

/// @method position
/// @description Get the position as a Vec2.
/// @returns [Vec2] The position vector
/// @example pos = transform.position
static mrb_value mrb_transform_position(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    return create_vec2(mrb, t->position.x, t->position.y);
}

/// @method rotation
/// @description Get the rotation in degrees.
/// @returns [Float] The rotation angle in degrees
/// @example angle = transform.rotation
static mrb_value mrb_transform_rotation(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    // Return in degrees
    return mrb_float_value(mrb, t->rotation * RAD_TO_DEG);
}

/// @method scale_x
/// @description Get the X scale factor.
/// @returns [Float] The X scale (1.0 = normal size)
/// @example sx = transform.scale_x
static mrb_value mrb_transform_scale_x(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    return mrb_float_value(mrb, t->scale.x);
}

/// @method scale_y
/// @description Get the Y scale factor.
/// @returns [Float] The Y scale (1.0 = normal size)
/// @example sy = transform.scale_y
static mrb_value mrb_transform_scale_y(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    return mrb_float_value(mrb, t->scale.y);
}

/// @method origin_x
/// @description Get the X origin (pivot point) for rotation and scaling.
/// @returns [Float] The X origin offset in pixels
/// @example ox = transform.origin_x
static mrb_value mrb_transform_origin_x(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    return mrb_float_value(mrb, t->origin.x);
}

/// @method origin_y
/// @description Get the Y origin (pivot point) for rotation and scaling.
/// @returns [Float] The Y origin offset in pixels
/// @example oy = transform.origin_y
static mrb_value mrb_transform_origin_y(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    return mrb_float_value(mrb, t->origin.y);
}

// ============================================================================
// Property Setters
// ============================================================================

/// @method x=
/// @description Set the X position of the transform.
/// @param value [Float] The new X position
/// @returns [Float] The value that was set
/// @example transform.x = 100
/// @example transform.x += 5  # Move right by 5 pixels
static mrb_value mrb_transform_set_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    t->position.x = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

/// @method y=
/// @description Set the Y position of the transform.
/// @param value [Float] The new Y position
/// @returns [Float] The value that was set
/// @example transform.y = 200
/// @example transform.y += 10  # Move down by 10 pixels
static mrb_value mrb_transform_set_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    t->position.y = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

/// @method position=
/// @description Set the position using a Vec2.
/// @param value [Vec2] The new position vector
/// @returns [Vec2] The value that was set
/// @example transform.position = Vec2.new(100, 200)
/// @example transform.position = player.position  # Copy another position
static mrb_value mrb_transform_set_position(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    t->position = extract_vec2(mrb, val);
    TransformManager::instance().mark_dirty(data->handle);
    return val;
}

/// @method rotation=
/// @description Set the rotation in degrees. Positive values rotate clockwise.
/// @param value [Float] The new rotation angle in degrees
/// @returns [Float] The value that was set
/// @example transform.rotation = 45
/// @example transform.rotation += 90 * dt  # Rotate 90 degrees per second
static mrb_value mrb_transform_set_rotation(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    // Input in degrees, store in radians
    t->rotation = static_cast<float>(val) * DEG_TO_RAD;
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

/// @method scale_x=
/// @description Set the X scale factor. Values greater than 1 stretch horizontally,
///   less than 1 shrink. Negative values flip horizontally.
/// @param value [Float] The new X scale (1.0 = normal size)
/// @returns [Float] The value that was set
/// @example transform.scale_x = 2.0   # Double width
/// @example transform.scale_x = -1.0  # Flip horizontally
static mrb_value mrb_transform_set_scale_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    t->scale.x = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

/// @method scale_y=
/// @description Set the Y scale factor. Values greater than 1 stretch vertically,
///   less than 1 shrink. Negative values flip vertically.
/// @param value [Float] The new Y scale (1.0 = normal size)
/// @returns [Float] The value that was set
/// @example transform.scale_y = 0.5   # Half height
/// @example transform.scale_y = -1.0  # Flip vertically
static mrb_value mrb_transform_set_scale_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    t->scale.y = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

/// @method origin_x=
/// @description Set the X origin (pivot point) for rotation and scaling.
///   The origin is the point around which the transform rotates and scales.
/// @param value [Float] The X origin offset in pixels from top-left
/// @returns [Float] The value that was set
/// @example transform.origin_x = 16  # Pivot 16px from left edge
static mrb_value mrb_transform_set_origin_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    t->origin.x = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

/// @method origin_y=
/// @description Set the Y origin (pivot point) for rotation and scaling.
///   The origin is the point around which the transform rotates and scales.
/// @param value [Float] The Y origin offset in pixels from top-left
/// @returns [Float] The value that was set
/// @example transform.origin_y = 16  # Pivot 16px from top edge
static mrb_value mrb_transform_set_origin_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    t->origin.y = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

// ============================================================================
// Parent Hierarchy
// ============================================================================

/// @method parent
/// @description Get the parent transform. Returns nil if no parent is set.
///   When a transform has a parent, its position, rotation, and scale are
///   relative to the parent's world transform.
/// @returns [Transform2D, nil] The parent transform, or nil if none
/// @example if transform.parent
///   puts "Has a parent!"
/// end
static mrb_value mrb_transform_parent(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);
    Transform2DState* t = TransformManager::instance().get(data->handle);
    GMR_REQUIRE_TRANSFORM_STATE(t, data->handle);
    if (t->parent == INVALID_HANDLE) return mrb_nil_value();  // Legitimate: no parent

    // We can't easily return the parent Ruby object without storing it
    // For now, return the handle as an integer (or nil if no parent)
    // A full implementation would store the Ruby object reference
    return mrb_fixnum_value(t->parent);
}

/// @method parent=
/// @description Set the parent transform for hierarchical transformations.
///   When parented, this transform's position, rotation, and scale become
///   relative to the parent. Set to nil to remove the parent.
/// @param value [Transform2D, nil] The parent transform, or nil to clear
/// @returns [Transform2D, nil] The value that was set
/// @example # Create a turret hierarchy
///   base = Transform2D.new(x: 200, y: 200)
///   gun = Transform2D.new
///   gun.parent = base
///   gun.y = -20  # Offset from base
///   base.rotation += 1  # Gun rotates with base!
/// @example transform.parent = nil  # Remove parent
static mrb_value mrb_transform_set_parent(mrb_state* mrb, mrb_value self) {
    mrb_value parent_val;
    mrb_get_args(mrb, "o", &parent_val);

    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    if (mrb_nil_p(parent_val)) {
        TransformManager::instance().clear_parent(data->handle);
    } else {
        TransformData* parent_data = get_transform_data(mrb, parent_val);
        if (parent_data) {
            TransformManager::instance().set_parent(data->handle, parent_data->handle);
        }
    }

    return parent_val;
}

// ============================================================================
// World Position (after hierarchy composition)
// ============================================================================

/// @method world_position
/// @description Get the final world position after applying all parent transforms.
///   For transforms without a parent, this equals the local position. For parented
///   transforms, this returns the actual screen position after parent transformations.
/// @returns [Vec2] The world position after parent hierarchy composition
/// @example # Get world position of a child transform
///   parent = Transform2D.new(x: 100, y: 100)
///   parent.rotation = 90
///   child = Transform2D.new(x: 50, y: 0)
///   child.parent = parent
///   pos = child.world_position  # Position after rotation by parent
static mrb_value mrb_transform_world_position(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    const Matrix2D& world = TransformManager::instance().get_world_matrix(data->handle);
    // The world matrix tx/ty gives us the world position
    return create_vec2(mrb, world.tx, world.ty);
}

// ============================================================================
// New Features: Cached World Transforms
// ============================================================================

/// @method world_rotation
/// @description Get the final world rotation after applying all parent transforms.
///   For transforms without a parent, this equals the local rotation. For parented
///   transforms, this returns the combined rotation of the entire hierarchy.
/// @returns [Float] The world rotation in degrees
/// @example child_world_rot = transform.world_rotation
static mrb_value mrb_transform_world_rotation(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    float world_rot = TransformManager::instance().get_world_rotation(data->handle);
    return mrb_float_value(mrb, world_rot * RAD_TO_DEG);  // Convert to degrees
}

/// @method world_scale
/// @description Get the final world scale after applying all parent transforms.
///   For transforms without a parent, this equals the local scale. For parented
///   transforms, this returns the combined scale of the entire hierarchy.
/// @returns [Vec2] The world scale
/// @example world_s = transform.world_scale
static mrb_value mrb_transform_world_scale(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    Vec2 world_scale = TransformManager::instance().get_world_scale(data->handle);
    return create_vec2(mrb, world_scale.x, world_scale.y);
}

// ============================================================================
// New Features: Direction Queries
// ============================================================================

/// @method forward
/// @description Get the forward direction vector after world rotation.
///   The forward vector points in the direction the transform is facing (0 degrees = right).
/// @returns [Vec2] The normalized forward direction
/// @example forward = transform.forward
///   transform.x += forward.x * speed * dt  # Move forward
static mrb_value mrb_transform_forward(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    Vec2 forward = TransformManager::instance().get_world_forward(data->handle);
    return create_vec2(mrb, forward.x, forward.y);
}

/// @method right
/// @description Get the right direction vector after world rotation.
///   The right vector points perpendicular to forward (90 degrees clockwise).
/// @returns [Vec2] The normalized right direction
/// @example right = transform.right
///   transform.x += right.x * strafe_speed * dt  # Strafe right
static mrb_value mrb_transform_right(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    Vec2 right = TransformManager::instance().get_world_right(data->handle);
    return create_vec2(mrb, right.x, right.y);
}

// ============================================================================
// New Features: Utility Functions
// ============================================================================

/// @method snap_to_grid!
/// @description Snap the position to the nearest grid cell.
///   Useful for tile-based games or ensuring pixel-perfect alignment.
/// @param grid_size [Float] The size of each grid cell in pixels
/// @returns [nil]
/// @example transform.snap_to_grid!(16)  # Snap to 16x16 grid
static mrb_value mrb_transform_snap_to_grid(mrb_state* mrb, mrb_value self) {
    mrb_float grid_size;
    mrb_get_args(mrb, "f", &grid_size);

    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    TransformManager::instance().snap_position_to_grid(data->handle, static_cast<float>(grid_size));
    return mrb_nil_value();
}

/// @method round_to_pixel!
/// @description Round the position to the nearest integer pixel coordinates.
///   Useful for pixel-art games to avoid sub-pixel rendering artifacts.
/// @returns [nil]
/// @example transform.round_to_pixel!  # Ensure crisp pixel alignment
static mrb_value mrb_transform_round_to_pixel(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    GMR_REQUIRE_TRANSFORM_DATA(data);

    TransformManager::instance().round_position_to_pixel(data->handle);
    return mrb_nil_value();
}

// ============================================================================
// New Features: Interpolation (Class Methods)
// ============================================================================

/// @method Transform2D.lerp_position
/// @description Linearly interpolate between two positions.
/// @param a [Vec2] Start position
/// @param b [Vec2] End position
/// @param t [Float] Interpolation factor (0.0 to 1.0)
/// @returns [Vec2] The interpolated position
/// @example pos = Transform2D.lerp_position(start_pos, end_pos, 0.5)
static mrb_value mrb_transform_lerp_position(mrb_state* mrb, mrb_value self) {
    mrb_value a_val, b_val;
    mrb_float t;
    mrb_get_args(mrb, "oof", &a_val, &b_val, &t);

    Vec2 a = extract_vec2(mrb, a_val);
    Vec2 b = extract_vec2(mrb, b_val);
    Vec2 result = TransformManager::lerp_position(a, b, static_cast<float>(t));

    return create_vec2(mrb, result.x, result.y);
}

/// @method Transform2D.lerp_rotation
/// @description Interpolate between two rotation angles using shortest path.
///   Automatically handles wrapping (e.g., 350° to 10° goes through 0°, not 340°).
/// @param a [Float] Start rotation in degrees
/// @param b [Float] End rotation in degrees
/// @param t [Float] Interpolation factor (0.0 to 1.0)
/// @returns [Float] The interpolated rotation in degrees
/// @example rot = Transform2D.lerp_rotation(0, 270, 0.5)  # Returns 315 (shortest path)
static mrb_value mrb_transform_lerp_rotation(mrb_state* mrb, mrb_value self) {
    mrb_float a, b, t;
    mrb_get_args(mrb, "fff", &a, &b, &t);

    float a_rad = static_cast<float>(a) * DEG_TO_RAD;
    float b_rad = static_cast<float>(b) * DEG_TO_RAD;
    float result_rad = TransformManager::lerp_rotation(a_rad, b_rad, static_cast<float>(t));

    return mrb_float_value(mrb, result_rad * RAD_TO_DEG);
}

/// @method Transform2D.lerp_scale
/// @description Linearly interpolate between two scale vectors.
/// @param a [Vec2] Start scale
/// @param b [Vec2] End scale
/// @param t [Float] Interpolation factor (0.0 to 1.0)
/// @returns [Vec2] The interpolated scale
/// @example scale = Transform2D.lerp_scale(Vec2.new(1, 1), Vec2.new(2, 2), 0.5)
static mrb_value mrb_transform_lerp_scale(mrb_state* mrb, mrb_value self) {
    mrb_value a_val, b_val;
    mrb_float t;
    mrb_get_args(mrb, "oof", &a_val, &b_val, &t);

    Vec2 a = extract_vec2(mrb, a_val);
    Vec2 b = extract_vec2(mrb, b_val);
    Vec2 result = TransformManager::lerp_scale(a, b, static_cast<float>(t));

    return create_vec2(mrb, result.x, result.y);
}

// ============================================================================
// Registration
// ============================================================================

void register_transform(mrb_state* mrb) {
    // Transform2D class under GMR::Graphics
    RClass* gmr = get_gmr_module(mrb);
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
    RClass* transform_class = mrb_define_class_under(mrb, graphics, "Transform2D", mrb->object_class);
    MRB_SET_INSTANCE_TT(transform_class, MRB_TT_CDATA);

    // Constructor
    mrb_define_method(mrb, transform_class, "initialize", mrb_transform_initialize, MRB_ARGS_OPT(1));

    // Position
    mrb_define_method(mrb, transform_class, "x", mrb_transform_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "y", mrb_transform_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "x=", mrb_transform_set_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, transform_class, "y=", mrb_transform_set_y, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, transform_class, "position", mrb_transform_position, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "position=", mrb_transform_set_position, MRB_ARGS_REQ(1));

    // Rotation (degrees in Ruby)
    mrb_define_method(mrb, transform_class, "rotation", mrb_transform_rotation, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "rotation=", mrb_transform_set_rotation, MRB_ARGS_REQ(1));

    // Scale
    mrb_define_method(mrb, transform_class, "scale_x", mrb_transform_scale_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "scale_y", mrb_transform_scale_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "scale_x=", mrb_transform_set_scale_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, transform_class, "scale_y=", mrb_transform_set_scale_y, MRB_ARGS_REQ(1));

    // Origin
    mrb_define_method(mrb, transform_class, "origin_x", mrb_transform_origin_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "origin_y", mrb_transform_origin_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "origin_x=", mrb_transform_set_origin_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, transform_class, "origin_y=", mrb_transform_set_origin_y, MRB_ARGS_REQ(1));

    // Parent hierarchy
    mrb_define_method(mrb, transform_class, "parent", mrb_transform_parent, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "parent=", mrb_transform_set_parent, MRB_ARGS_REQ(1));

    // World position
    mrb_define_method(mrb, transform_class, "world_position", mrb_transform_world_position, MRB_ARGS_NONE());

    // New: Cached world transforms
    mrb_define_method(mrb, transform_class, "world_rotation", mrb_transform_world_rotation, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "world_scale", mrb_transform_world_scale, MRB_ARGS_NONE());

    // New: Direction queries
    mrb_define_method(mrb, transform_class, "forward", mrb_transform_forward, MRB_ARGS_NONE());
    mrb_define_method(mrb, transform_class, "right", mrb_transform_right, MRB_ARGS_NONE());

    // New: Utility functions
    mrb_define_method(mrb, transform_class, "snap_to_grid!", mrb_transform_snap_to_grid, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, transform_class, "round_to_pixel!", mrb_transform_round_to_pixel, MRB_ARGS_NONE());

    // New: Interpolation (class methods)
    mrb_define_class_method(mrb, transform_class, "lerp_position", mrb_transform_lerp_position, MRB_ARGS_REQ(3));
    mrb_define_class_method(mrb, transform_class, "lerp_rotation", mrb_transform_lerp_rotation, MRB_ARGS_REQ(3));
    mrb_define_class_method(mrb, transform_class, "lerp_scale", mrb_transform_lerp_scale, MRB_ARGS_REQ(3));
}

} // namespace bindings
} // namespace gmr
