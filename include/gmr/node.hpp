#ifndef GMR_NODE_HPP
#define GMR_NODE_HPP

#include "gmr/types.hpp"
#include <unordered_map>
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

// Node - exactly as specified, no extra fields
struct Node {
    Transform local;
    Transform world;
    Node* parent;
    Node** children;
    int child_count;
    bool active;
};

// NodeManager - singleton, owns all Node memory
class NodeManager {
public:
    static NodeManager& instance();

    // Lifecycle
    NodeHandle create();
    void destroy(NodeHandle handle);
    Node* get(NodeHandle handle);
    bool valid(NodeHandle handle) const;

    // Hierarchy management
    void add_child(NodeHandle parent, NodeHandle child);
    void remove_child(NodeHandle parent, NodeHandle child);

    // World transform computation (explicit, caller-controlled)
    void update_world_transforms(NodeHandle root);

    // Active state query (checks full hierarchy)
    bool is_active_in_hierarchy(NodeHandle handle) const;

    // Depth-first traversal
    using TraversalCallback = void(*)(Node* node, void* user_data);
    void traverse_depth_first(NodeHandle root, TraversalCallback callback, void* user_data);

    // Get handle for a node pointer (needed for child lookups)
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
    NodeHandle next_id_{0};
};

} // namespace gmr

#endif
