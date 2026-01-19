#ifndef GMR_NODE_HPP
#define GMR_NODE_HPP

#include "gmr/types.hpp"
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cmath>

namespace gmr {

// Handle type for safe Ruby access
using NodeHandle = int32_t;
constexpr NodeHandle INVALID_NODE_HANDLE = -1;

// Transform - position, rotation, scale only
struct Transform {
    Vec2 position{0.0f, 0.0f};
    float rotation{0.0f};       // Radians internally
    Vec2 scale{1.0f, 1.0f};
};

// Node - hierarchical transform node
// Ownership: NodeManager owns all Node instances via nodes_ map
// Children: Stored in NodeManager::children_ map (not in Node struct)
struct Node {
    Transform local;
    Transform world;
    NodeHandle parent{INVALID_NODE_HANDLE};  // Handle-based parent reference (safe)
    bool active{true};
};

// NodeManager - singleton, owns all Node memory
// Ownership model:
//   - nodes_ map owns all Node instances
//   - children_ map owns parent->children relationships
//   - Node::parent stores parent handle (not pointer)
class NodeManager {
public:
    static NodeManager& instance();

    // Lifecycle
    NodeHandle create();
    void destroy(NodeHandle handle);

    /// Get a node by handle. Returns nullptr if handle is invalid.
    /// IMPORTANT: The returned pointer is only valid until the next create() or destroy() call.
    /// Do not store this pointer; re-fetch it on each use.
    Node* get(NodeHandle handle);
    bool valid(NodeHandle handle) const;

    // Hierarchy management
    void add_child(NodeHandle parent, NodeHandle child);
    void remove_child(NodeHandle parent, NodeHandle child);

    // Child access (handle-based, safe)
    std::vector<NodeHandle> get_children(NodeHandle handle) const;
    size_t child_count(NodeHandle handle) const;

    // World transform computation (explicit, caller-controlled)
    void update_world_transforms(NodeHandle root);

    // Active state query (checks full hierarchy)
    bool is_active_in_hierarchy(NodeHandle handle) const;

    // Depth-first traversal
    using TraversalCallback = void(*)(Node* node, void* user_data);
    void traverse_depth_first(NodeHandle root, TraversalCallback callback, void* user_data);

    // Get handle for a node pointer (O(n) - prefer using handles directly)
    // Kept for backwards compatibility; internal code uses handle-based access
    NodeHandle get_handle(Node* node) const;

    // Clear all nodes
    void clear();

    // Debug
    size_t count() const { return nodes_.size(); }

private:
    NodeManager() = default;
    NodeManager(const NodeManager&) = delete;
    NodeManager& operator=(const NodeManager&) = delete;

    // Internal helpers
    bool would_create_cycle(NodeHandle parent, NodeHandle child) const;
    void remove_from_parent(NodeHandle handle);
    void compute_world_transform(Node* node, const Transform* parent_world);

    std::unordered_map<NodeHandle, Node> nodes_;
    std::unordered_map<NodeHandle, std::vector<NodeHandle>> children_;  // parent -> children
    NodeHandle next_id_{0};
};

} // namespace gmr

#endif
