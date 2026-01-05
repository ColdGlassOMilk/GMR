#ifndef GMR_SCENE_HPP
#define GMR_SCENE_HPP

#include <mruby.h>
#include <vector>

namespace gmr {

// Scene entry - holds reference to Ruby scene object
struct Scene {
    mrb_value object;      // The Ruby scene instance
    bool initialized;      // Has init been called?

    Scene() : object(mrb_nil_value()), initialized(false) {}
    explicit Scene(mrb_value obj) : object(obj), initialized(false) {}
};

// SceneManager - stack-based scene lifecycle manager with overlay support
class SceneManager {
public:
    static SceneManager& instance();

    // Stack operations (called from bindings)
    void load(mrb_state* mrb, mrb_value scene);    // Clear stack, load scene
    void push(mrb_state* mrb, mrb_value scene);    // Push scene onto stack
    void pop(mrb_state* mrb);                       // Pop top scene

    // Overlay operations - overlays render on top of main scene
    void add_overlay(mrb_state* mrb, mrb_value scene);     // Add overlay
    void remove_overlay(mrb_state* mrb, mrb_value scene);  // Remove overlay
    bool has_overlay(mrb_state* mrb, mrb_value scene) const; // Check if overlay exists

    // Per-frame calls (called from main loop)
    void update(mrb_state* mrb, float dt);
    void draw(mrb_state* mrb);

    // Query
    bool empty() const { return stack_.empty(); }
    size_t size() const { return stack_.size(); }
    size_t overlay_count() const { return overlays_.size(); }

    // Cleanup (for hot reload / shutdown)
    void clear(mrb_state* mrb);

private:
    SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // Internal lifecycle helpers
    void call_init(mrb_state* mrb, Scene& scene);
    void call_unload(mrb_state* mrb, Scene& scene);
    void call_update(mrb_state* mrb, Scene& scene, float dt);
    void call_draw(mrb_state* mrb, Scene& scene);

    std::vector<Scene> stack_;
    std::vector<Scene> overlays_;  // Overlays drawn on top of main scene
};

} // namespace gmr

#endif
