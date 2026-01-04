#include "gmr/bindings/transform.hpp"
#include "gmr/transform.hpp"
#include "gmr/types.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <cstring>
#include <cmath>

namespace gmr {
namespace bindings {

// Degrees <-> Radians conversion
static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;

// ============================================================================
// Transform2D Binding Data
// ============================================================================

struct TransformData {
    TransformHandle handle;
};

static void transform_free(mrb_state* mrb, void* ptr) {
    TransformData* data = static_cast<TransformData*>(ptr);
    if (data) {
        TransformManager::instance().destroy(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type transform_data_type = {
    "Transform2D", transform_free
};

static TransformData* get_transform_data(mrb_state* mrb, mrb_value self) {
    return static_cast<TransformData*>(mrb_data_get_ptr(mrb, self, &transform_data_type));
}

// ============================================================================
// Helper: Create Vec2 Ruby Object
// ============================================================================

static mrb_value create_vec2(mrb_state* mrb, float x, float y) {
    RClass* vec2_class = mrb_class_get(mrb, "Vec2");
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
        mrb_value x = mrb_funcall(mrb, val, "x", 0);
        mrb_value y = mrb_funcall(mrb, val, "y", 0);
        return {static_cast<float>(mrb_as_float(mrb, x)),
                static_cast<float>(mrb_as_float(mrb, y))};
    }

    mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec2 or object with x/y methods");
    return {0.0f, 0.0f};
}

// ============================================================================
// Transform2D.new(x:, y:, rotation:, scale_x:, scale_y:, origin_x:, origin_y:)
// ============================================================================

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

// ============================================================================
// Property Getters
// ============================================================================

static mrb_value mrb_transform_x(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    return mrb_float_value(mrb, t->position.x);
}

static mrb_value mrb_transform_y(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    return mrb_float_value(mrb, t->position.y);
}

static mrb_value mrb_transform_position(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    return create_vec2(mrb, t->position.x, t->position.y);
}

static mrb_value mrb_transform_rotation(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    // Return in degrees
    return mrb_float_value(mrb, t->rotation * RAD_TO_DEG);
}

static mrb_value mrb_transform_scale_x(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    return mrb_float_value(mrb, t->scale.x);
}

static mrb_value mrb_transform_scale_y(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    return mrb_float_value(mrb, t->scale.y);
}

static mrb_value mrb_transform_origin_x(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    return mrb_float_value(mrb, t->origin.x);
}

static mrb_value mrb_transform_origin_y(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    return mrb_float_value(mrb, t->origin.y);
}

// ============================================================================
// Property Setters
// ============================================================================

static mrb_value mrb_transform_set_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    t->position.x = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_transform_set_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    t->position.y = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_transform_set_position(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    t->position = extract_vec2(mrb, val);
    TransformManager::instance().mark_dirty(data->handle);
    return val;
}

static mrb_value mrb_transform_set_rotation(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    // Input in degrees, store in radians
    t->rotation = static_cast<float>(val) * DEG_TO_RAD;
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_transform_set_scale_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    t->scale.x = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_transform_set_scale_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    t->scale.y = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_transform_set_origin_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    t->origin.x = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_transform_set_origin_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t) return mrb_nil_value();
    t->origin.y = static_cast<float>(val);
    TransformManager::instance().mark_dirty(data->handle);
    return mrb_float_value(mrb, val);
}

// ============================================================================
// Parent Hierarchy
// ============================================================================

static mrb_value mrb_transform_parent(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();
    Transform2DState* t = TransformManager::instance().get(data->handle);
    if (!t || t->parent == INVALID_HANDLE) return mrb_nil_value();

    // We can't easily return the parent Ruby object without storing it
    // For now, return the handle as an integer (or nil if no parent)
    // A full implementation would store the Ruby object reference
    return mrb_fixnum_value(t->parent);
}

static mrb_value mrb_transform_set_parent(mrb_state* mrb, mrb_value self) {
    mrb_value parent_val;
    mrb_get_args(mrb, "o", &parent_val);

    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();

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

static mrb_value mrb_transform_world_position(mrb_state* mrb, mrb_value self) {
    TransformData* data = get_transform_data(mrb, self);
    if (!data) return mrb_nil_value();

    const Matrix2D& world = TransformManager::instance().get_world_matrix(data->handle);
    // The world matrix tx/ty gives us the world position
    return create_vec2(mrb, world.tx, world.ty);
}

// ============================================================================
// Registration
// ============================================================================

void register_transform(mrb_state* mrb) {
    // Transform2D class (top-level)
    RClass* transform_class = mrb_define_class(mrb, "Transform2D", mrb->object_class);
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
}

} // namespace bindings
} // namespace gmr
