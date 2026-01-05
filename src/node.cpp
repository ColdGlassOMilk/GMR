#include "gmr/node.hpp"
#include <cstdlib>

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
    node.parent = nullptr;
    node.children = nullptr;
    node.child_count = 0;
    node.active = true;
    return handle;
}

void NodeManager::destroy(NodeHandle handle) {
    auto it = nodes_.find(handle);
    if (it == nodes_.end()) return;

    Node* node = &it->second;

    // Remove from parent's child array
    if (node->parent) {
        remove_from_parent(handle);
    }

    // Orphan all children (set their parent to nullptr)
    for (int i = 0; i < node->child_count; ++i) {
        if (node->children[i]) {
            node->children[i]->parent = nullptr;
        }
    }

    // Free children array
    if (node->children) {
        free(node->children);
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
        const Node& node = it->second;
        if (!node.parent) break;
        current = get_handle(node.parent);
    }
    return false;
}

void NodeManager::remove_from_parent(NodeHandle handle) {
    Node* node = get(handle);
    if (!node || !node->parent) return;

    Node* parent = node->parent;

    // Find this node in parent's children array and remove it
    for (int i = 0; i < parent->child_count; ++i) {
        if (parent->children[i] == node) {
            // Shift remaining children down
            for (int j = i; j < parent->child_count - 1; ++j) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;

            // Shrink or free array
            if (parent->child_count == 0) {
                free(parent->children);
                parent->children = nullptr;
            } else {
                parent->children = static_cast<Node**>(
                    realloc(parent->children, sizeof(Node*) * parent->child_count));
            }
            break;
        }
    }

    node->parent = nullptr;
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
    if (child->parent) {
        remove_from_parent(child_handle);
    }

    // Add to new parent's children array
    parent->children = static_cast<Node**>(
        realloc(parent->children, sizeof(Node*) * (parent->child_count + 1)));
    parent->children[parent->child_count] = child;
    parent->child_count++;

    child->parent = parent;
}

void NodeManager::remove_child(NodeHandle parent_handle, NodeHandle child_handle) {
    Node* parent = get(parent_handle);
    Node* child = get(child_handle);
    if (!parent || !child) return;
    if (child->parent != parent) return;

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
    Transform* parent_world = node->parent ? &node->parent->world : nullptr;

    compute_world_transform(node, parent_world);

    // Recurse to children
    for (int i = 0; i < node->child_count; ++i) {
        NodeHandle child_handle = get_handle(node->children[i]);
        if (child_handle != INVALID_NODE_HANDLE) {
            update_world_transforms(child_handle);
        }
    }
}

bool NodeManager::is_active_in_hierarchy(NodeHandle handle) const {
    auto it = nodes_.find(handle);
    while (it != nodes_.end()) {
        const Node& node = it->second;
        if (!node.active) return false;
        if (!node.parent) return true;

        // Find parent's handle and continue walking up
        NodeHandle parent_handle = get_handle(node.parent);
        if (parent_handle == INVALID_NODE_HANDLE) return true;
        it = nodes_.find(parent_handle);
    }
    return true;
}

void NodeManager::traverse_depth_first(NodeHandle root, TraversalCallback callback, void* user_data) {
    Node* node = get(root);
    if (!node) return;

    callback(node, user_data);

    for (int i = 0; i < node->child_count; ++i) {
        NodeHandle child_handle = get_handle(node->children[i]);
        if (child_handle != INVALID_NODE_HANDLE) {
            traverse_depth_first(child_handle, callback, user_data);
        }
    }
}

void NodeManager::clear() {
    // Free all children arrays
    for (auto& [handle, node] : nodes_) {
        if (node.children) {
            free(node.children);
        }
    }
    nodes_.clear();
    next_id_ = 0;
}

} // namespace gmr
