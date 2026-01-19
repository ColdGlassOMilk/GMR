#include "gmr/resources/tilemap_manager.hpp"

namespace gmr {

TilemapManager& TilemapManager::instance() {
    static TilemapManager instance;
    return instance;
}

TilemapHandle TilemapManager::create(int32_t width, int32_t height, int32_t tile_width, int32_t tile_height, TextureHandle tileset) {
    auto tilemap = std::make_unique<TilemapData>(width, height, tile_width, tile_height, tileset);

    // Reuse a free handle if available
    if (!free_handles_.empty()) {
        TilemapHandle handle = free_handles_.back();
        free_handles_.pop_back();
        tilemaps_[handle] = std::move(tilemap);
        return handle;
    }

    // Otherwise allocate a new slot
    TilemapHandle handle = static_cast<TilemapHandle>(tilemaps_.size());
    tilemaps_.push_back(std::move(tilemap));
    return handle;
}

TilemapData* TilemapManager::get(TilemapHandle handle) {
    if (handle < 0 || handle >= static_cast<TilemapHandle>(tilemaps_.size())) {
        return nullptr;
    }
    return tilemaps_[handle].get();
}

const TilemapData* TilemapManager::get(TilemapHandle handle) const {
    if (handle < 0 || handle >= static_cast<TilemapHandle>(tilemaps_.size())) {
        return nullptr;
    }
    return tilemaps_[handle].get();
}

bool TilemapManager::valid(TilemapHandle handle) const {
    if (handle < 0 || handle >= static_cast<TilemapHandle>(tilemaps_.size())) {
        return false;
    }
    return tilemaps_[handle] != nullptr;
}

void TilemapManager::destroy(TilemapHandle handle) {
    if (handle >= 0 && handle < static_cast<TilemapHandle>(tilemaps_.size())) {
        tilemaps_[handle].reset();
        free_handles_.push_back(handle);
    }
}

void TilemapManager::clear() {
    tilemaps_.clear();
    free_handles_.clear();
}

} // namespace gmr
