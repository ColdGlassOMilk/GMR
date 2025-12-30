#ifndef GMR_STATE_HPP
#define GMR_STATE_HPP

#include "gmr/types.hpp"

namespace gmr {

class State {
public:
    static State& instance();
    
    // Disable copy/move
    State(const State&) = delete;
    State& operator=(const State&) = delete;
    
    // Drawing state
    Color current_color{255, 255, 255, 255};
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
    
private:
    State() = default;
};

} // namespace gmr

#endif
