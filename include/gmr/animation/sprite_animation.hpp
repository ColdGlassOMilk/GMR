#ifndef GMR_ANIMATION_SPRITE_ANIMATION_HPP
#define GMR_ANIMATION_SPRITE_ANIMATION_HPP

#include "gmr/types.hpp"
#include <mruby.h>
#include <vector>

namespace gmr {
namespace animation {

// Internal state for a sprite frame animation
struct SpriteAnimationState {
    // Target sprite
    SpriteHandle sprite{INVALID_HANDLE};
    mrb_value sprite_ref;       // Ruby Sprite object reference (GC protection)

    // Frame data
    std::vector<int> frames;    // Frame indices to cycle through
    int current_frame_index{0}; // Current index into frames array

    // Timing
    float fps{12.0f};           // Frames per second
    float frame_duration{0.0f}; // Calculated: 1.0 / fps
    float elapsed{0.0f};        // Time since last frame change

    // Frame dimensions (for source_rect calculation)
    int frame_width{0};         // Width of each frame in pixels
    int frame_height{0};        // Height of each frame in pixels
    int columns{1};             // Number of columns in spritesheet

    // State flags
    bool loop{true};            // Should animation loop?
    bool playing{false};        // Is animation currently playing?
    bool completed{false};      // Has animation finished? (non-looping only)

    // Callbacks
    mrb_value on_complete;      // Proc to call when animation completes (non-looping)
    mrb_value on_frame_change;  // Proc to call when frame changes (receives frame index)

    // The Ruby SpriteAnimation object itself (for GC registration)
    mrb_value ruby_anim_obj;

    SpriteAnimationState()
        : sprite_ref(mrb_nil_value())
        , on_complete(mrb_nil_value())
        , on_frame_change(mrb_nil_value())
        , ruby_anim_obj(mrb_nil_value()) {}

    // Get the current frame index from the frames array
    int current_frame() const {
        if (frames.empty() || current_frame_index < 0 ||
            current_frame_index >= static_cast<int>(frames.size())) {
            return 0;
        }
        return frames[current_frame_index];
    }

    // Calculate frame duration from fps
    void update_frame_duration() {
        frame_duration = fps > 0.0f ? 1.0f / fps : 0.0f;
    }

    // Check if the animation should be updated this frame
    bool should_update() const {
        return playing && !completed && !frames.empty();
    }
};

} // namespace animation
} // namespace gmr

#endif
