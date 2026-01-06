#include "gmr/bindings/camera.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/camera.hpp"
#include "gmr/types.hpp"
#include "gmr/scripting/helpers.hpp"
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

/// @class Camera2D
/// @description A 2D camera for scrolling, zooming, and rotating the view.
///   Supports smooth following, screen shake, bounds constraints, and coordinate conversion.
///   Use the camera.use { } block to render in camera space.
/// @example # Complete platformer camera with player following and bounds
///   class GameScene < GMR::Scene
///     def init
///       @player = Player.new(100, 300)
///       @world_bounds = Rect.new(0, 0, 3200, 600)  # Large level
///
///       # Set up camera centered on screen
///       @camera = Camera2D.new
///       @camera.offset = Vec2.new(GMR::Window.width / 2, GMR::Window.height / 2)
///       @camera.zoom = 1.0
///
///       # Follow player with smooth tracking and deadzone
///       @camera.follow(@player, smoothing: 0.08, deadzone: Rect.new(
///         GMR::Window.width / 2 - 50,
///         GMR::Window.height / 2 - 30,
///         100, 60
///       ))
///
///       # Constrain camera to world bounds
///       @camera.bounds = @world_bounds
///     end
///
///     def update(dt)
///       @player.update(dt)
///       @camera.update(dt)  # Updates follow smoothing
///     end
///
///     def draw
///       # Draw world in camera space
///       @camera.use do
///         draw_background
///         draw_tilemap
///         @player.draw
///         @enemies.each(&:draw)
///       end
///
///       # Draw HUD in screen space (outside camera.use)
///       draw_hud
///     end
///   end
/// @example # Camera effects: shake on damage, zoom for scope mode
///   class Player
///     def take_damage(amount)
///       @health -= amount
///       # Screen shake intensity based on damage
///       Camera2D.current.shake(strength: amount * 0.5, duration: 0.3)
///       GMR::Audio::Sound.play("assets/hit.wav")
///     end
///
///     def toggle_scope
///       @scoped = !@scoped
///       target_zoom = @scoped ? 2.0 : 1.0
///       # Smooth zoom transition
///       GMR::Tween.to(Camera2D.current, :zoom, target_zoom, duration: 0.25, ease: :out_cubic)
///     end
///   end
/// @example # Mouse-to-world coordinate conversion for point-and-click
///   def update(dt)
///     if GMR::Input.mouse_pressed?(:left)
///       # Convert screen mouse position to world coordinates
///       world_pos = @camera.screen_to_world(GMR::Input.mouse_x, GMR::Input.mouse_y)
///       @player.move_to(world_pos.x, world_pos.y)
///     end
///   end
/// @example # Multiple cameras for minimap
///   class GameScene < GMR::Scene
///     def init
///       @main_camera = Camera2D.new
///       @main_camera.offset = Vec2.new(400, 300)
///
///       @minimap_camera = Camera2D.new
///       @minimap_camera.zoom = 0.1  # Zoomed out for overview
///       @minimap_camera.offset = Vec2.new(700, 50)  # Top-right corner
///     end
///
///     def draw
///       # Main view
///       @main_camera.use do
///         draw_world
///       end
///
///       # Minimap overlay
///       GMR::Graphics.draw_rect(620, 10, 170, 130, [0, 0, 0, 150])
///       @minimap_camera.use do
///         draw_world_minimap
///       end
///     end
///   end

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
        mrb_value x = scripting::safe_method_call(mrb, val, "x");
        mrb_value y = scripting::safe_method_call(mrb, val, "y");
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
        mrb_value x = scripting::safe_method_call(mrb, val, "x");
        mrb_value y = scripting::safe_method_call(mrb, val, "y");
        mrb_value w = scripting::safe_method_call(mrb, val, "w");
        mrb_value h = scripting::safe_method_call(mrb, val, "h");
        return {static_cast<float>(mrb_as_float(mrb, x)),
                static_cast<float>(mrb_as_float(mrb, y)),
                static_cast<float>(mrb_as_float(mrb, w)),
                static_cast<float>(mrb_as_float(mrb, h))};
    }

    mrb_raise(mrb, E_TYPE_ERROR, "Expected Rect or object with x/y/w/h methods");
    return {0.0f, 0.0f, 0.0f, 0.0f};
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
// Camera2D.new(target:, offset:, zoom:, rotation:)
// ============================================================================

/// @method initialize
/// @description Create a new Camera2D with optional initial values.
/// @param target [Vec2] World position the camera looks at (default: 0,0)
/// @param offset [Vec2] Screen position offset, typically screen center (default: 0,0)
/// @param zoom [Float] Zoom level, 1.0 = normal (default: 1.0)
/// @param rotation [Float] Rotation in degrees (default: 0)
/// @returns [Camera2D] The new camera
/// @example cam = Camera2D.new
/// @example cam = Camera2D.new(target: Vec2.new(100, 100), zoom: 2.0)
/// @example cam = Camera2D.new(offset: Vec2.new(400, 300))  # Center on 800x600 screen
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

/// @method target
/// @description Get the world position the camera is looking at.
/// @returns [Vec2] The camera's target position
/// @example target = camera.target
static mrb_value mrb_camera_target(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return create_vec2(mrb, cam->target.x, cam->target.y);
}

/// @method offset
/// @description Get the screen position offset (where the target appears on screen).
///   Typically set to screen center for centered camera following.
/// @returns [Vec2] The camera's offset position
/// @example offset = camera.offset
static mrb_value mrb_camera_offset(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return create_vec2(mrb, cam->offset.x, cam->offset.y);
}

/// @method zoom
/// @description Get the zoom level. 1.0 = normal, 2.0 = 2x magnification, 0.5 = zoomed out.
/// @returns [Float] The zoom level
/// @example z = camera.zoom
static mrb_value mrb_camera_zoom(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return mrb_float_value(mrb, cam->zoom);
}

/// @method rotation
/// @description Get the camera rotation in degrees.
/// @returns [Float] The rotation angle
/// @example angle = camera.rotation
static mrb_value mrb_camera_rotation(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam) return mrb_nil_value();
    return mrb_float_value(mrb, cam->rotation);
}

// ============================================================================
// Property Setters
// ============================================================================

/// @method target=
/// @description Set the world position the camera looks at.
/// @param value [Vec2] The target position
/// @returns [Vec2] The value that was set
/// @example camera.target = Vec2.new(player.x, player.y)
/// @example camera.target = player.position
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

/// @method offset=
/// @description Set the screen position offset. The target appears at this screen position.
///   Set to screen center for centered following.
/// @param value [Vec2] The offset position
/// @returns [Vec2] The value that was set
/// @example camera.offset = Vec2.new(400, 300)  # Center on 800x600 screen
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

/// @method zoom=
/// @description Set the zoom level. 1.0 = normal, 2.0 = 2x magnification, 0.5 = zoomed out.
/// @param value [Float] The zoom level (must be > 0)
/// @returns [Float] The value that was set
/// @example camera.zoom = 2.0    # Zoom in 2x
/// @example camera.zoom = 0.5    # Zoom out (see more of the world)
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

/// @method rotation=
/// @description Set the camera rotation in degrees.
/// @param value [Float] The rotation angle in degrees
/// @returns [Float] The value that was set
/// @example camera.rotation = 45
/// @example camera.rotation += 10 * dt  # Rotate over time
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

/// @method follow
/// @description Configure the camera to follow a target object with optional smoothing and deadzone.
///   The target must respond to `position` (returning Vec2) or have `x`/`y` methods.
///   Call with nil to stop following.
/// @param target [Object, nil] Object with position/x/y to follow, or nil to stop
/// @param smoothing [Float] Smoothing factor 0-1 (0=instant, 0.1=smooth, default: 0)
/// @param deadzone [Rect] Rectangle where target can move without camera moving
/// @returns [Camera2D] self for chaining
/// @example # Simple follow
///   camera.follow(@player)
/// @example # Smooth follow with deadzone
///   camera.follow(@player, smoothing: 0.1, deadzone: Rect.new(380, 280, 40, 40))
/// @example # Stop following
///   camera.follow(nil)
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

/// @method bounds=
/// @description Set camera bounds to constrain movement within a world region.
///   The camera will not show areas outside these bounds. Set to nil to remove bounds.
/// @param value [Rect, nil] The world bounds, or nil to remove
/// @returns [Rect, nil] The value that was set
/// @example # Constrain to level size
///   camera.bounds = Rect.new(0, 0, level_width, level_height)
/// @example camera.bounds = nil  # No bounds
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

/// @method bounds
/// @description Get the current camera bounds. Returns nil if no bounds are set.
/// @returns [Rect, nil] The world bounds, or nil if unbounded
/// @example rect = camera.bounds
static mrb_value mrb_camera_bounds(mrb_state* mrb, mrb_value self) {
    CameraData* data = get_camera_data(mrb, self);
    Camera2DState* cam = CameraManager::instance().get(data->handle);
    if (!cam || !cam->has_bounds) return mrb_nil_value();

    RClass* gmr = get_gmr_module(mrb);
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
    RClass* rect_class = mrb_class_get_under(mrb, graphics, "Rect");
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

/// @method shake
/// @description Trigger a screen shake effect. The shake decays over the duration.
/// @param strength [Float] Maximum shake offset in pixels (default: 5.0)
/// @param duration [Float] How long the shake lasts in seconds (default: 0.3)
/// @param frequency [Float] Shake oscillation frequency in Hz (default: 30.0)
/// @returns [Camera2D] self for chaining
/// @example camera.shake(strength: 10, duration: 0.5)
/// @example camera.shake(strength: 3, duration: 0.2, frequency: 20)
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

/// @method world_to_screen
/// @description Convert a world position to screen coordinates.
///   Useful for placing UI elements relative to game objects.
/// @param position [Vec2] The world position
/// @returns [Vec2] The screen position
/// @example screen_pos = camera.world_to_screen(player.position)
/// @example health_bar_x = camera.world_to_screen(enemy.position).x
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

/// @method screen_to_world
/// @description Convert a screen position to world coordinates.
///   Useful for mouse picking and click-to-move.
/// @param position [Vec2] The screen position
/// @returns [Vec2] The world position
/// @example world_pos = camera.screen_to_world(GMR::Input.mouse_position)
/// @example click_target = camera.screen_to_world(Vec2.new(mouse_x, mouse_y))
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

/// @method use
/// @description Execute a block with this camera's transform applied. All drawing
///   within the block will be transformed by the camera (position, zoom, rotation).
///   The camera mode is automatically ended when the block completes.
/// @returns [Object] The return value of the block
/// @example camera.use do
///   draw_tilemap()
///   @player.draw
///   @enemies.each(&:draw)
/// end
/// @example # Nested cameras
///   world_camera.use do
///     draw_world()
///   end
///   # UI drawn outside camera (screen space)
///   draw_ui()
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

    // Yield to block - use safe_yield to catch exceptions
    // We must call EndMode2D regardless of whether an exception occurs
    mrb_value result = scripting::safe_yield(mrb, block, mrb_nil_value());

    // Exit camera mode (always called, even if block raised)
    EndMode2D();

    return result;
}

// ============================================================================
// Class Methods
// ============================================================================

/// @method current=
/// @description Set the current active camera (class method). This camera will be used
///   for sprite rendering and coordinate transformations.
/// @param value [Camera2D, nil] The camera to make current, or nil to clear
/// @returns [Camera2D, nil] The value that was set
/// @example Camera2D.current = @main_camera
/// @example Camera2D.current = nil  # No camera
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

/// @method current
/// @description Get the current active camera (class method). Returns nil if no camera is set.
/// @returns [Camera2D, nil] The current camera, or nil
/// @example cam = Camera2D.current
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
    // Camera2D class under GMR::Graphics
    RClass* gmr = get_gmr_module(mrb);
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
    RClass* camera_class = mrb_define_class_under(mrb, graphics, "Camera2D", mrb->object_class);
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
