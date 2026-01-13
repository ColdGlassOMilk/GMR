#ifndef GMR_UI_NODE_HPP
#define GMR_UI_NODE_HPP

#include "gmr/ui/ui_types.hpp"
#include "gmr/types.hpp"
#include <mruby.h>
#include <string>

namespace gmr {
namespace ui {

// Complete state for a UI node
struct UINodeState {
    // Type
    UINodeType type{UINodeType::PANEL};

    // Hierarchy
    UINodeHandle parent{INVALID_UI_NODE_HANDLE};
    std::string id;  // Optional identifier for lookup

    // Position/Size (local space, relative to parent)
    float x{0};
    float y{0};
    float width{0};
    float height{0};
    Anchor anchor{Anchor::TOP_LEFT};

    // Layout (for containers)
    LayoutMode layout{LayoutMode::NONE};
    float spacing{0};  // Gap between children
    UISpacing padding;

    // Computed bounds (set by layout system, screen space)
    Rect computed_bounds{0, 0, 0, 0};
    Rect content_bounds{0, 0, 0, 0};  // Inside padding

    // Visual properties
    Color background_color{0, 0, 0, 0};  // Transparent by default
    Color hover_background{0, 0, 0, 0};
    Color pressed_background{0, 0, 0, 0};
    Color border_color{0, 0, 0, 0};
    float border_width{0};
    float corner_radius{0};

    // Text (for Label, Button)
    std::string text;
    FontHandle font{INVALID_HANDLE};
    int font_size{16};
    Color text_color{255, 255, 255, 255};

    // Texture (for Image)
    TextureHandle texture{INVALID_HANDLE};
    Rect source_rect{0, 0, 0, 0};

    // State flags
    bool visible{true};
    bool enabled{true};
    bool hovered{false};
    bool pressed{false};
    bool focused{false};

    // Z-index for sorting among siblings
    int16_t z_index{0};

    // Ruby callbacks (GC-protected)
    mrb_value callbacks[static_cast<size_t>(UICallbackType::CALLBACK_COUNT)];

    // Reference to Ruby object for callback invocation
    mrb_value ruby_self{mrb_nil_value()};

    UINodeState() {
        for (auto& cb : callbacks) {
            cb = mrb_nil_value();
        }
    }
};

} // namespace ui
} // namespace gmr

#endif
