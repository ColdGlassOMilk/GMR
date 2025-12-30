#include "gmr/bindings/window.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include <cstdio>

namespace gmr {
namespace bindings {

// Render target for virtual resolution
static RenderTexture2D render_target;

RenderTexture2D& get_render_target() {
    return render_target;
}

static mrb_value mrb_screen_width(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(State::instance().screen_width);
}

static mrb_value mrb_screen_height(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(State::instance().screen_height);
}

static mrb_value mrb_set_window_size(mrb_state* mrb, mrb_value) {
    mrb_int w, h;
    mrb_get_args(mrb, "ii", &w, &h);
    
    auto& state = State::instance();
    
    if (!state.is_fullscreen) {
        state.windowed_width = w;
        state.windowed_height = h;
        SetWindowSize(w, h);
    }
    
    if (!state.use_virtual_resolution) {
        state.screen_width = w;
        state.screen_height = h;
    }
    
    return mrb_nil_value();
}

static mrb_value mrb_set_window_title(mrb_state* mrb, mrb_value) {
    const char* title;
    mrb_get_args(mrb, "z", &title);
    SetWindowTitle(title);
    return mrb_nil_value();
}

static mrb_value mrb_toggle_fullscreen(mrb_state* mrb, mrb_value) {
    auto& state = State::instance();
    
    if (state.is_fullscreen) {
        ClearWindowState(FLAG_FULLSCREEN_MODE);
        SetWindowSize(state.windowed_width, state.windowed_height);
        
        int monitor = GetCurrentMonitor();
        int mx = GetMonitorWidth(monitor);
        int my = GetMonitorHeight(monitor);
        SetWindowPosition((mx - state.windowed_width) / 2, (my - state.windowed_height) / 2);
        
        if (!state.use_virtual_resolution) {
            state.screen_width = state.windowed_width;
            state.screen_height = state.windowed_height;
        }
        state.is_fullscreen = false;
    } else {
        state.windowed_width = GetScreenWidth();
        state.windowed_height = GetScreenHeight();
        
        int monitor = GetCurrentMonitor();
        int mw = GetMonitorWidth(monitor);
        int mh = GetMonitorHeight(monitor);
        
        SetWindowSize(mw, mh);
        SetWindowState(FLAG_FULLSCREEN_MODE);
        
        if (!state.use_virtual_resolution) {
            state.screen_width = mw;
            state.screen_height = mh;
        }
        state.is_fullscreen = true;
    }
    
    return mrb_true_value();
}

static mrb_value mrb_set_fullscreen(mrb_state* mrb, mrb_value) {
    mrb_bool fullscreen;
    mrb_get_args(mrb, "b", &fullscreen);
    
    auto& state = State::instance();
    
    if (fullscreen && !state.is_fullscreen) {
        state.windowed_width = GetScreenWidth();
        state.windowed_height = GetScreenHeight();
        
        int monitor = GetCurrentMonitor();
        int mw = GetMonitorWidth(monitor);
        int mh = GetMonitorHeight(monitor);
        
        SetWindowSize(mw, mh);
        SetWindowState(FLAG_FULLSCREEN_MODE);
        
        if (!state.use_virtual_resolution) {
            state.screen_width = mw;
            state.screen_height = mh;
        }
        state.is_fullscreen = true;
    } else if (!fullscreen && state.is_fullscreen) {
        ClearWindowState(FLAG_FULLSCREEN_MODE);
        SetWindowSize(state.windowed_width, state.windowed_height);
        
        int monitor = GetCurrentMonitor();
        int mx = GetMonitorWidth(monitor);
        int my = GetMonitorHeight(monitor);
        SetWindowPosition((mx - state.windowed_width) / 2, (my - state.windowed_height) / 2);
        
        if (!state.use_virtual_resolution) {
            state.screen_width = state.windowed_width;
            state.screen_height = state.windowed_height;
        }
        state.is_fullscreen = false;
    }
    
    return mrb_true_value();
}

static mrb_value mrb_is_fullscreen(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, State::instance().is_fullscreen);
}

static mrb_value mrb_get_monitor_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetMonitorCount());
}

static mrb_value mrb_get_monitor_width(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorWidth(monitor));
}

static mrb_value mrb_get_monitor_height(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorHeight(monitor));
}

static mrb_value mrb_get_monitor_refresh_rate(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorRefreshRate(monitor));
}

static mrb_value mrb_get_monitor_name(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_str_new_cstr(mrb, GetMonitorName(monitor));
}

static mrb_value mrb_set_target_fps(mrb_state* mrb, mrb_value) {
    mrb_int fps;
    mrb_get_args(mrb, "i", &fps);
    SetTargetFPS(fps);
    return mrb_nil_value();
}

static mrb_value mrb_get_fps(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetFPS());
}

static mrb_value mrb_get_time(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetTime());
}

static mrb_value mrb_get_delta_time(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetFrameTime());
}

static mrb_value mrb_set_virtual_resolution(mrb_state* mrb, mrb_value) {
    mrb_int w, h;
    mrb_get_args(mrb, "ii", &w, &h);
    
    auto& state = State::instance();
    
    if (state.use_virtual_resolution) {
        UnloadRenderTexture(render_target);
    }
    
    state.virtual_width = w;
    state.virtual_height = h;
    render_target = LoadRenderTexture(w, h);
    SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);
    state.use_virtual_resolution = true;
    
    state.screen_width = w;
    state.screen_height = h;
    
    printf("Virtual resolution set to %dx%d\n", static_cast<int>(w), static_cast<int>(h));
    
    return mrb_true_value();
}

static mrb_value mrb_clear_virtual_resolution(mrb_state* mrb, mrb_value) {
    auto& state = State::instance();
    
    if (state.use_virtual_resolution) {
        UnloadRenderTexture(render_target);
        state.use_virtual_resolution = false;
    }
    
    if (state.is_fullscreen) {
        int monitor = GetCurrentMonitor();
        state.screen_width = GetMonitorWidth(monitor);
        state.screen_height = GetMonitorHeight(monitor);
    } else {
        state.screen_width = GetScreenWidth();
        state.screen_height = GetScreenHeight();
    }
    
    return mrb_true_value();
}

static mrb_value mrb_is_virtual_resolution(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, State::instance().use_virtual_resolution);
}

static mrb_value mrb_set_filter_point(mrb_state* mrb, mrb_value) {
    if (State::instance().use_virtual_resolution) {
        SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);
    }
    return mrb_nil_value();
}

static mrb_value mrb_set_filter_bilinear(mrb_state* mrb, mrb_value) {
    if (State::instance().use_virtual_resolution) {
        SetTextureFilter(render_target.texture, TEXTURE_FILTER_BILINEAR);
    }
    return mrb_nil_value();
}

void register_window(mrb_state* mrb) {
    define_method(mrb, "screen_width", mrb_screen_width, MRB_ARGS_NONE());
    define_method(mrb, "screen_height", mrb_screen_height, MRB_ARGS_NONE());
    define_method(mrb, "set_window_size", mrb_set_window_size, MRB_ARGS_REQ(2));
    define_method(mrb, "set_window_title", mrb_set_window_title, MRB_ARGS_REQ(1));
    define_method(mrb, "toggle_fullscreen", mrb_toggle_fullscreen, MRB_ARGS_NONE());
    define_method(mrb, "set_fullscreen", mrb_set_fullscreen, MRB_ARGS_REQ(1));
    define_method(mrb, "fullscreen?", mrb_is_fullscreen, MRB_ARGS_NONE());
    define_method(mrb, "monitor_count", mrb_get_monitor_count, MRB_ARGS_NONE());
    define_method(mrb, "monitor_width", mrb_get_monitor_width, MRB_ARGS_REQ(1));
    define_method(mrb, "monitor_height", mrb_get_monitor_height, MRB_ARGS_REQ(1));
    define_method(mrb, "monitor_refresh_rate", mrb_get_monitor_refresh_rate, MRB_ARGS_REQ(1));
    define_method(mrb, "monitor_name", mrb_get_monitor_name, MRB_ARGS_REQ(1));
    define_method(mrb, "set_target_fps", mrb_set_target_fps, MRB_ARGS_REQ(1));
    define_method(mrb, "get_fps", mrb_get_fps, MRB_ARGS_NONE());
    define_method(mrb, "get_time", mrb_get_time, MRB_ARGS_NONE());
    define_method(mrb, "get_delta_time", mrb_get_delta_time, MRB_ARGS_NONE());
    
    define_method(mrb, "set_virtual_resolution", mrb_set_virtual_resolution, MRB_ARGS_REQ(2));
    define_method(mrb, "clear_virtual_resolution", mrb_clear_virtual_resolution, MRB_ARGS_NONE());
    define_method(mrb, "virtual_resolution?", mrb_is_virtual_resolution, MRB_ARGS_NONE());
    define_method(mrb, "set_filter_point", mrb_set_filter_point, MRB_ARGS_NONE());
    define_method(mrb, "set_filter_bilinear", mrb_set_filter_bilinear, MRB_ARGS_NONE());
}

void cleanup_window() {
    if (State::instance().use_virtual_resolution) {
        UnloadRenderTexture(render_target);
    }
}

} // namespace bindings
} // namespace gmr
