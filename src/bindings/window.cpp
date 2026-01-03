#include "gmr/bindings/window.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include <cstdio>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

namespace gmr {
namespace bindings {

// Render target for virtual resolution
static RenderTexture2D render_target;

#if defined(PLATFORM_WEB)
// JavaScript to get actual window size and resize canvas
EM_JS(int, js_get_window_width, (), {
    return window.innerWidth;
});

EM_JS(int, js_get_window_height, (), {
    return window.innerHeight;
});

EM_JS(bool, js_is_fullscreen, (), {
    return document.fullscreenElement != null ||
           document.webkitFullscreenElement != null ||
           document.mozFullScreenElement != null;
});

EM_JS(void, js_resize_canvas, (int width, int height), {
    var canvas = document.getElementById('canvas');
    if (canvas) {
        canvas.width = width;
        canvas.height = height;
    }
});

static bool was_fullscreen = false;

// Update screen dimensions from actual canvas size on web
void update_web_screen_size() {
    auto& state = State::instance();
    if (state.use_virtual_resolution) return;

    bool is_fs = js_is_fullscreen();

    // Detect fullscreen state change
    if (is_fs != was_fullscreen) {
        was_fullscreen = is_fs;

        if (is_fs) {
            // Entered fullscreen - resize canvas to window size
            int w = js_get_window_width();
            int h = js_get_window_height();
            js_resize_canvas(w, h);
            SetWindowSize(w, h);
            state.screen_width = w;
            state.screen_height = h;
            state.is_fullscreen = true;
        } else {
            // Exited fullscreen - restore original size
            js_resize_canvas(state.windowed_width, state.windowed_height);
            SetWindowSize(state.windowed_width, state.windowed_height);
            state.screen_width = state.windowed_width;
            state.screen_height = state.windowed_height;
            state.is_fullscreen = false;
        }
    } else if (is_fs) {
        // While in fullscreen, check if window was resized
        int w = js_get_window_width();
        int h = js_get_window_height();
        if (w != state.screen_width || h != state.screen_height) {
            js_resize_canvas(w, h);
            SetWindowSize(w, h);
            state.screen_width = w;
            state.screen_height = h;
        }
    }
}
#endif

RenderTexture2D& get_render_target() {
    return render_target;
}

// ============================================================================
// GMR::Window Module Functions
// ============================================================================

// GMR::Window.width (virtual resolution width)
static mrb_value mrb_window_width(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(State::instance().screen_width);
}

// GMR::Window.height (virtual resolution height)
static mrb_value mrb_window_height(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(State::instance().screen_height);
}

// GMR::Window.actual_width (real window size)
static mrb_value mrb_window_actual_width(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetScreenWidth());
}

// GMR::Window.actual_height (real window size)
static mrb_value mrb_window_actual_height(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetScreenHeight());
}

// GMR::Window.set_size(w, h)
static mrb_value mrb_window_set_size(mrb_state* mrb, mrb_value) {
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

// GMR::Window.set_title(title)
static mrb_value mrb_window_set_title(mrb_state* mrb, mrb_value) {
    const char* title;
    mrb_get_args(mrb, "z", &title);
    SetWindowTitle(title);
    return mrb_nil_value();
}

// GMR::Window.toggle_fullscreen
static mrb_value mrb_window_toggle_fullscreen(mrb_state* mrb, mrb_value) {
    auto& state = State::instance();

#if defined(PLATFORM_WEB)
    if (state.is_fullscreen) {
        emscripten_exit_soft_fullscreen();
        emscripten_exit_fullscreen();
        state.is_fullscreen = false;
    } else {
        EmscriptenFullscreenStrategy strategy = {};
        strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
        strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE;
        strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_BILINEAR;

        EMSCRIPTEN_RESULT result = emscripten_enter_soft_fullscreen("#canvas", &strategy);
        if (result != EMSCRIPTEN_RESULT_SUCCESS) {
            emscripten_request_fullscreen("#canvas", true);
        }
        state.is_fullscreen = true;
    }
#else
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
#endif

    return mrb_true_value();
}

// GMR::Window.fullscreen = bool
static mrb_value mrb_window_set_fullscreen(mrb_state* mrb, mrb_value) {
    mrb_bool fullscreen;
    mrb_get_args(mrb, "b", &fullscreen);

    auto& state = State::instance();

#if defined(PLATFORM_WEB)
    if (fullscreen && !state.is_fullscreen) {
        EmscriptenFullscreenStrategy strategy = {};
        strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
        strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE;
        strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_BILINEAR;

        EMSCRIPTEN_RESULT result = emscripten_enter_soft_fullscreen("#canvas", &strategy);
        if (result != EMSCRIPTEN_RESULT_SUCCESS) {
            emscripten_request_fullscreen("#canvas", true);
        }
        state.is_fullscreen = true;
    } else if (!fullscreen && state.is_fullscreen) {
        emscripten_exit_fullscreen();
        emscripten_exit_soft_fullscreen();
        state.is_fullscreen = false;
    }
#else
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
#endif

    return to_mrb_bool(mrb, fullscreen);
}

// GMR::Window.fullscreen?
static mrb_value mrb_window_is_fullscreen(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, State::instance().is_fullscreen);
}

// GMR::Window.set_virtual_resolution(w, h)
static mrb_value mrb_window_set_virtual_resolution(mrb_state* mrb, mrb_value) {
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

// GMR::Window.clear_virtual_resolution
static mrb_value mrb_window_clear_virtual_resolution(mrb_state* mrb, mrb_value) {
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

// GMR::Window.virtual_resolution?
static mrb_value mrb_window_is_virtual_resolution(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, State::instance().use_virtual_resolution);
}

// GMR::Window.set_filter_point
static mrb_value mrb_window_set_filter_point(mrb_state* mrb, mrb_value) {
    if (State::instance().use_virtual_resolution) {
        SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);
    }
    return mrb_nil_value();
}

// GMR::Window.set_filter_bilinear
static mrb_value mrb_window_set_filter_bilinear(mrb_state* mrb, mrb_value) {
    if (State::instance().use_virtual_resolution) {
        SetTextureFilter(render_target.texture, TEXTURE_FILTER_BILINEAR);
    }
    return mrb_nil_value();
}

// GMR::Window.monitor_count
static mrb_value mrb_window_monitor_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetMonitorCount());
}

// GMR::Window.monitor_width(index)
static mrb_value mrb_window_monitor_width(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorWidth(monitor));
}

// GMR::Window.monitor_height(index)
static mrb_value mrb_window_monitor_height(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorHeight(monitor));
}

// GMR::Window.monitor_refresh_rate(index)
static mrb_value mrb_window_monitor_refresh_rate(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorRefreshRate(monitor));
}

// GMR::Window.monitor_name(index)
static mrb_value mrb_window_monitor_name(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_str_new_cstr(mrb, GetMonitorName(monitor));
}

// ============================================================================
// GMR::Time Module Functions
// ============================================================================

// GMR::Time.delta
static mrb_value mrb_time_delta(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetFrameTime());
}

// GMR::Time.elapsed
static mrb_value mrb_time_elapsed(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetTime());
}

// GMR::Time.fps
static mrb_value mrb_time_fps(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetFPS());
}

// GMR::Time.set_target_fps(fps)
static mrb_value mrb_time_set_target_fps(mrb_state* mrb, mrb_value) {
    mrb_int fps;
    mrb_get_args(mrb, "i", &fps);
    SetTargetFPS(fps);
    return mrb_nil_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_window(mrb_state* mrb) {
    // GMR::Window module
    RClass* window = get_gmr_submodule(mrb, "Window");

    mrb_define_module_function(mrb, window, "width", mrb_window_width, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "height", mrb_window_height, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "actual_width", mrb_window_actual_width, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "actual_height", mrb_window_actual_height, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "set_size", mrb_window_set_size, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, window, "set_title", mrb_window_set_title, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "toggle_fullscreen", mrb_window_toggle_fullscreen, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "fullscreen=", mrb_window_set_fullscreen, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "fullscreen?", mrb_window_is_fullscreen, MRB_ARGS_NONE());

    mrb_define_module_function(mrb, window, "set_virtual_resolution", mrb_window_set_virtual_resolution, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, window, "clear_virtual_resolution", mrb_window_clear_virtual_resolution, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "virtual_resolution?", mrb_window_is_virtual_resolution, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "set_filter_point", mrb_window_set_filter_point, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "set_filter_bilinear", mrb_window_set_filter_bilinear, MRB_ARGS_NONE());

    mrb_define_module_function(mrb, window, "monitor_count", mrb_window_monitor_count, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "monitor_width", mrb_window_monitor_width, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "monitor_height", mrb_window_monitor_height, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "monitor_refresh_rate", mrb_window_monitor_refresh_rate, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "monitor_name", mrb_window_monitor_name, MRB_ARGS_REQ(1));

    // GMR::Time module
    RClass* time_mod = get_gmr_submodule(mrb, "Time");

    mrb_define_module_function(mrb, time_mod, "delta", mrb_time_delta, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, time_mod, "elapsed", mrb_time_elapsed, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, time_mod, "fps", mrb_time_fps, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, time_mod, "set_target_fps", mrb_time_set_target_fps, MRB_ARGS_REQ(1));
}

void cleanup_window() {
    if (State::instance().use_virtual_resolution) {
        UnloadRenderTexture(render_target);
    }
}

} // namespace bindings
} // namespace gmr
