#include "gmr/state_machine/state_machine_manager.hpp"
#include "gmr/animation/animation_manager.hpp"
#include "gmr/input/input_manager.hpp"
#include "gmr/event/event_queue.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/variable.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/class.h>

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

    // Cache transition info before any callbacks
    mrb_sym from = machine->current_state;
    mrb_sym to = trans->target_state;
    mrb_value condition = trans->condition;

    // Check condition if present (pass handle, not reference, to avoid invalidation)
    if (!mrb_nil_p(condition)) {
        if (!check_condition(mrb, handle, condition)) {
            return false;  // Condition not met or machine destroyed
        }
        // Re-get machine after condition check (callback might have changed things)
        machine = get(handle);
        if (!machine || !machine->active) return false;
    }

    // Perform the transition (using handle, not reference)
    perform_transition(mrb, handle, from, to);
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
    perform_transition(mrb, handle, from, state);
}

void StateMachineManager::perform_transition(mrb_state* mrb, StateMachineHandle handle,
                                             mrb_sym from_state, mrb_sym to_state) {
    StateMachineState* machine = get(handle);
    if (!machine) return;

    // Cache owner before any operations that might trigger GC
    mrb_value owner = machine->owner;
    if (mrb_nil_p(owner)) return;

    // 1. Call exit callback on current state (if we have a current state)
    if (from_state != 0) {
        StateDefinition* from_def = machine->get_state_def(from_state);
        if (from_def && !mrb_nil_p(from_def->exit_callback)) {
            // Cache callback before call (in case machine gets invalidated)
            mrb_value exit_cb = from_def->exit_callback;
            invoke_callback(mrb, exit_cb, owner);
            // Re-get machine pointer after callback (it might have been moved)
            machine = get(handle);
            if (!machine) return;
        }
    }

    // 2. Stop current animation
    stop_current_animation(handle);

    // Re-get machine after stop_current_animation
    machine = get(handle);
    if (!machine) return;

    // 3. Update current state
    machine->current_state = to_state;

    // 4. Start animation for new state (if defined)
    StateDefinition* to_def = machine->get_state_def(to_state);
    if (to_def) {
        if (to_def->animation_name != 0) {
            play_animation(mrb, handle, to_def->animation_name);
            // Re-get machine after play_animation
            machine = get(handle);
            if (!machine) return;
            to_def = machine->get_state_def(to_state);
            if (!to_def) return;
        }

        // 5. Call enter callback
        if (!mrb_nil_p(to_def->enter_callback)) {
            // Cache callback before call
            mrb_value enter_cb = to_def->enter_callback;
            invoke_callback(mrb, enter_cb, owner);
        }
    }
}

bool StateMachineManager::check_condition(mrb_state* mrb, StateMachineHandle handle, mrb_value condition) {
    // Early return if no condition
    if (mrb_nil_p(condition)) return true;

    // Get machine fresh (don't use stale references)
    StateMachineState* machine = get(handle);
    if (!machine) return false;  // Machine destroyed, don't transition
    if (mrb_nil_p(machine->owner)) return true;  // No owner, allow transition

    // Cache owner before callback (owner is GC-protected via ruby_machine_obj's @owner)
    mrb_value owner = machine->owner;

    // Execute the condition proc in the owner's context using instance_exec (protected)
    // IMPORTANT: Must use safe_instance_exec - mrb_funcall_with_block for proper block passing
    // Note: After this call, machine pointer may be invalid (don't use it)
    mrb_value result = scripting::safe_instance_exec(mrb, owner, condition);
    return mrb_test(result);
}

void StateMachineManager::invoke_callback(mrb_state* mrb, mrb_value callback, mrb_value owner) {
    if (mrb_nil_p(callback)) return;
    if (mrb_nil_p(owner)) return;  // Safety check - owner must be valid

    // Execute the callback proc in the owner's context using instance_exec (protected)
    // IMPORTANT: Must use safe_instance_exec - mrb_funcall_with_block for proper block passing
    scripting::safe_instance_exec(mrb, owner, callback);
}

// ============================================================================
// Sprite Detection
// ============================================================================

void StateMachineManager::detect_sprite(mrb_state* mrb, StateMachineHandle handle) {
    StateMachineState* machine = get(handle);
    if (!machine) return;

    // Default to invalid - sprite detection is optional
    machine->sprite_handle = INVALID_HANDLE;

    // Check if owner is valid
    if (mrb_nil_p(machine->owner)) {
        return;
    }

    // Try to get @sprite instance variable from owner
    // This is optional - state machine works without a sprite
    mrb_value sprite_val = mrb_iv_get(mrb, machine->owner, mrb_intern_lit(mrb, "@sprite"));
    if (mrb_nil_p(sprite_val)) {
        return;
    }

    // Store sprite reference for GC protection
    machine->sprite_ref = sprite_val;

    // Also store in instance variable on the machine wrapper for GC protection
    if (!mrb_nil_p(machine->ruby_machine_obj)) {
        mrb_iv_set(mrb, machine->ruby_machine_obj,
            mrb_intern_cstr(mrb, "@_sprite_ref"), sprite_val);
    }

    // For now, don't try to extract the handle - we can add this later if needed
    // The state machine can work without direct sprite handle access
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
    if (mrb_nil_p(machine->owner)) return;  // Owner must be valid

    // Try to get @animations hash from owner
    mrb_value anims_hash = mrb_iv_get(mrb, machine->owner, mrb_intern_lit(mrb, "@animations"));
    if (mrb_nil_p(anims_hash) || !mrb_hash_p(anims_hash)) {
        return;  // No @animations hash, that's okay
    }

    // Get the SpriteAnimation class for type validation
    // This prevents type confusion crashes if @animations contains non-SpriteAnimation objects
    RClass* gmr_module = mrb_module_get(mrb, "GMR");
    if (!gmr_module) return;
    RClass* sprite_anim_class = mrb_class_get_under(mrb, gmr_module, "SpriteAnimation");
    if (!sprite_anim_class) return;

    // Iterate over all states that have animation names
    for (auto& [state_sym, state_def] : machine->states) {
        if (state_def.animation_name == 0) continue;

        // Look up animation in hash by symbol key
        mrb_value anim_val = mrb_hash_get(mrb, anims_hash,
            mrb_symbol_value(state_def.animation_name));

        if (mrb_nil_p(anim_val)) continue;

        // TYPE SAFETY: Verify this is actually a SpriteAnimation before extracting data
        // This prevents crashes from type confusion (e.g., @animations[:idle] = "wrong type")
        if (!mrb_obj_is_kind_of(mrb, anim_val, sprite_anim_class)) {
            continue;  // Skip non-SpriteAnimation values
        }

        // Now safe to extract - we know it's the correct type
        // Still use nullptr for data_type since sprite_animation_data_type is static in another file
        // but the class check above ensures type safety
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

    // Subscribe to input events on first update (lazy initialization)
    // We capture 'this' and 'mrb' - mrb is passed each frame so this is safe
    if (input_subscription_ == event::INVALID_SUBSCRIPTION) {
        input_subscription_ = event::EventQueue::instance().subscribe<event::InputActionEvent>(
            [this, mrb](const event::InputActionEvent& e) {
                handle_input_event(mrb, e);
            });
    }

    // Collect handles to process FIRST (NOT references - safe if map is modified)
    // This prevents iterator invalidation when callbacks create/destroy state machines
    std::vector<StateMachineHandle> handles_to_init;
    for (const auto& [handle, machine] : machines_) {
        if (machine.active && !machine.initialized && machine.initial_state != 0) {
            handles_to_init.push_back(handle);
        }
    }

    // Process collected handles (safe even if map is modified during callbacks)
    for (StateMachineHandle handle : handles_to_init) {
        StateMachineState* machine = get(handle);
        if (!machine || machine->initialized) continue;  // Skip if destroyed or already done

        // Detect sprite if not already set
        if (machine->sprite_handle == INVALID_HANDLE) {
            detect_sprite(mrb, handle);
        }

        // Re-get after detect_sprite (might have triggered GC/map modification)
        machine = get(handle);
        if (!machine) continue;

        // Cache initial_state before set_state (callbacks could modify machine)
        mrb_sym initial = machine->initial_state;

        // Enter initial state
        set_state(mrb, handle, initial);

        // Re-get after set_state (callbacks might have modified map)
        machine = get(handle);
        if (machine) {
            machine->initialized = true;
        }
    }
}

// ============================================================================
// Input Event Handling (via EventQueue subscription)
// ============================================================================

void StateMachineManager::handle_input_event(mrb_state* mrb,
                                              const event::InputActionEvent& event) {
    // Get state machine input bindings from InputManager
    auto& input_mgr = input::InputManager::instance();
    const auto& bindings = input_mgr.get_sm_bindings();

    // Collect bindings to process (avoid modification during iteration)
    std::vector<const input::StateMachineInputBinding*> to_process;
    for (const auto& binding : bindings) {
        if (binding.action == event.action && binding.phase == event.phase) {
            to_process.push_back(&binding);
        }
    }

    for (const auto* binding : to_process) {
        // Get the state machine
        auto* machine = get(binding->machine);
        if (!machine || !machine->active) continue;

        // Check if we're in the correct state for this binding
        if (machine->current_state != binding->current_state) continue;

        // Check condition if present (skip if forced)
        if (!binding->forced && !mrb_nil_p(binding->condition)) {
            if (!check_condition(mrb, binding->machine, binding->condition)) {
                continue;
            }
            // Re-get machine after condition check
            machine = get(binding->machine);
            if (!machine || !machine->active) continue;
        }

        // Trigger the state transition
        set_state(mrb, binding->machine, binding->target_state);
    }
}

// ============================================================================
// Cleanup
// ============================================================================

void StateMachineManager::clear(mrb_state* mrb) {
    // Unsubscribe from input events
    if (input_subscription_ != event::INVALID_SUBSCRIPTION) {
        event::EventQueue::instance().unsubscribe(mrb, input_subscription_);
        input_subscription_ = event::INVALID_SUBSCRIPTION;
    }

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
