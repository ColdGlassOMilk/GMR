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

    // Screen state
    int screen_width = 800;
    int screen_height = 600;
    bool is_fullscreen = false;
    int windowed_width = 800;
    int windowed_height = 600;

    // Virtual resolution
    int virtual_width = 800;
    int virtual_height = 600;
    bool use_virtual_resolution = false;

    // Input action mappings (action_name -> list of key codes)
    std::unordered_map<std::string, std::vector<int>> input_actions;

private:
    State() = default;
};

} // namespace gmr

#endif
