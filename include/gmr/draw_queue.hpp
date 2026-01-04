#ifndef GMR_DRAW_QUEUE_HPP
#define GMR_DRAW_QUEUE_HPP

#include "gmr/types.hpp"
#include <vector>
#include <cstdint>
#include <limits>

namespace gmr {

// Draw command for deferred rendering with hybrid z-ordering
struct DrawCommand {
    enum class Type {
        SPRITE
        // Future: RECT, CIRCLE, LINE, TEXT for unified z-sorting
    };

    Type type;
    float z;              // Final z value (explicit or from draw_order)
    uint32_t draw_order;  // Insertion order for stable sort

    // Type-specific data
    SpriteHandle sprite_handle;  // For SPRITE type

    DrawCommand() = default;
    DrawCommand(Type t, float z_val, uint32_t order, SpriteHandle sprite = INVALID_HANDLE)
        : type(t), z(z_val), draw_order(order), sprite_handle(sprite) {}
};

// Singleton draw queue for deferred, z-sorted rendering
class DrawQueue {
public:
    static DrawQueue& instance();

    // Queue a sprite for drawing
    // Uses sprite.z if set, otherwise uses draw_order (later = higher = on top)
    void queue_sprite(SpriteHandle handle);

    // Sort by z and execute all queued draws, then clear
    void flush();

    // Clear without drawing (e.g., on error or reload)
    void clear();

    // Reset draw order counter (called at start of each frame)
    void begin_frame();

    // Debug info
    size_t pending_count() const { return commands_.size(); }

private:
    DrawQueue() = default;

    void draw_sprite(const DrawCommand& cmd);

    std::vector<DrawCommand> commands_;
    uint32_t next_draw_order_{0};

    // Large base z for draw-order-based sprites to ensure they sort after explicit z
    static constexpr float DRAW_ORDER_Z_BASE = 1000000.0f;
};

} // namespace gmr

#endif
