#include "gmr/resources/texture_manager.hpp"

namespace gmr {

TextureManager& TextureManager::instance() {
    static TextureManager instance;
    return instance;
}

std::optional<Texture2D> TextureManager::load_resource(const std::string& path) {
    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        return std::nullopt;
    }
    return texture;
}

void TextureManager::unload_resource(Texture2D& texture) {
    UnloadTexture(texture);
}

int TextureManager::get_width(TextureHandle handle) const {
    if (auto* tex = get(handle)) {
        return tex->width;
    }
    return 0;
}

int TextureManager::get_height(TextureHandle handle) const {
    if (auto* tex = get(handle)) {
        return tex->height;
    }
    return 0;
}

} // namespace gmr
