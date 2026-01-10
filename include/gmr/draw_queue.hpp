#ifndef GMR_DRAW_QUEUE_HPP
#define GMR_DRAW_QUEUE_HPP

#include "gmr/types.hpp"
#include "gmr/camera.hpp"
#include <vector>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace gmr {

// Render layer enumeration for organizing draw order
// Lower values render first (background), higher values render last (foreground/UI)
enum class RenderLayer : uint8_t {
    BACKGROUND = 0,    // Parallax backgrounds, far scenery
    WORLD = 50,        // Tilemaps, world sprites, terrain
    ENTITIES = 100,    // Player, enemies, NPCs (DEFAULT for sprites/primitives)
    EFFECTS = 150,     // Particles, VFX, overlays
    UI = 200,          // HUD, menus, text overlays (DEFAULT for text)
    DEBUG_OVERLAY = 250  // Debug overlays, collision viz
};

// Color struct for draw commands
struct DrawColor {
    uint8_t r{255}, g{255}, b{255}, a{255};
};

// Tilemap draw parameters
struct TilemapDrawParams {
    TilemapHandle handle{INVALID_HANDLE};
    float offset_x{0};
    float offset_y{0};
    DrawColor tint;
    // Optional region (if not set, draw entire tilemap)
    bool use_region{false};
    int32_t region_x{0};
    int32_t region_y{0};
    int32_t region_w{0};
    int32_t region_h{0};
};

// Rectangle draw parameters
struct RectDrawParams {
    TransformHandle transform{INVALID_HANDLE};  // NEW: Transform-based rendering
    float width{0}, height{0};   // NEW: Local-space dimensions (when using transform)
    float x{0}, y{0}, w{0}, h{0}; // OLD: World-space coordinates (backward compat)
    float rotation{0};
    DrawColor color;
    bool filled{true};
};

// Circle draw parameters
struct CircleDrawParams {
    TransformHandle transform{INVALID_HANDLE};  // NEW: Transform-based rendering
    float radius{0};             // NEW: Local-space radius (when using transform)
    float x{0}, y{0};            // OLD: World-space coordinates (backward compat)
    DrawColor color;
    DrawColor color2;  // For gradient
    bool filled{true};
    bool gradient{false};
};

// Line draw parameters
struct LineDrawParams {
    TransformHandle transform{INVALID_HANDLE};  // NEW: Transform-based rendering
    float x1{0}, y1{0}, x2{0}, y2{0};  // Endpoints (local-space when using transform)
    float thickness{1};
    DrawColor color;
};

// Triangle draw parameters
struct TriangleDrawParams {
    TransformHandle transform{INVALID_HANDLE};  // NEW: Transform-based rendering
    float x1{0}, y1{0}, x2{0}, y2{0}, x3{0}, y3{0};  // Vertices (local-space when using transform)
    DrawColor color;
    bool filled{true};
};

// Text draw parameters
struct TextDrawParams {
    TransformHandle transform{INVALID_HANDLE};  // Transform-based rendering
    FontHandle font{INVALID_HANDLE};            // Custom font (INVALID_HANDLE = default font)
    float x{0}, y{0};  // World-space coordinates (backward compat)
    int font_size{20};
    DrawColor color;
    std::string content;
};

// Draw command for deferred rendering with three-level sorting (layer → z → draw_order)
struct DrawCommand {
    enum class Type {
        SPRITE,
        TILEMAP,
        RECT,
        CIRCLE,
        LINE,
        TRIANGLE,
        TEXT,
        CAMERA_BEGIN,
        CAMERA_END
    };

    Type type{Type::SPRITE};
    uint8_t layer{static_cast<uint8_t>(RenderLayer::ENTITIES)};  // Primary sort key
    float z{0};                                                   // Secondary sort key
    uint32_t draw_order{0};                                       // Tertiary sort key

    // Type-specific data
    SpriteHandle sprite_handle{INVALID_HANDLE};
    CameraHandle camera_handle{INVALID_CAMERA_HANDLE};
    TilemapDrawParams tilemap;
    RectDrawParams rect;
    CircleDrawParams circle;
    LineDrawParams line;
    TriangleDrawParams triangle;
    TextDrawParams text;

    DrawCommand() = default;
    DrawCommand(Type t, uint8_t layer_val, float z_val, uint32_t order, SpriteHandle sprite = INVALID_HANDLE)
        : type(t), layer(layer_val), z(z_val), draw_order(order), sprite_handle(sprite) {}
};

// Singleton draw queue for deferred, z-sorted rendering
class DrawQueue {
public:
    static DrawQueue& instance();

    // Queue a sprite for drawing
    // Uses sprite.z if set, otherwise uses draw_order (later = higher = on top)
    void queue_sprite(SpriteHandle handle);

    // Queue tilemap for drawing
    void queue_tilemap(TilemapHandle handle, float offset_x, float offset_y, const DrawColor& tint);
    void queue_tilemap_region(TilemapHandle handle, float offset_x, float offset_y,
                              int32_t region_x, int32_t region_y, int32_t region_w, int32_t region_h,
                              const DrawColor& tint);

    // Queue primitives for drawing (with optional layer/z control)
    void queue_rect(float x, float y, float w, float h, const DrawColor& color, bool filled = true,
                    uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_rect_rotated(float x, float y, float w, float h, float rotation, const DrawColor& color,
                            uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_circle(float x, float y, float radius, const DrawColor& color, bool filled = true,
                      uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_circle_gradient(float x, float y, float radius, const DrawColor& inner, const DrawColor& outer,
                               uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_line(float x1, float y1, float x2, float y2, const DrawColor& color, float thickness = 1.0f,
                    uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_triangle(float x1, float y1, float x2, float y2, float x3, float y3,
                        const DrawColor& color, bool filled = true,
                        uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_text(float x, float y, const std::string& content, int font_size, const DrawColor& color,
                    uint8_t layer = static_cast<uint8_t>(RenderLayer::UI), float z = 0.0f);
    // With custom font
    void queue_text(float x, float y, const std::string& content, int font_size, const DrawColor& color,
                    FontHandle font, uint8_t layer = static_cast<uint8_t>(RenderLayer::UI), float z = 0.0f);

    // Transform-based queue functions for unified Transform2D support
    void queue_rect(TransformHandle transform, float width, float height, const DrawColor& color,
                    bool filled = true, uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_circle(TransformHandle transform, float radius, const DrawColor& color,
                      bool filled = true, uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_triangle(TransformHandle transform, float x1, float y1, float x2, float y2, float x3, float y3,
                        const DrawColor& color, bool filled = true,
                        uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_line(TransformHandle transform, float x1, float y1, float x2, float y2, const DrawColor& color,
                    float thickness = 1.0f, uint8_t layer = static_cast<uint8_t>(RenderLayer::ENTITIES), float z = 0.0f);
    void queue_text(TransformHandle transform, const std::string& content, int font_size, const DrawColor& color,
                    uint8_t layer = static_cast<uint8_t>(RenderLayer::UI), float z = 0.0f);
    // Transform-based with custom font
    void queue_text(TransformHandle transform, const std::string& content, int font_size, const DrawColor& color,
                    FontHandle font, uint8_t layer = static_cast<uint8_t>(RenderLayer::UI), float z = 0.0f);

    // Queue camera begin/end commands for deferred camera transforms
    void queue_camera_begin(CameraHandle handle);
    void queue_camera_end();

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
    void draw_tilemap(const DrawCommand& cmd);
    void draw_rect(const DrawCommand& cmd);
    void draw_circle(const DrawCommand& cmd);
    void draw_line(const DrawCommand& cmd);
    void draw_triangle(const DrawCommand& cmd);
    void draw_text(const DrawCommand& cmd);
    void apply_camera_begin(CameraHandle handle);
    void apply_camera_end();

    std::vector<DrawCommand> commands_;
    std::vector<CameraHandle> camera_stack_;  // Track camera nesting during queuing
    CameraHandle active_camera_{INVALID_CAMERA_HANDLE};  // Current camera during flush
    uint32_t next_draw_order_{0};

    // Large base z for draw-order-based sprites to ensure they sort after explicit z
    static constexpr float DRAW_ORDER_Z_BASE = 1000000.0f;
};

} // namespace gmr

#endif
