#include "gmr/resources/texture_manager.hpp"
#include "gmr/paths.hpp"
#include "raylib.h"

namespace gmr {

TextureManager& TextureManager::instance() {
    static TextureManager instance;
    return instance;
}

std::optional<Texture2D> TextureManager::load_resource(const std::string& path) {
    std::string resolved = resolve_asset_path(path);
    Texture2D texture = LoadTexture(resolved.c_str());
    if (texture.id == 0) {
        TraceLog(LOG_ERROR, "TEXTURE: Failed to load texture: %s (resolved: %s)",
                 path.c_str(), resolved.c_str());
        return std::nullopt;
    }
    TraceLog(LOG_DEBUG, "TEXTURE: Loaded %s - id=%u, size=%dx%d, format=%d, mipmaps=%d",
             path.c_str(), texture.id, texture.width, texture.height,
             texture.format, texture.mipmaps);
    // Use point filtering by default to prevent bleeding between pixels/tiles
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
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
