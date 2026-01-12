#ifndef GMR_TRANSITION_MANAGER_HPP
#define GMR_TRANSITION_MANAGER_HPP

#include "gmr/animation/easing.hpp"
#include "gmr/rendering/surface_pool.hpp"
#include "raylib.h"
#include <mruby.h>
#include <functional>

namespace gmr {
namespace transition {

// Transition types
enum class TransitionType {
    NONE = 0,
    FADE,           // Fade to color then fade in
    WIPE_LEFT,      // Wipe from right to left
    WIPE_RIGHT,     // Wipe from left to right
    WIPE_UP,        // Wipe from bottom to top
    WIPE_DOWN       // Wipe from top to bottom
};

// Transition state
enum class TransitionState {
    NONE,           // No transition active
    OUT,            // Transitioning out (old scene fading/wiping out)
    IN              // Transitioning in (new scene fading/wiping in)
};

// Transition configuration
struct TransitionConfig {
    TransitionType type = TransitionType::FADE;
    animation::EasingType easing = animation::EasingType::LINEAR;
    float duration = 0.3f;          // Total duration (half for out, half for in)
    Color color = {0, 0, 0, 255};   // Color for fade transitions
    mrb_value callback = {};        // Zero-initialized (checked via has_callback)
    bool has_callback = false;
};

// Manages scene transitions with render target capture
class TransitionManager {
public:
    static TransitionManager& instance();

    // Start a transition. Returns true if transition started.
    // The actual scene switch happens when transition_out completes.
    bool start(mrb_state* mrb, const TransitionConfig& config);

    // Check if a transition is active
    bool is_active() const { return state_ != TransitionState::NONE; }

    // Get current state
    TransitionState state() const { return state_; }

    // Update transition progress (call from main loop)
    void update(float dt);

    // Draw transition overlay (call after scene draw)
    void draw();

    // Signal that the scene switch is complete (called by SceneManager)
    // Transitions from OUT to IN state
    void scene_switched();

    // Cancel any active transition
    void cancel();

    // Cleanup (for hot reload / shutdown)
    void clear();

    // Get progress (0.0 to 1.0) for current phase
    float progress() const { return progress_; }

    TransitionManager(const TransitionManager&) = delete;
    TransitionManager& operator=(const TransitionManager&) = delete;

private:
    TransitionManager() = default;

    void release_surface();
    void draw_fade();
    void draw_wipe();

    TransitionState state_ = TransitionState::NONE;
    TransitionConfig config_;
    float elapsed_ = 0.0f;
    float progress_ = 0.0f;         // Eased progress 0.0 to 1.0

    // Captured scene for wipe transitions
    SurfaceHandle captured_surface_ = INVALID_SURFACE_HANDLE;
    int capture_width_ = 0;
    int capture_height_ = 0;

    // mruby state for callback
    mrb_state* mrb_ = nullptr;
};

// Convert Ruby symbol to TransitionType
TransitionType symbol_to_transition_type(mrb_state* mrb, mrb_sym sym);

} // namespace transition
} // namespace gmr

#endif
