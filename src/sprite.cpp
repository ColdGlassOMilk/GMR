#include "gmr/sprite.hpp"

namespace gmr {

SpriteManager& SpriteManager::instance() {
    static SpriteManager instance;
    return instance;
}

SpriteHandle SpriteManager::create() {
    SpriteHandle handle = next_id_++;
    sprites_[handle] = SpriteState{};
    return handle;
}

void SpriteManager::destroy(SpriteHandle handle) {
    sprites_.erase(handle);
}

SpriteState* SpriteManager::get(SpriteHandle handle) {
    auto it = sprites_.find(handle);
    return (it != sprites_.end()) ? &it->second : nullptr;
}

bool SpriteManager::valid(SpriteHandle handle) const {
    return sprites_.find(handle) != sprites_.end();
}

void SpriteManager::clear() {
    sprites_.clear();
    next_id_ = 0;
}

} // namespace gmr
