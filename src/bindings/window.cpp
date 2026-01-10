#include "gmr/bindings/window.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include <cstdio>

// Global delta time from main loop (defined in main.cpp)
extern float g_frame_delta;

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

/// @module GMR::Window
/// @description Window management and display settings. Controls window size, fullscreen,
///   virtual resolution, and provides monitor information.
/// @example # Complete display settings menu
///   class DisplaySettingsScene < GMR::Scene
///     def init
///       @resolutions = [[640, 480], [800, 600], [1024, 768], [1280, 720], [1920, 1080]]
///       @selected_res = 3  # Default to 1280x720
///       @fullscreen = GMR::Window.fullscreen?
///       @virtual_res_enabled = false
///
///       GMR::Input.push_context(:menu)
///     end
///
///     def update(dt)
///       # Navigate resolution options
///       if GMR::Input.action_pressed?(:nav_up) && @selected_res > 0
///         @selected_res -= 1
///         GMR::Audio::Sound.play("assets/menu_move.wav")
///       end
///       if GMR::Input.action_pressed?(:nav_down) && @selected_res < @resolutions.length - 1
///         @selected_res += 1
///         GMR::Audio::Sound.play("assets/menu_move.wav")
///       end
///
///       # Apply resolution
///       if GMR::Input.action_pressed?(:confirm)
///         w, h = @resolutions[@selected_res]
///         GMR::Window.set_size(w, h)
///         GMR::Audio::Sound.play("assets/menu_select.wav")
///       end
///
///       # Toggle fullscreen
///       if GMR::Input.key_pressed?(:f)
///         GMR::Window.toggle_fullscreen
///       end
///
///       # Toggle virtual resolution for pixel-perfect rendering
///       if GMR::Input.key_pressed?(:v)
///         if @virtual_res_enabled
///           GMR::Window.clear_virtual_resolution
///           @virtual_res_enabled = false
///         else
///           GMR::Window.set_virtual_resolution(320, 240)
///           GMR::Window.set_filter_point  # Crisp pixels
///           @virtual_res_enabled = true
///         end
///       end
///     end
///
///     def draw
///       GMR::Graphics.draw_text("Display Settings", 100, 50, 32, [255, 255, 255])
///
///       @resolutions.each_with_index do |(w, h), i|
///         color = i == @selected_res ? [255, 255, 0] : [180, 180, 180]
///         prefix = i == @selected_res ? "> " : "  "
///         GMR::Graphics.draw_text("#{prefix}#{w} x #{h}", 100, 120 + i * 30, 20, color)
///       end
///
///       y = 320
///       GMR::Graphics.draw_text("[F] Fullscreen: #{GMR::Window.fullscreen?}", 100, y, 18, [200, 200, 200])
///       GMR::Graphics.draw_text("[V] Virtual Resolution: #{@virtual_res_enabled}", 100, y + 25, 18, [200, 200, 200])
///       GMR::Graphics.draw_text("Current: #{GMR::Window.width}x#{GMR::Window.height}", 100, y + 50, 18, [150, 150, 150])
///       GMR::Graphics.draw_text("Actual: #{GMR::Window.actual_width}x#{GMR::Window.actual_height}", 100, y + 75, 18, [150, 150, 150])
///     end
///
///     def unload
///       GMR::Input.pop_context
///     end
///   end
/// @example # Retro-style game with virtual resolution
///   class RetroGame
///     def init
///       # Render at 320x240 (classic resolution) but display at window size
///       GMR::Window.set_virtual_resolution(320, 240)
///       GMR::Window.set_filter_point  # No smoothing - crisp pixels
///       GMR::Window.set_title("Pixel Quest")
///       GMR::Time.set_target_fps(60)
///     end
///
///     def update(dt)
///       # Game logic works with 320x240 coordinates
///       @player.x = [@player.x, 0].max
///       @player.x = [@player.x, GMR::Window.width - 16].min  # Returns 320
///     end
///   end

/// @function width
/// @description Get the logical width of the game screen. Returns virtual resolution
///   width if set, otherwise the actual window width.
/// @returns [Integer] Screen width in pixels
/// @example screen_w = GMR::Window.width
static mrb_value mrb_window_width(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(State::instance().screen_width);
}

/// @function height
/// @description Get the logical height of the game screen. Returns virtual resolution
///   height if set, otherwise the actual window height.
/// @returns [Integer] Screen height in pixels
/// @example screen_h = GMR::Window.height
static mrb_value mrb_window_height(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(State::instance().screen_height);
}

/// @function actual_width
/// @description Get the actual window width in pixels, ignoring virtual resolution.
/// @returns [Integer] Actual window width
/// @example real_w = GMR::Window.actual_width
static mrb_value mrb_window_actual_width(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetScreenWidth());
}

/// @function actual_height
/// @description Get the actual window height in pixels, ignoring virtual resolution.
/// @returns [Integer] Actual window height
/// @example real_h = GMR::Window.actual_height
static mrb_value mrb_window_actual_height(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetScreenHeight());
}

/// @function set_size
/// @description Set the window size. Has no effect in fullscreen mode.
/// @param w [Integer] Window width in pixels
/// @param h [Integer] Window height in pixels
/// @returns [Module] self for chaining
/// @example GMR::Window.set_size(1280, 720).set_title("My Game")
static mrb_value mrb_window_set_size(mrb_state* mrb, mrb_value self) {
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

    return self;
}

/// @function set_title
/// @description Set the window title bar text.
/// @param title [String] The window title
/// @returns [Module] self for chaining
/// @example GMR::Window.set_title("My Awesome Game")
static mrb_value mrb_window_set_title(mrb_state* mrb, mrb_value self) {
    const char* title;
    mrb_get_args(mrb, "z", &title);
    SetWindowTitle(title);
    return self;
}

/// @function toggle_fullscreen
/// @description Toggle between fullscreen and windowed mode.
/// @returns [Boolean] true
/// @example GMR::Window.toggle_fullscreen
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

/// @function fullscreen=
/// @description Set fullscreen mode on or off.
/// @param fullscreen [Boolean] true for fullscreen, false for windowed
/// @returns [Boolean] The fullscreen state that was set
/// @example GMR::Window.fullscreen = true
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

/// @function fullscreen?
/// @description Check if the window is currently in fullscreen mode.
/// @returns [Boolean] true if fullscreen
/// @example if GMR::Window.fullscreen?
///   show_windowed_mode_button
/// end
static mrb_value mrb_window_is_fullscreen(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, State::instance().is_fullscreen);
}

/// @function set_virtual_resolution
/// @description Set a virtual resolution for pixel-perfect rendering. The game renders
///   to this resolution and scales to fit the window with letterboxing.
/// @param w [Integer] Virtual width in pixels
/// @param h [Integer] Virtual height in pixels
/// @returns [Module] self for chaining
/// @example # Render at 320x240 for retro-style game
///   GMR::Window.set_virtual_resolution(320, 240).set_filter_point
static mrb_value mrb_window_set_virtual_resolution(mrb_state* mrb, mrb_value self) {
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

    return self;
}

/// @function clear_virtual_resolution
/// @description Disable virtual resolution and render directly at window size.
/// @returns [Module] self for chaining
/// @example GMR::Window.clear_virtual_resolution
static mrb_value mrb_window_clear_virtual_resolution(mrb_state* mrb, mrb_value self) {
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

    return self;
}

/// @function virtual_resolution?
/// @description Check if virtual resolution is currently enabled.
/// @returns [Boolean] true if virtual resolution is active
/// @example if GMR::Window.virtual_resolution?
///   puts "Using virtual resolution"
/// end
static mrb_value mrb_window_is_virtual_resolution(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, State::instance().use_virtual_resolution);
}

/// @function set_filter_point
/// @description Set nearest-neighbor (point) filtering for virtual resolution scaling.
///   Produces crisp, pixelated look. Only works when virtual resolution is enabled.
/// @returns [Module] self for chaining
/// @example GMR::Window.set_virtual_resolution(320, 240).set_filter_point
static mrb_value mrb_window_set_filter_point(mrb_state* mrb, mrb_value self) {
    if (State::instance().use_virtual_resolution) {
        SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);
    }
    return self;
}

/// @function set_filter_bilinear
/// @description Set bilinear filtering for virtual resolution scaling.
///   Produces smoother, blended scaling. Only works when virtual resolution is enabled.
/// @returns [Module] self for chaining
/// @example GMR::Window.set_virtual_resolution(320, 240).set_filter_bilinear
static mrb_value mrb_window_set_filter_bilinear(mrb_state* mrb, mrb_value self) {
    if (State::instance().use_virtual_resolution) {
        SetTextureFilter(render_target.texture, TEXTURE_FILTER_BILINEAR);
    }
    return self;
}

/// @function monitor_count
/// @description Get the number of connected monitors.
/// @returns [Integer] Number of monitors
/// @example count = GMR::Window.monitor_count
static mrb_value mrb_window_monitor_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetMonitorCount());
}

/// @function monitor_width
/// @description Get the width of a specific monitor.
/// @param index [Integer] Monitor index (0-based)
/// @returns [Integer] Monitor width in pixels
/// @example w = GMR::Window.monitor_width(0)  # Primary monitor
static mrb_value mrb_window_monitor_width(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorWidth(monitor));
}

/// @function monitor_height
/// @description Get the height of a specific monitor.
/// @param index [Integer] Monitor index (0-based)
/// @returns [Integer] Monitor height in pixels
/// @example h = GMR::Window.monitor_height(0)
static mrb_value mrb_window_monitor_height(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorHeight(monitor));
}

/// @function monitor_refresh_rate
/// @description Get the refresh rate of a specific monitor.
/// @param index [Integer] Monitor index (0-based)
/// @returns [Integer] Refresh rate in Hz
/// @example hz = GMR::Window.monitor_refresh_rate(0)
static mrb_value mrb_window_monitor_refresh_rate(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_fixnum_value(GetMonitorRefreshRate(monitor));
}

/// @function monitor_name
/// @description Get the name of a specific monitor.
/// @param index [Integer] Monitor index (0-based)
/// @returns [String] Monitor name
/// @example name = GMR::Window.monitor_name(0)
static mrb_value mrb_window_monitor_name(mrb_state* mrb, mrb_value) {
    mrb_int monitor;
    mrb_get_args(mrb, "i", &monitor);
    return mrb_str_new_cstr(mrb, GetMonitorName(monitor));
}

// ============================================================================
// GMR::Time Module Functions
// ============================================================================

/// @module GMR::Time
/// @description Time and frame rate utilities. Provides delta time for frame-independent
///   movement, elapsed time tracking, and FPS management.
/// @example # Frame-independent movement and animation
///   class Player
///     SPEED = 200  # Pixels per second
///
///     def update
///       dt = GMR::Time.delta
///
///       # Movement works the same at 30fps or 144fps
///       @x += @vx * SPEED * dt
///       @y += @vy * SPEED * dt
///
///       # Animation timing
///       @anim_timer += dt
///       if @anim_timer >= 0.1  # Change frame every 0.1 seconds
///         @frame = (@frame + 1) % @frame_count
///         @anim_timer = 0
///       end
///     end
///   end
/// @example # Time-based effects and cooldowns
///   class Ability
///     def initialize
///       @cooldown = 0
///       @duration = 0
///       @active = false
///     end
///
///     def update
///       dt = GMR::Time.delta
///
///       # Decrease cooldown over time
///       @cooldown -= dt if @cooldown > 0
///
///       # Track active duration
///       if @active
///         @duration -= dt
///         if @duration <= 0
///           @active = false
///           deactivate_effect
///         end
///       end
///     end
///
///     def use
///       return if @cooldown > 0
///       @active = true
///       @duration = 5.0   # Active for 5 seconds
///       @cooldown = 10.0  # 10 second cooldown
///       activate_effect
///     end
///   end
/// @example # Debug overlay with FPS and timing info
///   class DebugOverlay
///     def draw
///       y = 10
///       color = [0, 255, 0]
///
///       # Show current FPS
///       fps = GMR::Time.fps
///       fps_color = fps < 30 ? [255, 0, 0] : fps < 55 ? [255, 255, 0] : [0, 255, 0]
///       GMR::Graphics.draw_text("FPS: #{fps}", 10, y, 16, fps_color)
///       y += 20
///
///       # Show frame time
///       frame_ms = (GMR::Time.delta * 1000).round(2)
///       GMR::Graphics.draw_text("Frame: #{frame_ms}ms", 10, y, 16, color)
///       y += 20
///
///       # Show total elapsed time
///       elapsed = GMR::Time.elapsed
///       minutes = (elapsed / 60).to_i
///       seconds = (elapsed % 60).to_i
///       GMR::Graphics.draw_text("Time: #{minutes}:#{seconds.to_s.rjust(2, '0')}", 10, y, 16, color)
///     end
///   end

/// @function delta
/// @description Get the time elapsed since the last frame in seconds. Use this for
///   frame-independent movement and animation.
/// @returns [Float] Delta time in seconds
/// @example # Move at 100 pixels per second regardless of frame rate
///   player.x += 100 * GMR::Time.delta
static mrb_value mrb_time_delta(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, g_frame_delta);
}

/// @function elapsed
/// @description Get the total time elapsed since the game started in seconds.
/// @returns [Float] Total elapsed time in seconds
/// @example # Flash effect every 0.5 seconds
///   visible = (GMR::Time.elapsed % 1.0) < 0.5
static mrb_value mrb_time_elapsed(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, GetTime());
}

/// @function fps
/// @description Get the current frames per second.
/// @returns [Integer] Current FPS
/// @example puts "FPS: #{GMR::Time.fps}"
static mrb_value mrb_time_fps(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(GetFPS());
}

/// @function set_target_fps
/// @description Set the target frame rate. The game will try to maintain this FPS.
///   Set to 0 for unlimited frame rate.
/// @param fps [Integer] Target frames per second
/// @returns [Module] self for chaining
/// @example GMR::Time.set_target_fps(60)  # Lock to 60 FPS
static mrb_value mrb_time_set_target_fps(mrb_state* mrb, mrb_value self) {
    mrb_int fps;
    mrb_get_args(mrb, "i", &fps);
    SetTargetFPS(fps);
    return self;
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
