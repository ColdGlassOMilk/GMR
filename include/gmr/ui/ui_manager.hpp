#ifndef GMR_UI_MANAGER_HPP
#define GMR_UI_MANAGER_HPP

#include "gmr/ui/ui_types.hpp"
#include "gmr/ui/ui_node.hpp"
#include <mruby.h>
#include <unordered_map>
#include <vector>

namespace gmr {
namespace ui {

// Singleton manager for UI nodes
// Owns all UI node state, provides layout computation, hit testing, and rendering
class UIManager {
public:
    static UIManager& instance();

    // ========================================================================
    // Node Lifecycle
    // ========================================================================

    // Create a new UI node, returns handle
    UINodeHandle create(UINodeType type = UINodeType::PANEL);

    // Destroy a node and unregister its callbacks
    void destroy(mrb_state* mrb, UINodeHandle handle);

    // Get node state by handle (returns nullptr if invalid)
    UINodeState* get(UINodeHandle handle);
    const UINodeState* get(UINodeHandle handle) const;

    // Check if handle is valid
    bool valid(UINodeHandle handle) const;

    // ========================================================================
    // Hierarchy
    // ========================================================================

    // Set parent-child relationship
    void set_parent(UINodeHandle child, UINodeHandle parent);

    // Remove child from parent
    void clear_parent(UINodeHandle child);

    // Get children of a node
    std::vector<UINodeHandle> get_children(UINodeHandle parent) const;

    // ========================================================================
    // Root Management
    // ========================================================================

    // Set the root node for rendering (replaces any existing root)
    void set_root(UINodeHandle root);

    // Get current root
    UINodeHandle get_root() const { return root_; }

    // Clear root (stops rendering)
    void clear_root();

    // ========================================================================
    // Layout
    // ========================================================================

    // Recompute layout for the entire tree (call before render)
    void compute_layout();

    // Mark layout as needing recomputation
    void mark_layout_dirty();

    // ========================================================================
    // Hit Testing
    // ========================================================================

    // Returns topmost visible, enabled node at screen position
    // Returns INVALID_UI_NODE_HANDLE if no hit
    UINodeHandle hit_test(float screen_x, float screen_y) const;

    // ========================================================================
    // Input Processing
    // ========================================================================

    // Process mouse input, update hover/pressed states, invoke callbacks
    // Call once per frame before render
    void process_input(mrb_state* mrb);

    // ========================================================================
    // Rendering
    // ========================================================================

    // Queue all visible nodes to DrawQueue
    void render();

    // ========================================================================
    // Cleanup
    // ========================================================================

    // Destroy all nodes and clear state
    void clear(mrb_state* mrb);

    // ========================================================================
    // Debug
    // ========================================================================

    size_t node_count() const { return nodes_.size(); }

private:
    UIManager() = default;
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    // Layout helpers
    void compute_node_layout(UINodeHandle handle, const Rect& available);
    void layout_children_none(UINodeHandle handle);
    void layout_children_stack(UINodeHandle handle, bool vertical);

    // Hit test recursive helper
    UINodeHandle hit_test_node(UINodeHandle handle, float x, float y) const;

    // Render recursive helper
    void render_node(UINodeHandle handle);

    // Callback invocation
    void invoke_callback(mrb_state* mrb, UINodeHandle handle, UICallbackType type);

    // Build render order (children sorted by z_index)
    std::vector<UINodeHandle> get_sorted_children(UINodeHandle parent) const;

    // Node storage
    std::unordered_map<UINodeHandle, UINodeState> nodes_;

    // Children lookup
    std::unordered_map<UINodeHandle, std::vector<UINodeHandle>> children_;

    // Current root for rendering
    UINodeHandle root_{INVALID_UI_NODE_HANDLE};

    // Interaction state
    UINodeHandle hovered_node_{INVALID_UI_NODE_HANDLE};
    UINodeHandle pressed_node_{INVALID_UI_NODE_HANDLE};
    UINodeHandle focused_node_{INVALID_UI_NODE_HANDLE};

    // Layout dirty flag
    bool layout_dirty_{true};

    // Last known screen size (to detect resizes)
    float last_screen_w_{0};
    float last_screen_h_{0};

    // Handle allocation
    UINodeHandle next_id_{0};
};

} // namespace ui
} // namespace gmr

#endif
