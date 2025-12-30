#ifndef GMR_TEXTURE_MANAGER_HPP
#define GMR_TEXTURE_MANAGER_HPP

#include "gmr/resources/resource_manager.hpp"
#include "raylib.h"

namespace gmr {

class TextureManager : public ResourceManager<TextureHandle, Texture2D> {
public:
    static TextureManager& instance();
    
    int get_width(TextureHandle handle) const;
    int get_height(TextureHandle handle) const;
    
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    
protected:
    std::optional<Texture2D> load_resource(const std::string& path) override;
    void unload_resource(Texture2D& texture) override;
    
private:
    TextureManager() = default;
};

} // namespace gmr

#endif
