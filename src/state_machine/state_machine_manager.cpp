#include "gmr/state_machine/state_machine_manager.hpp"
#include "gmr/animation/animation_manager.hpp"
#include "gmr/input/input_manager.hpp"
#include <mruby/variable.h>
#include <mruby/data.h>
#include <mruby/hash.h>

namespace gmr {
namespace state_machine {

StateMachineManager& StateMachineManager::instance() {
    static StateMachineManager instance;
    return instance;
}

// ============================================================================
// Machine Lifecycle
// ============================================================================

StateMachineHandle StateMachineManager::create() {
    StateMachineHandle handle = next_id_++;
    machines_[handle] = StateMachineState();
    return handle;
}

void StateMachineManager::destroy(StateMachineHandle handle) {
    auto it = machines_.find(handle);
    if (it != machines_.end()) {
        machines_.erase(it);
    }
}

StateMachineState* StateMachineManager::get(StateMachineHandle handle) {
    auto it = machines_.find(handle);
    if (it != machines_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// State Transitions
// ============================================================================

bool StateMachineManager::trigger(mrb_state* mrb, StateMachineHandle handle, mrb_sym event) {
    StateMachineState* machine = get(handle);
    if (!machine || !machine->active) return false;

    // Find transition for this event in current state
    const TransitionDefinition* trans = machine->find_transition(event);
    if (!trans) return false;  // Silent ignore (design decision)

    // Check condition if present
    if (!mrb_nil_p(trans->condition)) {
        if (!check_condition(mrb, *machine, *trans)) {
            return false;  // Condition not met, ignore
        }
    }

    // Perform the transition
    mrb_sym from = machine->current_state;
    mrb_sym to = trans->target_state;
    perform_transition(mrb, *machine, from, to);
    return true;
}

void StateMachineManager::set_state(mrb_state* mrb, StateMachineHandle handle, mrb_sym state) {
    StateMachineState* machine = get(handle);
    if (!machine || !machine->active) return;

    // Check if state exists
    if (machine->states.find(state) == machine->states.end()) {
        return;  // Silent ignore for invalid state
    }

    mrb_sym from = machine->current_state;
    perform_transition(mrb, *machine, from, state);
}

void StateMachineManager::perform_transition(mrb_state* mrb, StateMachineState& machine,
                                             mrb_sym from_state, mrb_sym to_state) {
    // 1. Call exit callback on current state (if we have a current state)
    if (from_state != 0) {
        StateDefinition* from_def = machine.get_state_def(from_state);
        if (from_def && !mrb_nil_p(from_def->exit_callback)) {
            invoke_callback(mrb, from_def->exit_callback, machine.owner);
        }
    }

    // 2. Stop current animation
    stop_current_animation(/* need handle */ INVALID_HANDLE);

    // 3. Update current state
    machine.current_state = to_state;

    // 4. Start animation for new state (if defined)
    StateDefinition* to_def = machine.get_state_def(to_state);
    if (to_def) {
        if (to_def->animation_name != 0) {
            // Find the handle for this machine
            for (auto& [h, m] : machines_) {
                if (&m == &machine) {
                    play_animation(mrb, h, to_def->animation_name);
                    break;
                }
            }
        }

        // 5. Call enter callback
        if (!mrb_nil_p(to_def->enter_callback)) {
            invoke_callback(mrb, to_def->enter_callback, machine.owner);
        }
    }
}

bool StateMachineManager::check_condition(mrb_state* mrb, StateMachineState& machine,
                                          const TransitionDefinition& trans) {
    if (mrb_nil_p(trans.condition)) return true;

    // Execute the condition proc in the owner's context using instance_exec
    mrb_value result = mrb_funcall(mrb, machine.owner, "instance_exec",
                                   1, trans.condition);

    if (mrb->exc) {
        // Clear exception and treat as false
        mrb->exc = nullptr;
        return false;
    }

    return mrb_test(result);
}

void StateMachineManager::invoke_callback(mrb_state* mrb, mrb_value callback, mrb_value owner) {
    if (mrb_nil_p(callback)) return;

    // Execute the callback proc in the owner's context using instance_exec
    mrb_funcall(mrb, owner, "instance_exec", 1, callback);

    if (mrb->exc) {
        // Clear exception silently
        mrb->exc = nullptr;
    }
}

// ============================================================================
// Sprite Detection
// ============================================================================

// Forward declaration for sprite data extraction
struct SpriteBindingData {
    SpriteHandle handle;
};

void StateMachineManager::detect_sprite(mrb_state* mrb, StateMachineHandle handle) {
    StateMachineState* machine = get(handle);
    if (!machine) return;

    // Try to get @sprite instance variable from owner
    mrb_value sprite_val = mrb_iv_get(mrb, machine->owner, mrb_intern_lit(mrb, "@sprite"));
    if (mrb_nil_p(sprite_val)) {
        machine->sprite_handle = INVALID_HANDLE;
        return;
    }

    // Store sprite reference
    machine->sprite_ref = sprite_val;

    // Extract SpriteHandle from Ruby Sprite object
    void* ptr = mrb_data_get_ptr(mrb, sprite_val, nullptr);
    if (ptr) {
        SpriteBindingData* data = static_cast<SpriteBindingData*>(ptr);
        machine->sprite_handle = data->handle;
    } else {
        machine->sprite_handle = INVALID_HANDLE;
    }
}

// ============================================================================
// Animation Integration
// ============================================================================

// Forward declaration for animation data extraction
struct SpriteAnimationBindingData {
    SpriteAnimationHandle handle;
};

void StateMachineManager::cache_animations(mrb_state* mrb, StateMachineHandle handle) {
    StateMachineState* machine = get(handle);
    if (!machine) return;

    // Try to get @animations hash from owner
    mrb_value anims_hash = mrb_iv_get(mrb, machine->owner, mrb_intern_lit(mrb, "@animations"));
    if (mrb_nil_p(anims_hash) || !mrb_hash_p(anims_hash)) {
        return;  // No @animations hash, that's okay
    }

    // Iterate over all states that have animation names
    for (auto& [state_sym, state_def] : machine->states) {
        if (state_def.animation_name == 0) continue;

        // Look up animation in hash by symbol key
        mrb_value anim_val = mrb_hash_get(mrb, anims_hash,
            mrb_symbol_value(state_def.animation_name));

        if (mrb_nil_p(anim_val)) continue;

        // Extract SpriteAnimationHandle from Ruby SpriteAnimation object
        void* ptr = mrb_data_get_ptr(mrb, anim_val, nullptr);
        if (ptr) {
            SpriteAnimationBindingData* data = static_cast<SpriteAnimationBindingData*>(ptr);
            machine->animations[state_def.animation_name] = data->handle;
        }
    }
}

void StateMachineManager::play_animation(mrb_state* mrb, StateMachineHandle handle, mrb_sym name) {
    StateMachineState* machine = get(handle);
    if (!machine) return;

    // First, cache animations if not done yet
    if (machine->animations.empty()) {
        cache_animations(mrb, handle);
    }

    // Stop current animation
    stop_current_animation(handle);

    // Find animation by name
    auto it = machine->animations.find(name);
    if (it == machine->animations.end()) return;

    machine->current_animation = it->second;

    // Get the animation state and play it
    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(it->second);
    if (anim) {
        anim->current_frame_index = 0;
        anim->elapsed = 0.0f;
        anim->playing = true;
        anim->completed = false;
    }
}

void StateMachineManager::stop_current_animation(StateMachineHandle handle) {
    StateMachineState* machine = get(handle);
    if (!machine || machine->current_animation == INVALID_HANDLE) return;

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(machine->current_animation);
    if (anim) {
        anim->playing = false;
    }

    machine->current_animation = INVALID_HANDLE;
}

// ============================================================================
// Update Loop
// ============================================================================

void StateMachineManager::update(mrb_state* mrb, float dt) {
    (void)dt;  // State machines are event-driven, no time-based updates

    // Initialize machines that haven't entered initial state yet
    for (auto& [handle, machine] : machines_) {
        if (machine.active && !machine.initialized && machine.initial_state != 0) {
            // Detect sprite if not already set
            if (machine.sprite_handle == INVALID_HANDLE) {
                detect_sprite(mrb, handle);
            }

            // Enter initial state
            set_state(mrb, handle, machine.initial_state);
            machine.initialized = true;
        }
    }
}

// ============================================================================
// Cleanup
// ============================================================================

void StateMachineManager::clear(mrb_state* mrb) {
    // Unregister input bindings for all state machines
    for (auto& [handle, machine] : machines_) {
        input::InputManager::instance().unregister_state_machine_bindings(mrb, handle);
    }

    // Unregister all Ruby objects from GC
    for (auto& [handle, machine] : machines_) {
        if (!mrb_nil_p(machine.ruby_machine_obj)) {
            mrb_gc_unregister(mrb, machine.ruby_machine_obj);
        }
    }

    machines_.clear();
    // Don't reset next_id_ - keeps handles unique across reloads
}

} // namespace state_machine
} // namespace gmr
