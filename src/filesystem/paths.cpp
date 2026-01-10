#include "gmr/filesystem/paths.hpp"

#include <cstring>
#include <filesystem>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

namespace fs = std::filesystem;

namespace gmr {
namespace filesystem {

std::string resolve_path(const std::string& path, Root root) {
#ifdef PLATFORM_WEB
    // Web: /assets/ (preloaded read-only) or /data/ (IDBFS writable)
    switch (root) {
        case Root::Assets:
            return "/assets/" + path;
        case Root::Data:
            return "/data/" + path;
        default:
            return "/" + path;
    }
#else
    // Native: game/assets/ (read-only content) or game/data/ (writable saves)
    switch (root) {
        case Root::Assets:
            return "game/assets/" + path;
        case Root::Data:
            return "game/data/" + path;
        default:
            return "game/" + path;
    }
#endif
}

bool is_valid_path(const std::string& path) {
    // Reject empty paths
    if (path.empty()) {
        return false;
    }

    // Reject absolute paths (Unix: starts with /, Windows: starts with drive letter)
    if (path[0] == '/' || path[0] == '\\') {
        return false;
    }

    // Reject Windows drive letters (C:, D:, etc.)
    if (path.find(':') != std::string::npos) {
        return false;
    }

    // Reject directory traversal attempts
    if (path.find("..") != std::string::npos) {
        return false;
    }

    // Reject platform-specific invalid characters
#ifdef _WIN32
    // Windows disallows: < > : " | ? *
    const char* invalid_chars = "<>:\"|?*";
    for (char c : path) {
        if (strchr(invalid_chars, c)) {
            return false;
        }
    }
#endif

    return true;
}

bool ensure_directory(const std::string& path) {
#ifdef PLATFORM_WEB
    // Web: Use Emscripten FS API to create directories
    // Extract parent directory from path
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos) {
        // No directory in path, just a filename
        return true;
    }

    std::string dir_path = path.substr(0, last_slash);

    // Create directories recursively using Emscripten FS
    // We need to create each level of the directory hierarchy
    std::string current_path;
    size_t pos = 0;

    while (pos < dir_path.length()) {
        size_t next_slash = dir_path.find('/', pos);
        if (next_slash == std::string::npos) {
            next_slash = dir_path.length();
        }

        current_path = dir_path.substr(0, next_slash);

        // Try to create directory (ignore errors if it already exists)
        EM_ASM({
            const path = UTF8ToString($0);
            try {
                FS.mkdir(path);
            } catch (e) {
                if (e.code !== 'EEXIST') {
                    console.error('Failed to create directory:', path, e);
                }
            }
        }, current_path.c_str());

        pos = next_slash + 1;
    }

    return true;
#else
    // Native: Create directory if it doesn't exist
    try {
        fs::path dir_path(path);

        // If path has a filename component, get the parent directory
        if (dir_path.has_filename()) {
            dir_path = dir_path.parent_path();
        }

        // If directory already exists, return success
        if (fs::exists(dir_path)) {
            return fs::is_directory(dir_path);
        }

        // Create directory (including parent directories)
        return fs::create_directories(dir_path);
    } catch (...) {
        return false;
    }
#endif
}

}  // namespace filesystem
}  // namespace gmr
