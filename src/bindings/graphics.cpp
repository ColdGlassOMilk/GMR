#include "gmr/bindings/graphics.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "raylib.h"

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

// GMR::Graphics.clear(color)
static mrb_value mrb_graphics_clear(mrb_state* mrb, mrb_value) {
    mrb_value color_val;
    mrb_get_args(mrb, "A", &color_val);

    Color c = parse_color_value(mrb, color_val, State::instance().clear_color);
    ClearBackground(to_raylib(c));
    return mrb_nil_value();
}

// GMR::Graphics.draw_rect(x, y, w, h, color)
static mrb_value mrb_graphics_draw_rect(mrb_state* mrb, mrb_value) {
    mrb_int x, y, w, h;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiiA", &x, &y, &w, &h, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawRectangle(x, y, w, h, to_raylib(c));
    return mrb_nil_value();
}

// GMR::Graphics.draw_rect_outline(x, y, w, h, color)
static mrb_value mrb_graphics_draw_rect_outline(mrb_state* mrb, mrb_value) {
    mrb_int x, y, w, h;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiiA", &x, &y, &w, &h, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawRectangleLines(x, y, w, h, to_raylib(c));
    return mrb_nil_value();
}

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

// GMR::Graphics.draw_line(x1, y1, x2, y2, color)
static mrb_value mrb_graphics_draw_line(mrb_state* mrb, mrb_value) {
    mrb_int x1, y1, x2, y2;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiiA", &x1, &y1, &x2, &y2, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawLine(x1, y1, x2, y2, to_raylib(c));
    return mrb_nil_value();
}

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

// GMR::Graphics.draw_circle(x, y, radius, color)
static mrb_value mrb_graphics_draw_circle(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiA", &x, &y, &radius, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawCircle(x, y, static_cast<float>(radius), to_raylib(c));
    return mrb_nil_value();
}

// GMR::Graphics.draw_circle_outline(x, y, radius, color)
static mrb_value mrb_graphics_draw_circle_outline(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_value color_val;
    mrb_get_args(mrb, "iiiA", &x, &y, &radius, &color_val);

    Color c = parse_color_value(mrb, color_val, WHITE_COLOR);
    DrawCircleLines(x, y, static_cast<float>(radius), to_raylib(c));
    return mrb_nil_value();
}

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

static const mrb_data_type texture_data_type = {
    "GMR::Graphics::Texture", texture_free
};

// Helper to get TextureData from self
static TextureData* get_texture_data(mrb_state* mrb, mrb_value self) {
    return static_cast<TextureData*>(mrb_data_get_ptr(mrb, self, &texture_data_type));
}

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

// texture.width
static mrb_value mrb_texture_width(mrb_state* mrb, mrb_value self) {
    TextureData* data = get_texture_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    return mrb_fixnum_value(TextureManager::instance().get_width(data->handle));
}

// texture.height
static mrb_value mrb_texture_height(mrb_state* mrb, mrb_value self) {
    TextureData* data = get_texture_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);
    return mrb_fixnum_value(TextureManager::instance().get_height(data->handle));
}

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
}

} // namespace bindings
} // namespace gmr
