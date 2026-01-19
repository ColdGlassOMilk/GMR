#include "gmr/node.hpp"
#include <algorithm>

namespace gmr {

NodeManager& NodeManager::instance() {
    static NodeManager inst;
    return inst;
}

NodeHandle NodeManager::create() {
    NodeHandle handle = next_id_++;
    Node& node = nodes_[handle];
    node.local = Transform{};
    node.world = Transform{};
    node.parent = INVALID_NODE_HANDLE;
    node.active = true;
    return handle;
}

void NodeManager::destroy(NodeHandle handle) {
    auto it = nodes_.find(handle);
    if (it == nodes_.end()) return;

    Node& node = it->second;

    // Remove from parent's children list
    if (node.parent != INVALID_NODE_HANDLE) {
        auto parent_it = children_.find(node.parent);
        if (parent_it != children_.end()) {
            auto& siblings = parent_it->second;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), handle), siblings.end());
            if (siblings.empty()) {
                children_.erase(parent_it);
            }
        }
    }

    // Orphan all children (set their parent to INVALID_NODE_HANDLE)
    auto children_it = children_.find(handle);
    if (children_it != children_.end()) {
        for (NodeHandle child_handle : children_it->second) {
            auto* child = get(child_handle);
            if (child) {
                child->parent = INVALID_NODE_HANDLE;
            }
        }
        children_.erase(children_it);
    }

    nodes_.erase(it);
}

Node* NodeManager::get(NodeHandle handle) {
    auto it = nodes_.find(handle);
    if (it == nodes_.end()) return nullptr;
    return &it->second;
}

bool NodeManager::valid(NodeHandle handle) const {
    return nodes_.find(handle) != nodes_.end();
}

NodeHandle NodeManager::get_handle(Node* node) const {
    if (!node) return INVALID_NODE_HANDLE;
    for (const auto& [h, n] : nodes_) {
        if (&n == node) return h;
    }
    return INVALID_NODE_HANDLE;
}

bool NodeManager::would_create_cycle(NodeHandle parent_handle, NodeHandle child_handle) const {
    // If child would become parent's ancestor, that's a cycle
    // Walk up from parent; if we reach child, reject
    NodeHandle current = parent_handle;
    while (current != INVALID_NODE_HANDLE) {
        if (current == child_handle) return true;
        auto it = nodes_.find(current);
        if (it == nodes_.end()) break;
        current = it->second.parent;  // Direct handle access, no O(n) lookup
    }
    return false;
}

void NodeManager::remove_from_parent(NodeHandle handle) {
    Node* node = get(handle);
    if (!node || node->parent == INVALID_NODE_HANDLE) return;

    NodeHandle parent_handle = node->parent;

    // Remove from parent's children vector
    auto it = children_.find(parent_handle);
    if (it != children_.end()) {
        auto& siblings = it->second;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), handle), siblings.end());
        if (siblings.empty()) {
            children_.erase(it);
        }
    }

    node->parent = INVALID_NODE_HANDLE;
}

void NodeManager::add_child(NodeHandle parent_handle, NodeHandle child_handle) {
    Node* parent = get(parent_handle);
    Node* child = get(child_handle);
    if (!parent || !child) return;

    // Cannot add self as child
    if (parent_handle == child_handle) return;

    // Cycle check
    if (would_create_cycle(parent_handle, child_handle)) return;

    // Remove from previous parent if any
    if (child->parent != INVALID_NODE_HANDLE) {
        remove_from_parent(child_handle);
    }

    // Add to new parent's children vector
    children_[parent_handle].push_back(child_handle);
    child->parent = parent_handle;
}

void NodeManager::remove_child(NodeHandle parent_handle, NodeHandle child_handle) {
    Node* parent = get(parent_handle);
    Node* child = get(child_handle);
    if (!parent || !child) return;
    if (child->parent != parent_handle) return;

    remove_from_parent(child_handle);
}

void NodeManager::compute_world_transform(Node* node, const Transform* parent_world) {
    if (!parent_world) {
        // Root node: world = local
        node->world = node->local;
        return;
    }

    // Position: rotate local position by parent rotation, scale by parent scale, add parent position
    float cos_r = cosf(parent_world->rotation);
    float sin_r = sinf(parent_world->rotation);

    Vec2 scaled_local{
        node->local.position.x * parent_world->scale.x,
        node->local.position.y * parent_world->scale.y
    };

    Vec2 rotated{
        scaled_local.x * cos_r - scaled_local.y * sin_r,
        scaled_local.x * sin_r + scaled_local.y * cos_r
    };

    node->world.position.x = parent_world->position.x + rotated.x;
    node->world.position.y = parent_world->position.y + rotated.y;

    // Rotation: additive
    node->world.rotation = parent_world->rotation + node->local.rotation;

    // Scale: multiplicative
    node->world.scale.x = parent_world->scale.x * node->local.scale.x;
    node->world.scale.y = parent_world->scale.y * node->local.scale.y;
}

void NodeManager::update_world_transforms(NodeHandle root) {
    Node* node = get(root);
    if (!node) return;

    // Get parent's world transform (or nullptr for root)
    Transform* parent_world = nullptr;
    if (node->parent != INVALID_NODE_HANDLE) {
        Node* parent = get(node->parent);
        if (parent) parent_world = &parent->world;
    }

    compute_world_transform(node, parent_world);

    // Recurse to children using handle-based lookup
    auto it = children_.find(root);
    if (it != children_.end()) {
        for (NodeHandle child_handle : it->second) {
            update_world_transforms(child_handle);
        }
    }
}

bool NodeManager::is_active_in_hierarchy(NodeHandle handle) const {
    auto it = nodes_.find(handle);
    while (it != nodes_.end()) {
        const Node& node = it->second;
        if (!node.active) return false;
        if (node.parent == INVALID_NODE_HANDLE) return true;

        // Direct handle access, continue walking up
        it = nodes_.find(node.parent);
    }
    return true;
}

void NodeManager::traverse_depth_first(NodeHandle root, TraversalCallback callback, void* user_data) {
    Node* node = get(root);
    if (!node) return;

    callback(node, user_data);

    // Recurse to children using handle-based lookup
    auto it = children_.find(root);
    if (it != children_.end()) {
        for (NodeHandle child_handle : it->second) {
            traverse_depth_first(child_handle, callback, user_data);
        }
    }
}

void NodeManager::clear() {
    nodes_.clear();
    children_.clear();
    next_id_ = 0;
}

std::vector<NodeHandle> NodeManager::get_children(NodeHandle handle) const {
    auto it = children_.find(handle);
    if (it != children_.end()) {
        return it->second;
    }
    return {};
}

size_t NodeManager::child_count(NodeHandle handle) const {
    auto it = children_.find(handle);
    return (it != children_.end()) ? it->second.size() : 0;
}

} // namespace gmr
