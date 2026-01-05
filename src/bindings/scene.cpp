#include "gmr/bindings/scene.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/scene.hpp"
#include <mruby/class.h>

namespace gmr {
namespace bindings {

/// @class GMR::Scene
/// @description Base class for scenes. Subclass and override lifecycle methods.
///   Scenes are managed by GMR::SceneManager using a stack. Only the top scene
///   receives update and draw calls.
/// @example class TitleScene < GMR::Scene
///   def init
///     @title = "My Game"
///   end
///
///   def update(dt)
///     if GMR::Input.key_pressed?(:enter)
///       GMR::SceneManager.push(GameScene.new)
///     end
///   end
///
///   def draw
///     GMR::Graphics.draw_text(@title, 100, 100, 32, [255, 255, 255])
///   end
///
///   def unload
///     puts "Title scene unloaded"
///   end
/// end

// ============================================================================
// GMR::Scene Base Class Methods (empty defaults)
// ============================================================================

/// @method init
/// @description Called once when the scene becomes active.
///   Override to initialize scene state.
static mrb_value mrb_scene_init(mrb_state* mrb, mrb_value self) {
    (void)mrb; (void)self;
    return mrb_nil_value();
}

/// @method update
/// @description Called every frame while this scene is on top of the stack.
///   Override to update game logic.
/// @param dt [Float] Delta time in seconds since last frame
static mrb_value mrb_scene_update(mrb_state* mrb, mrb_value self) {
    (void)mrb; (void)self;
    return mrb_nil_value();
}

/// @method draw
/// @description Called every frame while this scene is on top of the stack.
///   Override to draw scene content.
static mrb_value mrb_scene_draw(mrb_state* mrb, mrb_value self) {
    (void)mrb; (void)self;
    return mrb_nil_value();
}

/// @method unload
/// @description Called once when the scene is removed from the stack.
///   Override to clean up scene resources.
static mrb_value mrb_scene_unload(mrb_state* mrb, mrb_value self) {
    (void)mrb; (void)self;
    return mrb_nil_value();
}

// ============================================================================
// GMR::SceneManager Module Methods
// ============================================================================

/// @module GMR::SceneManager
/// @description Stack-based scene lifecycle manager.
///   Use load to switch scenes (clears stack), push to add a scene on top,
///   and pop to return to the previous scene.

/// @method load
/// @description Clear the scene stack and load a new scene.
///   Calls unload on all existing scenes (top to bottom),
///   then init on the new scene.
/// @param scene [GMR::Scene] The scene to load
/// @example GMR::SceneManager.load(TitleScene.new)
static mrb_value mrb_scene_manager_load(mrb_state* mrb, mrb_value self) {
    (void)self;
    mrb_value scene;
    mrb_get_args(mrb, "o", &scene);

    // Validate it's a Scene
    RClass* scene_class = mrb_class_get_under(mrb, get_gmr_module(mrb), "Scene");
    if (!mrb_obj_is_kind_of(mrb, scene, scene_class)) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected GMR::Scene");
    }

    SceneManager::instance().load(mrb, scene);
    return mrb_nil_value();
}

/// @method register
/// @description Alias for load. Clear the scene stack and load a new scene.
/// @param scene [GMR::Scene] The scene to load
/// @example GMR::SceneManager.register(TitleScene.new)
static mrb_value mrb_scene_manager_register(mrb_state* mrb, mrb_value self) {
    return mrb_scene_manager_load(mrb, self);
}

/// @method push
/// @description Push a new scene onto the stack. The current scene is paused
///   (no longer receives update/draw). Calls init on the new scene.
/// @param scene [GMR::Scene] The scene to push
/// @example GMR::SceneManager.push(PauseScene.new)
static mrb_value mrb_scene_manager_push(mrb_state* mrb, mrb_value self) {
    (void)self;
    mrb_value scene;
    mrb_get_args(mrb, "o", &scene);

    // Validate it's a Scene
    RClass* scene_class = mrb_class_get_under(mrb, get_gmr_module(mrb), "Scene");
    if (!mrb_obj_is_kind_of(mrb, scene, scene_class)) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected GMR::Scene");
    }

    SceneManager::instance().push(mrb, scene);
    return mrb_nil_value();
}

/// @method pop
/// @description Remove the top scene from the stack.
///   Calls unload on the removed scene. The previous scene resumes
///   receiving update/draw calls.
/// @example GMR::SceneManager.pop
static mrb_value mrb_scene_manager_pop(mrb_state* mrb, mrb_value self) {
    (void)self;
    SceneManager::instance().pop(mrb);
    return mrb_nil_value();
}

/// @method update
/// @description Call update on the top scene. Call this from your game's update function.
/// @param dt [Float] Delta time in seconds
/// @example def update(dt)
///   GMR::SceneManager.update(dt)
/// end
static mrb_value mrb_scene_manager_update(mrb_state* mrb, mrb_value self) {
    (void)self;
    mrb_float dt;
    mrb_get_args(mrb, "f", &dt);

    SceneManager::instance().update(mrb, static_cast<float>(dt));
    return mrb_nil_value();
}

/// @method draw
/// @description Call draw on the top scene. Call this from your game's draw function.
/// @example def draw
///   GMR::SceneManager.draw
/// end
static mrb_value mrb_scene_manager_draw(mrb_state* mrb, mrb_value self) {
    (void)self;
    SceneManager::instance().draw(mrb);
    return mrb_nil_value();
}

// ============================================================================
// Overlay Methods
// ============================================================================

/// @method add_overlay
/// @description Add an overlay scene that renders on top of the main scene.
///   Overlays receive update and draw calls. Multiple overlays can be active.
/// @param scene [GMR::Scene] The overlay scene to add
/// @example GMR::SceneManager.add_overlay(MinimapOverlay.new)
static mrb_value mrb_scene_manager_add_overlay(mrb_state* mrb, mrb_value self) {
    (void)self;
    mrb_value scene;
    mrb_get_args(mrb, "o", &scene);

    // Validate it's a Scene
    RClass* scene_class = mrb_class_get_under(mrb, get_gmr_module(mrb), "Scene");
    if (!mrb_obj_is_kind_of(mrb, scene, scene_class)) {
        mrb_raise(mrb, E_TYPE_ERROR, "Expected GMR::Scene");
    }

    SceneManager::instance().add_overlay(mrb, scene);
    return mrb_nil_value();
}

/// @method remove_overlay
/// @description Remove an overlay scene.
/// @param scene [GMR::Scene] The overlay scene to remove
/// @example GMR::SceneManager.remove_overlay(@minimap)
static mrb_value mrb_scene_manager_remove_overlay(mrb_state* mrb, mrb_value self) {
    (void)self;
    mrb_value scene;
    mrb_get_args(mrb, "o", &scene);

    SceneManager::instance().remove_overlay(mrb, scene);
    return mrb_nil_value();
}

/// @method has_overlay?
/// @description Check if an overlay is currently active.
/// @param scene [GMR::Scene] The overlay scene to check
/// @returns [Boolean] true if the overlay is active
/// @example if GMR::SceneManager.has_overlay?(@minimap)
static mrb_value mrb_scene_manager_has_overlay(mrb_state* mrb, mrb_value self) {
    (void)self;
    mrb_value scene;
    mrb_get_args(mrb, "o", &scene);

    bool result = SceneManager::instance().has_overlay(mrb, scene);
    return mrb_bool_value(result);
}

// ============================================================================
// Registration
// ============================================================================

void register_scene(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // GMR::Scene base class
    RClass* scene_class = mrb_define_class_under(mrb, gmr, "Scene", mrb->object_class);

    // Default empty lifecycle methods (subclasses override)
    mrb_define_method(mrb, scene_class, "init", mrb_scene_init, MRB_ARGS_NONE());
    mrb_define_method(mrb, scene_class, "update", mrb_scene_update, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, scene_class, "draw", mrb_scene_draw, MRB_ARGS_NONE());
    mrb_define_method(mrb, scene_class, "unload", mrb_scene_unload, MRB_ARGS_NONE());

    // GMR::SceneManager module
    RClass* scene_manager = mrb_define_module_under(mrb, gmr, "SceneManager");

    mrb_define_module_function(mrb, scene_manager, "load", mrb_scene_manager_load, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, scene_manager, "register", mrb_scene_manager_register, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, scene_manager, "push", mrb_scene_manager_push, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, scene_manager, "pop", mrb_scene_manager_pop, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, scene_manager, "update", mrb_scene_manager_update, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, scene_manager, "draw", mrb_scene_manager_draw, MRB_ARGS_NONE());

    // Overlay methods
    mrb_define_module_function(mrb, scene_manager, "add_overlay", mrb_scene_manager_add_overlay, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, scene_manager, "remove_overlay", mrb_scene_manager_remove_overlay, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, scene_manager, "has_overlay?", mrb_scene_manager_has_overlay, MRB_ARGS_REQ(1));
}

} // namespace bindings
} // namespace gmr
