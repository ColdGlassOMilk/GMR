#ifndef GMR_STATE_MACHINE_MANAGER_HPP
#define GMR_STATE_MACHINE_MANAGER_HPP

#include "gmr/state_machine/state_machine.hpp"
#include "gmr/event/event_queue.hpp"
#include <mruby.h>
#include <unordered_map>

namespace gmr {

// Forward declaration
using StateMachineHandle = int32_t;

namespace state_machine {

class StateMachineManager {
public:
    static StateMachineManager& instance();

    // Per-frame update (called from main.cpp after AnimationManager)
    void update(mrb_state* mrb, float dt);

    // === Machine Lifecycle ===
    StateMachineHandle create();
    void destroy(StateMachineHandle handle);
    StateMachineState* get(StateMachineHandle handle);

    // === State Transitions ===
    // Trigger an event on a machine (returns true if transition occurred)
    bool trigger(mrb_state* mrb, StateMachineHandle handle, mrb_sym event);

    // Force state change (bypasses transitions, still calls enter/exit)
    void set_state(mrb_state* mrb, StateMachineHandle handle, mrb_sym state);

    // === Animation Integration ===
    // Cache animations from owner's @animations hash
    void cache_animations(mrb_state* mrb, StateMachineHandle handle);

    // Play animation by name on a machine
    void play_animation(mrb_state* mrb, StateMachineHandle handle, mrb_sym name);

    // Stop current animation
    void stop_current_animation(StateMachineHandle handle);

    // === Sprite Detection ===
    // Auto-detect sprite from owner's @sprite instance variable
    void detect_sprite(mrb_state* mrb, StateMachineHandle handle);

    // === Cleanup ===
    void clear(mrb_state* mrb);

    // === Debug ===
    size_t count() const { return machines_.size(); }

private:
    StateMachineManager() = default;
    StateMachineManager(const StateMachineManager&) = delete;
    StateMachineManager& operator=(const StateMachineManager&) = delete;

    // Transition logic
    void perform_transition(mrb_state* mrb, StateMachineHandle handle,
                           mrb_sym from_state, mrb_sym to_state);

    // Check conditional transition (uses handle to avoid reference invalidation)
    bool check_condition(mrb_state* mrb, StateMachineHandle handle, mrb_value condition);

    // Callback invocation
    void invoke_callback(mrb_state* mrb, mrb_value callback, mrb_value owner);

    // Handle input events from EventQueue subscription
    void handle_input_event(mrb_state* mrb, const event::InputActionEvent& event);

    // Storage
    std::unordered_map<StateMachineHandle, StateMachineState> machines_;
    StateMachineHandle next_id_{0};

    // Event subscription handle for input events
    event::SubscriptionHandle input_subscription_{event::INVALID_SUBSCRIPTION};
};

} // namespace state_machine
} // namespace gmr

#endif
