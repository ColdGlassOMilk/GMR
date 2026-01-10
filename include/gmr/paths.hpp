#ifndef GMR_PATHS_HPP
#define GMR_PATHS_HPP

#include <string>

namespace gmr {

// Resolve an asset path to the correct location based on platform
// Scripts use paths relative to the assets folder: "images/player.png"
// Native builds resolve to: "game/assets/images/player.png"
// Web builds resolve to: "/assets/images/player.png"
inline std::string resolve_asset_path(const std::string& path) {
    // Skip if path already has assets/ prefix (backwards compatibility)
    bool has_assets_prefix = (path.rfind("assets/", 0) == 0 || path.rfind("assets\\", 0) == 0);

#ifdef PLATFORM_WEB
    // Web: assets are preloaded at /assets/
    if (has_assets_prefix) {
        // "assets/foo.png" -> "/assets/foo.png"
        return "/" + path;
    }
    // "foo.png" -> "/assets/foo.png"
    return "/assets/" + path;
#else
    // Native: assets are in game/assets/
    // Skip if path already starts with "game/"
    if (path.rfind("game/", 0) == 0 || path.rfind("game\\", 0) == 0) {
        return path;
    }
    if (has_assets_prefix) {
        // "assets/foo.png" -> "game/assets/foo.png"
        return "game/" + path;
    }
    // "foo.png" -> "game/assets/foo.png"
    return "game/assets/" + path;
#endif
}

} // namespace gmr

#endif
