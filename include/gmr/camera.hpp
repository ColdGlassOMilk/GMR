#ifndef GMR_CAMERA_HPP
#define GMR_CAMERA_HPP

#include "gmr/types.hpp"
#include <mruby.h>
#include <unordered_map>
#include <cstdint>

// Forward declare raylib Camera2D to avoid including raylib.h in header
struct Camera2D;

namespace gmr {

// Camera handle type
using CameraHandle = int32_t;
constexpr CameraHandle INVALID_CAMERA_HANDLE = -1;

// Internal camera state - owns all camera logic data
struct Camera2DState {
    // Core camera properties (maps to raylib Camera2D)
    Vec2 target;      // Camera target (world position to follow)
    Vec2 offset;      // Camera offset (screen-space, usually screen center)
    float zoom;       // Camera zoom (1.0 = normal)
    float rotation;   // Camera rotation in degrees

    // Follow system
    mrb_value follow_target;   // Ruby object to follow (or nil)
    float smoothing;           // 0.0 = instant, approaching 1.0 = very smooth
    Rect deadzone;             // Screen-space deadzone
    bool has_deadzone;

    // World bounds
    Rect bounds;
    bool has_bounds;

    // Screen shake
    float shake_strength;
    float shake_duration;
    float shake_time_remaining;
    float shake_frequency;
    Vec2 shake_offset;         // Current frame's computed shake offset

    // State tracking
    bool dirty;

    Camera2DState();
};

// Singleton camera manager - owns all cameras and handles per-frame updates
class CameraManager {
public:
    static CameraManager& instance();

    // Camera lifecycle
    CameraHandle create();
    void destroy(CameraHandle handle);
    Camera2DState* get(CameraHandle handle);

    // Per-frame update (called from C++ main loop, before Ruby update)
    void update(mrb_state* mrb, float dt);

    // Current camera management
    void set_current(CameraHandle handle);
    CameraHandle current() const;
    Camera2DState* get_current();

    // Clear all cameras (for cleanup/reload)
    void clear();

private:
    CameraManager() = default;
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    // Internal helpers
    Vec2 get_target_position(mrb_state* mrb, mrb_value target);
    Vec2 apply_deadzone(Camera2DState& cam, const Vec2& target_pos);
    void clamp_to_bounds(Camera2DState& cam);
    void update_shake(Camera2DState& cam, float dt);

    std::unordered_map<CameraHandle, Camera2DState> cameras_;
    CameraHandle current_camera_ = INVALID_CAMERA_HANDLE;
    CameraHandle next_id_ = 0;
};

} // namespace gmr

#endif
