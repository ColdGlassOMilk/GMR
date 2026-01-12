#include "gmr/scene.hpp"
#include "gmr/transition/transition_manager.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/gc.h>
#include <cstdio>

namespace gmr {

SceneManager& SceneManager::instance() {
    static SceneManager inst;
    return inst;
}

void SceneManager::call_init(mrb_state* mrb, Scene& scene) {
    if (scene.initialized) return;

    scripting::safe_method_call(mrb, scene.object, "init");
    scene.initialized = true;
}

void SceneManager::call_unload(mrb_state* mrb, Scene& scene) {
    scripting::safe_method_call(mrb, scene.object, "unload");
}

void SceneManager::call_update(mrb_state* mrb, Scene& scene, float dt) {
    scripting::safe_method_call(mrb, scene.object, "update", {mrb_float_value(mrb, dt)});
}

void SceneManager::call_draw(mrb_state* mrb, Scene& scene) {
    scripting::safe_method_call(mrb, scene.object, "draw");
}

void SceneManager::load(mrb_state* mrb, mrb_value scene_obj) {
    // Unload all existing scenes (top to bottom)
    while (!stack_.empty()) {
        call_unload(mrb, stack_.back());
        mrb_gc_unregister(mrb, stack_.back().object);
        stack_.pop_back();
    }

    // Protect from GC and push new scene
    mrb_gc_register(mrb, scene_obj);
    stack_.emplace_back(scene_obj);
    call_init(mrb, stack_.back());
}

void SceneManager::push(mrb_state* mrb, mrb_value scene_obj) {
    // Protect from GC and push
    mrb_gc_register(mrb, scene_obj);
    stack_.emplace_back(scene_obj);
    call_init(mrb, stack_.back());
}

void SceneManager::pop(mrb_state* mrb) {
    if (stack_.empty()) return;

    // Unload top scene
    call_unload(mrb, stack_.back());
    mrb_gc_unregister(mrb, stack_.back().object);
    stack_.pop_back();
}

void SceneManager::add_overlay(mrb_state* mrb, mrb_value scene_obj) {
    // Don't add duplicates
    for (const auto& overlay : overlays_) {
        if (mrb_obj_equal(mrb, overlay.object, scene_obj)) {
            return;
        }
    }

    // Protect from GC and add
    mrb_gc_register(mrb, scene_obj);
    overlays_.emplace_back(scene_obj);
    call_init(mrb, overlays_.back());
}

void SceneManager::remove_overlay(mrb_state* mrb, mrb_value scene_obj) {
    for (auto it = overlays_.begin(); it != overlays_.end(); ++it) {
        if (mrb_obj_equal(mrb, it->object, scene_obj)) {
            call_unload(mrb, *it);
            mrb_gc_unregister(mrb, it->object);
            overlays_.erase(it);
            return;
        }
    }
}

bool SceneManager::has_overlay(mrb_state* mrb, mrb_value scene_obj) const {
    for (const auto& overlay : overlays_) {
        if (mrb_obj_equal(mrb, overlay.object, scene_obj)) {
            return true;
        }
    }
    return false;
}

void SceneManager::update(mrb_state* mrb, float dt) {
    // Update top scene on stack
    if (!stack_.empty()) {
        call_update(mrb, stack_.back(), dt);
    }

    // Update all overlays
    for (auto& overlay : overlays_) {
        call_update(mrb, overlay, dt);
    }
}

void SceneManager::draw(mrb_state* mrb) {
    // Draw top scene on stack
    if (!stack_.empty()) {
        call_draw(mrb, stack_.back());
    }

    // Draw all overlays on top
    for (auto& overlay : overlays_) {
        call_draw(mrb, overlay);
    }
}

void SceneManager::load_with_transition(mrb_state* mrb, mrb_value scene_obj, const transition::TransitionConfig& config) {
    auto& tm = transition::TransitionManager::instance();

    // If transition starts successfully, defer the scene switch
    if (tm.start(mrb, config)) {
        // Protect pending scene from GC
        mrb_gc_register(mrb, scene_obj);
        pending_scene_ = scene_obj;
        pending_op_ = PendingOperation::LOAD;
    } else {
        // Transition failed to start (e.g., already transitioning), do immediate load
        load(mrb, scene_obj);
    }
}

void SceneManager::push_with_transition(mrb_state* mrb, mrb_value scene_obj, const transition::TransitionConfig& config) {
    auto& tm = transition::TransitionManager::instance();

    if (tm.start(mrb, config)) {
        mrb_gc_register(mrb, scene_obj);
        pending_scene_ = scene_obj;
        pending_op_ = PendingOperation::PUSH;
    } else {
        push(mrb, scene_obj);
    }
}

void SceneManager::pop_with_transition(mrb_state* mrb, const transition::TransitionConfig& config) {
    auto& tm = transition::TransitionManager::instance();

    if (tm.start(mrb, config)) {
        pending_op_ = PendingOperation::POP;
        // No pending_scene_ needed for pop
    } else {
        pop(mrb);
    }
}

void SceneManager::execute_pending(mrb_state* mrb) {
    if (pending_op_ == PendingOperation::NONE) {
        return;
    }

    switch (pending_op_) {
        case PendingOperation::LOAD:
            // Unregister from our temporary hold (load will re-register)
            mrb_gc_unregister(mrb, pending_scene_);
            load(mrb, pending_scene_);
            break;
        case PendingOperation::PUSH:
            mrb_gc_unregister(mrb, pending_scene_);
            push(mrb, pending_scene_);
            break;
        case PendingOperation::POP:
            pop(mrb);
            break;
        default:
            break;
    }

    pending_op_ = PendingOperation::NONE;
    pending_scene_ = mrb_nil_value();

    // Signal transition manager that the scene switch is complete
    transition::TransitionManager::instance().scene_switched();
}

void SceneManager::clear(mrb_state* mrb) {
    // Cancel any pending transition
    if (pending_op_ != PendingOperation::NONE) {
        if (!mrb_nil_p(pending_scene_)) {
            mrb_gc_unregister(mrb, pending_scene_);
        }
        pending_op_ = PendingOperation::NONE;
        pending_scene_ = mrb_nil_value();
    }

    // Unload all overlays first
    while (!overlays_.empty()) {
        call_unload(mrb, overlays_.back());
        mrb_gc_unregister(mrb, overlays_.back().object);
        overlays_.pop_back();
    }

    // Unload all scenes (for cleanup/hot reload)
    while (!stack_.empty()) {
        call_unload(mrb, stack_.back());
        mrb_gc_unregister(mrb, stack_.back().object);
        stack_.pop_back();
    }
}

} // namespace gmr
