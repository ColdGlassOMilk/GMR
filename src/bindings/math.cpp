#include "gmr/bindings/math.hpp"
#include "gmr/types.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>

namespace gmr {
namespace bindings {

// ============================================================================
// Vec2 Class
// ============================================================================

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

// Vec2.new(x = 0, y = 0)
static mrb_value mrb_vec2_initialize(mrb_state* mrb, mrb_value self) {
    mrb_float x = 0.0f, y = 0.0f;
    mrb_get_args(mrb, "|ff", &x, &y);

    Vec2Data* data = static_cast<Vec2Data*>(mrb_malloc(mrb, sizeof(Vec2Data)));
    data->x = static_cast<float>(x);
    data->y = static_cast<float>(y);
    mrb_data_init(self, data, &vec2_data_type);

    return self;
}

// vec2.x
static mrb_value mrb_vec2_x(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);
    return mrb_float_value(mrb, data->x);
}

// vec2.y
static mrb_value mrb_vec2_y(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);
    return mrb_float_value(mrb, data->y);
}

// Helper to create a new Vec2 Ruby object
static mrb_value create_vec2(mrb_state* mrb, float x, float y) {
    RClass* vec2_class = mrb_class_get(mrb, "Vec2");
    mrb_value obj = mrb_obj_new(mrb, vec2_class, 0, nullptr);

    Vec2Data* data = static_cast<Vec2Data*>(mrb_malloc(mrb, sizeof(Vec2Data)));
    data->x = x;
    data->y = y;
    mrb_data_init(obj, data, &vec2_data_type);

    return obj;
}

// vec2 + other
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

// vec2 - other
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

// vec2 * scalar
static mrb_value mrb_vec2_mul(mrb_state* mrb, mrb_value self) {
    mrb_float scalar;
    mrb_get_args(mrb, "f", &scalar);

    Vec2Data* data = get_vec2_data(mrb, self);
    return create_vec2(mrb, data->x * static_cast<float>(scalar),
                           data->y * static_cast<float>(scalar));
}

// vec2 / scalar
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

// vec2.to_s
static mrb_value mrb_vec2_to_s(mrb_state* mrb, mrb_value self) {
    Vec2Data* data = get_vec2_data(mrb, self);
    char buf[64];
    snprintf(buf, sizeof(buf), "Vec2(%.2f, %.2f)", data->x, data->y);
    return mrb_str_new_cstr(mrb, buf);
}

// ============================================================================
// Vec3 Class
// ============================================================================

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

// Vec3.new(x = 0, y = 0, z = 0)
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

// vec3.x
static mrb_value mrb_vec3_x(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    return mrb_float_value(mrb, data->x);
}

// vec3.y
static mrb_value mrb_vec3_y(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    return mrb_float_value(mrb, data->y);
}

// vec3.z
static mrb_value mrb_vec3_z(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    return mrb_float_value(mrb, data->z);
}

// Helper to create a new Vec3 Ruby object
static mrb_value create_vec3(mrb_state* mrb, float x, float y, float z) {
    RClass* vec3_class = mrb_class_get(mrb, "Vec3");
    mrb_value obj = mrb_obj_new(mrb, vec3_class, 0, nullptr);

    Vec3Data* data = static_cast<Vec3Data*>(mrb_malloc(mrb, sizeof(Vec3Data)));
    data->x = x;
    data->y = y;
    data->z = z;
    mrb_data_init(obj, data, &vec3_data_type);

    return obj;
}

// vec3 + other
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

// vec3 - other
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

// vec3 * scalar
static mrb_value mrb_vec3_mul(mrb_state* mrb, mrb_value self) {
    mrb_float scalar;
    mrb_get_args(mrb, "f", &scalar);

    Vec3Data* data = get_vec3_data(mrb, self);
    return create_vec3(mrb, data->x * static_cast<float>(scalar),
                           data->y * static_cast<float>(scalar),
                           data->z * static_cast<float>(scalar));
}

// vec3 / scalar
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

// vec3.to_s
static mrb_value mrb_vec3_to_s(mrb_state* mrb, mrb_value self) {
    Vec3Data* data = get_vec3_data(mrb, self);
    char buf[96];
    snprintf(buf, sizeof(buf), "Vec3(%.2f, %.2f, %.2f)", data->x, data->y, data->z);
    return mrb_str_new_cstr(mrb, buf);
}

// ============================================================================
// Rect Class
// ============================================================================

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

// Rect.new(x = 0, y = 0, w = 0, h = 0)
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

// rect.x
static mrb_value mrb_rect_x(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->x);
}

// rect.y
static mrb_value mrb_rect_y(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->y);
}

// rect.w
static mrb_value mrb_rect_w(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->w);
}

// rect.h
static mrb_value mrb_rect_h(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    return mrb_float_value(mrb, data->h);
}

// rect.to_s
static mrb_value mrb_rect_to_s(mrb_state* mrb, mrb_value self) {
    RectData* data = get_rect_data(mrb, self);
    char buf[96];
    snprintf(buf, sizeof(buf), "Rect(%.2f, %.2f, %.2f, %.2f)",
             data->x, data->y, data->w, data->h);
    return mrb_str_new_cstr(mrb, buf);
}

// ============================================================================
// Registration
// ============================================================================

void register_math(mrb_state* mrb) {
    // Vec2 class (top-level)
    RClass* vec2_class = mrb_define_class(mrb, "Vec2", mrb->object_class);
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

    // Vec3 class (top-level)
    RClass* vec3_class = mrb_define_class(mrb, "Vec3", mrb->object_class);
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

    // Rect class (top-level)
    RClass* rect_class = mrb_define_class(mrb, "Rect", mrb->object_class);
    MRB_SET_INSTANCE_TT(rect_class, MRB_TT_CDATA);

    mrb_define_method(mrb, rect_class, "initialize", mrb_rect_initialize, MRB_ARGS_OPT(4));
    mrb_define_method(mrb, rect_class, "x", mrb_rect_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "y", mrb_rect_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "w", mrb_rect_w, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "h", mrb_rect_h, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "to_s", mrb_rect_to_s, MRB_ARGS_NONE());
    mrb_define_method(mrb, rect_class, "inspect", mrb_rect_to_s, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
