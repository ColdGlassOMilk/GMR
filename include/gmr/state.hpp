#ifndef GMR_STATE_HPP
#define GMR_STATE_HPP

#include "gmr/types.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace gmr {

class State {
public:
    static State& instance();

    // Disable copy/move
    State(const State&) = delete;
    State& operator=(const State&) = delete;

    // Default clear color (used by GMR::Graphics.clear when no color provided)
    Color clear_color{0, 0, 0, 255};

    // Screen state (logical dimensions - what Ruby sees)
    int screen_width = 800;
    int screen_height = 600;
    bool is_fullscreen = false;
    int windowed_width = 800;
    int windowed_height = 600;

    // CSS/layout dimensions (what the browser uses for layout, mouse events)
    // These are the dimensions in CSS pixels, independent of devicePixelRatio
    int css_width = 800;
    int css_height = 600;

    // Physical render dimensions (actual backing buffer size)
    // When use_native_dpr is true, this is css_* * device_pixel_ratio
    // When use_native_dpr is false, this equals css_* (default)
    int render_width = 800;
    int render_height = 600;

    // Device pixel ratio (1.0 on standard displays, 2.0 on retina, etc.)
    float device_pixel_ratio = 1.0f;

    // Whether to render at native device pixel ratio (sharp on retina)
    // When false (default), renders at CSS resolution for better performance
    bool use_native_dpr = false;

    // Flag set when fullscreen changes on web (so main loop can notify Ruby)
    bool fullscreen_changed = false;

    // Flag set when canvas size changes on web (independent of fullscreen)
    bool canvas_resize_pending = false;

    // Virtual resolution
    int virtual_width = 800;
    int virtual_height = 600;
    bool use_virtual_resolution = false;

    // UI scale factor - automatically calculated from virtual resolution
    // Based on 360px height as baseline for UI elements
    // At 360p: scale = 1.0, at 1080p: scale = 3.0, at 128p: scale = 0.36
    float ui_scale() const {
        if (use_virtual_resolution) {
            return static_cast<float>(virtual_height) / 360.0f;
        }
        return static_cast<float>(screen_height) / 360.0f;
    }

    // Input action mappings (action_name -> list of key codes)
    std::unordered_map<std::string, std::vector<int>> input_actions;

    // Frame timing - set by main loop at start of each frame
    float frame_delta = 0.0f;

    // Time scale - multiplier for game time (1.0 = normal, 0.5 = half speed, 0 = paused)
    float time_scale = 1.0f;

    // Quit flag - set by GMR.quit() or window close
    bool should_quit = false;

    // VSync state - controls buffer swap synchronization with display refresh
    // When enabled, frame presentation waits for vertical blank to prevent tearing
    bool vsync_enabled = true;

    // Init flag - set after Ruby init() is called
    // Used to prevent on_resize from being called before init
    bool init_called = false;

private:
    State() = default;
};

} // namespace gmr

#endif
