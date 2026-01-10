#include "gmr/resources/font_manager.hpp"
#include "gmr/paths.hpp"

namespace gmr {

FontManager& FontManager::instance() {
    static FontManager instance;
    return instance;
}

FontHandle FontManager::load(const std::string& path, int font_size) {
    // Create unique key combining path and size (fonts are size-specific)
    std::string cache_key = path + ":" + std::to_string(font_size);

    // Store size for load_resource callback
    current_load_size_ = font_size;

    // Use parent's load with the combined key for caching
    return ResourceManager::load(cache_key);
}

std::optional<Font> FontManager::load_resource(const std::string& cache_key) {
    // Extract actual path (everything before the last colon)
    size_t colon_pos = cache_key.rfind(':');
    if (colon_pos == std::string::npos) {
        return std::nullopt;
    }
    std::string path = cache_key.substr(0, colon_pos);

    std::string resolved = resolve_asset_path(path);

    // LoadFontEx: path, fontSize, codepoints (NULL = default ASCII), codepointCount (0 = default 95 chars)
    Font font = LoadFontEx(resolved.c_str(), current_load_size_, NULL, 0);

    if (font.texture.id == 0) {
        return std::nullopt;
    }

    // Use bilinear filtering for smooth text rendering at various sizes
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    return font;
}

void FontManager::unload_resource(Font& font) {
    UnloadFont(font);
}

int FontManager::get_base_size(FontHandle handle) const {
    if (auto* font = get(handle)) {
        return font->baseSize;
    }
    return 0;
}

int FontManager::get_glyph_count(FontHandle handle) const {
    if (auto* font = get(handle)) {
        return font->glyphCount;
    }
    return 0;
}

} // namespace gmr
