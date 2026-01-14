#include "gmr/bindings/window.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/state.hpp"
#include <cstdio>
#include <cmath>   // For std::abs
#include "rlgl.h"  // For rlViewport to update WebGL viewport on resize


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

// Safety net polling for edge cases during fullscreen (orientation changes, etc.)
// Primary resize handling is done through on_resize() called from JavaScript
void update_web_screen_size() {
    auto& state = State::instance();

    // Only poll during fullscreen as safety net for cases ResizeObserver might miss
    if (!state.is_fullscreen) {
        return;
    }

    // Check if window size changed (orientation, browser resize)
    int w = js_get_window_width();
    int h = js_get_window_height();

    // Determine render size based on DPR setting
    int render_w = state.use_native_dpr ?
        static_cast<int>(w * state.device_pixel_ratio) : w;
    int render_h = state.use_native_dpr ?
        static_cast<int>(h * state.device_pixel_ratio) : h;

    if (render_w != state.render_width || render_h != state.render_height) {
        // Update state
        state.css_width = w;
        state.css_height = h;
        state.render_width = render_w;
        state.render_height = render_h;

        // Apply changes
        js_resize_canvas(render_w, render_h);
        SetWindowSize(render_w, render_h);
        rlViewport(0, 0, render_w, render_h);

        if (!state.use_virtual_resolution) {
            state.screen_width = w;
            state.screen_height = h;
        }

        state.canvas_resize_pending = true;
    }
}

// Exported functions for JavaScript
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void set_master_volume(float volume) {
        SetMasterVolume(volume);
    }

    // Unified resize handler - single entry point for all resize events from JavaScript
    // Called by ResizeObserver, fullscreen changes, and DPR changes
    EMSCRIPTEN_KEEPALIVE
    void on_resize(int css_w, int css_h, int phys_w, int phys_h, int dpr_x100, int is_fullscreen) {
        auto& state = State::instance();

        // Extract DPR (passed as int * 100 for precision without floats in JS interop)
        float dpr = static_cast<float>(dpr_x100) / 100.0f;

        // Determine actual render dimensions based on DPR setting
        int render_w = state.use_native_dpr ? phys_w : css_w;
        int render_h = state.use_native_dpr ? phys_h : css_h;

        // Track state changes
        bool dimensions_changed = (render_w != state.render_width ||
                                   render_h != state.render_height);
        bool fullscreen_changed = ((is_fullscreen != 0) != state.is_fullscreen);
        bool dpr_changed = (std::abs(dpr - state.device_pixel_ratio) > 0.001f);

        // Early exit if nothing changed
        if (!dimensions_changed && !fullscreen_changed && !dpr_changed) {
            return;
        }

        // Update CSS dimensions and DPR
        state.css_width = css_w;
        state.css_height = css_h;
        state.device_pixel_ratio = dpr;

        // Handle fullscreen state change
        bool was_fullscreen = state.is_fullscreen;
        state.is_fullscreen = (is_fullscreen != 0);

        if (fullscreen_changed) {
            if (state.is_fullscreen) {
                // Entering fullscreen - windowed dimensions should already be saved
            } else {
                // Exiting fullscreen - restore windowed dimensions
                render_w = state.windowed_width;
                render_h = state.windowed_height;
                if (state.use_native_dpr) {
                    render_w = static_cast<int>(render_w * dpr);
                    render_h = static_cast<int>(render_h * dpr);
                }
            }
        }

        // Apply render dimensions if changed
        if (render_w != state.render_width || render_h != state.render_height) {
            state.render_width = render_w;
            state.render_height = render_h;

            // Update canvas backing buffer
            js_resize_canvas(render_w, render_h);

            // Update raylib's window size
            SetWindowSize(render_w, render_h);

            // CRITICAL: Update WebGL viewport to match new canvas size
            // SetWindowSize() on Emscripten doesn't automatically update the viewport
            rlViewport(0, 0, render_w, render_h);
        }

        // Update logical screen size if not using virtual resolution
        // Logical size uses CSS dimensions (what Ruby code expects for positioning)
        if (!state.use_virtual_resolution) {
            state.screen_width = css_w;
            state.screen_height = css_h;
        }

        // Signal main loop to notify Ruby
        state.canvas_resize_pending = true;
        if (fullscreen_changed) {
            state.fullscreen_changed = true;
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
/// @description Get the logical width of the game screen in UI coordinate space.
///   This is the width you should use for positioning UI elements.
///   The engine uses a 360p baseline, so at 540p this returns 640 (960/1.5).
/// @returns [Integer] Logical screen width
/// @example screen_w = GMR::Window.width
static mrb_value mrb_window_width(mrb_state* mrb, mrb_value) {
    auto& state = State::instance();
    float ui_scale = state.ui_scale();
    return mrb_fixnum_value(static_cast<int>(state.screen_width / ui_scale));
}

/// @function height
/// @description Get the logical height of the game screen in UI coordinate space.
///   This is the height you should use for positioning UI elements.
///   The engine uses a 360p baseline, so this always returns 360.
/// @returns [Integer] Logical screen height (always 360 due to baseline)
/// @example screen_h = GMR::Window.height
static mrb_value mrb_window_height(mrb_state* mrb, mrb_value) {
    auto& state = State::instance();
    float ui_scale = state.ui_scale();
    return mrb_fixnum_value(static_cast<int>(state.screen_height / ui_scale));
}

/// @function screen_width
/// @description Get the actual screen width in pixels.
///   Use this when you need real pixel dimensions (e.g., for shaders).
/// @returns [Integer] Screen width in pixels
/// @example pixels_w = GMR::Window.screen_width
static mrb_value mrb_window_screen_width(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(State::instance().screen_width);
}

/// @function screen_height
/// @description Get the actual screen height in pixels.
///   Use this when you need real pixel dimensions (e.g., for shaders).
/// @returns [Integer] Screen height in pixels
/// @example pixels_h = GMR::Window.screen_height
static mrb_value mrb_window_screen_height(mrb_state* mrb, mrb_value) {
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
        state.css_width = w;
        state.css_height = h;

        // Calculate render dimensions based on DPR setting
        int render_w = state.use_native_dpr ?
            static_cast<int>(w * state.device_pixel_ratio) : w;
        int render_h = state.use_native_dpr ?
            static_cast<int>(h * state.device_pixel_ratio) : h;

        state.render_width = render_w;
        state.render_height = render_h;
        SetWindowSize(render_w, render_h);

#if defined(PLATFORM_WEB)
        js_resize_canvas(render_w, render_h);
        rlViewport(0, 0, render_w, render_h);
#endif
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
        // Note: Actual state updates happen in on_resize() callback from JS
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
        // Note: Actual state updates happen in on_resize() callback from JS
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

        state.css_width = state.windowed_width;
        state.css_height = state.windowed_height;
        state.render_width = state.windowed_width;
        state.render_height = state.windowed_height;
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

        state.css_width = mw;
        state.css_height = mh;
        state.render_width = mw;
        state.render_height = mh;
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
        // Note: Actual state updates happen in on_resize() callback from JS
        state.is_fullscreen = true;
    } else if (!fullscreen && state.is_fullscreen) {
        emscripten_exit_fullscreen();
        emscripten_exit_soft_fullscreen();
        // Note: Actual state updates happen in on_resize() callback from JS
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

        state.css_width = mw;
        state.css_height = mh;
        state.render_width = mw;
        state.render_height = mh;
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

        state.css_width = state.windowed_width;
        state.css_height = state.windowed_height;
        state.render_width = state.windowed_width;
        state.render_height = state.windowed_height;
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

    // Use actual canvas/window size (GetScreenWidth works on all platforms)
    state.screen_width = GetScreenWidth();
    state.screen_height = GetScreenHeight();

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

/// @function use_native_dpr
/// @description Enable high-DPI rendering at native device pixel ratio.
///   Results in sharper rendering on retina displays but may impact performance.
///   Only affects web builds. On native platforms this has no effect.
/// @returns [Module] self for chaining
/// @example GMR::Window.use_native_dpr  # Enable sharp retina rendering
static mrb_value mrb_window_use_native_dpr(mrb_state* mrb, mrb_value self) {
#if defined(PLATFORM_WEB)
    auto& state = State::instance();
    if (!state.use_native_dpr) {
        state.use_native_dpr = true;

        // Calculate new render dimensions
        int phys_w = static_cast<int>(state.css_width * state.device_pixel_ratio);
        int phys_h = static_cast<int>(state.css_height * state.device_pixel_ratio);

        state.render_width = phys_w;
        state.render_height = phys_h;

        js_resize_canvas(phys_w, phys_h);
        SetWindowSize(phys_w, phys_h);
        rlViewport(0, 0, phys_w, phys_h);

        state.canvas_resize_pending = true;
    }
#endif
    return self;
}

/// @function use_css_dpr
/// @description Render at CSS pixel resolution (1:1 with layout pixels).
///   Better performance on high-DPI displays but may appear less sharp.
///   This is the default behavior. Only affects web builds.
/// @returns [Module] self for chaining
/// @example GMR::Window.use_css_dpr  # Default, better performance
static mrb_value mrb_window_use_css_dpr(mrb_state* mrb, mrb_value self) {
#if defined(PLATFORM_WEB)
    auto& state = State::instance();
    if (state.use_native_dpr) {
        state.use_native_dpr = false;

        state.render_width = state.css_width;
        state.render_height = state.css_height;

        js_resize_canvas(state.css_width, state.css_height);
        SetWindowSize(state.css_width, state.css_height);
        rlViewport(0, 0, state.css_width, state.css_height);

        state.canvas_resize_pending = true;
    }
#endif
    return self;
}

/// @function device_pixel_ratio
/// @description Get the current device pixel ratio.
///   Returns 1.0 on standard displays, 2.0 on retina displays, etc.
///   On native platforms, always returns 1.0.
/// @returns [Float] Device pixel ratio
/// @example dpr = GMR::Window.device_pixel_ratio  # 2.0 on retina
static mrb_value mrb_window_device_pixel_ratio(mrb_state* mrb, mrb_value) {
#if defined(PLATFORM_WEB)
    return mrb_float_value(mrb, State::instance().device_pixel_ratio);
#else
    return mrb_float_value(mrb, 1.0f);
#endif
}

/// @function native_dpr?
/// @description Check if native DPR rendering is enabled.
/// @returns [Boolean] true if rendering at native device pixel ratio
/// @example if GMR::Window.native_dpr?
///   puts "Rendering at #{GMR::Window.device_pixel_ratio}x resolution"
/// end
static mrb_value mrb_window_is_native_dpr(mrb_state* mrb, mrb_value) {
    return to_mrb_bool(mrb, State::instance().use_native_dpr);
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
    return mrb_float_value(mrb, State::instance().frame_delta);
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

/// @function scale
/// @description Get the current time scale. Affects how fast game time passes.
///   1.0 = normal speed, 0.5 = half speed (slow motion), 0.0 = paused.
/// @returns [Float] Current time scale
/// @example current_scale = GMR::Time.scale
static mrb_value mrb_time_scale_get(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, State::instance().time_scale);
}

/// @function scale=
/// @description Set the time scale. Affects how fast game time passes.
///   Timers created with `scaled: true` (default) will respect this.
///   Set to 0.0 to pause, 0.5 for slow motion, 2.0 for fast forward.
/// @param scale [Float] The new time scale (must be >= 0)
/// @returns [Float] The new time scale
/// @example GMR::Time.scale = 0.5  # Slow motion
/// @example GMR::Time.scale = 0.0  # Pause game time
/// @example GMR::Time.scale = 1.0  # Normal speed
static mrb_value mrb_time_scale_set(mrb_state* mrb, mrb_value) {
    mrb_float scale;
    mrb_get_args(mrb, "f", &scale);

    if (scale < 0.0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Time.scale cannot be negative");
        return mrb_nil_value();
    }

    State::instance().time_scale = static_cast<float>(scale);
    return mrb_float_value(mrb, scale);
}

// ============================================================================
// VSync Control
// ============================================================================

/// @function vsync=
/// @description Enable or disable vertical synchronization (VSync). When enabled,
///   frame presentation waits for the display's vertical blank interval, eliminating
///   screen tearing. When disabled, frames are presented immediately.
/// @param enabled [Boolean] true to enable VSync, false to disable
/// @returns [Boolean] The new VSync state
/// @example GMR::Window.vsync = true   # Enable VSync (default)
/// @example GMR::Window.vsync = false  # Disable VSync for lower latency
static mrb_value mrb_window_set_vsync(mrb_state* mrb, mrb_value) {
    mrb_bool enabled;
    mrb_get_args(mrb, "b", &enabled);

    if (enabled) {
        SetWindowState(FLAG_VSYNC_HINT);
        SetTargetFPS(0);  // Let VSync control frame pacing
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
        SetTargetFPS(60); // Software limiter as fallback
    }
    State::instance().vsync_enabled = enabled;
    return mrb_bool_value(enabled);
}

/// @function vsync?
/// @description Check if vertical synchronization (VSync) is currently enabled.
/// @returns [Boolean] true if VSync is enabled, false otherwise
/// @example if GMR::Window.vsync?
///   puts "VSync is enabled"
/// end
static mrb_value mrb_window_is_vsync(mrb_state* mrb, mrb_value) {
    return mrb_bool_value(IsWindowState(FLAG_VSYNC_HINT));
}

// ============================================================================
// Registration
// ============================================================================

void register_window(mrb_state* mrb) {
    // GMR::Window module
    RClass* window = get_gmr_submodule(mrb, "Window");

    mrb_define_module_function(mrb, window, "width", mrb_window_width, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "height", mrb_window_height, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "screen_width", mrb_window_screen_width, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "screen_height", mrb_window_screen_height, MRB_ARGS_NONE());
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

    // DPR (Device Pixel Ratio) control for web builds
    mrb_define_module_function(mrb, window, "use_native_dpr", mrb_window_use_native_dpr, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "use_css_dpr", mrb_window_use_css_dpr, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "device_pixel_ratio", mrb_window_device_pixel_ratio, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "native_dpr?", mrb_window_is_native_dpr, MRB_ARGS_NONE());

    mrb_define_module_function(mrb, window, "monitor_count", mrb_window_monitor_count, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "monitor_width", mrb_window_monitor_width, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "monitor_height", mrb_window_monitor_height, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "monitor_refresh_rate", mrb_window_monitor_refresh_rate, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "monitor_name", mrb_window_monitor_name, MRB_ARGS_REQ(1));

    // VSync control
    mrb_define_module_function(mrb, window, "vsync=", mrb_window_set_vsync, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, window, "vsync?", mrb_window_is_vsync, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, window, "set_vsync", mrb_window_set_vsync, MRB_ARGS_REQ(1));

    // GMR::Time module
    RClass* time_mod = get_gmr_submodule(mrb, "Time");

    mrb_define_module_function(mrb, time_mod, "delta", mrb_time_delta, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, time_mod, "elapsed", mrb_time_elapsed, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, time_mod, "fps", mrb_time_fps, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, time_mod, "set_target_fps", mrb_time_set_target_fps, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, time_mod, "scale", mrb_time_scale_get, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, time_mod, "scale=", mrb_time_scale_set, MRB_ARGS_REQ(1));
}

void cleanup_window() {
    if (State::instance().use_virtual_resolution) {
        UnloadRenderTexture(render_target);
    }
}

} // namespace bindings
} // namespace gmr
