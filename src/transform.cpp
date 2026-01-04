#include "gmr/transform.hpp"

namespace gmr {

// Matrix2D implementation

Matrix2D Matrix2D::identity() {
    return Matrix2D{};
}

Matrix2D Matrix2D::from_transform(float x, float y, float rotation,
                                   float scale_x, float scale_y,
                                   float origin_x, float origin_y) {
    float cos_r = std::cos(rotation);
    float sin_r = std::sin(rotation);

    Matrix2D m;
    m.a = scale_x * cos_r;
    m.b = -scale_y * sin_r;
    m.c = scale_x * sin_r;
    m.d = scale_y * cos_r;

    // Translation includes origin offset
    // Final position = position - rotated_scaled_origin
    m.tx = x - (origin_x * m.a + origin_y * m.b);
    m.ty = y - (origin_x * m.c + origin_y * m.d);

    return m;
}

Matrix2D Matrix2D::operator*(const Matrix2D& o) const {
    Matrix2D result;
    result.a = a * o.a + b * o.c;
    result.b = a * o.b + b * o.d;
    result.tx = a * o.tx + b * o.ty + tx;
    result.c = c * o.a + d * o.c;
    result.d = c * o.b + d * o.d;
    result.ty = c * o.tx + d * o.ty + ty;
    return result;
}

Vec2 Matrix2D::transform_point(const Vec2& p) const {
    return Vec2{
        a * p.x + b * p.y + tx,
        c * p.x + d * p.y + ty
    };
}

// TransformManager implementation

TransformManager& TransformManager::instance() {
    static TransformManager instance;
    return instance;
}

TransformHandle TransformManager::create() {
    TransformHandle handle = next_id_++;
    transforms_[handle] = Transform2DState{};
    return handle;
}

void TransformManager::destroy(TransformHandle handle) {
    auto it = transforms_.find(handle);
    if (it != transforms_.end()) {
        // Clear parent reference from any children
        for (auto& [h, state] : transforms_) {
            if (state.parent == handle) {
                state.parent = INVALID_HANDLE;
                state.dirty = true;
            }
        }
        transforms_.erase(it);
    }
}

Transform2DState* TransformManager::get(TransformHandle handle) {
    auto it = transforms_.find(handle);
    return (it != transforms_.end()) ? &it->second : nullptr;
}

bool TransformManager::valid(TransformHandle handle) const {
    return transforms_.find(handle) != transforms_.end();
}

const Matrix2D& TransformManager::get_world_matrix(TransformHandle handle) {
    auto* state = get(handle);
    if (!state) {
        static Matrix2D identity = Matrix2D::identity();
        return identity;
    }

    if (state->dirty) {
        recompute_world_matrix(handle);
    }

    return state->world_matrix;
}

void TransformManager::recompute_world_matrix(TransformHandle handle) {
    auto* state = get(handle);
    if (!state) return;

    // Compute local matrix
    Matrix2D local = Matrix2D::from_transform(
        state->position.x, state->position.y,
        state->rotation,
        state->scale.x, state->scale.y,
        state->origin.x, state->origin.y
    );

    // If we have a parent, multiply with parent's world matrix
    if (state->parent != INVALID_HANDLE) {
        const Matrix2D& parent_world = get_world_matrix(state->parent);
        state->world_matrix = parent_world * local;
    } else {
        state->world_matrix = local;
    }

    state->dirty = false;
}

void TransformManager::mark_dirty(TransformHandle handle) {
    auto* state = get(handle);
    if (state && !state->dirty) {
        state->dirty = true;
        // Also mark all children dirty
        mark_children_dirty(handle);
    }
}

void TransformManager::mark_children_dirty(TransformHandle parent) {
    for (auto& [handle, state] : transforms_) {
        if (state.parent == parent) {
            state.dirty = true;
            mark_children_dirty(handle);
        }
    }
}

void TransformManager::set_parent(TransformHandle child, TransformHandle parent) {
    auto* child_state = get(child);
    if (!child_state) return;

    // Avoid circular references
    if (parent != INVALID_HANDLE) {
        TransformHandle current = parent;
        while (current != INVALID_HANDLE) {
            if (current == child) {
                // Would create a cycle, don't allow
                return;
            }
            auto* parent_state = get(current);
            current = parent_state ? parent_state->parent : INVALID_HANDLE;
        }
    }

    child_state->parent = parent;
    child_state->dirty = true;
    mark_children_dirty(child);
}

void TransformManager::clear_parent(TransformHandle child) {
    set_parent(child, INVALID_HANDLE);
}

std::vector<TransformHandle> TransformManager::get_children(TransformHandle parent) const {
    std::vector<TransformHandle> children;
    for (const auto& [handle, state] : transforms_) {
        if (state.parent == parent) {
            children.push_back(handle);
        }
    }
    return children;
}

void TransformManager::clear() {
    transforms_.clear();
    next_id_ = 0;
}

} // namespace gmr
