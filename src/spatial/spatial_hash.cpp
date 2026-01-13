#include "gmr/spatial/spatial_hash.hpp"
#include <mruby/array.h>
#include <mruby/object.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace gmr {
namespace spatial {

SpatialHash& SpatialHash::instance() {
    static SpatialHash instance;
    return instance;
}

void SpatialHash::set_cell_size(float size) {
    if (size > 0.0f) {
        cell_size_ = size;
    }
}

void SpatialHash::get_overlapping_cells(float x, float y, float w, float h,
                                         int& min_cx, int& min_cy, int& max_cx, int& max_cy) const {
    min_cx = world_to_cell(x);
    min_cy = world_to_cell(y);
    max_cx = world_to_cell(x + w);
    max_cy = world_to_cell(y + h);
}

void SpatialHash::add(mrb_state* mrb, mrb_value entity, float x, float y, float w, float h) {
    uint64_t id = mrb_obj_id(entity);

    // Remove if already exists
    if (entities_.find(id) != entities_.end()) {
        remove(mrb, entity);
    }

    // Create entity data
    EntityData data;
    data.entity = entity;
    data.x = x;
    data.y = y;
    data.w = w;
    data.h = h;

    // Find overlapping cells
    int min_cx, min_cy, max_cx, max_cy;
    get_overlapping_cells(x, y, w, h, min_cx, min_cy, max_cx, max_cy);

    // Add to cells
    for (int cx = min_cx; cx <= max_cx; ++cx) {
        for (int cy = min_cy; cy <= max_cy; ++cy) {
            int64_t key = cell_key(cx, cy);
            cells_[key].insert(id);
            data.cells.push_back(key);
        }
    }

    entities_[id] = data;

    // Register with GC to keep Ruby object alive
    mrb_gc_register(mrb, entity);
}

void SpatialHash::update(mrb_state* mrb, mrb_value entity, float x, float y, float w, float h) {
    uint64_t id = mrb_obj_id(entity);

    auto it = entities_.find(id);
    if (it == entities_.end()) {
        // Not in hash, add it
        add(mrb, entity, x, y, w, h);
        return;
    }

    EntityData& data = it->second;

    // Check if cells changed
    int old_min_cx, old_min_cy, old_max_cx, old_max_cy;
    get_overlapping_cells(data.x, data.y, data.w, data.h, old_min_cx, old_min_cy, old_max_cx, old_max_cy);

    int new_min_cx, new_min_cy, new_max_cx, new_max_cy;
    get_overlapping_cells(x, y, w, h, new_min_cx, new_min_cy, new_max_cx, new_max_cy);

    // If cells are the same, just update bounds
    if (old_min_cx == new_min_cx && old_min_cy == new_min_cy &&
        old_max_cx == new_max_cx && old_max_cy == new_max_cy) {
        data.x = x;
        data.y = y;
        data.w = w;
        data.h = h;
        return;
    }

    // Cells changed - remove from old cells
    for (int64_t key : data.cells) {
        auto cell_it = cells_.find(key);
        if (cell_it != cells_.end()) {
            cell_it->second.erase(id);
            if (cell_it->second.empty()) {
                cells_.erase(cell_it);
            }
        }
    }
    data.cells.clear();

    // Add to new cells
    for (int cx = new_min_cx; cx <= new_max_cx; ++cx) {
        for (int cy = new_min_cy; cy <= new_max_cy; ++cy) {
            int64_t key = cell_key(cx, cy);
            cells_[key].insert(id);
            data.cells.push_back(key);
        }
    }

    // Update bounds
    data.x = x;
    data.y = y;
    data.w = w;
    data.h = h;
}

void SpatialHash::remove(mrb_state* mrb, mrb_value entity) {
    uint64_t id = mrb_obj_id(entity);

    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return;
    }

    EntityData& data = it->second;

    // Remove from all cells
    for (int64_t key : data.cells) {
        auto cell_it = cells_.find(key);
        if (cell_it != cells_.end()) {
            cell_it->second.erase(id);
            if (cell_it->second.empty()) {
                cells_.erase(cell_it);
            }
        }
    }

    // Unregister from GC
    mrb_gc_unregister(mrb, data.entity);

    entities_.erase(it);
}

mrb_value SpatialHash::query_rect(mrb_state* mrb, float x, float y, float w, float h) {
    mrb_value result = mrb_ary_new(mrb);

    // Track which entities we've already added (to handle multi-cell entities)
    std::unordered_set<uint64_t> seen;

    int min_cx, min_cy, max_cx, max_cy;
    get_overlapping_cells(x, y, w, h, min_cx, min_cy, max_cx, max_cy);

    for (int cx = min_cx; cx <= max_cx; ++cx) {
        for (int cy = min_cy; cy <= max_cy; ++cy) {
            int64_t key = cell_key(cx, cy);
            auto cell_it = cells_.find(key);
            if (cell_it == cells_.end()) continue;

            for (uint64_t id : cell_it->second) {
                if (seen.count(id)) continue;
                seen.insert(id);

                auto entity_it = entities_.find(id);
                if (entity_it == entities_.end()) continue;

                const EntityData& data = entity_it->second;

                // AABB intersection test
                if (data.x < x + w && data.x + data.w > x &&
                    data.y < y + h && data.y + data.h > y) {
                    mrb_ary_push(mrb, result, data.entity);
                }
            }
        }
    }

    return result;
}

mrb_value SpatialHash::query_circle(mrb_state* mrb, float cx, float cy, float radius) {
    mrb_value result = mrb_ary_new(mrb);

    // First query the bounding rect of the circle
    float x = cx - radius;
    float y = cy - radius;
    float size = radius * 2.0f;

    std::unordered_set<uint64_t> seen;

    int min_cx_cell, min_cy_cell, max_cx_cell, max_cy_cell;
    get_overlapping_cells(x, y, size, size, min_cx_cell, min_cy_cell, max_cx_cell, max_cy_cell);

    float radius_sq = radius * radius;

    for (int cell_x = min_cx_cell; cell_x <= max_cx_cell; ++cell_x) {
        for (int cell_y = min_cy_cell; cell_y <= max_cy_cell; ++cell_y) {
            int64_t key = cell_key(cell_x, cell_y);
            auto cell_it = cells_.find(key);
            if (cell_it == cells_.end()) continue;

            for (uint64_t id : cell_it->second) {
                if (seen.count(id)) continue;
                seen.insert(id);

                auto entity_it = entities_.find(id);
                if (entity_it == entities_.end()) continue;

                const EntityData& data = entity_it->second;

                // Find closest point on AABB to circle center
                float closest_x = std::max(data.x, std::min(cx, data.x + data.w));
                float closest_y = std::max(data.y, std::min(cy, data.y + data.h));

                float dx = cx - closest_x;
                float dy = cy - closest_y;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq <= radius_sq) {
                    mrb_ary_push(mrb, result, data.entity);
                }
            }
        }
    }

    return result;
}

mrb_value SpatialHash::query_point(mrb_state* mrb, float px, float py) {
    mrb_value result = mrb_ary_new(mrb);

    int cell_x = world_to_cell(px);
    int cell_y = world_to_cell(py);
    int64_t key = cell_key(cell_x, cell_y);

    auto cell_it = cells_.find(key);
    if (cell_it == cells_.end()) {
        return result;
    }

    for (uint64_t id : cell_it->second) {
        auto entity_it = entities_.find(id);
        if (entity_it == entities_.end()) continue;

        const EntityData& data = entity_it->second;

        // Point in AABB test
        if (px >= data.x && px < data.x + data.w &&
            py >= data.y && py < data.y + data.h) {
            mrb_ary_push(mrb, result, data.entity);
        }
    }

    return result;
}

mrb_value SpatialHash::query_nearest(mrb_state* mrb, float x, float y, float max_distance) {
    float best_dist_sq = max_distance * max_distance;
    mrb_value best_entity = mrb_nil_value();

    // Query in expanding rings from center
    int center_cx = world_to_cell(x);
    int center_cy = world_to_cell(y);

    // Calculate how many cells the max distance covers
    int cell_range = static_cast<int>(std::ceil(max_distance / cell_size_)) + 1;

    std::unordered_set<uint64_t> seen;

    for (int dx = -cell_range; dx <= cell_range; ++dx) {
        for (int dy = -cell_range; dy <= cell_range; ++dy) {
            int64_t key = cell_key(center_cx + dx, center_cy + dy);
            auto cell_it = cells_.find(key);
            if (cell_it == cells_.end()) continue;

            for (uint64_t id : cell_it->second) {
                if (seen.count(id)) continue;
                seen.insert(id);

                auto entity_it = entities_.find(id);
                if (entity_it == entities_.end()) continue;

                const EntityData& data = entity_it->second;

                // Distance to center of AABB
                float entity_cx = data.x + data.w * 0.5f;
                float entity_cy = data.y + data.h * 0.5f;
                float ddx = entity_cx - x;
                float ddy = entity_cy - y;
                float dist_sq = ddx * ddx + ddy * ddy;

                if (dist_sq < best_dist_sq) {
                    best_dist_sq = dist_sq;
                    best_entity = data.entity;
                }
            }
        }
    }

    return best_entity;
}

void SpatialHash::clear(mrb_state* mrb) {
    // Unregister all entities from GC
    for (auto& [id, data] : entities_) {
        mrb_gc_unregister(mrb, data.entity);
    }

    entities_.clear();
    cells_.clear();
}

} // namespace spatial
} // namespace gmr
