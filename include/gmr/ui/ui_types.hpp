#ifndef GMR_UI_TYPES_HPP
#define GMR_UI_TYPES_HPP

#include <cstdint>

namespace gmr {
namespace ui {

// Handle type for UI nodes - Ruby only sees these, never raw pointers
using UINodeHandle = int32_t;
constexpr UINodeHandle INVALID_UI_NODE_HANDLE = -1;

// Layout mode for containers
enum class LayoutMode : uint8_t {
    NONE,       // Absolute positioning (manual x, y)
    VERTICAL,   // Stack children top-to-bottom
    HORIZONTAL  // Stack children left-to-right
};

// Anchor point for positioning relative to parent
enum class Anchor : uint8_t {
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

// UI node type
enum class UINodeType : uint8_t {
    PANEL,      // Container with background
    LABEL,      // Text display
    BUTTON,     // Interactive button
    IMAGE       // Image display
};

// Callback types for UI events
enum class UICallbackType : uint8_t {
    ON_CLICK,
    ON_HOVER_ENTER,
    ON_HOVER_EXIT,
    ON_FOCUS,
    ON_BLUR,
    CALLBACK_COUNT  // Must be last
};

// Padding/spacing helper
struct UISpacing {
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;

    UISpacing() = default;
    UISpacing(float all) : top(all), right(all), bottom(all), left(all) {}
    UISpacing(float vertical, float horizontal)
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {}
    UISpacing(float t, float r, float b, float l)
        : top(t), right(r), bottom(b), left(l) {}

    float horizontal() const { return left + right; }
    float vertical() const { return top + bottom; }
};

} // namespace ui
} // namespace gmr

#endif
