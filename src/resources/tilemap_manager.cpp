#include "gmr/resources/tilemap_manager.hpp"

namespace gmr {

TilemapManager& TilemapManager::instance() {
    static TilemapManager instance;
    return instance;
}

TilemapHandle TilemapManager::create(int32_t width, int32_t height, int32_t tile_width, int32_t tile_height, TextureHandle tileset) {
    TilemapData* tilemap = new TilemapData(width, height, tile_width, tile_height, tileset);

    // Reuse a free handle if available
    if (!free_handles_.empty()) {
        TilemapHandle handle = free_handles_.back();
        free_handles_.pop_back();
        tilemaps_[handle] = tilemap;
        return handle;
    }

    // Otherwise allocate a new slot
    TilemapHandle handle = static_cast<TilemapHandle>(tilemaps_.size());
    tilemaps_.push_back(tilemap);
    return handle;
}

TilemapData* TilemapManager::get(TilemapHandle handle) {
    if (handle < 0 || handle >= static_cast<TilemapHandle>(tilemaps_.size())) {
        return nullptr;
    }
    return tilemaps_[handle];
}

const TilemapData* TilemapManager::get(TilemapHandle handle) const {
    if (handle < 0 || handle >= static_cast<TilemapHandle>(tilemaps_.size())) {
        return nullptr;
    }
    return tilemaps_[handle];
}

bool TilemapManager::valid(TilemapHandle handle) const {
    if (handle < 0 || handle >= static_cast<TilemapHandle>(tilemaps_.size())) {
        return false;
    }
    return tilemaps_[handle] != nullptr;
}

void TilemapManager::destroy(TilemapHandle handle) {
    if (handle >= 0 && handle < static_cast<TilemapHandle>(tilemaps_.size())) {
        delete tilemaps_[handle];
        tilemaps_[handle] = nullptr;
        free_handles_.push_back(handle);
    }
}

void TilemapManager::clear() {
    for (auto* tilemap : tilemaps_) {
        delete tilemap;
    }
    tilemaps_.clear();
    free_handles_.clear();
}

} // namespace gmr
