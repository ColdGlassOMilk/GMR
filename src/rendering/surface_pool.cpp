#include "gmr/rendering/surface_pool.hpp"
#include "raylib.h"

namespace gmr {

SurfacePool& SurfacePool::instance() {
    static SurfacePool instance;
    return instance;
}

int SurfacePool::next_power_of_two(int n) {
    if (n <= 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

SurfaceHandle SurfacePool::acquire(int min_width, int min_height) {
    // Clamp to reasonable minimum
    min_width = (min_width < 1) ? 1 : min_width;
    min_height = (min_height < 1) ? 1 : min_height;

    // Round up to power of two for efficient reuse
    int target_width = next_power_of_two(min_width);
    int target_height = next_power_of_two(min_height);

    // First, try to find an existing surface that fits
    for (size_t i = 0; i < surfaces_.size(); ++i) {
        Surface& surface = surfaces_[i];
        if (!surface.in_use && surface.valid &&
            surface.width >= target_width && surface.height >= target_height) {
            // Found a suitable surface
            surface.in_use = true;
            surface.unused_frames = 0;
            TraceLog(LOG_DEBUG, "SURFACE_POOL: Reusing surface [ID %zu] %dx%d for request %dx%d",
                     i, surface.width, surface.height, min_width, min_height);
            return static_cast<SurfaceHandle>(i);
        }
    }

    // No suitable surface found, create a new one
    RenderTexture2D rt = LoadRenderTexture(target_width, target_height);

    if (!IsRenderTextureValid(rt)) {
        TraceLog(LOG_ERROR, "SURFACE_POOL: Failed to create render texture %dx%d",
                 target_width, target_height);
        return INVALID_SURFACE_HANDLE;
    }

    // Set texture filtering to bilinear for smooth results
    SetTextureFilter(rt.texture, TEXTURE_FILTER_BILINEAR);

    Surface new_surface;
    new_surface.texture = rt;
    new_surface.width = target_width;
    new_surface.height = target_height;
    new_surface.in_use = true;
    new_surface.unused_frames = 0;
    new_surface.valid = true;

    SurfaceHandle handle = static_cast<SurfaceHandle>(surfaces_.size());
    surfaces_.push_back(std::move(new_surface));

    TraceLog(LOG_INFO, "SURFACE_POOL: Created new surface [ID %d] %dx%d for request %dx%d",
             handle, target_width, target_height, min_width, min_height);

    return handle;
}

SurfaceHandle SurfacePool::acquire_exact(int width, int height) {
    // Clamp to reasonable minimum
    width = (width < 1) ? 1 : width;
    height = (height < 1) ? 1 : height;

    // Look for an exact match first (no power-of-two rounding)
    for (size_t i = 0; i < surfaces_.size(); ++i) {
        Surface& surface = surfaces_[i];
        if (!surface.in_use && surface.valid &&
            surface.width == width && surface.height == height) {
            // Found exact match
            surface.in_use = true;
            surface.unused_frames = 0;
            TraceLog(LOG_DEBUG, "SURFACE_POOL: Reusing exact surface [ID %zu] %dx%d",
                     i, surface.width, surface.height);
            return static_cast<SurfaceHandle>(i);
        }
    }

    // No exact match, create a new one with exact dimensions
    RenderTexture2D rt = LoadRenderTexture(width, height);

    if (!IsRenderTextureValid(rt)) {
        TraceLog(LOG_ERROR, "SURFACE_POOL: Failed to create exact render texture %dx%d",
                 width, height);
        return INVALID_SURFACE_HANDLE;
    }

    // Set texture filtering to bilinear for smooth results
    SetTextureFilter(rt.texture, TEXTURE_FILTER_BILINEAR);

    Surface new_surface;
    new_surface.texture = rt;
    new_surface.width = width;
    new_surface.height = height;
    new_surface.in_use = true;
    new_surface.unused_frames = 0;
    new_surface.valid = true;

    SurfaceHandle handle = static_cast<SurfaceHandle>(surfaces_.size());
    surfaces_.push_back(std::move(new_surface));

    TraceLog(LOG_INFO, "SURFACE_POOL: Created exact surface [ID %d] %dx%d",
             handle, width, height);

    return handle;
}

void SurfacePool::release(SurfaceHandle handle) {
    if (handle < 0 || handle >= static_cast<SurfaceHandle>(surfaces_.size())) {
        return;
    }

    Surface& surface = surfaces_[handle];
    if (surface.in_use) {
        surface.in_use = false;
        surface.unused_frames = 0;
        TraceLog(LOG_DEBUG, "SURFACE_POOL: Released surface [ID %d]", handle);
    }
}

RenderTexture2D* SurfacePool::get(SurfaceHandle handle) {
    if (handle < 0 || handle >= static_cast<SurfaceHandle>(surfaces_.size())) {
        return nullptr;
    }

    Surface& surface = surfaces_[handle];
    if (!surface.valid) {
        return nullptr;
    }

    return &surface.texture;
}

void SurfacePool::end_frame() {
    // Track unused frames and clean up surfaces that haven't been used
    for (size_t i = 0; i < surfaces_.size(); ++i) {
        Surface& surface = surfaces_[i];
        if (!surface.valid) continue;

        if (!surface.in_use) {
            surface.unused_frames++;

            // Clean up surfaces that have been unused for too long
            if (surface.unused_frames > MAX_UNUSED_FRAMES) {
                UnloadRenderTexture(surface.texture);
                surface.valid = false;
                TraceLog(LOG_INFO, "SURFACE_POOL: Cleaned up unused surface [ID %zu] %dx%d",
                         i, surface.width, surface.height);
            }
        }
    }
}

void SurfacePool::clear() {
    for (size_t i = 0; i < surfaces_.size(); ++i) {
        Surface& surface = surfaces_[i];
        if (surface.valid) {
            UnloadRenderTexture(surface.texture);
            surface.valid = false;
        }
    }
    surfaces_.clear();
    TraceLog(LOG_INFO, "SURFACE_POOL: Cleared all surfaces");
}

} // namespace gmr
