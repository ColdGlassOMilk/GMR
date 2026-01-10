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

    // World-space configuration for resolution-independent coordinates
    // Two modes:
    //   1. view_height only (view_width=0): Unity-style, width derived from aspect ratio
    //   2. view_size (both set): Retro-style, fixed view with letterboxing if needed
    // PPU is derived: pixels_per_unit = viewport_size.y / view_height
    float view_width;       // How many world units wide (0 = derive from aspect ratio)
    float view_height;      // How many world units tall the camera view is (default: 7.5)
    Vec2 viewport_size;     // Viewport dimensions in pixels (render target size)

    // Derived value - automatically calculated when view_height or viewport_size changes
    float pixels_per_unit;

    // Helper to recalculate PPU from view_height and viewport_size
    void update_pixels_per_unit();

    // Check if using fixed view size (retro mode) vs height-only (Unity mode)
    bool is_fixed_view_size() const { return view_width > 0.0f; }

    // Follow system
    mrb_value follow_target;   // Ruby object to follow (or nil)
    float smoothing;           // 0.0 = instant, approaching 1.0 = very smooth
    Rect deadzone;             // World-space deadzone (width/height in world units)
    bool has_deadzone;

    // World bounds
    Rect bounds;               // World-space bounds for camera clamping
    bool has_bounds;

    // Screen shake
    float shake_strength;      // Shake strength in world units
    float shake_duration;
    float shake_time_remaining;
    float shake_frequency;
    Vec2 shake_offset;         // Current frame's computed shake offset (world units)

    // State tracking
    bool dirty;

    Camera2DState();

    // World-space projection methods
    // Returns the effective scale factor (pixels_per_unit * zoom)
    float get_effective_scale() const;

    // Convert world coordinates to screen coordinates
    Vec2 world_to_screen(const Vec2& world) const;

    // Convert screen coordinates to world coordinates
    Vec2 screen_to_world(const Vec2& screen) const;

    // Get the visible world bounds (what portion of world is on screen)
    Rect get_visible_bounds() const;

    // Get visible world dimensions
    float get_visible_width() const;
    float get_visible_height() const;
};

// Singleton camera manager - owns all cameras and handles per-frame updates
class CameraManager {
public:
    static CameraManager& instance();

    // Camera lifecycle
    CameraHandle create();
    void destroy(mrb_state* mrb, CameraHandle handle);
    Camera2DState* get(CameraHandle handle);

    // Per-frame update (called from C++ main loop, before Ruby update)
    void update(mrb_state* mrb, float dt);

    // Current camera management
    void set_current(CameraHandle handle);
    CameraHandle current() const;
    Camera2DState* get_current();

    // Clear all cameras (for cleanup/reload)
    void clear(mrb_state* mrb);

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
