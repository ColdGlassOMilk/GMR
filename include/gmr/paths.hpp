#ifndef GMR_PATHS_HPP
#define GMR_PATHS_HPP

#include <string>

namespace gmr {

// Resolve an asset path to the correct location based on platform
// Scripts use paths relative to the game folder: "assets/logo.png"
// Native builds prepend "game/": "game/assets/logo.png"
// Web builds use the path as-is (assets mounted at /assets)
inline std::string resolve_asset_path(const std::string& path) {
#ifdef PLATFORM_WEB
    // Web: assets are preloaded at their relative paths
    return path;
#else
    // Native: assets are in the game/ subfolder
    // Skip if path already starts with "game/"
    if (path.rfind("game/", 0) == 0 || path.rfind("game\\", 0) == 0) {
        return path;
    }
    return "game/" + path;
#endif
}

} // namespace gmr

#endif
