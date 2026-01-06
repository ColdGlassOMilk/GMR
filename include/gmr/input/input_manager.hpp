#ifndef GMR_INPUT_MANAGER_HPP
#define GMR_INPUT_MANAGER_HPP

#include "gmr/input/input_event.hpp"
#include <mruby.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace gmr {
namespace input {

class InputManager {
public:
    static InputManager& instance();

    // === Action Management ===
    void define_action(const std::string& name, const std::vector<InputBinding>& bindings);
    void remove_action(const std::string& name);
    void clear_actions();
    ActionDefinition* get_action(const std::string& name);
    const std::unordered_map<std::string, ActionDefinition>& get_all_actions() const { return actions_; }

    // === Callback Registration ===
    // Returns callback ID for later removal
    int on(mrb_state* mrb, const std::string& action, InputPhase phase,
           mrb_value callback, mrb_value context = mrb_nil_value());
    void off(mrb_state* mrb, int callback_id);
    void off_all(mrb_state* mrb, const std::string& action);

    // === State Machine Integration ===
    void register_state_machine_binding(mrb_state* mrb, const StateMachineInputBinding& binding);
    void unregister_state_machine_bindings(mrb_state* mrb, StateMachineHandle machine);
    void unregister_state_machine_bindings_for_state(mrb_state* mrb,
                                                      StateMachineHandle machine,
                                                      mrb_sym state);

    // === Per-Frame Update (called from main.cpp) ===
    void poll_and_dispatch(mrb_state* mrb);

    // === Query (for backward compatibility and direct polling) ===
    bool action_pressed(const std::string& action);
    bool action_released(const std::string& action);
    bool action_down(const std::string& action);

    // === Cleanup ===
    void clear(mrb_state* mrb);

    // === Debug ===
    size_t action_count() const { return actions_.size(); }
    size_t callback_count() const { return callbacks_.size(); }
    size_t sm_binding_count() const { return sm_bindings_.size(); }

private:
    InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // Check if an action's bindings match a given phase
    bool check_action_phase(const ActionDefinition& action, InputPhase phase);

    // Dispatch to callbacks
    void dispatch_callbacks(mrb_state* mrb, const std::string& action, InputPhase phase);

    // Dispatch to state machines
    void dispatch_to_state_machines(mrb_state* mrb, const std::string& action, InputPhase phase);

    // Storage
    std::unordered_map<std::string, ActionDefinition> actions_;
    std::unordered_map<int, InputCallback> callbacks_;
    std::vector<StateMachineInputBinding> sm_bindings_;
    int next_callback_id_{1};  // Start at 1 so 0 can indicate "no callback"
};

} // namespace input
} // namespace gmr

#endif
