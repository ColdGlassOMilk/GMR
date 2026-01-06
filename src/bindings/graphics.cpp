#include "gmr/bindings/graphics.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/resources/tilemap_manager.hpp"
#include "gmr/draw_queue.hpp"
#include "raylib.h"
#include <cstring>

namespace gmr {
namespace bindings {

// Convert our Color to raylib Color
static ::Color to_raylib(const Color& c) {
    return ::Color{c.r, c.g, c.b, c.a};
}

// Default white color for when none is specified
static const Color WHITE_COLOR{255, 255, 255, 255};

// ============================================================================
// GMR::Graphics Module Functions (Stateless)
// ============================================================================

/// @module GMR::Graphics
/// @description Drawing primitives and texture management
/// @example # Complete HUD system with health bar, score, and minimap
///   class HUD
///     def initialize(player)
///       @player = player
///       @font = GMR::Graphics::Font.load("assets/fonts/pixel.ttf", 16)
///       @heart_icon = GMR::Graphics::Texture.load("assets/ui/heart.png")
///     end
///
///     def draw
///       # Health bar background
///       GMR::Graphics.draw_rect(20, 20, 204, 24, [40, 40, 40])
///       # Health bar fill (red to green gradient based on health)
///       health_pct = @player.health / @player.max_health.to_f
///       bar_color = [
///         ((1.0 - health_pct) * 255).to_i,
///         (health_pct * 255).to_i,
///         0
///       ]
///       GMR::Graphics.draw_rect(22, 22, (200 * health_pct).to_i, 20, bar_color)
///       GMR::Graphics.draw_rect_outline(20, 20, 204, 24, [200, 200, 200])
///
///       # Score display
///       GMR::Graphics.draw_text("Score: #{@player.score}", 650, 20, 24, [255, 255, 255])
///
///       # Lives display with icons
///       @player.lives.times do |i|
///         GMR::Graphics.draw_texture(@heart_icon, 20 + i * 30, 50)
///       end
///     end
///   end
/// @example # Debug visualization overlay
///   class DebugOverlay
///     def draw
///       return unless @debug_enabled
///
///       # FPS counter
///       fps = GMR::Time.fps
///       color = fps >= 55 ? [0, 255, 0] : (fps >= 30 ? [255, 255, 0] : [255, 0, 0])
///       GMR::Graphics.draw_text("FPS: #{fps}", 10, 10, 16, color)
///
///       # Draw collision boxes
///       @entities.each do |e|
///         bounds = e.bounds
///         GMR::Graphics.draw_rect_outline(bounds.x, bounds.y, bounds.w, bounds.h, [255, 0, 255])
///       end
///
///       # Draw entity positions
///       @entities.each do |e|
///         GMR::Graphics.draw_circle(e.x, e.y, 3, [255, 255, 0])
///       end
///     end
///   end
/// @example # Custom progress bar with rounded corners effect
///   def draw_progress_bar(x, y, width, height, progress, bg_color, fill_color)
///     # Background
///     GMR::Graphics.draw_rect(x, y, width, height, bg_color)
///     # Fill
///     fill_width = (width * progress).to_i
///     GMR::Graphics.draw_rect(x, y, fill_width, height, fill_color) if fill_width > 0
///     # Border
///     GMR::Graphics.draw_rect_outline(x, y, width, height, [100, 100, 100])
///   end

/// @function clear
/// @description Clear the screen with a solid color
/// @param color [Color] The background color
/// @returns [nil]
/// @example GMR::Graphics.clear([20, 20, 40])
// GMR::Graphics.clear(color)
static mrb_value mrb_graphics_clear(mrb_state* mrb, mrb_value) {
    mrb_value color_val;
    mrb_get_args(mrb, "A", &color_val);

    Color c = parse_color_value(mrb, color_val, State::instance().clear_color);
    ClearBackground(to_raylib(c));
    return mrb_nil_value();
}

/// @function draw_rect
/// @description Draw a filled rectangle
/// @param x [Integer] X position (left edge)
/// @param y [Integer] Y position (top edge)
/// @param w [Integer] Width in pixels
/// @param h [Integer] Height in pixels
/// @param color [Color] Fill color
/// @returns [nil]
/// @example GMR::Graphics.draw_rect(100, 100, 50, 30, [255, 0, 0])
// GMR::Graphics.draw_rect(x, y, w, h, color)
static mrb_value mrb_graphics_draw_rect(mrb_state* mrb, mrb_value) {
    mrb_int x, y, w, h;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiiA", &x, &y, &w, &h, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawRectangle(x, y, w, h, to_raylib(c));
    return mrb_nil_value();
}

/// @function draw_rect_outline
/// @description Draw a rectangle outline (not filled)
/// @param x [Integer] X position (left edge)
/// @param y [Integer] Y position (top edge)
/// @param w [Integer] Width in pixels
/// @param h [Integer] Height in pixels
/// @param color [Color] Outline color
/// @returns [nil]
/// @example GMR::Graphics.draw_rect_outline(100, 100, 50, 30, [255, 255, 255])
// GMR::Graphics.draw_rect_outline(x, y, w, h, color)
static mrb_value mrb_graphics_draw_rect_outline(mrb_state* mrb, mrb_value) {
    mrb_int x, y, w, h;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiiA", &x, &y, &w, &h, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawRectangleLines(x, y, w, h, to_raylib(c));
    return mrb_nil_value();
}

/// @function draw_rect_rotated
/// @description Draw a filled rectangle rotated around its center
/// @param x [Float] X position (center)
/// @param y [Float] Y position (center)
/// @param w [Float] Width in pixels
/// @param h [Float] Height in pixels
/// @param angle [Float] Rotation angle in degrees
/// @param color [Color] Fill color
/// @returns [nil]
/// @example GMR::Graphics.draw_rect_rotated(160, 120, 40, 20, 45.0, [0, 255, 0])
// GMR::Graphics.draw_rect_rotated(x, y, w, h, angle, color)
static mrb_value mrb_graphics_draw_rect_rotated(mrb_state* mrb, mrb_value) {
    mrb_float x, y, w, h, angle;
    mrb_value color_val;
    mrb_get_args(mrb, "fffffA", &x, &y, &w, &h, &angle, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    Rectangle rec = {static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(w), static_cast<float>(h)};
    Vector2 origin = {static_cast<float>(w) / 2.0f, static_cast<float>(h) / 2.0f};
    DrawRectanglePro(rec, origin, static_cast<float>(angle), to_raylib(c));
    return mrb_nil_value();
}

/// @function draw_line
/// @description Draw a line between two points
/// @param x1 [Integer] Start X position
/// @param y1 [Integer] Start Y position
/// @param x2 [Integer] End X position
/// @param y2 [Integer] End Y position
/// @param color [Color] Line color
/// @returns [nil]
/// @example GMR::Graphics.draw_line(0, 0, 100, 100, [255, 255, 255])
// GMR::Graphics.draw_line(x1, y1, x2, y2, color)
static mrb_value mrb_graphics_draw_line(mrb_state* mrb, mrb_value) {
    mrb_int x1, y1, x2, y2;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiiA", &x1, &y1, &x2, &y2, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawLine(x1, y1, x2, y2, to_raylib(c));
    return mrb_nil_value();
}

/// @function draw_line_thick
/// @description Draw a thick line between two points
/// @param x1 [Float] Start X position
/// @param y1 [Float] Start Y position
/// @param x2 [Float] End X position
/// @param y2 [Float] End Y position
/// @param thickness [Float] Line thickness in pixels
/// @param color [Color] Line color
/// @returns [nil]
/// @example GMR::Graphics.draw_line_thick(0, 0, 100, 100, 3.0, [255, 200, 100])
// GMR::Graphics.draw_line_thick(x1, y1, x2, y2, thickness, color)
static mrb_value mrb_graphics_draw_line_thick(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2, thick;
    mrb_value color_val;
    mrb_get_args(mrb, "fffffA", &x1, &y1, &x2, &y2, &thick, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawLineEx(
        Vector2{static_cast<float>(x1), static_cast<float>(y1)},
        Vector2{static_cast<float>(x2), static_cast<float>(y2)},
        static_cast<float>(thick),
        to_raylib(c)
    );
    return mrb_nil_value();
}

/// @function draw_circle
/// @description Draw a filled circle
/// @param x [Integer] Center X position
/// @param y [Integer] Center Y position
/// @param radius [Integer] Circle radius in pixels
/// @param color [Color] Fill color
/// @returns [nil]
/// @example GMR::Graphics.draw_circle(160, 120, 25, [100, 200, 255])
// GMR::Graphics.draw_circle(x, y, radius, color)
static mrb_value mrb_graphics_draw_circle(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiA", &x, &y, &radius, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawCircle(x, y, static_cast<float>(radius), to_raylib(c));
    return mrb_nil_value();
}

/// @function draw_circle_outline
/// @description Draw a circle outline
/// @param x [Integer] Center X position
/// @param y [Integer] Center Y position
/// @param radius [Integer] Circle radius in pixels
/// @param color [Color] Outline color
/// @returns [nil]
/// @example GMR::Graphics.draw_circle_outline(160, 120, 25, [255, 255, 255])
// GMR::Graphics.draw_circle_outline(x, y, radius, color)
static mrb_value mrb_graphics_draw_circle_outline(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiA", &x, &y, &radius, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawCircleLines(x, y, static_cast<float>(radius), to_raylib(c));
    return mrb_nil_value();
}

/// @function draw_circle_gradient
/// @description Draw a circle with a radial gradient from inner to outer color
/// @param x [Integer] Center X position
/// @param y [Integer] Center Y position
/// @param radius [Integer] Circle radius in pixels
/// @param inner_color [Color] Color at center
/// @param outer_color [Color] Color at edge
/// @returns [nil]
/// @example GMR::Graphics.draw_circle_gradient(160, 120, 50, [255, 255, 255], [255, 0, 0, 0])
// GMR::Graphics.draw_circle_gradient(x, y, radius, inner_color, outer_color)
static mrb_value mrb_graphics_draw_circle_gradient(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_value inner_val, outer_val;
    mrb_get_args(mrb, "iiiAA", &x, &y, &radius, &inner_val, &outer_val);

    Color inner = parse_color_value(mrb, inner_val, WHITE_COLOR);
    Color outer = parse_color_value(mrb, outer_val, Color{0, 0, 0, 0});

    DrawCircleGradient(x, y, static_cast<float>(radius), to_raylib(inner), to_raylib(outer));
    return mrb_nil_value();
}

/// @function draw_triangle
/// @description Draw a filled triangle
/// @param x1 [Float] First vertex X
/// @param y1 [Float] First vertex Y
/// @param x2 [Float] Second vertex X
/// @param y2 [Float] Second vertex Y
/// @param x3 [Float] Third vertex X
/// @param y3 [Float] Third vertex Y
/// @param color [Color] Fill color
/// @returns [nil]
/// @example GMR::Graphics.draw_triangle(100, 50, 50, 150, 150, 150, [255, 0, 0])
// GMR::Graphics.draw_triangle(x1, y1, x2, y2, x3, y3, color)
static mrb_value mrb_graphics_draw_triangle(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2, x3, y3;
    mrb_value color_val;
    mrb_get_args(mrb, "ffffffA", &x1, &y1, &x2, &y2, &x3, &y3, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawTriangle(
        Vector2{static_cast<float>(x1), static_cast<float>(y1)},
        Vector2{static_cast<float>(x2), static_cast<float>(y2)},
        Vector2{static_cast<float>(x3), static_cast<float>(y3)},
        to_raylib(c)
    );
    return mrb_nil_value();
}

/// @function draw_triangle_outline
/// @description Draw a triangle outline
/// @param x1 [Float] First vertex X
/// @param y1 [Float] First vertex Y
/// @param x2 [Float] Second vertex X
/// @param y2 [Float] Second vertex Y
/// @param x3 [Float] Third vertex X
/// @param y3 [Float] Third vertex Y
/// @param color [Color] Outline color
/// @returns [nil]
/// @example GMR::Graphics.draw_triangle_outline(100, 50, 50, 150, 150, 150, [255, 255, 255])
// GMR::Graphics.draw_triangle_outline(x1, y1, x2, y2, x3, y3, color)
static mrb_value mrb_graphics_draw_triangle_outline(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2, x3, y3;
    mrb_value color_val;
    mrb_get_args(mrb, "ffffffA", &x1, &y1, &x2, &y2, &x3, &y3, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawTriangleLines(
        Vector2{static_cast<float>(x1), static_cast<float>(y1)},
        Vector2{static_cast<float>(x2), static_cast<float>(y2)},
        Vector2{static_cast<float>(x3), static_cast<float>(y3)},
        to_raylib(c)
    );
    return mrb_nil_value();
}

/// @function draw_text
/// @description Draw text at a position
/// @param text [String] The text to draw
/// @param x [Integer] X position (left edge)
/// @param y [Integer] Y position (top edge)
/// @param size [Integer] Font size in pixels
/// @param color [Color] Text color
/// @returns [nil]
/// @example GMR::Graphics.draw_text("Hello!", 10, 10, 20, [255, 255, 255])
// GMR::Graphics.draw_text(text, x, y, size, color)
static mrb_value mrb_graphics_draw_text(mrb_state* mrb, mrb_value) {
    const char* text;
    mrb_int x, y, size;
    mrb_value color_val;
    mrb_get_args(mrb, "ziiiA", &text, &x, &y, &size, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawText(text, x, y, size, to_raylib(c));
    return mrb_nil_value();
}

/// @function measure_text
/// @description Measure the width of text in pixels
/// @param text [String] The text to measure
/// @param size [Integer] Font size in pixels
/// @returns [Integer] Width in pixels
/// @example width = GMR::Graphics.measure_text("Hello", 20)
// GMR::Graphics.measure_text(text, size)
static mrb_value mrb_graphics_measure_text(mrb_state* mrb, mrb_value) {
    const char* text;
    mrb_int size;
    mrb_get_args(mrb, "zi", &text, &size);
    return mrb_fixnum_value(MeasureText(text, size));
}

// ============================================================================
// GMR::Graphics::Texture Class
// ============================================================================

/// @class Texture
/// @parent GMR::Graphics
/// @description A loaded image texture for drawing sprites and images

// Texture data structure - holds the handle
struct TextureData {
    TextureHandle handle;
};

// Data type for garbage collection
static void texture_free(mrb_state* mrb, void* ptr) {
    // Don't unload the texture - TextureManager owns it
    // Just free the wrapper struct
    mrb_free(mrb, ptr);
}

// NOTE: extern const - exported for type-safe mrb_data_get_ptr in other bindings
// (e.g., sprite.cpp needs to extract TextureHandle from Texture objects)
extern const mrb_data_type texture_data_type = {
    "GMR::Graphics::Texture", texture_free
};

// Helper to get TextureData from self
static TextureData* get_texture_data(mrb_state* mrb, mrb_value self) {
    return static_cast<TextureData*>(mrb_data_get_ptr(mrb, self, &texture_data_type));
}

/// @classmethod load
/// @description Load a texture from a file. Supports PNG, JPG, BMP, and other common formats.
/// @param path [String] Path to the image file (relative to game root)
/// @returns [Texture] The loaded texture object
/// @raises [RuntimeError] if the file cannot be loaded
/// @example sprite = GMR::Graphics::Texture.load("assets/player.png")
// GMR::Graphics::Texture.load(path) - class method
static mrb_value mrb_texture_load(mrb_state* mrb, mrb_value klass) {
    const char* path;
    mrb_get_args(mrb, "z", &path);

    TextureHandle handle = TextureManager::instance().load(path);
    if (handle == INVALID_HANDLE) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load texture: %s", path);
        return mrb_nil_value();
    }

    // Create new instance
    RClass* texture_class = mrb_class_ptr(klass);
    mrb_value obj = mrb_obj_new(mrb, texture_class, 0, nullptr);

    // Allocate and set data
    TextureData* data = static_cast<TextureData*>(mrb_malloc(mrb, sizeof(TextureData)));
    data->handle = handle;
    mrb_data_init(obj, data, &texture_data_type);

    return obj;
}

/// @method width
/// @description Get the texture width in pixels
/// @returns [Integer] Width in pixels
/// @example puts sprite.width
// texture.width
static mrb_value mrb_texture_width(mrb_state* mrb, mrb_value self) {
    TextureData* data = get_texture_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    return mrb_fixnum_value(TextureManager::instance().get_width(data->handle));
}

/// @method height
/// @description Get the texture height in pixels
/// @returns [Integer] Height in pixels
/// @example puts sprite.height
// texture.height
static mrb_value mrb_texture_height(mrb_state* mrb, mrb_value self) {
    TextureData* data = get_texture_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    return mrb_fixnum_value(TextureManager::instance().get_height(data->handle));
}

/// @method draw
/// @description Draw the texture at a position, optionally with a color tint
/// @param x [Integer] X position (left edge)
/// @param y [Integer] Y position (top edge)
/// @param color [Color] (optional, default: [255, 255, 255]) Color tint (multiplied with texture)
/// @returns [nil]
/// @example sprite.draw(100, 100)
// texture.draw(x, y) or texture.draw(x, y, color)
static mrb_value mrb_texture_draw(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_value color_val = mrb_nil_value();
    mrb_int argc = mrb_get_args(mrb, "ii|A", &x, &y, &color_val);

    TextureData* data = get_texture_data(mrb, self);
    if (!data) return mrb_nil_value();

    Color c = (argc > 2) ? parse_color_value(mrb, color_val, WHITE_COLOR) : WHITE_COLOR;

    if (auto* texture = TextureManager::instance().get(data->handle)) {
        DrawTexture(*texture, x, y, to_raylib(c));
    }
    return mrb_nil_value();
}

/// @method draw_ex
/// @description Draw the texture with rotation and scaling
/// @param x [Float] X position
/// @param y [Float] Y position
/// @param rotation [Float] Rotation angle in degrees
/// @param scale [Float] Scale multiplier (1.0 = original size)
/// @param color [Color] (optional, default: [255, 255, 255]) Color tint
/// @returns [nil]
/// @example sprite.draw_ex(160, 120, 45.0, 2.0)
// texture.draw_ex(x, y, rotation, scale) or texture.draw_ex(x, y, rotation, scale, color)
static mrb_value mrb_texture_draw_ex(mrb_state* mrb, mrb_value self) {
    mrb_float x, y, rotation, scale;
    mrb_value color_val = mrb_nil_value();
    mrb_int argc = mrb_get_args(mrb, "ffff|A", &x, &y, &rotation, &scale, &color_val);

    TextureData* data = get_texture_data(mrb, self);
    if (!data) return mrb_nil_value();

    Color c = (argc > 4) ? parse_color_value(mrb, color_val, WHITE_COLOR) : WHITE_COLOR;

    if (auto* texture = TextureManager::instance().get(data->handle)) {
        DrawTextureEx(*texture,
            Vector2{static_cast<float>(x), static_cast<float>(y)},
            static_cast<float>(rotation),
            static_cast<float>(scale),
            to_raylib(c)
        );
    }
    return mrb_nil_value();
}

/// @method draw_pro
/// @description Draw a portion of the texture to a destination rectangle with rotation. Origin is at center of destination.
/// @param sx [Float] Source X (top-left of region)
/// @param sy [Float] Source Y (top-left of region)
/// @param sw [Float] Source width
/// @param sh [Float] Source height
/// @param dx [Float] Destination X (center)
/// @param dy [Float] Destination Y (center)
/// @param dw [Float] Destination width
/// @param dh [Float] Destination height
/// @param rotation [Float] Rotation angle in degrees
/// @param color [Color] (optional, default: [255, 255, 255]) Color tint
/// @returns [nil]
/// @example sprite.draw_pro(0, 0, 32, 32, 160, 120, 64, 64, 0)
// texture.draw_pro(sx, sy, sw, sh, dx, dy, dw, dh, rotation) or with color
static mrb_value mrb_texture_draw_pro(mrb_state* mrb, mrb_value self) {
    mrb_float sx, sy, sw, sh, dx, dy, dw, dh, rotation;
    mrb_value color_val = mrb_nil_value();
    mrb_int argc = mrb_get_args(mrb, "fffffffff|A", &sx, &sy, &sw, &sh, &dx, &dy, &dw, &dh, &rotation, &color_val);

    TextureData* data = get_texture_data(mrb, self);
    if (!data) return mrb_nil_value();

    Color c = (argc > 9) ? parse_color_value(mrb, color_val, WHITE_COLOR) : WHITE_COLOR;

    if (auto* texture = TextureManager::instance().get(data->handle)) {
        Rectangle source = {static_cast<float>(sx), static_cast<float>(sy),
                           static_cast<float>(sw), static_cast<float>(sh)};
        Rectangle dest = {static_cast<float>(dx), static_cast<float>(dy),
                         static_cast<float>(dw), static_cast<float>(dh)};
        Vector2 origin = {static_cast<float>(dw) / 2.0f, static_cast<float>(dh) / 2.0f};

        DrawTexturePro(*texture, source, dest, origin, static_cast<float>(rotation), to_raylib(c));
    }
    return mrb_nil_value();
}

// ============================================================================
// GMR::Graphics::Tilemap Class
// ============================================================================

/// @class Tilemap
/// @parent GMR::Graphics
/// @description A tile-based map for efficient rendering of large worlds using a tileset texture

// Tilemap data structure - holds the handle
struct TilemapBindingData {
    TilemapHandle handle;
};

// Data type for garbage collection
static void tilemap_free(mrb_state* mrb, void* ptr) {
    if (ptr) {
        TilemapBindingData* data = static_cast<TilemapBindingData*>(ptr);
        TilemapManager::instance().destroy(data->handle);
        mrb_free(mrb, ptr);
    }
}

static const mrb_data_type tilemap_data_type = {
    "GMR::Graphics::Tilemap", tilemap_free
};

// Helper to get TilemapBindingData from self
static TilemapBindingData* get_tilemap_data(mrb_state* mrb, mrb_value self) {
    return static_cast<TilemapBindingData*>(mrb_data_get_ptr(mrb, self, &tilemap_data_type));
}

// Exposed helper for other modules (like Collision) to access tilemap data
TilemapData* get_tilemap_from_value(mrb_state* mrb, mrb_value tilemap_obj) {
    TilemapBindingData* data = static_cast<TilemapBindingData*>(
        mrb_data_get_ptr(mrb, tilemap_obj, &tilemap_data_type));
    if (!data) return nullptr;
    return TilemapManager::instance().get(data->handle);
}

/// @classmethod new
/// @description Create a new tilemap with the specified dimensions. All tiles are initialized to -1 (empty/transparent).
/// @param tileset [Texture] The tileset texture containing all tile graphics
/// @param tile_width [Integer] Width of each tile in pixels
/// @param tile_height [Integer] Height of each tile in pixels
/// @param width [Integer] Map width in tiles
/// @param height [Integer] Map height in tiles
/// @returns [Tilemap] The new tilemap object
/// @raises [ArgumentError] if dimensions are not positive
/// @example tileset = GMR::Graphics::Texture.load("assets/tiles.png")
/// map = GMR::Graphics::Tilemap.new(tileset, 16, 16, 100, 50)
// GMR::Graphics::Tilemap.new(tileset:, tile_width:, tile_height:, width:, height:)
// Using positional args: Tilemap.new(tileset, tile_width, tile_height, width, height)
static mrb_value mrb_tilemap_new(mrb_state* mrb, mrb_value klass) {
    mrb_value tileset_obj;
    mrb_int tile_width, tile_height, width, height;
    mrb_get_args(mrb, "oiiii", &tileset_obj, &tile_width, &tile_height, &width, &height);

    // Get texture handle from tileset object
    TextureData* tex_data = static_cast<TextureData*>(mrb_data_get_ptr(mrb, tileset_obj, &texture_data_type));
    if (!tex_data) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected Texture object for tileset");
        return mrb_nil_value();
    }

    // Validate dimensions
    if (width <= 0 || height <= 0 || tile_width <= 0 || tile_height <= 0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Tilemap dimensions must be positive");
        return mrb_nil_value();
    }

    // Create the tilemap
    TilemapHandle handle = TilemapManager::instance().create(
        static_cast<int32_t>(width),
        static_cast<int32_t>(height),
        static_cast<int32_t>(tile_width),
        static_cast<int32_t>(tile_height),
        tex_data->handle
    );

    // Create new Ruby instance
    RClass* tilemap_class = mrb_class_ptr(klass);
    mrb_value obj = mrb_obj_new(mrb, tilemap_class, 0, nullptr);

    // Allocate and set data
    TilemapBindingData* data = static_cast<TilemapBindingData*>(mrb_malloc(mrb, sizeof(TilemapBindingData)));
    data->handle = handle;
    mrb_data_init(obj, data, &tilemap_data_type);

    return obj;
}

// tilemap.width - returns width in tiles
static mrb_value mrb_tilemap_width(mrb_state* mrb, mrb_value self) {
    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
        return mrb_fixnum_value(tilemap->width);
    }
    return mrb_fixnum_value(0);
}

// tilemap.height - returns height in tiles
static mrb_value mrb_tilemap_height(mrb_state* mrb, mrb_value self) {
    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
        return mrb_fixnum_value(tilemap->height);
    }
    return mrb_fixnum_value(0);
}

// tilemap.tile_width
static mrb_value mrb_tilemap_tile_width(mrb_state* mrb, mrb_value self) {
    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
        return mrb_fixnum_value(tilemap->tile_width);
    }
    return mrb_fixnum_value(0);
}

// tilemap.tile_height
static mrb_value mrb_tilemap_tile_height(mrb_state* mrb, mrb_value self) {
    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
        return mrb_fixnum_value(tilemap->tile_height);
    }
    return mrb_fixnum_value(0);
}

// tilemap.get(x, y) - get tile index at position
static mrb_value mrb_tilemap_get(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_fixnum_value(-1);
    if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
        return mrb_fixnum_value(tilemap->get(static_cast<int32_t>(x), static_cast<int32_t>(y)));
    }
    return mrb_fixnum_value(-1);
}

// tilemap.set(x, y, tile_index) - set tile index at position
static mrb_value mrb_tilemap_set(mrb_state* mrb, mrb_value self) {
    mrb_int x, y, tile_index;
    mrb_get_args(mrb, "iii", &x, &y, &tile_index);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (data) {
        if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
            tilemap->set(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(tile_index));
        }
    }
    return mrb_nil_value();
}

// tilemap.fill(tile_index) - fill entire map with tile
static mrb_value mrb_tilemap_fill(mrb_state* mrb, mrb_value self) {
    mrb_int tile_index;
    mrb_get_args(mrb, "i", &tile_index);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (data) {
        if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
            tilemap->fill(static_cast<int32_t>(tile_index));
        }
    }
    return mrb_nil_value();
}

// tilemap.fill_rect(x, y, w, h, tile_index) - fill rectangular region
static mrb_value mrb_tilemap_fill_rect(mrb_state* mrb, mrb_value self) {
    mrb_int x, y, w, h, tile_index;
    mrb_get_args(mrb, "iiiii", &x, &y, &w, &h, &tile_index);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (data) {
        if (auto* tilemap = TilemapManager::instance().get(data->handle)) {
            tilemap->fill_rect(
                static_cast<int32_t>(x),
                static_cast<int32_t>(y),
                static_cast<int32_t>(w),
                static_cast<int32_t>(h),
                static_cast<int32_t>(tile_index)
            );
        }
    }
    return mrb_nil_value();
}

// tilemap.draw(x, y) or tilemap.draw(x, y, color)
// Draws all tiles to the screen using deferred rendering
static mrb_value mrb_tilemap_draw(mrb_state* mrb, mrb_value self) {
    mrb_float offset_x, offset_y;
    mrb_value color_val = mrb_nil_value();
    mrb_int argc = mrb_get_args(mrb, "ff|A", &offset_x, &offset_y, &color_val);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_nil_value();

    Color c = (argc > 2) ? parse_color_value(mrb, color_val, WHITE_COLOR) : WHITE_COLOR;
    DrawColor tint{c.r, c.g, c.b, c.a};

    // Queue tilemap for deferred rendering
    DrawQueue::instance().queue_tilemap(
        data->handle,
        static_cast<float>(offset_x),
        static_cast<float>(offset_y),
        tint
    );

    return mrb_nil_value();
}

// tilemap.draw_region(x, y, start_tile_x, start_tile_y, tiles_wide, tiles_tall) or with color
// Draws a portion of the tilemap (for scrolling/culling) using deferred rendering
static mrb_value mrb_tilemap_draw_region(mrb_state* mrb, mrb_value self) {
    mrb_float offset_x, offset_y;
    mrb_int start_x, start_y, tiles_w, tiles_h;
    mrb_value color_val = mrb_nil_value();
    mrb_int argc = mrb_get_args(mrb, "ffiiii|A", &offset_x, &offset_y, &start_x, &start_y, &tiles_w, &tiles_h, &color_val);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_nil_value();

    Color c = (argc > 6) ? parse_color_value(mrb, color_val, WHITE_COLOR) : WHITE_COLOR;
    DrawColor tint{c.r, c.g, c.b, c.a};

    // Queue tilemap region for deferred rendering
    DrawQueue::instance().queue_tilemap_region(
        data->handle,
        static_cast<float>(offset_x),
        static_cast<float>(offset_y),
        static_cast<int32_t>(start_x),
        static_cast<int32_t>(start_y),
        static_cast<int32_t>(tiles_w),
        static_cast<int32_t>(tiles_h),
        tint
    );

    return mrb_nil_value();
}

// tilemap.define_tile(tile_index, properties_hash) - define properties for a tile type
// Hash keys: :solid, :hazard, :platform, :ladder, :water, :slippery, :damage
static mrb_value mrb_tilemap_define_tile(mrb_state* mrb, mrb_value self) {
    mrb_int tile_index;
    mrb_value props_hash;
    mrb_get_args(mrb, "iH", &tile_index, &props_hash);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_nil_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_nil_value();

    // Parse hash into TileProperties
    TileProperties props;

    mrb_value solid_key = mrb_symbol_value(mrb_intern_lit(mrb, "solid"));
    mrb_value hazard_key = mrb_symbol_value(mrb_intern_lit(mrb, "hazard"));
    mrb_value platform_key = mrb_symbol_value(mrb_intern_lit(mrb, "platform"));
    mrb_value ladder_key = mrb_symbol_value(mrb_intern_lit(mrb, "ladder"));
    mrb_value water_key = mrb_symbol_value(mrb_intern_lit(mrb, "water"));
    mrb_value slippery_key = mrb_symbol_value(mrb_intern_lit(mrb, "slippery"));
    mrb_value damage_key = mrb_symbol_value(mrb_intern_lit(mrb, "damage"));

    props.set_solid(mrb_test(mrb_hash_get(mrb, props_hash, solid_key)));
    props.set_hazard(mrb_test(mrb_hash_get(mrb, props_hash, hazard_key)));
    props.set_platform(mrb_test(mrb_hash_get(mrb, props_hash, platform_key)));
    props.set_ladder(mrb_test(mrb_hash_get(mrb, props_hash, ladder_key)));
    props.set_water(mrb_test(mrb_hash_get(mrb, props_hash, water_key)));
    props.set_slippery(mrb_test(mrb_hash_get(mrb, props_hash, slippery_key)));

    mrb_value damage_val = mrb_hash_get(mrb, props_hash, damage_key);
    if (mrb_fixnum_p(damage_val)) {
        props.damage = static_cast<int16_t>(mrb_fixnum(damage_val));
    }

    tilemap->define_tile(static_cast<int32_t>(tile_index), props);

    return mrb_nil_value();
}

// tilemap.tile_properties(x, y) - get properties hash for tile at position
static mrb_value mrb_tilemap_tile_properties(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_nil_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_nil_value();

    const TileProperties* props = tilemap->get_props_at(
        static_cast<int32_t>(x), static_cast<int32_t>(y));
    if (!props) return mrb_nil_value();

    // Build hash from C++ properties
    mrb_value hash = mrb_hash_new(mrb);
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "solid")), mrb_bool_value(props->solid()));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "hazard")), mrb_bool_value(props->hazard()));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "platform")), mrb_bool_value(props->platform()));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "ladder")), mrb_bool_value(props->ladder()));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "water")), mrb_bool_value(props->water()));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "slippery")), mrb_bool_value(props->slippery()));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "damage")), mrb_fixnum_value(props->damage));

    return hash;
}

// tilemap.tile_property(x, y, key) - get a specific property for tile at position
static mrb_value mrb_tilemap_tile_property(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_sym key_sym;
    mrb_get_args(mrb, "iin", &x, &y, &key_sym);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_nil_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_nil_value();

    const TileProperties* props = tilemap->get_props_at(
        static_cast<int32_t>(x), static_cast<int32_t>(y));
    if (!props) return mrb_nil_value();

    // Check which property was requested
    const char* key_name = mrb_sym_name(mrb, key_sym);
    if (strcmp(key_name, "solid") == 0) return mrb_bool_value(props->solid());
    if (strcmp(key_name, "hazard") == 0) return mrb_bool_value(props->hazard());
    if (strcmp(key_name, "platform") == 0) return mrb_bool_value(props->platform());
    if (strcmp(key_name, "ladder") == 0) return mrb_bool_value(props->ladder());
    if (strcmp(key_name, "water") == 0) return mrb_bool_value(props->water());
    if (strcmp(key_name, "slippery") == 0) return mrb_bool_value(props->slippery());
    if (strcmp(key_name, "damage") == 0) return mrb_fixnum_value(props->damage);

    return mrb_nil_value();
}

// tilemap.solid?(x, y) - check if tile is solid
static mrb_value mrb_tilemap_solid(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_false_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_false_value();

    return mrb_bool_value(tilemap->is_solid(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// tilemap.wall?(x, y) - alias for solid?
static mrb_value mrb_tilemap_wall(mrb_state* mrb, mrb_value self) {
    return mrb_tilemap_solid(mrb, self);
}

// tilemap.hazard?(x, y) - check if tile is a hazard
static mrb_value mrb_tilemap_hazard(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_false_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_false_value();

    return mrb_bool_value(tilemap->is_hazard(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// tilemap.platform?(x, y) - check if tile is a platform (one-way)
static mrb_value mrb_tilemap_platform(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_false_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_false_value();

    return mrb_bool_value(tilemap->is_platform(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// tilemap.ladder?(x, y) - check if tile is a ladder
static mrb_value mrb_tilemap_ladder(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_false_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_false_value();

    return mrb_bool_value(tilemap->is_ladder(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// tilemap.water?(x, y) - check if tile is water
static mrb_value mrb_tilemap_water(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_false_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_false_value();

    return mrb_bool_value(tilemap->is_water(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// tilemap.slippery?(x, y) - check if tile is slippery (ice)
static mrb_value mrb_tilemap_slippery(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_false_value();

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_false_value();

    return mrb_bool_value(tilemap->is_slippery(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// tilemap.damage(x, y) - get damage value for tile
static mrb_value mrb_tilemap_damage(mrb_state* mrb, mrb_value self) {
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    TilemapBindingData* data = get_tilemap_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);

    auto* tilemap = TilemapManager::instance().get(data->handle);
    if (!tilemap) return mrb_fixnum_value(0);

    return mrb_fixnum_value(tilemap->get_damage(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// ============================================================================
// Registration
// ============================================================================

void register_graphics(mrb_state* mrb) {
    RClass* graphics = get_gmr_submodule(mrb, "Graphics");

    // Module functions (stateless drawing)
    mrb_define_module_function(mrb, graphics, "clear", mrb_graphics_clear, MRB_ARGS_REQ(1));

    mrb_define_module_function(mrb, graphics, "draw_rect", mrb_graphics_draw_rect, MRB_ARGS_REQ(5));
    mrb_define_module_function(mrb, graphics, "draw_rect_outline", mrb_graphics_draw_rect_outline, MRB_ARGS_REQ(5));
    mrb_define_module_function(mrb, graphics, "draw_rect_rotated", mrb_graphics_draw_rect_rotated, MRB_ARGS_REQ(6));

    mrb_define_module_function(mrb, graphics, "draw_line", mrb_graphics_draw_line, MRB_ARGS_REQ(5));
    mrb_define_module_function(mrb, graphics, "draw_line_thick", mrb_graphics_draw_line_thick, MRB_ARGS_REQ(6));

    mrb_define_module_function(mrb, graphics, "draw_circle", mrb_graphics_draw_circle, MRB_ARGS_REQ(4));
    mrb_define_module_function(mrb, graphics, "draw_circle_outline", mrb_graphics_draw_circle_outline, MRB_ARGS_REQ(4));
    mrb_define_module_function(mrb, graphics, "draw_circle_gradient", mrb_graphics_draw_circle_gradient, MRB_ARGS_REQ(5));

    mrb_define_module_function(mrb, graphics, "draw_triangle", mrb_graphics_draw_triangle, MRB_ARGS_REQ(7));
    mrb_define_module_function(mrb, graphics, "draw_triangle_outline", mrb_graphics_draw_triangle_outline, MRB_ARGS_REQ(7));

    mrb_define_module_function(mrb, graphics, "draw_text", mrb_graphics_draw_text, MRB_ARGS_REQ(5));
    mrb_define_module_function(mrb, graphics, "measure_text", mrb_graphics_measure_text, MRB_ARGS_REQ(2));

    // Texture class
    RClass* texture_class = mrb_define_class_under(mrb, graphics, "Texture", mrb->object_class);
    MRB_SET_INSTANCE_TT(texture_class, MRB_TT_CDATA);

    // Class method
    mrb_define_class_method(mrb, texture_class, "load", mrb_texture_load, MRB_ARGS_REQ(1));

    // Instance methods
    mrb_define_method(mrb, texture_class, "width", mrb_texture_width, MRB_ARGS_NONE());
    mrb_define_method(mrb, texture_class, "height", mrb_texture_height, MRB_ARGS_NONE());
    mrb_define_method(mrb, texture_class, "draw", mrb_texture_draw, MRB_ARGS_ARG(2, 1));
    mrb_define_method(mrb, texture_class, "draw_ex", mrb_texture_draw_ex, MRB_ARGS_ARG(4, 1));
    mrb_define_method(mrb, texture_class, "draw_pro", mrb_texture_draw_pro, MRB_ARGS_ARG(9, 1));

    // Tilemap class
    RClass* tilemap_class = mrb_define_class_under(mrb, graphics, "Tilemap", mrb->object_class);
    MRB_SET_INSTANCE_TT(tilemap_class, MRB_TT_CDATA);

    // Class method - new(tileset, tile_width, tile_height, width, height)
    mrb_define_class_method(mrb, tilemap_class, "new", mrb_tilemap_new, MRB_ARGS_REQ(5));

    // Instance methods
    mrb_define_method(mrb, tilemap_class, "width", mrb_tilemap_width, MRB_ARGS_NONE());
    mrb_define_method(mrb, tilemap_class, "height", mrb_tilemap_height, MRB_ARGS_NONE());
    mrb_define_method(mrb, tilemap_class, "tile_width", mrb_tilemap_tile_width, MRB_ARGS_NONE());
    mrb_define_method(mrb, tilemap_class, "tile_height", mrb_tilemap_tile_height, MRB_ARGS_NONE());
    mrb_define_method(mrb, tilemap_class, "get", mrb_tilemap_get, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "set", mrb_tilemap_set, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, tilemap_class, "fill", mrb_tilemap_fill, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, tilemap_class, "fill_rect", mrb_tilemap_fill_rect, MRB_ARGS_REQ(5));
    mrb_define_method(mrb, tilemap_class, "draw", mrb_tilemap_draw, MRB_ARGS_ARG(2, 1));
    mrb_define_method(mrb, tilemap_class, "draw_region", mrb_tilemap_draw_region, MRB_ARGS_ARG(6, 1));

    // Tile properties
    mrb_define_method(mrb, tilemap_class, "define_tile", mrb_tilemap_define_tile, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "tile_properties", mrb_tilemap_tile_properties, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "tile_property", mrb_tilemap_tile_property, MRB_ARGS_REQ(3));

    // Common property shortcuts
    mrb_define_method(mrb, tilemap_class, "solid?", mrb_tilemap_solid, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "wall?", mrb_tilemap_wall, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "hazard?", mrb_tilemap_hazard, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "platform?", mrb_tilemap_platform, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "ladder?", mrb_tilemap_ladder, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "water?", mrb_tilemap_water, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "slippery?", mrb_tilemap_slippery, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, tilemap_class, "damage", mrb_tilemap_damage, MRB_ARGS_REQ(2));
}

} // namespace bindings
} // namespace gmr
