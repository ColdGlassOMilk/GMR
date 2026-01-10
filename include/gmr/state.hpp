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

    // Actual canvas/render dimensions (for scaling virtual resolution to screen)
    // On Emscripten, GetScreenWidth() doesn't update after SetWindowSize(),
    // so we track this ourselves for reliable fullscreen handling.
    int canvas_width = 800;
    int canvas_height = 600;

    // Flag set when fullscreen changes on web (so main loop can notify Ruby)
    bool fullscreen_changed = false;

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

private:
    State() = default;
};

} // namespace gmr

#endif
