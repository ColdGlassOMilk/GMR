#ifndef GMR_INPUT_EVENT_HPP
#define GMR_INPUT_EVENT_HPP

#include "gmr/types.hpp"
#include <mruby.h>
#include <string>
#include <vector>

namespace gmr {
namespace input {

// Input source types
enum class InputSource {
    Keyboard,
    Mouse,
    Gamepad  // Future-proofed, not yet implemented
};

// Input phase (when the callback fires)
enum class InputPhase {
    Pressed,   // Just pressed this frame
    Released,  // Just released this frame
    Held       // Held down (fires every frame while down)
};

// A single input binding (key, mouse button, or gamepad button)
struct InputBinding {
    InputSource source{InputSource::Keyboard};
    int code{0};  // KEY_* for keyboard, MOUSE_BUTTON_* for mouse
};

// An action definition with its bindings
struct ActionDefinition {
    std::string name;
    std::vector<InputBinding> bindings;
};

// A callback registration for standalone input handling
struct InputCallback {
    int id{0};                              // Unique callback ID
    std::string action;                     // Action name to listen for
    InputPhase phase{InputPhase::Pressed};  // When to fire
    mrb_value callback{mrb_nil_value()};    // Ruby proc/lambda
    mrb_value context{mrb_nil_value()};     // Object for instance_exec (optional)
    bool active{true};

    InputCallback() = default;
};

// State machine input subscription (for on_input DSL and verb-style DSL)
struct StateMachineInputBinding {
    StateMachineHandle machine{INVALID_HANDLE};
    mrb_sym current_state{0};               // State this binding belongs to
    std::string action;                     // Input action name
    mrb_sym target_state{0};                // State to transition to
    InputPhase phase{InputPhase::Pressed};  // When to trigger
    mrb_value condition{mrb_nil_value()};   // Optional if: condition
    bool forced{false};                     // Skip condition check (for action! syntax)

    StateMachineInputBinding() = default;
};

} // namespace input
} // namespace gmr

#endif
