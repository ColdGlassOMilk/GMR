#ifndef GMR_SCENE_HPP
#define GMR_SCENE_HPP

#include <mruby.h>
#include <vector>

namespace gmr {

// Forward declare transition types
namespace transition {
    struct TransitionConfig;
}

// Pending scene operation (deferred until transition completes)
enum class PendingOperation {
    NONE,
    LOAD,
    PUSH,
    POP
};

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

    // Stack operations with transitions
    void load_with_transition(mrb_state* mrb, mrb_value scene, const transition::TransitionConfig& config);
    void push_with_transition(mrb_state* mrb, mrb_value scene, const transition::TransitionConfig& config);
    void pop_with_transition(mrb_state* mrb, const transition::TransitionConfig& config);

    // Overlay operations - overlays render on top of main scene
    void add_overlay(mrb_state* mrb, mrb_value scene);     // Add overlay
    void remove_overlay(mrb_state* mrb, mrb_value scene);  // Remove overlay
    bool has_overlay(mrb_state* mrb, mrb_value scene) const; // Check if overlay exists

    // Per-frame calls (called from main loop)
    void update(mrb_state* mrb, float dt);
    void draw(mrb_state* mrb);

    // Execute pending scene operation (called when transition OUT completes)
    void execute_pending(mrb_state* mrb);

    // Check if there's a pending operation
    bool has_pending() const { return pending_op_ != PendingOperation::NONE; }

    // Query
    bool empty() const { return stack_.empty(); }
    size_t size() const { return stack_.size(); }
    size_t overlay_count() const { return overlays_.size(); }
    mrb_value current() const { return stack_.empty() ? mrb_nil_value() : stack_.back().object; }

    // Cleanup (for hot reload / shutdown)
    void clear(mrb_state* mrb);

private:
    SceneManager() : pending_scene_(mrb_nil_value()) {}
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // Internal lifecycle helpers
    void call_init(mrb_state* mrb, Scene& scene);
    void call_unload(mrb_state* mrb, Scene& scene);
    void call_update(mrb_state* mrb, Scene& scene, float dt);
    void call_draw(mrb_state* mrb, Scene& scene);

    std::vector<Scene> stack_;
    std::vector<Scene> overlays_;  // Overlays drawn on top of main scene

    // Pending operation state (for transitions)
    PendingOperation pending_op_ = PendingOperation::NONE;
    mrb_value pending_scene_;  // Scene for pending load/push (initialized in constructor)
};

} // namespace gmr

#endif
