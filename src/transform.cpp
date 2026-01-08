#include "gmr/transform.hpp"
#include <algorithm>

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

bool Matrix2D::is_invertible() const {
    // Calculate determinant
    float det = a * d - b * c;
    // Matrix is invertible if determinant is non-zero
    return std::abs(det) > 1e-6f;
}

Matrix2D Matrix2D::inverse() const {
    float det = a * d - b * c;

    // Handle degenerate case
    if (std::abs(det) < 1e-6f) {
        return Matrix2D::identity();
    }

    float inv_det = 1.0f / det;

    Matrix2D result;
    result.a = d * inv_det;
    result.b = -b * inv_det;
    result.c = -c * inv_det;
    result.d = a * inv_det;
    result.tx = (b * ty - d * tx) * inv_det;
    result.ty = (c * tx - a * ty) * inv_det;

    return result;
}

Vec2 Matrix2D::transform_inverse_point(const Vec2& p) const {
    return inverse().transform_point(p);
}

Vec2 Matrix2D::transform_direction(const Vec2& dir) const {
    // Transform direction without translation
    return Vec2{
        a * dir.x + b * dir.y,
        c * dir.x + d * dir.y
    };
}

Vec2 Matrix2D::transform_inverse_direction(const Vec2& dir) const {
    return inverse().transform_direction(dir);
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
        // Remove from parent's children list
        if (it->second.parent != INVALID_HANDLE) {
            auto parent_it = children_.find(it->second.parent);
            if (parent_it != children_.end()) {
                auto& siblings = parent_it->second;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), handle), siblings.end());
                if (siblings.empty()) {
                    children_.erase(parent_it);
                }
            }
        }

        // Clear parent reference from any children using fast lookup
        auto children_it = children_.find(handle);
        if (children_it != children_.end()) {
            for (TransformHandle child : children_it->second) {
                auto* child_state = get(child);
                if (child_state) {
                    child_state->parent = INVALID_HANDLE;
                    child_state->dirty = true;
                }
            }
            children_.erase(children_it);
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

    // Cache decomposed world values from the matrix
    // Extract world position from translation component
    state->cached_world_position.x = state->world_matrix.tx;
    state->cached_world_position.y = state->world_matrix.ty;

    // Extract world rotation using atan2
    state->cached_world_rotation = std::atan2(state->world_matrix.c, state->world_matrix.a);

    // Extract world scale (length of transformed basis vectors)
    state->cached_world_scale.x = std::sqrt(
        state->world_matrix.a * state->world_matrix.a +
        state->world_matrix.c * state->world_matrix.c
    );
    state->cached_world_scale.y = std::sqrt(
        state->world_matrix.b * state->world_matrix.b +
        state->world_matrix.d * state->world_matrix.d
    );

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
    // Fast O(1) lookup instead of O(n) scan
    auto it = children_.find(parent);
    if (it != children_.end()) {
        for (TransformHandle child : it->second) {
            auto* state = get(child);
            if (state && !state->dirty) {  // Short-circuit if already dirty
                state->dirty = true;
                mark_children_dirty(child);  // Recursive for grandchildren
            }
        }
    }
}

void TransformManager::set_parent(TransformHandle child, TransformHandle parent) {
    auto* child_state = get(child);
    if (!child_state) return;

    // Remove from old parent's children list
    if (child_state->parent != INVALID_HANDLE) {
        auto it = children_.find(child_state->parent);
        if (it != children_.end()) {
            auto& siblings = it->second;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
            if (siblings.empty()) {
                children_.erase(it);  // Clean up empty vectors
            }
        }
    }

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

    // Set new parent
    child_state->parent = parent;

    // Add to new parent's children list
    if (parent != INVALID_HANDLE) {
        children_[parent].push_back(child);
    }

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
    children_.clear();
    next_id_ = 0;
}

// Cached world transform accessors
Vec2 TransformManager::get_world_position(TransformHandle handle) {
    // Ensure world matrix is up to date (triggers cache update)
    get_world_matrix(handle);
    auto* state = get(handle);
    return state ? state->cached_world_position : Vec2{0.0f, 0.0f};
}

float TransformManager::get_world_rotation(TransformHandle handle) {
    // Ensure world matrix is up to date (triggers cache update)
    get_world_matrix(handle);
    auto* state = get(handle);
    return state ? state->cached_world_rotation : 0.0f;
}

Vec2 TransformManager::get_world_scale(TransformHandle handle) {
    // Ensure world matrix is up to date (triggers cache update)
    get_world_matrix(handle);
    auto* state = get(handle);
    return state ? state->cached_world_scale : Vec2{1.0f, 1.0f};
}

// Transform direction queries
Vec2 TransformManager::get_world_forward(TransformHandle handle) {
    const Matrix2D& mat = get_world_matrix(handle);
    // Forward vector is the first column of rotation (after removing scale)
    auto* state = get(handle);
    if (!state) return Vec2{1.0f, 0.0f};

    float cos_r = std::cos(state->cached_world_rotation);
    float sin_r = std::sin(state->cached_world_rotation);
    return Vec2{cos_r, sin_r};
}

Vec2 TransformManager::get_world_right(TransformHandle handle) {
    const Matrix2D& mat = get_world_matrix(handle);
    // Right vector is perpendicular to forward
    auto* state = get(handle);
    if (!state) return Vec2{0.0f, 1.0f};

    float cos_r = std::cos(state->cached_world_rotation);
    float sin_r = std::sin(state->cached_world_rotation);
    return Vec2{-sin_r, cos_r};
}

// Utility functions
void TransformManager::snap_position_to_grid(TransformHandle handle, float grid_size) {
    auto* state = get(handle);
    if (!state || grid_size <= 0.0f) return;

    state->position.x = std::round(state->position.x / grid_size) * grid_size;
    state->position.y = std::round(state->position.y / grid_size) * grid_size;
    mark_dirty(handle);
}

void TransformManager::round_position_to_pixel(TransformHandle handle) {
    auto* state = get(handle);
    if (!state) return;

    state->position.x = std::round(state->position.x);
    state->position.y = std::round(state->position.y);
    mark_dirty(handle);
}

// Interpolation utilities
Vec2 TransformManager::lerp_position(const Vec2& a, const Vec2& b, float t) {
    return Vec2{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}

float TransformManager::lerp_rotation(float a, float b, float t) {
    // Shortest path rotation interpolation
    float diff = b - a;

    // Wrap diff to [-PI, PI]
    while (diff > 3.14159265358979323846f) diff -= 2.0f * 3.14159265358979323846f;
    while (diff < -3.14159265358979323846f) diff += 2.0f * 3.14159265358979323846f;

    return a + diff * t;
}

Vec2 TransformManager::lerp_scale(const Vec2& a, const Vec2& b, float t) {
    return Vec2{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}

} // namespace gmr
