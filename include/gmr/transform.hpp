#ifndef GMR_TRANSFORM_HPP
#define GMR_TRANSFORM_HPP

#include "gmr/types.hpp"
#include <unordered_map>
#include <vector>
#include <cmath>

namespace gmr {

// 2D affine transformation matrix (row-major, 3x3)
// [a  b  tx]   [scale_x * cos(r)   -scale_y * sin(r)   tx]
// [c  d  ty] = [scale_x * sin(r)    scale_y * cos(r)   ty]
// [0  0  1 ]   [0                   0                   1 ]
struct Matrix2D {
    float a = 1.0f, b = 0.0f, tx = 0.0f;
    float c = 0.0f, d = 1.0f, ty = 0.0f;

    static Matrix2D identity();
    static Matrix2D from_transform(float x, float y, float rotation,
                                   float scale_x, float scale_y,
                                   float origin_x, float origin_y);

    Matrix2D operator*(const Matrix2D& other) const;
    Vec2 transform_point(const Vec2& point) const;
};

// Transform2D state - represents a 2D transformation
struct Transform2DState {
    Vec2 position{0.0f, 0.0f};
    float rotation{0.0f};         // Radians internally
    Vec2 scale{1.0f, 1.0f};
    Vec2 origin{0.0f, 0.0f};      // Pivot point (local space)

    TransformHandle parent{INVALID_HANDLE};

    // Cached world matrix (lazy evaluation)
    mutable Matrix2D world_matrix;
    mutable bool dirty{true};

    Transform2DState() = default;
};

// Singleton Transform2D manager
class TransformManager {
public:
    static TransformManager& instance();

    // Lifecycle
    TransformHandle create();
    void destroy(TransformHandle handle);
    Transform2DState* get(TransformHandle handle);
    bool valid(TransformHandle handle) const;

    // Matrix computation (lazy evaluation)
    const Matrix2D& get_world_matrix(TransformHandle handle);
    void mark_dirty(TransformHandle handle);

    // Hierarchy management
    void set_parent(TransformHandle child, TransformHandle parent);
    void clear_parent(TransformHandle child);

    // Get all children of a transform
    std::vector<TransformHandle> get_children(TransformHandle parent) const;

    // Clear all (for cleanup/reload)
    void clear();

    // Debug info
    size_t count() const { return transforms_.size(); }

private:
    TransformManager() = default;
    void recompute_world_matrix(TransformHandle handle);
    void mark_children_dirty(TransformHandle parent);

    std::unordered_map<TransformHandle, Transform2DState> transforms_;
    TransformHandle next_id_{0};
};

} // namespace gmr

#endif
