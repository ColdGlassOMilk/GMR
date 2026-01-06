#ifndef GMR_STATE_MACHINE_HPP
#define GMR_STATE_MACHINE_HPP

#include "gmr/types.hpp"
#include <mruby.h>
#include <vector>
#include <unordered_map>

namespace gmr {
namespace state_machine {

// Represents a single transition rule
struct TransitionDefinition {
    mrb_sym event;           // Event symbol that triggers this transition
    mrb_sym target_state;    // State to transition to
    mrb_value condition;     // Optional Ruby proc for conditional transition

    TransitionDefinition()
        : event(0)
        , target_state(0)
        , condition(mrb_nil_value()) {}
};

// Represents a single state with its behavior
struct StateDefinition {
    mrb_sym name;                              // State name symbol
    mrb_sym animation_name;                    // Animation to play (optional, 0 = none)
    mrb_value enter_callback;                  // Proc called on state entry
    mrb_value exit_callback;                   // Proc called on state exit
    std::vector<TransitionDefinition> transitions;  // Outbound transitions

    StateDefinition()
        : name(0)
        , animation_name(0)
        , enter_callback(mrb_nil_value())
        , exit_callback(mrb_nil_value()) {}
};

// Complete state machine instance
struct StateMachineState {
    // Owner reference
    mrb_value owner;                 // Ruby object this FSM is attached to
    mrb_value sprite_ref;            // Sprite for animation binding (auto-detected or explicit)
    SpriteHandle sprite_handle;      // C++ sprite handle for animation

    // State definitions
    std::unordered_map<mrb_sym, StateDefinition> states;

    // Current state
    mrb_sym current_state;           // Currently active state symbol
    mrb_sym initial_state;           // First defined state (default initial)

    // Animation integration
    std::unordered_map<mrb_sym, SpriteAnimationHandle> animations;  // name -> animation handle
    SpriteAnimationHandle current_animation;  // Currently playing animation

    // Ruby object references (GC protection)
    mrb_value ruby_machine_obj;      // The Ruby StateMachine wrapper object

    // State flags
    bool active{true};               // Is this FSM active?
    bool initialized{false};         // Has initial state been entered?

    StateMachineState()
        : owner(mrb_nil_value())
        , sprite_ref(mrb_nil_value())
        , sprite_handle(INVALID_HANDLE)
        , current_state(0)
        , initial_state(0)
        , current_animation(INVALID_HANDLE)
        , ruby_machine_obj(mrb_nil_value()) {}

    // Get current state definition (nullptr if not found)
    StateDefinition* get_current_state_def() {
        auto it = states.find(current_state);
        return it != states.end() ? &it->second : nullptr;
    }

    // Get state definition by symbol (nullptr if not found)
    StateDefinition* get_state_def(mrb_sym state_sym) {
        auto it = states.find(state_sym);
        return it != states.end() ? &it->second : nullptr;
    }

    // Check if a transition is valid for current state
    const TransitionDefinition* find_transition(mrb_sym event) const {
        auto it = states.find(current_state);
        if (it == states.end()) return nullptr;

        for (const auto& trans : it->second.transitions) {
            if (trans.event == event) {
                return &trans;
            }
        }
        return nullptr;
    }
};

} // namespace state_machine
} // namespace gmr

#endif
