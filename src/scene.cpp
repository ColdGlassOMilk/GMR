#include "gmr/scene.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/gc.h>

namespace gmr {

SceneManager& SceneManager::instance() {
    static SceneManager inst;
    return inst;
}

void SceneManager::call_init(mrb_state* mrb, Scene& scene) {
    if (scene.initialized) return;

    mrb_funcall(mrb, scene.object, "init", 0);
    scripting::check_error(mrb, "Scene#init");
    scene.initialized = true;
}

void SceneManager::call_unload(mrb_state* mrb, Scene& scene) {
    mrb_funcall(mrb, scene.object, "unload", 0);
    scripting::check_error(mrb, "Scene#unload");
}

void SceneManager::call_update(mrb_state* mrb, Scene& scene, float dt) {
    mrb_funcall(mrb, scene.object, "update", 1, mrb_float_value(mrb, dt));
    scripting::check_error(mrb, "Scene#update");
}

void SceneManager::call_draw(mrb_state* mrb, Scene& scene) {
    mrb_funcall(mrb, scene.object, "draw", 0);
    scripting::check_error(mrb, "Scene#draw");
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

void SceneManager::clear(mrb_state* mrb) {
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
