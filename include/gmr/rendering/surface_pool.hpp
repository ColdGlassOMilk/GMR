#ifndef GMR_SURFACE_POOL_HPP
#define GMR_SURFACE_POOL_HPP

#include "gmr/types.hpp"
#include "raylib.h"
#include <vector>
#include <cstdint>

namespace gmr {

// Handle type for pooled surfaces
using SurfaceHandle = int32_t;
constexpr SurfaceHandle INVALID_SURFACE_HANDLE = -1;

// Pool of reusable render textures for shader surface mode.
// Surfaces are allocated on demand and reused across frames to prevent
// allocation thrashing when multiple shader groups render per frame.
class SurfacePool {
public:
    static SurfacePool& instance();

    // Acquire a render target of at least the given size.
    // Returns handle to surface. Actual dimensions may be larger (power-of-two sizing).
    SurfaceHandle acquire(int min_width, int min_height);

    // Acquire a render target with EXACT dimensions (no power-of-two rounding).
    // Use for surface mode shaders where texture dimensions affect UV coordinates.
    SurfaceHandle acquire_exact(int width, int height);

    // Release render target back to pool for reuse.
    void release(SurfaceHandle handle);

    // Get raylib RenderTexture2D by handle.
    RenderTexture2D* get(SurfaceHandle handle);

    // End-of-frame cleanup. Releases unused surfaces after several frames.
    void end_frame();

    // Full cleanup on shutdown.
    void clear();

    SurfacePool(const SurfacePool&) = delete;
    SurfacePool& operator=(const SurfacePool&) = delete;

private:
    SurfacePool() = default;

    // Round up to next power of two for efficient reuse
    static int next_power_of_two(int n);

    struct Surface {
        RenderTexture2D texture{};
        int width = 0;
        int height = 0;
        bool in_use = false;
        int unused_frames = 0;  // Frames since last use (for cleanup)
        bool valid = false;
    };

    std::vector<Surface> surfaces_;

    // Maximum frames a surface can go unused before cleanup
    static constexpr int MAX_UNUSED_FRAMES = 60;  // ~1 second at 60fps
};

} // namespace gmr

#endif
