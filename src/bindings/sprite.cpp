#include "gmr/bindings/sprite.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/types.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/variable.h>
#include <cstring>
#include <raylib.h>
#include <cmath>

// Error handling macros for fail-loud philosophy (per CONTRIBUTING.md)
#define GMR_REQUIRE_SPRITE_DATA(data) \
    if (!data) { \
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Sprite: internal data is null"); \
        return mrb_nil_value(); \
    }

#define GMR_REQUIRE_SPRITE_STATE(state, handle) \
    if (!state) { \
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Invalid Sprite handle %d: sprite may have been destroyed", handle); \
        return mrb_nil_value(); \
    }

namespace gmr {
namespace bindings {

/// @class Sprite
/// @description A drawable 2D sprite that references a Transform2D for spatial properties.
///   Sprites are pure rendering components - they reference a texture and a transform.
///   All spatial properties (position, rotation, scale, origin) are managed by the Transform2D.
///   By default, draw order determines layering (later drawn = on top). Set an explicit z value
///   to override this behavior for specific sprites.
/// @example # Complete game entity with sprite, animation, and collision
///   class Player
///     attr_reader :sprite, :transform
///
///     def initialize(x, y)
///       @texture = GMR::Graphics::Texture.load("assets/player.png")
///       @transform = Transform2D.new(x: x, y: y)
///       @sprite = Sprite.new(@texture, @transform)
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
///       # Update position via transform
///       @transform.x += @velocity.x * dt
///       @transform.y += @velocity.y * dt
///     end
///
///     def draw
///       @sprite.draw
///     end
///
///     def bounds
///       Rect.new(@transform.x - 16, @transform.y - 24, 32, 48)
///     end
///   end
/// @example # Particle effect with many sprites
///   class ParticleSystem
///     def initialize(x, y, count: 50)
///       @texture = GMR::Graphics::Texture.load("assets/particle.png")
///       @particles = count.times.map do
///         t = Transform2D.new(x: x, y: y, scale_x: GMR::Math.random(0.3, 1.0), scale_y: GMR::Math.random(0.3, 1.0))
///         t.center_origin
///         s = Sprite.new(@texture, t)
///         s.alpha = 1.0
///         { sprite: s, transform: t, vx: GMR::Math.random(-100, 100), vy: GMR::Math.random(-150, -50), life: GMR::Math.random(0.5, 1.5) }
///       end
///     end
///
///     def update(dt)
///       @particles.each do |p|
///         p[:transform].x += p[:vx] * dt
///         p[:transform].y += p[:vy] * dt
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
///       @floor_transform = Transform2D.new
///       @floor = Sprite.new(floor_tex, @floor_transform)
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
///       @entities.sort_by! { |e| e.transform.y }
///       @entities.each { |e| e.sprite.z = e.transform.y; e.draw }
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

// NOTE: extern const - exported for type-safe mrb_data_get_ptr in other bindings
// In C++, const at namespace scope has internal linkage by default, so we need
// explicit extern to allow other translation units to link against this symbol.
extern const mrb_data_type sprite_data_type = {
    "Sprite", sprite_free
};

static SpriteData* get_sprite_data(mrb_state* mrb, mrb_value self) {
    return static_cast<SpriteData*>(mrb_data_get_ptr(mrb, self, &sprite_data_type));
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

// Forward declaration for transform data type
extern const mrb_data_type transform_data_type;

// Helper to get TransformHandle from Ruby Transform2D object
static TransformHandle get_transform_handle(mrb_state* mrb, mrb_value val) {
    // Get the transform_data_type from the transform binding
    struct TransformData {
        TransformHandle handle;
    };

    // Use proper type checking via transform_data_type
    void* ptr = mrb_data_get_ptr(mrb, val, &transform_data_type);
    if (ptr) {
        TransformData* data = static_cast<TransformData*>(ptr);
        return data->handle;
    }
    return INVALID_HANDLE;
}

// Forward declaration for texture data type (defined in graphics.cpp)
extern const mrb_data_type texture_data_type;

// Helper to get TextureHandle from Ruby Texture object
static TextureHandle get_texture_handle_from_value(mrb_state* mrb, mrb_value texture_val) {
    struct TextureData {
        TextureHandle handle;
    };

    void* ptr = mrb_data_get_ptr(mrb, texture_val, &texture_data_type);
    if (ptr) {
        TextureData* data = static_cast<TextureData*>(ptr);
        return data->handle;
    }
    return INVALID_HANDLE;
}

// ============================================================================
// Sprite.new(texture, **opts)
// ============================================================================

/// @method initialize
/// @description Create a new sprite with a texture and transform.
/// @param texture [Texture] The texture to render
/// @param transform [Transform2D] The transform defining position, rotation, scale, and origin
/// @returns [Sprite] The new sprite
/// @example
///   transform = Transform2D.new(x: 100, y: 100, rotation: 45)
///   sprite = Sprite.new(my_texture, transform)
static mrb_value mrb_sprite_initialize(mrb_state* mrb, mrb_value self) {
    mrb_value texture_val;
    mrb_value transform_val;
    mrb_get_args(mrb, "oo", &texture_val, &transform_val);

    // Create sprite
    SpriteHandle handle = SpriteManager::instance().create();
    SpriteState* s = SpriteManager::instance().get(handle);

    // Get texture handle from Ruby Texture object
    s->texture = get_texture_handle_from_value(mrb, texture_val);
    if (s->texture == INVALID_HANDLE) {
        SpriteManager::instance().destroy(handle);
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Invalid or nil texture object passed to Sprite.new");
        return mrb_nil_value();
    }

    // Get transform handle
    s->transform = get_transform_handle(mrb, transform_val);
    if (s->transform == INVALID_HANDLE) {
        SpriteManager::instance().destroy(handle);
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Invalid or nil transform object passed to Sprite.new");
        return mrb_nil_value();
    }

    // Attach handle to Ruby object
    SpriteData* data = static_cast<SpriteData*>(mrb_malloc(mrb, sizeof(SpriteData)));
    data->handle = handle;
    mrb_data_init(self, data, &sprite_data_type);

    // Store references to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@texture"), texture_val);
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@transform"), transform_val);

    return self;
}

// ============================================================================
// Position Getters/Setters
// ============================================================================

// ============================================================================
// Transform
// ============================================================================

/// @method transform
/// @description Get the Transform2D handle associated with this sprite.
/// @returns [Transform2D] The sprite's transform
/// @example t = sprite.transform
///   t.x = 100
static mrb_value mrb_sprite_transform(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

    if (s->transform == INVALID_HANDLE) {
        return mrb_nil_value();
    }

    // Return the existing Transform2D Ruby object
    // We need to create a new Ruby wrapper for the existing handle
    RClass* gmr = get_gmr_module(mrb);
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
    RClass* transform_class = mrb_class_get_under(mrb, graphics, "Transform2D");

    // Access the external transform_data_type
    extern const mrb_data_type transform_data_type;

    struct TransformData {
        TransformHandle handle;
    };
    TransformData* tdata = static_cast<TransformData*>(mrb_malloc(mrb, sizeof(TransformData)));
    tdata->handle = s->transform;

    mrb_value transform_obj = mrb_obj_value(mrb_data_object_alloc(mrb, transform_class, tdata, &transform_data_type));

    return transform_obj;
}

/// @method transform=
/// @description Set the Transform2D for this sprite.
/// @param value [Transform2D] The transform to use
/// @returns [Transform2D] The value that was set
/// @example sprite.transform = Transform2D.new(x: 100, y: 200)
static mrb_value mrb_sprite_set_transform(mrb_state* mrb, mrb_value self) {
    mrb_value val;
    mrb_get_args(mrb, "o", &val);

    SpriteData* data = get_sprite_data(mrb, self);
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

    // Get TransformHandle from Transform2D object
    extern const mrb_data_type transform_data_type;
    struct TransformData {
        TransformHandle handle;
    };
    TransformData* tdata = static_cast<TransformData*>(mrb_data_get_ptr(mrb, val, &transform_data_type));

    if (!tdata) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected Transform2D object");
        return mrb_nil_value();
    }

    s->transform = tdata->handle;
    return val;
}

/// @method center_origin
/// @description Set the transform's origin to the center of the sprite, so it rotates and scales
///   around its center. Uses texture dimensions or source_rect if set.
/// @returns [Sprite] self for chaining
/// @example sprite.center_origin
/// @example sprite = Sprite.new(tex, transform).center_origin  # Method chaining
static mrb_value mrb_sprite_center_origin(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

    // Get transform
    auto* transform = TransformManager::instance().get(s->transform);
    if (!transform) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Sprite has no valid transform");
        return mrb_nil_value();
    }

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

    transform->origin.x = w / 2.0f;
    transform->origin.y = h / 2.0f;
    TransformManager::instance().mark_dirty(s->transform);

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

    s->color = parse_color_value(mrb, color_val, Color{255, 255, 255, 255});
    return color_val;
}

/// @method alpha
/// @description Get the alpha (opacity) as a float from 0.0 (invisible) to 1.0 (opaque).
/// @returns [Float] The alpha value
/// @example a = sprite.alpha
static mrb_value mrb_sprite_alpha(mrb_state* mrb, mrb_value self) {
    SpriteData* data = get_sprite_data(mrb, self);
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);
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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);
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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);
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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);
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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);
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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

    // Get texture handle from Ruby Texture object (stored in C DATA struct, not ivar)
    TextureHandle tex_handle = get_texture_handle_from_value(mrb, texture_val);
    if (tex_handle == INVALID_HANDLE) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Invalid or nil texture object");
        return mrb_nil_value();
    }
    s->texture = tex_handle;

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);
    if (!s->use_source_rect) return mrb_nil_value();  // Legitimate: returns nil if using full texture

    RClass* gmr = get_gmr_module(mrb);
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
    RClass* rect_class = mrb_class_get_under(mrb, graphics, "Rect");
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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

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
    GMR_REQUIRE_SPRITE_DATA(data);
    SpriteState* s = SpriteManager::instance().get(data->handle);
    GMR_REQUIRE_SPRITE_STATE(s, data->handle);

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
// Parent Hierarchy - REMOVED
// ============================================================================
// Parenting is now handled by Transform2D hierarchy directly.
// Use sprite.transform.parent = other_transform instead of sprite.parent.

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
    GMR_REQUIRE_SPRITE_DATA(data);

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

    // Call play on the animation (protected)
    scripting::safe_method_call(mrb, anim, "play");

    return anim;
}

// ============================================================================
// Registration
// ============================================================================

void register_sprite(mrb_state* mrb) {
    // Sprite class under GMR::Graphics
    RClass* gmr = get_gmr_module(mrb);
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");
    RClass* sprite_class = mrb_define_class_under(mrb, graphics, "Sprite", mrb->object_class);
    MRB_SET_INSTANCE_TT(sprite_class, MRB_TT_CDATA);

    // Constructor
    mrb_define_method(mrb, sprite_class, "initialize", mrb_sprite_initialize, MRB_ARGS_REQ(2));

    // Transform (replaces position, rotation, scale, origin)
    mrb_define_method(mrb, sprite_class, "transform", mrb_sprite_transform, MRB_ARGS_NONE());
    mrb_define_method(mrb, sprite_class, "transform=", mrb_sprite_set_transform, MRB_ARGS_REQ(1));
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

    // Draw
    mrb_define_method(mrb, sprite_class, "draw", mrb_sprite_draw, MRB_ARGS_NONE());

    // Animation
    mrb_define_method(mrb, sprite_class, "play_animation", mrb_sprite_play_animation, MRB_ARGS_REQ(1));

    // Class methods
    mrb_define_class_method(mrb, sprite_class, "count", mrb_sprite_class_count, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
