#ifndef GMR_EVENT_HPP
#define GMR_EVENT_HPP

#include "gmr/input/input_event.hpp"
#include "gmr/types.hpp"
#include <mruby.h>
#include <string>
#include <variant>

namespace gmr {
namespace event {

// Event type enumeration for filtering subscriptions
enum class EventType {
    InputAction,    // Input action triggered (pressed/released/held)
    StateChanged,   // State machine transition completed (future use)
    Custom          // User-defined events (future Ruby integration)
};

// Input action event - published by InputManager
struct InputActionEvent {
    static constexpr EventType type = EventType::InputAction;
    std::string action;              // Action name (e.g., "jump", "attack")
    input::InputPhase phase;         // Pressed, Released, or Held
};

// State change event - published by StateMachineManager (future use)
struct StateChangedEvent {
    static constexpr EventType type = EventType::StateChanged;
    StateMachineHandle machine;
    mrb_sym from_state;
    mrb_sym to_state;
};

// Type-safe event variant
using Event = std::variant<InputActionEvent, StateChangedEvent>;

// Helper to get EventType from any event
inline EventType get_event_type(const Event& e) {
    return std::visit([](auto&& evt) { return evt.type; }, e);
}

} // namespace event
} // namespace gmr

#endif
