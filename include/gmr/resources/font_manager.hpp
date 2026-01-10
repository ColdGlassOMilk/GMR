#ifndef GMR_FONT_MANAGER_HPP
#define GMR_FONT_MANAGER_HPP

#include "gmr/resources/resource_manager.hpp"
#include "raylib.h"

namespace gmr {

class FontManager : public ResourceManager<FontHandle, Font> {
public:
    static FontManager& instance();

    // Load with specific size (fonts are size-specific in raylib)
    FontHandle load(const std::string& path, int font_size);

    int get_base_size(FontHandle handle) const;
    int get_glyph_count(FontHandle handle) const;

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

protected:
    // Override base load_resource - called by parent's load() with cache_key
    std::optional<Font> load_resource(const std::string& cache_key) override;
    void unload_resource(Font& font) override;

private:
    FontManager() = default;
    int current_load_size_ = 32;  // Temporary storage for load_resource callback
};

} // namespace gmr

#endif
