#include "gmr/bindings/camera.hpp"
#include "gmr/camera.hpp"
#include "gmr/types.hpp"
#include "raylib.h"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <cstring>

namespace gmr {
namespace bindings {

// ============================================================================
// Camera2D Binding Data
// ============================================================================

struct CameraData {
    CameraHandle handle;
};

static void camera_free(mrb_state* mrb, void* ptr) {
    CameraData* data = static_cast<CameraData*>(ptr);
    if (data) {
        CameraManager::instance().destroy(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type camera_data_type = {
    "Camera2D", camera_free
};

static CameraData* get_camera_data(mrb_state* mrb, mrb_value self) {
    return static_cast<CameraData*>(mrb_data_get_ptr(mrb, self, &camera_data_type));
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
        return {static_cast<float>(mrb_as_float(mrb,x)),
                static_cast<float>(mrb_as_float(mrb,y)),
                static_cast<float>(mrb_as_float(mrb,w)),
                static_cast<float>(mrb_as_float(mrb,h))};
    }

    mrb_raise(mrb, E_TYPE_ERROR, "Expected Rect or object with x/y/w/h methods");
    return {0.0f, 0.0f, 0.0f, 0.0f};
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
// Camera2D.new(target:, offset:, zoom:, rotation:)
// ============================================================================

static mrb_value mrb_camera_initialize(mrb_state* mrb, mrb_value self) {
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "|H", &kwargs);

    // Create camera
    CameraHandle handle = CameraManager::instance().create();
    Camera2DState* cam = CameraManager::instance().get(handle);

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
                if (strcmp(key_name, "target") == 0) {
                    cam->target = extract_vec2(mrb, val);
                } else if (strcmp(key_name, "offset") == 0) {
                    cam->offset = extract_vec2(mrb, val);
                } else if (strcmp(key_name, "zoom") == 0) {
                    cam->zoom = static_cast<float>(mrb_as_float(mrb,val));
                } else if (strcmp(key_name, "rotation") == 0) {
                    cam->rotation = static_cast<float>(mrb_as_float(mrb,val));
                }
            }
        }
    }

    // Attach handle to Ruby object
    CameraData* data = static_cast<CameraData*>(mrb_malloc(mrb, sizeof(CameraData)));
    data->handle = handle;
    mrb_data_init(self, data, &camera_data_type);

    return self;
}

// ============================================================================
// Property Getters
// ============================================================================

// camera.target
static mrb_value mrb_camera_target(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return create_vec2(mrb, cam->target.x, cam->target.y);
}

// camera.offset
static mrb_value mrb_camera_offset(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return create_vec2(mrb, cam->offset.x, cam->offset.y);
}

// camera.zoom
static mrb_value mrb_camera_zoom(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return mrb_float_value(mrb, cam->zoom);
}

// camera.rotation
static mrb_value mrb_camera_rotation(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return mrb_float_value(mrb, cam->rotation);
}

// ============================================================================
// Property Setters
// ============================================================================

// camera.target = vec2
static mrb_value mrb_camera_set_target(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    cam->target = extract_vec2(mrb, val);
    cam->dirty = true;
    return val;
}

// camera.offset = vec2
static mrb_value mrb_camera_set_offset(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    cam->offset = extract_vec2(mrb, val);
    cam->dirty = true;
    return val;
}

// camera.zoom = float
static mrb_value mrb_camera_set_zoom(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    cam->zoom = static_cast<float>(val);
    cam->dirty = true;
    return mrb_float_value(mrb, val);
}

// camera.rotation = float
static mrb_value mrb_camera_set_rotation(mrb_state* mrb, mrb_value self) {
    mrb_float val;
    mrb_get_args(mrb, "f", &val);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    cam->rotation = static_cast<float>(val);
    cam->dirty = true;
    return mrb_float_value(mrb, val);
}

// ============================================================================
// Follow System
// ============================================================================

// camera.follow(target, smoothing:, deadzone:)
static mrb_value mrb_camera_follow(mrb_state* mrb, mrb_value self) {
    mrb_value target = mrb_nil_value();
    mrb_value kwargs = mrb_nil_value();
    mrb_get_args(mrb, "|oH", &target, &kwargs);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    // Set follow target (nil to stop following)
    cam->follow_target = target;

    // Reset follow parameters
    cam->smoothing = 0.0f;
    cam->has_deadzone = false;

    // Parse optional keyword arguments
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
                if (strcmp(key_name, "smoothing") == 0) {
                    cam->smoothing = static_cast<float>(mrb_as_float(mrb,val));
                } else if (strcmp(key_name, "deadzone") == 0) {
                    cam->deadzone = extract_rect(mrb, val);
                    cam->has_deadzone = true;
                }
            }
        }
    }

    cam->dirty = true;
    return self;
}

// ============================================================================
// Bounds
// ============================================================================

// camera.bounds = rect (or nil)
static mrb_value mrb_camera_set_bounds(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    if (mrb_nil_p(val)) {
        cam->has_bounds = false;
    } else {
        cam->bounds = extract_rect(mrb, val);
        cam->has_bounds = true;
    }

    cam->dirty = true;
    return val;
}

// camera.bounds
static mrb_value mrb_camera_bounds(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam || !cam->has_bounds) return mrb_nil_value();

    RClass* rect_class = mrb_class_get(mrb, "Rect");
    mrb_value args[4] = {
        mrb_float_value(mrb, cam->bounds.x),
        mrb_float_value(mrb, cam->bounds.y),
        mrb_float_value(mrb, cam->bounds.width),
        mrb_float_value(mrb, cam->bounds.height)
    };
    return mrb_obj_new(mrb, rect_class, 4, args);
}

// ============================================================================
// Screen Shake
// ============================================================================

// camera.shake(strength:, duration:, frequency:)
static mrb_value mrb_camera_shake(mrb_state* mrb, mrb_value self) {
    mrb_value kwargs;
    mrb_get_args(mrb, "H", &kwargs);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    // Default values
    float strength = 5.0f;
    float duration = 0.3f;
    float frequency = 30.0f;

    // Parse keyword arguments
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
            if (strcmp(key_name, "strength") == 0) {
                strength = static_cast<float>(mrb_as_float(mrb,val));
            } else if (strcmp(key_name, "duration") == 0) {
                duration = static_cast<float>(mrb_as_float(mrb,val));
            } else if (strcmp(key_name, "frequency") == 0) {
                frequency = static_cast<float>(mrb_as_float(mrb,val));
            }
        }
    }

    cam->shake_strength = strength;
    cam->shake_duration = duration;
    cam->shake_time_remaining = duration;
    cam->shake_frequency = frequency;

    return self;
}

// ============================================================================
// Coordinate Helpers
// ============================================================================

// camera.world_to_screen(vec2)
static mrb_value mrb_camera_world_to_screen(mrb_state* mrb, mrb_value self) {
    mrb_value pos;
    mrb_get_args(mrb, "o", &pos);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    Vec2 world_pos = extract_vec2(mrb, pos);

    // Build raylib camera for GetWorldToScreen2D
    ::Camera2D raylib_cam = {};
    raylib_cam.target = {cam->target.x, cam->target.y};
    raylib_cam.offset = {cam->offset.x + cam->shake_offset.x,
                         cam->offset.y + cam->shake_offset.y};
    raylib_cam.rotation = cam->rotation;
    raylib_cam.zoom = cam->zoom;

    Vector2 screen_pos = GetWorldToScreen2D({world_pos.x, world_pos.y}, raylib_cam);
    return create_vec2(mrb, screen_pos.x, screen_pos.y);
}

// camera.screen_to_world(vec2)
static mrb_value mrb_camera_screen_to_world(mrb_state* mrb, mrb_value self) {
    mrb_value pos;
    mrb_get_args(mrb, "o", &pos);

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    Vec2 screen_pos = extract_vec2(mrb, pos);

    // Build raylib camera for GetScreenToWorld2D
    ::Camera2D raylib_cam = {};
    raylib_cam.target = {cam->target.x, cam->target.y};
    raylib_cam.offset = {cam->offset.x + cam->shake_offset.x,
                         cam->offset.y + cam->shake_offset.y};
    raylib_cam.rotation = cam->rotation;
    raylib_cam.zoom = cam->zoom;

    Vector2 world_pos = GetScreenToWorld2D({screen_pos.x, screen_pos.y}, raylib_cam);
    return create_vec2(mrb, world_pos.x, world_pos.y);
}

// ============================================================================
// Scoped Usage (camera.use { block })
// ============================================================================

static mrb_value mrb_camera_use(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Block required for camera.use");
        return mrb_nil_value();
    }

    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();

    // Build raylib camera with shake offset applied
    ::Camera2D raylib_cam = {};
    raylib_cam.target = {cam->target.x, cam->target.y};
    raylib_cam.offset = {cam->offset.x + cam->shake_offset.x,
                         cam->offset.y + cam->shake_offset.y};
    raylib_cam.rotation = cam->rotation;
    raylib_cam.zoom = cam->zoom;

    // Enter camera mode
    BeginMode2D(raylib_cam);

    // Yield to block (protected to ensure EndMode2D is called)
    mrb_value result = mrb_yield(mrb, block, mrb_nil_value());

    // Exit camera mode
    EndMode2D();

    // Check for exception after EndMode2D
    if (mrb->exc) {
        // Re-raise the exception
        mrb_exc_raise(mrb, mrb_obj_value(mrb->exc));
    }

    return result;
}

// ============================================================================
// Class Methods
// ============================================================================

// Camera2D.current = camera
static mrb_value mrb_camera_class_set_current(mrb_state* mrb, mrb_value klass) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);

    if (mrb_nil_p(val)) {
        CameraManager::instance().set_current(INVALID_CAMERA_HANDLE);
    } else {
        CameraData* data = get_camera_data(mrb, val);
        if (data) {
            CameraManager::instance().set_current(data->handle);
        }
    }

    // Store in class variable for Ruby access
    mrb_cv_set(mrb, klass, mrb_intern_cstr(mrb, "@@current"), val);
    return val;
}

// Camera2D.current
static mrb_value mrb_camera_class_current(mrb_state* mrb, mrb_value klass) {
    mrb_sym sym = mrb_intern_cstr(mrb, "@@current");
    if (mrb_cv_defined(mrb, klass, sym)) {
        return mrb_cv_get(mrb, klass, sym);
    }
    return mrb_nil_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_camera(mrb_state* mrb) {
    // Camera2D class (top-level)
    RClass* camera_class = mrb_define_class(mrb, "Camera2D", mrb->object_class);
    MRB_SET_INSTANCE_TT(camera_class, MRB_TT_CDATA);

    // Initialize class variable (mrb_cv_set needs mrb_value, not RClass*)
    mrb_value camera_class_val = mrb_obj_value(camera_class);
    mrb_cv_set(mrb, camera_class_val, mrb_intern_cstr(mrb, "@@current"), mrb_nil_value());

    // Constructor
    mrb_define_method(mrb, camera_class, "initialize", mrb_camera_initialize, MRB_ARGS_OPT(1));

    // Property getters
    mrb_define_method(mrb, camera_class, "target", mrb_camera_target, MRB_ARGS_NONE());
    mrb_define_method(mrb, camera_class, "offset", mrb_camera_offset, MRB_ARGS_NONE());
    mrb_define_method(mrb, camera_class, "zoom", mrb_camera_zoom, MRB_ARGS_NONE());
    mrb_define_method(mrb, camera_class, "rotation", mrb_camera_rotation, MRB_ARGS_NONE());

    // Property setters
    mrb_define_method(mrb, camera_class, "target=", mrb_camera_set_target, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, camera_class, "offset=", mrb_camera_set_offset, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, camera_class, "zoom=", mrb_camera_set_zoom, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, camera_class, "rotation=", mrb_camera_set_rotation, MRB_ARGS_REQ(1));

    // Follow system
    mrb_define_method(mrb, camera_class, "follow", mrb_camera_follow, MRB_ARGS_OPT(2));

    // Bounds
    mrb_define_method(mrb, camera_class, "bounds", mrb_camera_bounds, MRB_ARGS_NONE());
    mrb_define_method(mrb, camera_class, "bounds=", mrb_camera_set_bounds, MRB_ARGS_REQ(1));

    // Shake
    mrb_define_method(mrb, camera_class, "shake", mrb_camera_shake, MRB_ARGS_REQ(1));

    // Coordinate helpers
    mrb_define_method(mrb, camera_class, "world_to_screen", mrb_camera_world_to_screen, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, camera_class, "screen_to_world", mrb_camera_screen_to_world, MRB_ARGS_REQ(1));

    // Scoped usage
    mrb_define_method(mrb, camera_class, "use", mrb_camera_use, MRB_ARGS_BLOCK());

    // Class methods
    mrb_define_class_method(mrb, camera_class, "current", mrb_camera_class_current, MRB_ARGS_NONE());
    mrb_define_class_method(mrb, camera_class, "current=", mrb_camera_class_set_current, MRB_ARGS_REQ(1));
}

} // namespace bindings
} // namespace gmr
