#include "gmr/bindings/sprite.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/types.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/variable.h>
#include <cstring>
#include <cmath>

namespace gmr {
namespace bindings {

// Degrees <-> Radians conversion
static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;

// ============================================================================
// Sprite Binding Data
// ============================================================================

struct SpriteData {
    SpriteHandle handle;
};

static void sprite_free(mrb_state* mrb, void* ptr) {
    SpriteData* data = static_cast<SpriteData*>(ptr);
    if (data) {
        SpriteManager::instance().destroy(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type sprite_data_type = {
    "Sprite", sprite_free
};

static SpriteData* get_sprite_data(mrb_state* mrb, mrb_value self) {
    return static_cast<SpriteData*>(mrb_data_get_ptr(mrb, self, &sprite_data_type));
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
// Helper: Extract Rect from Ruby Value
// ============================================================================

static Rect extract_rect(mrb_state* mrb, mrb_value val) {
    mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
    mrb_sym y_sym = mrb_intern_cstr(mrb, "y");
    mrb_sym w_sym = mrb_intern_cstr(mrb, "w");
    mrb_sym h_sym = mrb_intern_cstr(mrb, "h");

    if (mrb_respond_to(mrb, val, x_sym) && mrb_respond_to(mrb, val, y_sym) &&
        mrb_respond_to(mrb, val, w_sym) && mrb_respond_to(mrb, val, h_sym)) {
        mrb_value x = mrb_funcall(mrb, val, "x", 0);
        mrb_value y = mrb_funcall(mrb, val, "y", 0);
        mrb_value w = mrb_funcall(mrb, val, "w", 0);
        mrb_value h = mrb_funcall(mrb, val, "h", 0);
        return {static_cast<float>(mrb_as_float(mrb, x)),
                static_cast<float>(mrb_as_float(mrb, y)),
                static_cast<float>(mrb_as_float(mrb, w)),
                static_cast<float>(mrb_as_float(mrb, h))};
    }

    mrb_raise(mrb, E_TYPE_ERROR, "Expected Rect or object with x/y/w/h methods");
    return {0.0f, 0.0f, 0.0f, 0.0f};
}

// Forward declaration for transform data type
extern const mrb_data_type transform_data_type;

// Helper to get TransformHandle from Ruby Transform2D object
static TransformHandle get_transform_handle(mrb_state* mrb, mrb_value val) {
    // Get the transform_data_type from the transform binding
    struct TransformData {
        TransformHandle handle;
    };

    void* ptr = mrb_data_get_ptr(mrb, val, nullptr);
    if (ptr) {
        TransformData* data = static_cast<TransformData*>(ptr);
        return data->handle;
    }
    return INVALID_HANDLE;
}

// ============================================================================
// Sprite.new(texture, **opts)
// ============================================================================

static mrb_value mrb_sprite_initialize(mrb_state* mrb, mrb_value self) {
    mrb_value texture_val;
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "o|H", &texture_val, &kwargs);

    // Create sprite
    SpriteHandle handle = SpriteManager::instance().create();
    SpriteState* s = SpriteManager::instance().get(handle);

    // Get texture handle from Ruby Texture object
    // Texture objects store handle in @handle instance variable
    mrb_sym handle_sym = mrb_intern_cstr(mrb, "@handle");
    mrb_value handle_val = mrb_iv_get(mrb, texture_val, handle_sym);
    if (!mrb_nil_p(handle_val)) {
        s->texture = static_cast<TextureHandle>(mrb_fixnum(handle_val));
    }

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
                    s->position.x = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "y") == 0) {
                    s->position.y = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "rotation") == 0) {
                    s->rotation = static_cast<float>(mrb_as_float(mrb, val)) * DEG_TO_RAD;
                } else if (strcmp(key_name, "scale_x") == 0) {
                    s->scale.x = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "scale_y") == 0) {
                    s->scale.y = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "z") == 0) {
                    s->z = static_cast<float>(mrb_as_float(mrb, val));
                } else if (strcmp(key_name, "source_rect") == 0) {
                    s->source_rect = extract_rect(mrb, val);
                    s->use_source_rect = true;
                }
            }
        }
    }

    // Attach handle to Ruby object
    SpriteData* data = static_cast<SpriteData*>(mrb_malloc(mrb, sizeof(SpriteData)));
    data->handle = handle;
    mrb_data_init(self, data, &sprite_data_type);

    // Store texture reference to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@texture"), texture_val);

    return self;
}

// ============================================================================
// Position Getters/Setters
// ============================================================================

static mrb_value mrb_sprite_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->position.x);
}

static mrb_value mrb_sprite_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->position.y);
}

static mrb_value mrb_sprite_set_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->position.x = static_cast<float>(val);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_sprite_set_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->position.y = static_cast<float>(val);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_sprite_position(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return create_vec2(mrb, s->position.x, s->position.y);
}

static mrb_value mrb_sprite_set_position(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->position = extract_vec2(mrb, val);
    return val;
}

// ============================================================================
// Rotation
// ============================================================================

static mrb_value mrb_sprite_rotation(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->rotation * RAD_TO_DEG);
}

static mrb_value mrb_sprite_set_rotation(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->rotation = static_cast<float>(val) * DEG_TO_RAD;
    return mrb_float_value(mrb, val);
}

// ============================================================================
// Scale
// ============================================================================

static mrb_value mrb_sprite_scale_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->scale.x);
}

static mrb_value mrb_sprite_scale_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->scale.y);
}

static mrb_value mrb_sprite_set_scale_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->scale.x = static_cast<float>(val);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_sprite_set_scale_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->scale.y = static_cast<float>(val);
    return mrb_float_value(mrb, val);
}

// ============================================================================
// Origin
// ============================================================================

static mrb_value mrb_sprite_origin_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->origin.x);
}

static mrb_value mrb_sprite_origin_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->origin.y);
}

static mrb_value mrb_sprite_set_origin_x(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->origin.x = static_cast<float>(val);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_sprite_set_origin_y(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->origin.y = static_cast<float>(val);
    return mrb_float_value(mrb, val);
}

static mrb_value mrb_sprite_center_origin(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    // Get texture dimensions
    float w, h;
    if (s->use_source_rect) {
        w = s->source_rect.width;
        h = s->source_rect.height;
    } else {
        auto* texture = TextureManager::instance().get(s->texture);
        if (texture) {
            w = static_cast<float>(texture->width);
            h = static_cast<float>(texture->height);
        } else {
            w = h = 0;
        }
    }

    s->origin.x = w / 2.0f;
    s->origin.y = h / 2.0f;

    return self;
}

// ============================================================================
// Z-Index
// ============================================================================

static mrb_value mrb_sprite_z(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    if (s->z.has_value()) {
        return mrb_float_value(mrb, s->z.value());
    }
    return mrb_nil_value();
}

static mrb_value mrb_sprite_set_z(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    if (mrb_nil_p(val)) {
        s->z = std::nullopt;
    } else {
        s->z = static_cast<float>(mrb_as_float(mrb, val));
    }
    return val;
}

// ============================================================================
// Color
// ============================================================================

static mrb_value mrb_sprite_color(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    mrb_value arr = mrb_ary_new_capa(mrb, 4);
    mrb_ary_push(mrb, arr, mrb_fixnum_value(s->color.r));
    mrb_ary_push(mrb, arr, mrb_fixnum_value(s->color.g));
    mrb_ary_push(mrb, arr, mrb_fixnum_value(s->color.b));
    mrb_ary_push(mrb, arr, mrb_fixnum_value(s->color.a));
    return arr;
}

static mrb_value mrb_sprite_set_color(mrb_state* mrb, mrb_value self) {
    mrb_value color_val;
    mrb_get_args(mrb, "A", &color_val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    s->color = parse_color_value(mrb, color_val, Color{255, 255, 255, 255});
    return color_val;
}

static mrb_value mrb_sprite_alpha(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->color.a / 255.0f);
}

static mrb_value mrb_sprite_set_alpha(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    // Clamp to 0-1 and convert to 0-255
    if (val < 0) val = 0;
    if (val > 1) val = 1;
    s->color.a = static_cast<uint8_t>(val * 255);
    return mrb_float_value(mrb, val);
}

// ============================================================================
// Flip
// ============================================================================

static mrb_value mrb_sprite_flip_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_bool_value(s->flip_x);
}

static mrb_value mrb_sprite_flip_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_bool_value(s->flip_y);
}

static mrb_value mrb_sprite_set_flip_x(mrb_state* mrb, mrb_value self) {
    mrb_bool val;
    mrb_get_args(mrb, "b", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->flip_x = val;
    return mrb_bool_value(val);
}

static mrb_value mrb_sprite_set_flip_y(mrb_state* mrb, mrb_value self) {
    mrb_bool val;
    mrb_get_args(mrb, "b", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->flip_y = val;
    return mrb_bool_value(val);
}

// ============================================================================
// Texture/Source Rect
// ============================================================================

static mrb_value mrb_sprite_texture(mrb_state* mrb, mrb_value self) {
    return mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "@texture"));
}

static mrb_value mrb_sprite_set_texture(mrb_state* mrb, mrb_value self) {
    mrb_value texture_val;
    mrb_get_args(mrb, "o", &texture_val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    // Get texture handle from Ruby Texture object
    mrb_sym handle_sym = mrb_intern_cstr(mrb, "@handle");
    mrb_value handle_val = mrb_iv_get(mrb, texture_val, handle_sym);
    if (!mrb_nil_p(handle_val)) {
        s->texture = static_cast<TextureHandle>(mrb_fixnum(handle_val));
    }

    // Store texture reference to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@texture"), texture_val);
    return texture_val;
}

static mrb_value mrb_sprite_source_rect(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s || !s->use_source_rect) return mrb_nil_value();

    RClass* rect_class = mrb_class_get(mrb, "Rect");
    mrb_value args[4] = {
        mrb_float_value(mrb, s->source_rect.x),
        mrb_float_value(mrb, s->source_rect.y),
        mrb_float_value(mrb, s->source_rect.width),
        mrb_float_value(mrb, s->source_rect.height)
    };
    return mrb_obj_new(mrb, rect_class, 4, args);
}

static mrb_value mrb_sprite_set_source_rect(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    if (mrb_nil_p(val)) {
        s->use_source_rect = false;
    } else {
        s->source_rect = extract_rect(mrb, val);
        s->use_source_rect = true;
    }
    return val;
}

// ============================================================================
// Dimensions
// ============================================================================

static mrb_value mrb_sprite_width(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    if (s->use_source_rect) {
        return mrb_float_value(mrb, s->source_rect.width);
    }

    auto* texture = TextureManager::instance().get(s->texture);
    if (texture) {
        return mrb_fixnum_value(texture->width);
    }
    return mrb_fixnum_value(0);
}

static mrb_value mrb_sprite_height(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    if (s->use_source_rect) {
        return mrb_float_value(mrb, s->source_rect.height);
    }

    auto* texture = TextureManager::instance().get(s->texture);
    if (texture) {
        return mrb_fixnum_value(texture->height);
    }
    return mrb_fixnum_value(0);
}

// ============================================================================
// Parent Hierarchy
// ============================================================================

static mrb_value mrb_sprite_parent(mrb_state* mrb, mrb_value self) {
    return mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "@parent"));
}

static mrb_value mrb_sprite_set_parent(mrb_state* mrb, mrb_value self) {
    mrb_value parent_val;
    mrb_get_args(mrb, "o", &parent_val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();

    if (mrb_nil_p(parent_val)) {
        s->parent_transform = INVALID_HANDLE;
    } else {
        s->parent_transform = get_transform_handle(mrb, parent_val);
    }

    // Store parent reference to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@parent"), parent_val);
    return parent_val;
}

// ============================================================================
// Draw
// ============================================================================

static mrb_value mrb_sprite_draw(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();

    // Queue sprite for deferred rendering
    DrawQueue::instance().queue_sprite(data->handle);

    return self;
}

// ============================================================================
// Class Methods
// ============================================================================

static mrb_value mrb_sprite_class_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(SpriteManager::instance().count());
}

// ============================================================================
// Registration
// ============================================================================

void register_sprite(mrb_state* mrb) {
    // Sprite class (top-level)
    RClass* sprite_class = mrb_define_class(mrb, "Sprite", mrb->object_class);
    MRB_SET_INSTANCE_TT(sprite_class, MRB_TT_CDATA);

    // Constructor
    mrb_define_method(mrb, sprite_class, "initialize", mrb_sprite_initialize, MRB_ARGS_ARG(1, 1));

    // Position
    mrb_define_method(mrb, sprite_class, "x", mrb_sprite_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "y", mrb_sprite_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "x=", mrb_sprite_set_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "y=", mrb_sprite_set_y, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "position", mrb_sprite_position, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "position=", mrb_sprite_set_position, MRB_ARGS_REQ(1));

    // Rotation
    mrb_define_method(mrb, sprite_class, "rotation", mrb_sprite_rotation, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "rotation=", mrb_sprite_set_rotation, MRB_ARGS_REQ(1));

    // Scale
    mrb_define_method(mrb, sprite_class, "scale_x", mrb_sprite_scale_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "scale_y", mrb_sprite_scale_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "scale_x=", mrb_sprite_set_scale_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "scale_y=", mrb_sprite_set_scale_y, MRB_ARGS_REQ(1));

    // Origin
    mrb_define_method(mrb, sprite_class, "origin_x", mrb_sprite_origin_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "origin_y", mrb_sprite_origin_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "origin_x=", mrb_sprite_set_origin_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "origin_y=", mrb_sprite_set_origin_y, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "center_origin", mrb_sprite_center_origin, MRB_ARGS_NONE());

    // Z-index
    mrb_define_method(mrb, sprite_class, "z", mrb_sprite_z, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "z=", mrb_sprite_set_z, MRB_ARGS_REQ(1));

    // Color
    mrb_define_method(mrb, sprite_class, "color", mrb_sprite_color, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "color=", mrb_sprite_set_color, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "alpha", mrb_sprite_alpha, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "alpha=", mrb_sprite_set_alpha, MRB_ARGS_REQ(1));

    // Flip
    mrb_define_method(mrb, sprite_class, "flip_x", mrb_sprite_flip_x, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "flip_y", mrb_sprite_flip_y, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "flip_x=", mrb_sprite_set_flip_x, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "flip_y=", mrb_sprite_set_flip_y, MRB_ARGS_REQ(1));

    // Texture/Source
    mrb_define_method(mrb, sprite_class, "texture", mrb_sprite_texture, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "texture=", mrb_sprite_set_texture, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sprite_class, "source_rect", mrb_sprite_source_rect, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "source_rect=", mrb_sprite_set_source_rect, MRB_ARGS_REQ(1));

    // Dimensions
    mrb_define_method(mrb, sprite_class, "width", mrb_sprite_width, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "height", mrb_sprite_height, MRB_ARGS_NONE());

    // Parent hierarchy
    mrb_define_method(mrb, sprite_class, "parent", mrb_sprite_parent, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "parent=", mrb_sprite_set_parent, MRB_ARGS_REQ(1));

    // Draw
    mrb_define_method(mrb, sprite_class, "draw", mrb_sprite_draw, MRB_ARGS_NONE());

    // Class methods
    mrb_define_class_method(mrb, sprite_class, "count", mrb_sprite_class_count, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
