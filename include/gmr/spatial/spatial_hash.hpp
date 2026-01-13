#ifndef GMR_SPATIAL_HASH_HPP
#define GMR_SPATIAL_HASH_HPP

#include <mruby.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <cmath>

namespace gmr {
namespace spatial {

// Spatial hash for efficient 2D queries
// Uses a grid-based approach where each cell contains references to entities
class SpatialHash {
public:
    static SpatialHash& instance();

    // Configuration
    void set_cell_size(float size);
    float cell_size() const { return cell_size_; }

    // Entity management
    // Entities are identified by their Ruby object value (mrb_value)
    void add(mrb_state* mrb, mrb_value entity, float x, float y, float w, float h);
    void update(mrb_state* mrb, mrb_value entity, float x, float y, float w, float h);
    void remove(mrb_state* mrb, mrb_value entity);

    // Queries - return Ruby arrays of entities
    mrb_value query_rect(mrb_state* mrb, float x, float y, float w, float h);
    mrb_value query_circle(mrb_state* mrb, float cx, float cy, float radius);
    mrb_value query_point(mrb_state* mrb, float x, float y);

    // Convenience queries
    mrb_value query_nearest(mrb_state* mrb, float x, float y, float max_distance);

    // Cleanup
    void clear(mrb_state* mrb);

    // Debug
    size_t entity_count() const { return entities_.size(); }
    size_t cell_count() const { return cells_.size(); }

    SpatialHash(const SpatialHash&) = delete;
    SpatialHash& operator=(const SpatialHash&) = delete;

private:
    SpatialHash() = default;

    // Cell key from world coordinates
    int64_t cell_key(int cx, int cy) const {
        // Pack two 32-bit ints into one 64-bit key
        return (static_cast<int64_t>(cx) << 32) | (static_cast<uint32_t>(cy));
    }

    // World to cell coordinates
    int world_to_cell(float coord) const {
        return static_cast<int>(std::floor(coord / cell_size_));
    }

    // Get cells that an AABB overlaps
    void get_overlapping_cells(float x, float y, float w, float h,
                               int& min_cx, int& min_cy, int& max_cx, int& max_cy) const;

    // Entity data
    struct EntityData {
        mrb_value entity;
        float x, y, w, h;  // Bounding box
        std::vector<int64_t> cells;  // Cells this entity occupies
    };

    // Map from Ruby object ID to entity data
    // We use mrb_obj_id() as the key for stable identity
    std::unordered_map<uint64_t, EntityData> entities_;

    // Map from cell key to set of entity IDs in that cell
    std::unordered_map<int64_t, std::unordered_set<uint64_t>> cells_;

    // Default cell size - should be roughly the size of typical entities
    float cell_size_ = 64.0f;
};

} // namespace spatial
} // namespace gmr

#endif // GMR_SPATIAL_HASH_HPP
