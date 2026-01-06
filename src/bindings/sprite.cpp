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

/// @class Sprite
/// @description A drawable 2D sprite with built-in transform properties.
///   Sprites combine a texture with position, rotation, scale, and origin for easy rendering.
///   By default, draw order determines layering (later drawn = on top). Set an explicit z value
///   to override this behavior for specific sprites.
/// @example # Complete game entity with sprite, animation, and collision
///   class Player
///     attr_reader :sprite
///
///     def initialize(x, y)
///       @texture = GMR::Graphics::Texture.load("assets/player.png")
///       @sprite = Sprite.new(@texture)
///       @sprite.x = x
///       @sprite.y = y
///       @sprite.source_rect = Rect.new(0, 0, 32, 48)
///       @sprite.center_origin
///       @sprite.z = 10  # Above background, below UI
///
///       @velocity = Vec2.new(0, 0)
///       @facing_right = true
///       setup_animations
///     end
///
///     def setup_animations
///       @animations = {
///         idle: GMR::SpriteAnimation.new(@sprite, frames: 0..3, fps: 8, columns: 8),
///         run: GMR::SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
///         jump: GMR::SpriteAnimation.new(@sprite, frames: 16..18, fps: 10, loop: false, columns: 8)
///       }
///       @animations[:idle].play
///     end
///
///     def update(dt)
///       # Flip sprite based on direction
///       @sprite.flip_x = !@facing_right
///
///       # Update position
///       @sprite.x += @velocity.x * dt
///       @sprite.y += @velocity.y * dt
///     end
///
///     def draw
///       @sprite.draw
///     end
///
///     def bounds
///       Rect.new(@sprite.x - 16, @sprite.y - 24, 32, 48)
///     end
///   end
/// @example # Particle effect with many sprites
///   class ParticleSystem
///     def initialize(x, y, count: 50)
///       @texture = GMR::Graphics::Texture.load("assets/particle.png")
///       @particles = count.times.map do
///         p = Sprite.new(@texture)
///         p.x = x
///         p.y = y
///         p.alpha = 1.0
///         p.center_origin
///         p.scale = GMR::Math.random(0.3, 1.0)
///         { sprite: p, vx: GMR::Math.random(-100, 100), vy: GMR::Math.random(-150, -50), life: GMR::Math.random(0.5, 1.5) }
///       end
///     end
///
///     def update(dt)
///       @particles.each do |p|
///         p[:sprite].x += p[:vx] * dt
///         p[:sprite].y += p[:vy] * dt
///         p[:vy] += 200 * dt  # Gravity
///         p[:life] -= dt
///         p[:sprite].alpha = [p[:life] / 0.5, 1.0].min
///         p[:sprite].visible = p[:life] > 0
///       end
///     end
///
///     def draw
///       @particles.each { |p| p[:sprite].draw if p[:sprite].visible }
///     end
///   end
/// @example # Sprite layering with z-index for isometric game
///   class GameWorld
///     def initialize
///       @floor = Sprite.new(floor_tex)
///       @floor.z = 0  # Always bottom layer
///
///       @entities = []
///       @entities << create_tree(100, 200)
///       @entities << create_tree(150, 180)
///       @entities << @player
///     end
///
///     def draw
///       @floor.draw
///       # Sort entities by y position for proper layering
///       @entities.sort_by! { |e| e.sprite.y }
///       @entities.each { |e| e.sprite.z = e.sprite.y; e.draw }
///     end
///   end

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

/// @method initialize
/// @description Create a new Sprite from a texture with optional initial values.
/// @param texture [Texture] The texture to use for this sprite
/// @param x [Float] Initial X position (default: 0)
/// @param y [Float] Initial Y position (default: 0)
/// @param rotation [Float] Initial rotation in degrees (default: 0)
/// @param scale_x [Float] Initial X scale (default: 1.0)
/// @param scale_y [Float] Initial Y scale (default: 1.0)
/// @param z [Float] Explicit z-index for layering (default: nil, uses draw order)
/// @param source_rect [Rect] Region of texture to draw (default: entire texture)
/// @returns [Sprite] The new sprite
/// @example sprite = Sprite.new(texture)
/// @example sprite = Sprite.new(texture, x: 100, y: 50, rotation: 45)
/// @example sprite = Sprite.new(spritesheet, source_rect: Rect.new(0, 0, 32, 32))
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

/// @method x
/// @description Get the X position of the sprite.
/// @returns [Float] The X position
/// @example x_pos = sprite.x
static mrb_value mrb_sprite_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->position.x);
}

/// @method y
/// @description Get the Y position of the sprite.
/// @returns [Float] The Y position
/// @example y_pos = sprite.y
static mrb_value mrb_sprite_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->position.y);
}

/// @method x=
/// @description Set the X position of the sprite.
/// @param value [Float] The new X position
/// @returns [Float] The value that was set
/// @example sprite.x = 100
/// @example sprite.x += 5  # Move right by 5 pixels
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

/// @method y=
/// @description Set the Y position of the sprite.
/// @param value [Float] The new Y position
/// @returns [Float] The value that was set
/// @example sprite.y = 200
/// @example sprite.y += 10  # Move down by 10 pixels
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

/// @method position
/// @description Get the position as a Vec2.
/// @returns [Vec2] The position vector
/// @example pos = sprite.position
static mrb_value mrb_sprite_position(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return create_vec2(mrb, s->position.x, s->position.y);
}

/// @method position=
/// @description Set the position using a Vec2.
/// @param value [Vec2] The new position vector
/// @returns [Vec2] The value that was set
/// @example sprite.position = Vec2.new(100, 200)
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

/// @method rotation
/// @description Get the rotation in degrees.
/// @returns [Float] The rotation angle in degrees
/// @example angle = sprite.rotation
static mrb_value mrb_sprite_rotation(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->rotation * RAD_TO_DEG);
}

/// @method rotation=
/// @description Set the rotation in degrees. Positive values rotate clockwise.
/// @param value [Float] The new rotation angle in degrees
/// @returns [Float] The value that was set
/// @example sprite.rotation = 45
/// @example sprite.rotation += 90 * dt  # Rotate 90 degrees per second
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

/// @method scale_x
/// @description Get the X scale factor.
/// @returns [Float] The X scale (1.0 = normal size)
/// @example sx = sprite.scale_x
static mrb_value mrb_sprite_scale_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->scale.x);
}

/// @method scale_y
/// @description Get the Y scale factor.
/// @returns [Float] The Y scale (1.0 = normal size)
/// @example sy = sprite.scale_y
static mrb_value mrb_sprite_scale_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->scale.y);
}

/// @method scale_x=
/// @description Set the X scale factor. Values greater than 1 stretch, less than 1 shrink.
/// @param value [Float] The new X scale (1.0 = normal size)
/// @returns [Float] The value that was set
/// @example sprite.scale_x = 2.0  # Double width
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

/// @method scale_y=
/// @description Set the Y scale factor. Values greater than 1 stretch, less than 1 shrink.
/// @param value [Float] The new Y scale (1.0 = normal size)
/// @returns [Float] The value that was set
/// @example sprite.scale_y = 0.5  # Half height
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

/// @method origin_x
/// @description Get the X origin (pivot point) for rotation and scaling.
/// @returns [Float] The X origin offset in pixels
/// @example ox = sprite.origin_x
static mrb_value mrb_sprite_origin_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->origin.x);
}

/// @method origin_y
/// @description Get the Y origin (pivot point) for rotation and scaling.
/// @returns [Float] The Y origin offset in pixels
/// @example oy = sprite.origin_y
static mrb_value mrb_sprite_origin_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->origin.y);
}

/// @method origin_x=
/// @description Set the X origin (pivot point) for rotation and scaling.
/// @param value [Float] The X origin offset in pixels
/// @returns [Float] The value that was set
/// @example sprite.origin_x = 16  # Pivot 16px from left
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

/// @method origin_y=
/// @description Set the Y origin (pivot point) for rotation and scaling.
/// @param value [Float] The Y origin offset in pixels
/// @returns [Float] The value that was set
/// @example sprite.origin_y = 16  # Pivot 16px from top
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

/// @method origin
/// @description Get the origin (pivot point) as a Vec2.
/// @returns [Vec2] The origin vector
/// @example origin = sprite.origin
static mrb_value mrb_sprite_origin(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return create_vec2(mrb, s->origin.x, s->origin.y);
}

/// @method origin=
/// @description Set the origin (pivot point) using a Vec2.
/// @param value [Vec2] The new origin vector
/// @returns [Vec2] The value that was set
/// @example sprite.origin = Vec2.new(16, 16)
static mrb_value mrb_sprite_set_origin(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    s->origin = extract_vec2(mrb, val);
    return val;
}

/// @method center_origin
/// @description Set the origin to the center of the sprite, so it rotates and scales
///   around its center. Uses texture dimensions or source_rect if set.
/// @returns [Sprite] self for chaining
/// @example sprite.center_origin
/// @example sprite = Sprite.new(tex).center_origin  # Method chaining
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

/// @method z
/// @description Get the explicit z-index for layering. Returns nil if using automatic
///   draw order (the default). Higher z values render on top of lower values.
/// @returns [Float, nil] The z-index, or nil if using draw order
/// @example z = sprite.z
/// @example if sprite.z.nil?
///   puts "Using automatic draw order"
/// end
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

/// @method z=
/// @description Set an explicit z-index for layering, or nil to use draw order.
///   By default (nil), sprites are layered by draw order - later drawn sprites appear
///   on top. Setting an explicit z gives you precise control over layering.
/// @param value [Float, nil] The z-index (higher = on top), or nil for draw order
/// @returns [Float, nil] The value that was set
/// @example sprite.z = 10        # Always above sprites with z < 10
/// @example sprite.z = nil       # Use draw order instead
/// @example # Typical usage pattern
///   @background.z = 0       # Always at back
///   @enemies.each { |e| e.z = 5 }  # Mid-layer
///   @player.z = 10          # Player on top
///   @ui.z = 100             # UI always on top
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

/// @method color
/// @description Get the color tint as an RGBA array. White [255,255,255,255] means no tint.
/// @returns [Array<Integer>] RGBA color array [r, g, b, a]
/// @example r, g, b, a = sprite.color
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

/// @method color=
/// @description Set the color tint as an RGBA array. The sprite is multiplied by this color.
/// @param value [Array<Integer>] RGBA color array [r, g, b] or [r, g, b, a]
/// @returns [Array<Integer>] The value that was set
/// @example sprite.color = [255, 0, 0]        # Red tint
/// @example sprite.color = [255, 255, 255, 128]  # 50% transparent
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

/// @method alpha
/// @description Get the alpha (opacity) as a float from 0.0 (invisible) to 1.0 (opaque).
/// @returns [Float] The alpha value
/// @example a = sprite.alpha
static mrb_value mrb_sprite_alpha(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_float_value(mrb, s->color.a / 255.0f);
}

/// @method alpha=
/// @description Set the alpha (opacity) from 0.0 (invisible) to 1.0 (opaque).
/// @param value [Float] The new alpha value (0.0 to 1.0)
/// @returns [Float] The value that was set
/// @example sprite.alpha = 0.5  # 50% transparent
/// @example sprite.alpha = 0    # Invisible
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

/// @method flip_x
/// @description Check if the sprite is flipped horizontally.
/// @returns [Boolean] true if flipped horizontally
/// @example if sprite.flip_x
///   puts "Facing left"
/// end
static mrb_value mrb_sprite_flip_x(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_bool_value(s->flip_x);
}

/// @method flip_y
/// @description Check if the sprite is flipped vertically.
/// @returns [Boolean] true if flipped vertically
/// @example if sprite.flip_y
///   puts "Flipped upside down"
/// end
static mrb_value mrb_sprite_flip_y(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    if (!data) return mrb_nil_value();
    SpriteState* s = SpriteManager::instance().get(data->handle);
    if (!s) return mrb_nil_value();
    return mrb_bool_value(s->flip_y);
}

/// @method flip_x=
/// @description Set whether the sprite is flipped horizontally. Useful for facing direction.
/// @param value [Boolean] true to flip horizontally
/// @returns [Boolean] The value that was set
/// @example sprite.flip_x = velocity.x < 0  # Face movement direction
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

/// @method flip_y=
/// @description Set whether the sprite is flipped vertically.
/// @param value [Boolean] true to flip vertically
/// @returns [Boolean] The value that was set
/// @example sprite.flip_y = true
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

/// @method texture
/// @description Get the current texture.
/// @returns [Texture] The sprite's texture
/// @example tex = sprite.texture
static mrb_value mrb_sprite_texture(mrb_state* mrb, mrb_value self) {
    return mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "@texture"));
}

/// @method texture=
/// @description Set the texture to draw.
/// @param value [Texture] The new texture
/// @returns [Texture] The value that was set
/// @example sprite.texture = new_texture
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

/// @method source_rect
/// @description Get the source rectangle (region of texture to draw). Returns nil if
///   using the entire texture.
/// @returns [Rect, nil] The source rectangle, or nil if using full texture
/// @example rect = sprite.source_rect
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

/// @method source_rect=
/// @description Set the source rectangle to draw only part of the texture.
///   Useful for sprite sheets and animations. Set to nil to draw the full texture.
/// @param value [Rect, nil] The source rectangle, or nil for full texture
/// @returns [Rect, nil] The value that was set
/// @example # Animation frames from a sprite sheet
///   frame_width = 32
///   @sprite.source_rect = Rect.new(frame * frame_width, 0, frame_width, 32)
/// @example @sprite.source_rect = nil  # Use full texture
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

/// @method width
/// @description Get the width of the sprite (from source_rect or texture).
/// @returns [Integer] The width in pixels
/// @example w = sprite.width
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

/// @method height
/// @description Get the height of the sprite (from source_rect or texture).
/// @returns [Integer] The height in pixels
/// @example h = sprite.height
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

/// @method parent
/// @description Get the parent Transform2D. Returns nil if no parent is set.
/// @returns [Transform2D, nil] The parent transform, or nil if none
/// @example parent = sprite.parent
static mrb_value mrb_sprite_parent(mrb_state* mrb, mrb_value self) {
    return mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "@parent"));
}

/// @method parent=
/// @description Set a Transform2D as the parent. The sprite will transform relative to
///   the parent's world transform. Set to nil to remove the parent.
/// @param value [Transform2D, nil] The parent transform, or nil to clear
/// @returns [Transform2D, nil] The value that was set
/// @example # Sprite follows a transform
///   turret_base = Transform2D.new(x: 200, y: 200)
///   @gun_sprite = Sprite.new(gun_tex)
///   @gun_sprite.parent = turret_base
///   @gun_sprite.y = -20  # Offset from turret
///   turret_base.rotation += 1  # Gun rotates with base!
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

/// @method draw
/// @description Queue the sprite for rendering. Sprites are drawn in z-order after all
///   draw() calls complete. By default, draw order determines layering (later = on top).
///   Set sprite.z to override with an explicit z-index.
/// @returns [Sprite] self for chaining
/// @example # Simple drawing (draw order = z order)
///   @background.draw  # drawn first, appears behind
///   @player.draw      # drawn second, appears on top
/// @example # Method chaining
///   @player.draw
///   @enemy.draw
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

/// @method count
/// @description Get the total number of active sprites (class method).
/// @returns [Integer] The number of active sprites
/// @example puts "Active sprites: #{Sprite.count}"
static mrb_value mrb_sprite_class_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(SpriteManager::instance().count());
}

// ============================================================================
// Animation
// ============================================================================

/// @method play_animation
/// @description Convenience method to create and play a sprite animation.
///   Creates a GMR::SpriteAnimation, calls play, and returns it for chaining.
/// @param frames: [Array<Integer>, Range] Frame indices to cycle through
/// @param fps: [Float] Frames per second (default: 12)
/// @param loop: [Boolean] Whether to loop (default: true)
/// @param frame_width: [Integer] Width of each frame (optional)
/// @param frame_height: [Integer] Height of each frame (optional)
/// @param columns: [Integer] Frames per row in spritesheet (default: 1)
/// @returns [SpriteAnimation] The animation instance (already playing)
/// @example sprite.play_animation(frames: 0..5, fps: 10, loop: false)
///   .on_complete { sprite.state = :idle }
/// @example sprite.play_animation(frames: [0, 1, 2, 3], fps: 12)
static mrb_value mrb_sprite_play_animation(mrb_state* mrb, mrb_value self) {
    mrb_value kwargs;
    mrb_get_args(mrb, "H", &kwargs);

    // Get GMR::SpriteAnimation class
    RClass* gmr = mrb_module_get(mrb, "GMR");
    RClass* anim_class = mrb_class_get_under(mrb, gmr, "SpriteAnimation");

    // Create animation with sprite and options
    mrb_value args[2] = { self, kwargs };
    mrb_value anim = mrb_obj_new(mrb, anim_class, 2, args);

    // Call play on the animation
    mrb_funcall(mrb, anim, "play", 0);

    return anim;
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
    mrb_define_method(mrb, sprite_class, "origin", mrb_sprite_origin, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "origin=", mrb_sprite_set_origin, MRB_ARGS_REQ(1));
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

    // Animation
    mrb_define_method(mrb, sprite_class, "play_animation", mrb_sprite_play_animation, MRB_ARGS_REQ(1));

    // Class methods
    mrb_define_class_method(mrb, sprite_class, "count", mrb_sprite_class_count, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
