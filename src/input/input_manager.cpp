#include "gmr/input/input_manager.hpp"
#include "gmr/event/event_queue.hpp"
#include "gmr/scripting/helpers.hpp"
#include "raylib.h"
#include <algorithm>

namespace gmr {
namespace input {

InputManager& InputManager::instance() {
    static InputManager instance;
    return instance;
}

// ============================================================================
// Action Management
// ============================================================================

void InputManager::define_action(const std::string& name,
                                  const std::vector<InputBinding>& bindings) {
    ActionDefinition def;
    def.name = name;
    def.bindings = bindings;
    actions_[name] = def;
}

void InputManager::define_action_in_context(const std::string& context,
                                             const std::string& action,
                                             const std::vector<InputBinding>& bindings) {
    ActionDefinition def;
    def.name = action;
    def.bindings = bindings;
    ContextStack::instance().define(context).actions[action] = def;
}

void InputManager::remove_action(const std::string& name) {
    actions_.erase(name);
}

void InputManager::clear_actions() {
    actions_.clear();
}

ActionDefinition* InputManager::get_action(const std::string& name) {
    auto it = actions_.find(name);
    return it != actions_.end() ? &it->second : nullptr;
}

// ============================================================================
// Callback Registration
// ============================================================================

int InputManager::on(mrb_state* mrb, const std::string& action, InputPhase phase,
                     mrb_value callback, mrb_value context) {
    InputCallback cb;
    cb.id = next_callback_id_++;
    cb.action = action;
    cb.phase = phase;
    cb.callback = callback;
    cb.context = context;
    cb.active = true;

    // GC protect the callback and context
    if (!mrb_nil_p(callback)) {
        mrb_gc_register(mrb, callback);
    }
    if (!mrb_nil_p(context)) {
        mrb_gc_register(mrb, context);
    }

    callbacks_[cb.id] = cb;
    return cb.id;
}

void InputManager::off(mrb_state* mrb, int callback_id) {
    auto it = callbacks_.find(callback_id);
    if (it != callbacks_.end()) {
        // GC unprotect
        if (!mrb_nil_p(it->second.callback)) {
            mrb_gc_unregister(mrb, it->second.callback);
        }
        if (!mrb_nil_p(it->second.context)) {
            mrb_gc_unregister(mrb, it->second.context);
        }
        callbacks_.erase(it);
    }
}

void InputManager::off_all(mrb_state* mrb, const std::string& action) {
    std::vector<int> to_remove;
    for (auto& [id, cb] : callbacks_) {
        if (cb.action == action) {
            to_remove.push_back(id);
        }
    }
    for (int id : to_remove) {
        off(mrb, id);
    }
}

// ============================================================================
// State Machine Integration
// ============================================================================

void InputManager::register_state_machine_binding(mrb_state* mrb,
                                                   const StateMachineInputBinding& binding) {
    // GC protect the condition if present
    if (!mrb_nil_p(binding.condition)) {
        mrb_gc_register(mrb, binding.condition);
    }
    sm_bindings_.push_back(binding);
}

void InputManager::unregister_state_machine_bindings(mrb_state* mrb,
                                                      StateMachineHandle machine) {
    auto it = std::remove_if(sm_bindings_.begin(), sm_bindings_.end(),
        [mrb, machine](StateMachineInputBinding& binding) {
            if (binding.machine == machine) {
                if (!mrb_nil_p(binding.condition)) {
                    mrb_gc_unregister(mrb, binding.condition);
                }
                return true;
            }
            return false;
        });
    sm_bindings_.erase(it, sm_bindings_.end());
}

void InputManager::unregister_state_machine_bindings_for_state(mrb_state* mrb,
                                                                StateMachineHandle machine,
                                                                mrb_sym state) {
    auto it = std::remove_if(sm_bindings_.begin(), sm_bindings_.end(),
        [mrb, machine, state](StateMachineInputBinding& binding) {
            if (binding.machine == machine && binding.current_state == state) {
                if (!mrb_nil_p(binding.condition)) {
                    mrb_gc_unregister(mrb, binding.condition);
                }
                return true;
            }
            return false;
        });
    sm_bindings_.erase(it, sm_bindings_.end());
}

// ============================================================================
// Input Phase Checking
// ============================================================================

bool InputManager::check_action_phase(const ActionDefinition& action, InputPhase phase) {
    for (const auto& binding : action.bindings) {
        bool match = false;

        if (binding.source == InputSource::Keyboard) {
            switch (phase) {
                case InputPhase::Pressed:  match = IsKeyPressed(binding.code); break;
                case InputPhase::Released: match = IsKeyReleased(binding.code); break;
                case InputPhase::Held:     match = IsKeyDown(binding.code); break;
            }
        } else if (binding.source == InputSource::Mouse) {
            switch (phase) {
                case InputPhase::Pressed:  match = IsMouseButtonPressed(binding.code); break;
                case InputPhase::Released: match = IsMouseButtonReleased(binding.code); break;
                case InputPhase::Held:     match = IsMouseButtonDown(binding.code); break;
            }
        }
        // Gamepad support would go here in the future

        if (match) return true;
    }
    return false;
}

// ============================================================================
// Query Methods (for backward compatibility)
// ============================================================================

bool InputManager::action_pressed(const std::string& action) {
    auto* def = get_action(action);
    return def ? check_action_phase(*def, InputPhase::Pressed) : false;
}

bool InputManager::action_released(const std::string& action) {
    auto* def = get_action(action);
    return def ? check_action_phase(*def, InputPhase::Released) : false;
}

bool InputManager::action_down(const std::string& action) {
    auto* def = get_action(action);
    return def ? check_action_phase(*def, InputPhase::Held) : false;
}

// ============================================================================
// Dispatch
// ============================================================================

void InputManager::dispatch_callbacks(mrb_state* mrb, const std::string& action,
                                       InputPhase phase) {
    // Collect callbacks to invoke (avoid modification during iteration)
    std::vector<InputCallback*> to_invoke;
    for (auto& [id, cb] : callbacks_) {
        if (cb.active && cb.action == action && cb.phase == phase) {
            to_invoke.push_back(&cb);
        }
    }

    // Invoke callbacks
    for (auto* cb : to_invoke) {
        if (!mrb_nil_p(cb->context)) {
            // instance_exec on context object
            scripting::safe_method_call(mrb, cb->context, "instance_exec", {cb->callback});
        } else {
            // Direct call
            scripting::safe_method_call(mrb, cb->callback, "call");
        }
    }
}

void InputManager::poll_and_dispatch(mrb_state* mrb) {
    auto& context_stack = ContextStack::instance();
    auto& event_queue = event::EventQueue::instance();

    // Helper lambda to process actions from a map
    auto process_actions = [&](std::unordered_map<std::string, ActionDefinition>& actions) {
        for (auto& [name, action] : actions) {
            // Check each phase and dispatch if active
            // Order: Pressed first, then Released, then Held
            // This ensures one-shot events fire before continuous ones

            if (check_action_phase(action, InputPhase::Pressed)) {
                dispatch_callbacks(mrb, name, InputPhase::Pressed);
                // Enqueue event for other subscribers (e.g., StateMachineManager)
                event_queue.enqueue(event::InputActionEvent{name, InputPhase::Pressed});
            }

            if (check_action_phase(action, InputPhase::Released)) {
                dispatch_callbacks(mrb, name, InputPhase::Released);
                event_queue.enqueue(event::InputActionEvent{name, InputPhase::Released});
            }

            if (check_action_phase(action, InputPhase::Held)) {
                dispatch_callbacks(mrb, name, InputPhase::Held);
                event_queue.enqueue(event::InputActionEvent{name, InputPhase::Held});
            }
        }
    };

    // Check if current context blocks global actions
    bool global_blocked = false;

    // Process actions from current context (if any)
    if (InputContext* ctx = context_stack.current()) {
        process_actions(ctx->actions);
        global_blocked = ctx->blocks_global;
    }

    // Process global actions unless blocked by current context
    if (!global_blocked) {
        process_actions(actions_);
    }
}

// ============================================================================
// Cleanup
// ============================================================================

void InputManager::clear(mrb_state* mrb) {
    // Unregister all callbacks from GC
    for (auto& [id, cb] : callbacks_) {
        if (!mrb_nil_p(cb.callback)) {
            mrb_gc_unregister(mrb, cb.callback);
        }
        if (!mrb_nil_p(cb.context)) {
            mrb_gc_unregister(mrb, cb.context);
        }
    }
    callbacks_.clear();

    // Unregister all state machine binding conditions from GC
    for (auto& binding : sm_bindings_) {
        if (!mrb_nil_p(binding.condition)) {
            mrb_gc_unregister(mrb, binding.condition);
        }
    }
    sm_bindings_.clear();

    // Clear actions
    actions_.clear();

    // Clear context stack
    ContextStack::instance().clear_all();
}

} // namespace input
} // namespace gmr
