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

static mrb_value mrb_set_color(mrb_state* mrb, mrb_value) {
    mrb_value* argv;
    mrb_int argc;
    mrb_get_args(mrb, "*", &argv, &argc);
    
    auto& state = State::instance();
    state.current_color = parse_color(mrb, argv, argc, state.current_color);
    return mrb_nil_value();
}

static mrb_value mrb_set_clear_color(mrb_state* mrb, mrb_value) {
    mrb_value* argv;
    mrb_int argc;
    mrb_get_args(mrb, "*", &argv, &argc);
    
    auto& state = State::instance();
    state.clear_color = parse_color(mrb, argv, argc, state.clear_color);
    return mrb_nil_value();
}

static mrb_value mrb_set_alpha(mrb_state* mrb, mrb_value) {
    mrb_int a;
    mrb_get_args(mrb, "i", &a);
    State::instance().current_color.a = static_cast<uint8_t>(a);
    return mrb_nil_value();
}

static mrb_value mrb_clear_screen(mrb_state* mrb, mrb_value) {
    mrb_value* argv;
    mrb_int argc;
    mrb_get_args(mrb, "*", &argv, &argc);
    
    auto& state = State::instance();
    Color c = (argc == 0) ? state.clear_color : parse_color(mrb, argv, argc, state.clear_color);
    ClearBackground(to_raylib(c));
    return mrb_nil_value();
}

static mrb_value mrb_draw_rect(mrb_state* mrb, mrb_value) {
    mrb_int x, y, w, h;
    mrb_get_args(mrb, "iiii", &x, &y, &w, &h);
    DrawRectangle(x, y, w, h, to_raylib(State::instance().current_color));
    return mrb_nil_value();
}

static mrb_value mrb_draw_rect_lines(mrb_state* mrb, mrb_value) {
    mrb_int x, y, w, h;
    mrb_get_args(mrb, "iiii", &x, &y, &w, &h);
    DrawRectangleLines(x, y, w, h, to_raylib(State::instance().current_color));
    return mrb_nil_value();
}

static mrb_value mrb_draw_rect_rotate(mrb_state* mrb, mrb_value) {
    mrb_float x, y, w, h, angle;
    mrb_get_args(mrb, "fffff", &x, &y, &w, &h, &angle);
    
    Rectangle rec = {static_cast<float>(x), static_cast<float>(y), 
                     static_cast<float>(w), static_cast<float>(h)};
    Vector2 origin = {static_cast<float>(w) / 2.0f, static_cast<float>(h) / 2.0f};
    DrawRectanglePro(rec, origin, static_cast<float>(angle), to_raylib(State::instance().current_color));
    return mrb_nil_value();
}

static mrb_value mrb_draw_line(mrb_state* mrb, mrb_value) {
    mrb_int x1, y1, x2, y2;
    mrb_get_args(mrb, "iiii", &x1, &y1, &x2, &y2);
    DrawLine(x1, y1, x2, y2, to_raylib(State::instance().current_color));
    return mrb_nil_value();
}

static mrb_value mrb_draw_line_thick(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2, thick;
    mrb_get_args(mrb, "fffff", &x1, &y1, &x2, &y2, &thick);
    DrawLineEx(
        Vector2{static_cast<float>(x1), static_cast<float>(y1)},
        Vector2{static_cast<float>(x2), static_cast<float>(y2)},
        static_cast<float>(thick),
        to_raylib(State::instance().current_color)
    );
    return mrb_nil_value();
}

static mrb_value mrb_draw_circle(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_get_args(mrb, "iii", &x, &y, &radius);
    DrawCircle(x, y, radius, to_raylib(State::instance().current_color));
    return mrb_nil_value();
}

static mrb_value mrb_draw_circle_lines(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_get_args(mrb, "iii", &x, &y, &radius);
    DrawCircleLines(x, y, radius, to_raylib(State::instance().current_color));
    return mrb_nil_value();
}

static mrb_value mrb_draw_circle_gradient(mrb_state* mrb, mrb_value) {
    mrb_int x, y, radius;
    mrb_value inner_arr, outer_arr;
    mrb_get_args(mrb, "iiiAA", &x, &y, &radius, &inner_arr, &outer_arr);
    
    Color inner = parse_color(mrb, &inner_arr, 1, Color{255, 255, 255, 255});
    Color outer = parse_color(mrb, &outer_arr, 1, Color{0, 0, 0, 0});
    
    DrawCircleGradient(x, y, radius, to_raylib(inner), to_raylib(outer));
    return mrb_nil_value();
}

static mrb_value mrb_draw_triangle(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2, x3, y3;
    mrb_get_args(mrb, "ffffff", &x1, &y1, &x2, &y2, &x3, &y3);
    DrawTriangle(
        Vector2{static_cast<float>(x1), static_cast<float>(y1)},
        Vector2{static_cast<float>(x2), static_cast<float>(y2)},
        Vector2{static_cast<float>(x3), static_cast<float>(y3)},
        to_raylib(State::instance().current_color)
    );
    return mrb_nil_value();
}

static mrb_value mrb_draw_triangle_lines(mrb_state* mrb, mrb_value) {
    mrb_float x1, y1, x2, y2, x3, y3;
    mrb_get_args(mrb, "ffffff", &x1, &y1, &x2, &y2, &x3, &y3);
    DrawTriangleLines(
        Vector2{static_cast<float>(x1), static_cast<float>(y1)},
        Vector2{static_cast<float>(x2), static_cast<float>(y2)},
        Vector2{static_cast<float>(x3), static_cast<float>(y3)},
        to_raylib(State::instance().current_color)
    );
    return mrb_nil_value();
}

static mrb_value mrb_draw_text(mrb_state* mrb, mrb_value) {
    const char* text;
    mrb_int x, y, size;
    mrb_get_args(mrb, "ziii", &text, &x, &y, &size);
    DrawText(text, x, y, size, to_raylib(State::instance().current_color));
    return mrb_nil_value();
}

static mrb_value mrb_measure_text(mrb_state* mrb, mrb_value) {
    const char* text;
    mrb_int size;
    mrb_get_args(mrb, "zi", &text, &size);
    return mrb_fixnum_value(MeasureText(text, size));
}

// --- Texture functions (handle-based) ---
static mrb_value mrb_load_texture(mrb_state* mrb, mrb_value) {
    const char* path;
    mrb_get_args(mrb, "z", &path);
    
    TextureHandle handle = TextureManager::instance().load(path);
    if (handle == INVALID_HANDLE) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load texture: %s", path);
    }
    return mrb_fixnum_value(handle);
}

static mrb_value mrb_draw_texture(mrb_state* mrb, mrb_value) {
    mrb_int handle, x, y;
    mrb_get_args(mrb, "iii", &handle, &x, &y);
    
    if (auto* texture = TextureManager::instance().get(static_cast<TextureHandle>(handle))) {
        DrawTexture(*texture, x, y, to_raylib(State::instance().current_color));
    }
    return mrb_nil_value();
}

static mrb_value mrb_draw_texture_ex(mrb_state* mrb, mrb_value) {
    mrb_int handle;
    mrb_float x, y, rotation, scale;
    mrb_get_args(mrb, "iffff", &handle, &x, &y, &rotation, &scale);
    
    if (auto* texture = TextureManager::instance().get(static_cast<TextureHandle>(handle))) {
        DrawTextureEx(*texture, 
            Vector2{static_cast<float>(x), static_cast<float>(y)},
            static_cast<float>(rotation),
            static_cast<float>(scale),
            to_raylib(State::instance().current_color)
        );
    }
    return mrb_nil_value();
}

static mrb_value mrb_draw_texture_pro(mrb_state* mrb, mrb_value) {
    mrb_int handle;
    mrb_float sx, sy, sw, sh, dx, dy, dw, dh, rotation;
    mrb_get_args(mrb, "ifffffffff", &handle, &sx, &sy, &sw, &sh, &dx, &dy, &dw, &dh, &rotation);
    
    if (auto* texture = TextureManager::instance().get(static_cast<TextureHandle>(handle))) {
        Rectangle source = {static_cast<float>(sx), static_cast<float>(sy),
                           static_cast<float>(sw), static_cast<float>(sh)};
        Rectangle dest = {static_cast<float>(dx), static_cast<float>(dy),
                         static_cast<float>(dw), static_cast<float>(dh)};
        Vector2 origin = {static_cast<float>(dw) / 2.0f, static_cast<float>(dh) / 2.0f};
        
        DrawTexturePro(*texture, source, dest, origin, static_cast<float>(rotation),
                      to_raylib(State::instance().current_color));
    }
    return mrb_nil_value();
}

static mrb_value mrb_texture_width(mrb_state* mrb, mrb_value) {
    mrb_int handle;
    mrb_get_args(mrb, "i", &handle);
    return mrb_fixnum_value(TextureManager::instance().get_width(static_cast<TextureHandle>(handle)));
}

static mrb_value mrb_texture_height(mrb_state* mrb, mrb_value) {
    mrb_int handle;
    mrb_get_args(mrb, "i", &handle);
    return mrb_fixnum_value(TextureManager::instance().get_height(static_cast<TextureHandle>(handle)));
}

void register_graphics(mrb_state* mrb) {
    define_method(mrb, "set_color", mrb_set_color, MRB_ARGS_ANY());
    define_method(mrb, "set_clear_color", mrb_set_clear_color, MRB_ARGS_ANY());
    define_method(mrb, "set_alpha", mrb_set_alpha, MRB_ARGS_REQ(1));
    define_method(mrb, "clear_screen", mrb_clear_screen, MRB_ARGS_ANY());
    
    define_method(mrb, "draw_rect", mrb_draw_rect, MRB_ARGS_REQ(4));
    define_method(mrb, "draw_rect_lines", mrb_draw_rect_lines, MRB_ARGS_REQ(4));
    define_method(mrb, "draw_rect_rotate", mrb_draw_rect_rotate, MRB_ARGS_REQ(5));
    
    define_method(mrb, "draw_line", mrb_draw_line, MRB_ARGS_REQ(4));
    define_method(mrb, "draw_line_thick", mrb_draw_line_thick, MRB_ARGS_REQ(5));
    
    define_method(mrb, "draw_circle", mrb_draw_circle, MRB_ARGS_REQ(3));
    define_method(mrb, "draw_circle_lines", mrb_draw_circle_lines, MRB_ARGS_REQ(3));
    define_method(mrb, "draw_circle_gradient", mrb_draw_circle_gradient, MRB_ARGS_REQ(5));
    
    define_method(mrb, "draw_triangle", mrb_draw_triangle, MRB_ARGS_REQ(6));
    define_method(mrb, "draw_triangle_lines", mrb_draw_triangle_lines, MRB_ARGS_REQ(6));
    
    define_method(mrb, "draw_text", mrb_draw_text, MRB_ARGS_REQ(4));
    define_method(mrb, "measure_text", mrb_measure_text, MRB_ARGS_REQ(2));
    
    // Textures
    define_method(mrb, "load_texture", mrb_load_texture, MRB_ARGS_REQ(1));
    define_method(mrb, "draw_texture", mrb_draw_texture, MRB_ARGS_REQ(3));
    define_method(mrb, "draw_texture_ex", mrb_draw_texture_ex, MRB_ARGS_REQ(5));
    define_method(mrb, "draw_texture_pro", mrb_draw_texture_pro, MRB_ARGS_REQ(10));
    define_method(mrb, "texture_width", mrb_texture_width, MRB_ARGS_REQ(1));
    define_method(mrb, "texture_height", mrb_texture_height, MRB_ARGS_REQ(1));
}

} // namespace bindings
} // namespace gmr
