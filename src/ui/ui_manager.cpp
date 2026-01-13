#include "gmr/ui/ui_manager.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/state.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/resources/font_manager.hpp"
#include "raylib.h"
#include <mruby/proc.h>
#include <algorithm>

namespace gmr {
namespace ui {

UIManager& UIManager::instance() {
    static UIManager instance;
    return instance;
}

// ============================================================================
// Node Lifecycle
// ============================================================================

UINodeHandle UIManager::create(UINodeType type) {
    UINodeHandle handle = next_id_++;
    UINodeState& state = nodes_[handle];
    state.type = type;
    layout_dirty_ = true;
    return handle;
}

void UIManager::destroy(mrb_state* mrb, UINodeHandle handle) {
    auto it = nodes_.find(handle);
    if (it == nodes_.end()) return;

    UINodeState& state = it->second;

    // Unregister callbacks from GC
    for (auto& cb : state.callbacks) {
        if (!mrb_nil_p(cb)) {
            mrb_gc_unregister(mrb, cb);
            cb = mrb_nil_value();
        }
    }

    // Unregister ruby_self from GC
    if (!mrb_nil_p(state.ruby_self)) {
        mrb_gc_unregister(mrb, state.ruby_self);
    }

    // Remove from parent's children list
    if (state.parent != INVALID_UI_NODE_HANDLE) {
        auto pit = children_.find(state.parent);
        if (pit != children_.end()) {
            auto& siblings = pit->second;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), handle), siblings.end());
        }
    }

    // Recursively destroy children
    auto cit = children_.find(handle);
    if (cit != children_.end()) {
        // Copy list to avoid iterator invalidation
        std::vector<UINodeHandle> child_handles = cit->second;
        for (UINodeHandle child : child_handles) {
            destroy(mrb, child);
        }
        children_.erase(cit);
    }

    // Clear interaction state if this node was involved
    if (hovered_node_ == handle) hovered_node_ = INVALID_UI_NODE_HANDLE;
    if (pressed_node_ == handle) pressed_node_ = INVALID_UI_NODE_HANDLE;
    if (focused_node_ == handle) focused_node_ = INVALID_UI_NODE_HANDLE;
    if (root_ == handle) root_ = INVALID_UI_NODE_HANDLE;

    nodes_.erase(it);
    layout_dirty_ = true;
}

UINodeState* UIManager::get(UINodeHandle handle) {
    auto it = nodes_.find(handle);
    return (it != nodes_.end()) ? &it->second : nullptr;
}

const UINodeState* UIManager::get(UINodeHandle handle) const {
    auto it = nodes_.find(handle);
    return (it != nodes_.end()) ? &it->second : nullptr;
}

bool UIManager::valid(UINodeHandle handle) const {
    return nodes_.find(handle) != nodes_.end();
}

// ============================================================================
// Hierarchy
// ============================================================================

void UIManager::set_parent(UINodeHandle child, UINodeHandle parent) {
    UINodeState* child_state = get(child);
    if (!child_state) return;

    // Remove from old parent
    if (child_state->parent != INVALID_UI_NODE_HANDLE) {
        auto it = children_.find(child_state->parent);
        if (it != children_.end()) {
            auto& siblings = it->second;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
        }
    }

    // Set new parent
    child_state->parent = parent;

    // Add to new parent's children
    if (parent != INVALID_UI_NODE_HANDLE) {
        children_[parent].push_back(child);
    }

    layout_dirty_ = true;
}

void UIManager::clear_parent(UINodeHandle child) {
    UINodeState* child_state = get(child);
    if (!child_state) return;

    if (child_state->parent != INVALID_UI_NODE_HANDLE) {
        auto it = children_.find(child_state->parent);
        if (it != children_.end()) {
            auto& siblings = it->second;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
        }
    }

    child_state->parent = INVALID_UI_NODE_HANDLE;
    layout_dirty_ = true;
}

std::vector<UINodeHandle> UIManager::get_children(UINodeHandle parent) const {
    auto it = children_.find(parent);
    return (it != children_.end()) ? it->second : std::vector<UINodeHandle>{};
}

// ============================================================================
// Root Management
// ============================================================================

void UIManager::set_root(UINodeHandle root) {
    root_ = root;
    layout_dirty_ = true;
}

void UIManager::clear_root() {
    root_ = INVALID_UI_NODE_HANDLE;
}

// ============================================================================
// Layout
// ============================================================================

void UIManager::mark_layout_dirty() {
    layout_dirty_ = true;
}

void UIManager::compute_layout() {
    if (root_ == INVALID_UI_NODE_HANDLE) return;

    // Get screen dimensions
    auto& state = State::instance();
    float screen_w, screen_h;
    if (state.use_virtual_resolution) {
        screen_w = static_cast<float>(state.virtual_width);
        screen_h = static_cast<float>(state.virtual_height);
    } else {
        screen_w = static_cast<float>(state.screen_width);
        screen_h = static_cast<float>(state.screen_height);
    }

    // Check if screen size changed - if so, mark layout dirty
    if (screen_w != last_screen_w_ || screen_h != last_screen_h_) {
        layout_dirty_ = true;
        last_screen_w_ = screen_w;
        last_screen_h_ = screen_h;
    }

    if (!layout_dirty_) return;

    Rect screen_bounds{0, 0, screen_w, screen_h};
    compute_node_layout(root_, screen_bounds);

    layout_dirty_ = false;
}

void UIManager::compute_node_layout(UINodeHandle handle, const Rect& available) {
    UINodeState* node = get(handle);
    if (!node || !node->visible) return;

    // Get UI scale factor - node dimensions are specified in 360p baseline coordinates
    // and scaled to the current virtual/screen resolution
    auto& state = State::instance();
    float ui_scale = state.ui_scale();

    // Scale node dimensions from 360p baseline to current resolution
    float scaled_width = node->width > 0 ? node->width * ui_scale : 0;
    float scaled_height = node->height > 0 ? node->height * ui_scale : 0;
    float scaled_x = node->x * ui_scale;
    float scaled_y = node->y * ui_scale;
    float scaled_padding_top = node->padding.top * ui_scale;
    float scaled_padding_right = node->padding.right * ui_scale;
    float scaled_padding_bottom = node->padding.bottom * ui_scale;
    float scaled_padding_left = node->padding.left * ui_scale;

    // Determine node size (0 means use available space)
    float w = scaled_width > 0 ? scaled_width : available.width;
    float h = scaled_height > 0 ? scaled_height : available.height;

    // Determine position based on anchor
    float x = available.x + scaled_x;
    float y = available.y + scaled_y;

    switch (node->anchor) {
        case Anchor::TOP_LEFT:
            break;
        case Anchor::TOP_CENTER:
            x += (available.width - w) / 2;
            break;
        case Anchor::TOP_RIGHT:
            x += available.width - w;
            break;
        case Anchor::CENTER_LEFT:
            y += (available.height - h) / 2;
            break;
        case Anchor::CENTER:
            x += (available.width - w) / 2;
            y += (available.height - h) / 2;
            break;
        case Anchor::CENTER_RIGHT:
            x += available.width - w;
            y += (available.height - h) / 2;
            break;
        case Anchor::BOTTOM_LEFT:
            y += available.height - h;
            break;
        case Anchor::BOTTOM_CENTER:
            x += (available.width - w) / 2;
            y += available.height - h;
            break;
        case Anchor::BOTTOM_RIGHT:
            x += available.width - w;
            y += available.height - h;
            break;
    }

    node->computed_bounds = {x, y, w, h};
    node->content_bounds = {
        x + scaled_padding_left,
        y + scaled_padding_top,
        w - (scaled_padding_left + scaled_padding_right),
        h - (scaled_padding_top + scaled_padding_bottom)
    };

    // Layout children
    switch (node->layout) {
        case LayoutMode::NONE:
            layout_children_none(handle);
            break;
        case LayoutMode::VERTICAL:
            layout_children_stack(handle, true);
            break;
        case LayoutMode::HORIZONTAL:
            layout_children_stack(handle, false);
            break;
    }
}

void UIManager::layout_children_none(UINodeHandle handle) {
    UINodeState* parent = get(handle);
    if (!parent) return;

    for (UINodeHandle child : get_children(handle)) {
        compute_node_layout(child, parent->content_bounds);
    }
}

void UIManager::layout_children_stack(UINodeHandle handle, bool vertical) {
    UINodeState* parent = get(handle);
    if (!parent) return;

    // Get UI scale factor for scaling child dimensions
    auto& state = State::instance();
    float ui_scale = state.ui_scale();

    Rect content = parent->content_bounds;
    float cursor = vertical ? content.y : content.x;

    // Scale parent spacing
    float scaled_spacing = parent->spacing * ui_scale;

    for (UINodeHandle child_handle : get_sorted_children(handle)) {
        UINodeState* child = get(child_handle);
        if (!child || !child->visible) continue;

        // Scale child dimensions from 360p baseline to current resolution
        float scaled_child_w = child->width > 0 ? child->width * ui_scale : content.width;
        float scaled_child_h = child->height > 0 ? child->height * ui_scale : 0;  // 0 height means auto
        float scaled_padding_top = child->padding.top * ui_scale;
        float scaled_padding_right = child->padding.right * ui_scale;
        float scaled_padding_bottom = child->padding.bottom * ui_scale;
        float scaled_padding_left = child->padding.left * ui_scale;

        // Position child directly (ignoring child's anchor for stacked layout)
        float child_x, child_y;
        if (vertical) {
            // Center horizontally within content area
            child_x = content.x + (content.width - scaled_child_w) / 2;
            child_y = cursor;
        } else {
            child_x = cursor;
            // Center vertically within content area
            child_y = content.y + (content.height - scaled_child_h) / 2;
        }

        // Set computed bounds directly
        child->computed_bounds = {child_x, child_y, scaled_child_w, scaled_child_h};
        child->content_bounds = {
            child_x + scaled_padding_left,
            child_y + scaled_padding_top,
            scaled_child_w - (scaled_padding_left + scaled_padding_right),
            scaled_child_h - (scaled_padding_top + scaled_padding_bottom)
        };

        // Recursively layout grandchildren if this child has children
        switch (child->layout) {
            case LayoutMode::NONE:
                layout_children_none(child_handle);
                break;
            case LayoutMode::VERTICAL:
                layout_children_stack(child_handle, true);
                break;
            case LayoutMode::HORIZONTAL:
                layout_children_stack(child_handle, false);
                break;
        }

        // Advance cursor using scaled dimensions and spacing
        if (vertical) {
            cursor = child_y + scaled_child_h + scaled_spacing;
        } else {
            cursor = child_x + scaled_child_w + scaled_spacing;
        }
    }
}

std::vector<UINodeHandle> UIManager::get_sorted_children(UINodeHandle parent) const {
    auto children = get_children(parent);

    // Sort by z_index
    std::sort(children.begin(), children.end(), [this](UINodeHandle a, UINodeHandle b) {
        const UINodeState* na = get(a);
        const UINodeState* nb = get(b);
        if (!na) return true;
        if (!nb) return false;
        return na->z_index < nb->z_index;
    });

    return children;
}

// ============================================================================
// Hit Testing
// ============================================================================

UINodeHandle UIManager::hit_test(float screen_x, float screen_y) const {
    if (root_ == INVALID_UI_NODE_HANDLE) return INVALID_UI_NODE_HANDLE;
    return hit_test_node(root_, screen_x, screen_y);
}

UINodeHandle UIManager::hit_test_node(UINodeHandle handle, float x, float y) const {
    const UINodeState* node = get(handle);
    if (!node || !node->visible || !node->enabled) return INVALID_UI_NODE_HANDLE;

    // Check if point is in bounds
    const Rect& b = node->computed_bounds;
    bool in_bounds = (x >= b.x && x < b.x + b.width && y >= b.y && y < b.y + b.height);

    if (!in_bounds) return INVALID_UI_NODE_HANDLE;

    // Check children in reverse z-order (topmost first)
    auto children = get_sorted_children(handle);
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        UINodeHandle child_hit = hit_test_node(*it, x, y);
        if (child_hit != INVALID_UI_NODE_HANDLE) {
            return child_hit;
        }
    }

    // No child hit, return this node if it's interactive (Button)
    if (node->type == UINodeType::BUTTON) {
        return handle;
    }

    // Panels only register hits if they have background
    if (node->type == UINodeType::PANEL && node->background_color.a > 0) {
        return handle;
    }

    return INVALID_UI_NODE_HANDLE;
}

// ============================================================================
// Input Processing
// ============================================================================

void UIManager::process_input(mrb_state* mrb) {
    if (root_ == INVALID_UI_NODE_HANDLE) return;

    // Get mouse position, converting to virtual resolution if needed
    auto& state = State::instance();
    float mx, my;

    if (state.use_virtual_resolution) {
        // Use CSS dimensions for mouse calculations (browser reports mouse in CSS pixels)
        float scale_x = static_cast<float>(state.css_width) / state.virtual_width;
        float scale_y = static_cast<float>(state.css_height) / state.virtual_height;
        float scale = (scale_x < scale_y) ? scale_x : scale_y;

        int scaled_width = static_cast<int>(state.virtual_width * scale);
        int scaled_height = static_cast<int>(state.virtual_height * scale);
        int offset_x = (state.css_width - scaled_width) / 2;
        int offset_y = (state.css_height - scaled_height) / 2;

        mx = static_cast<float>(GetMouseX() - offset_x) / scale;
        my = static_cast<float>(GetMouseY() - offset_y) / scale;
    } else {
        mx = static_cast<float>(GetMouseX());
        my = static_cast<float>(GetMouseY());
    }

    // Hit test
    UINodeHandle hit = hit_test(mx, my);

    // Handle hover state changes
    if (hit != hovered_node_) {
        // Mouse left old node
        if (hovered_node_ != INVALID_UI_NODE_HANDLE) {
            UINodeState* old_node = get(hovered_node_);
            if (old_node) {
                old_node->hovered = false;
                invoke_callback(mrb, hovered_node_, UICallbackType::ON_HOVER_EXIT);
            }
        }

        // Mouse entered new node
        if (hit != INVALID_UI_NODE_HANDLE) {
            UINodeState* new_node = get(hit);
            if (new_node && new_node->enabled) {
                new_node->hovered = true;
                invoke_callback(mrb, hit, UICallbackType::ON_HOVER_ENTER);
            }
        }

        hovered_node_ = hit;
    }

    // Handle mouse press
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hit != INVALID_UI_NODE_HANDLE) {
            UINodeState* node = get(hit);
            if (node && node->enabled) {
                node->pressed = true;
                pressed_node_ = hit;
            }
        }
    }

    // Handle mouse release
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (pressed_node_ != INVALID_UI_NODE_HANDLE) {
            UINodeState* node = get(pressed_node_);
            if (node) {
                node->pressed = false;

                // Click = release on same element we pressed
                if (pressed_node_ == hovered_node_) {
                    invoke_callback(mrb, pressed_node_, UICallbackType::ON_CLICK);
                }
            }
            pressed_node_ = INVALID_UI_NODE_HANDLE;
        }
    }
}

// ============================================================================
// Rendering
// ============================================================================

void UIManager::render() {
    if (root_ == INVALID_UI_NODE_HANDLE) return;
    render_node(root_);
}

void UIManager::render_node(UINodeHandle handle) {
    UINodeState* node = get(handle);
    if (!node || !node->visible) return;

    const Rect& b = node->computed_bounds;
    auto& queue = DrawQueue::instance();

    // Determine background color based on state
    Color bg = node->background_color;
    if (node->pressed && node->pressed_background.a > 0) {
        bg = node->pressed_background;
    } else if (node->hovered && node->hover_background.a > 0) {
        bg = node->hover_background;
    }

    // Draw background
    if (bg.a > 0) {
        DrawColor dc{bg.r, bg.g, bg.b, bg.a};
        queue.queue_rect(b.x, b.y, b.width, b.height, dc, true,
                        static_cast<uint8_t>(RenderLayer::UI), 0.0f);
    }

    // Draw border
    if (node->border_width > 0 && node->border_color.a > 0) {
        DrawColor bc{node->border_color.r, node->border_color.g, node->border_color.b, node->border_color.a};
        queue.queue_rect(b.x, b.y, b.width, b.height, bc, false,
                        static_cast<uint8_t>(RenderLayer::UI), 0.1f);
    }

    // Draw text (for Label, Button)
    if (!node->text.empty()) {
        DrawColor tc{node->text_color.r, node->text_color.g, node->text_color.b, node->text_color.a};

        // computed_bounds are in virtual resolution space (already scaled).
        // node->font_size is in 360p baseline coordinates.
        // Screen-space text rendering expects 360p baseline coordinates and applies ui_scale.
        auto& game_state = State::instance();
        float ui_scale = game_state.ui_scale();

        // Font size in virtual resolution pixels (for measuring)
        int font_size_scaled = static_cast<int>(static_cast<float>(node->font_size) * ui_scale);
        if (font_size_scaled < 1) font_size_scaled = 1;

        // Calculate text position in virtual resolution space
        float text_x = node->content_bounds.x;
        float text_y = node->content_bounds.y;

        if (node->type == UINodeType::BUTTON) {
            // Measure text to center it using scaled font size
            int text_width = MeasureText(node->text.c_str(), font_size_scaled);
            text_x = b.x + (b.width - static_cast<float>(text_width)) / 2;
            text_y = b.y + (b.height - static_cast<float>(font_size_scaled)) / 2;
        }

        // Convert from virtual resolution coords to 360p baseline coords
        // When draw_text applies ui_scale, it will end up at the correct virtual resolution position
        float text_x_baseline = text_x / ui_scale;
        float text_y_baseline = text_y / ui_scale;

        // Font size in 360p baseline (draw_text will scale it back up)
        int font_size_baseline = node->font_size;

        if (node->font != INVALID_HANDLE) {
            queue.queue_text(text_x_baseline, text_y_baseline, node->text, font_size_baseline, tc, node->font,
                            static_cast<uint8_t>(RenderLayer::UI), 0.2f);
        } else {
            queue.queue_text(text_x_baseline, text_y_baseline, node->text, font_size_baseline, tc,
                            static_cast<uint8_t>(RenderLayer::UI), 0.2f);
        }
    }

    // Render children
    for (UINodeHandle child : get_sorted_children(handle)) {
        render_node(child);
    }
}

// ============================================================================
// Callback Invocation
// ============================================================================

void UIManager::invoke_callback(mrb_state* mrb, UINodeHandle handle, UICallbackType type) {
    UINodeState* node = get(handle);
    if (!node) return;

    size_t idx = static_cast<size_t>(type);
    mrb_value callback = node->callbacks[idx];

    if (mrb_nil_p(callback)) return;

    // Use safe_yield to call the callback
    scripting::safe_yield(mrb, callback, mrb_nil_value());
}

// ============================================================================
// Cleanup
// ============================================================================

void UIManager::clear(mrb_state* mrb) {
    // Copy handles to avoid iterator invalidation
    std::vector<UINodeHandle> handles;
    handles.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        handles.push_back(pair.first);
    }

    for (UINodeHandle handle : handles) {
        destroy(mrb, handle);
    }

    nodes_.clear();
    children_.clear();
    root_ = INVALID_UI_NODE_HANDLE;
    hovered_node_ = INVALID_UI_NODE_HANDLE;
    pressed_node_ = INVALID_UI_NODE_HANDLE;
    focused_node_ = INVALID_UI_NODE_HANDLE;
    layout_dirty_ = true;
    last_screen_w_ = 0;
    last_screen_h_ = 0;
}

} // namespace ui
} // namespace gmr
