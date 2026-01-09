#include "gmr/filesystem/operations.hpp"

#include <filesystem>
#include "raylib.h"

#ifdef PLATFORM_WEB
#include <emscripten.h>
#endif

namespace fs = std::filesystem;

namespace gmr {
namespace filesystem {

std::optional<std::string> read_text(const std::string& path, Root root) {
    std::string resolved = resolve_path(path, root);

    // Use raylib's cross-platform LoadFileText
    char* text = LoadFileText(resolved.c_str());
    if (!text) {
        return std::nullopt;
    }

    std::string content(text);
    UnloadFileText(text);
    return content;
}

std::optional<std::vector<uint8_t>> read_bytes(const std::string& path, Root root) {
    std::string resolved = resolve_path(path, root);

    // Use raylib's cross-platform LoadFileData
    int size = 0;
    unsigned char* data = LoadFileData(resolved.c_str(), &size);
    if (!data || size < 0) {
        if (data) UnloadFileData(data);
        return std::nullopt;
    }

    std::vector<uint8_t> result(data, data + size);
    UnloadFileData(data);
    return result;
}

bool write_text(const std::string& path, const std::string& content, Root root) {
    // Reject writes to assets root
    if (root == Root::Assets) {
        return false;
    }

    std::string resolved = resolve_path(path, root);

    // Ensure parent directory exists
    if (!ensure_directory(resolved)) {
        return false;
    }

    // Use raylib's cross-platform SaveFileText
    bool success = SaveFileText(resolved.c_str(), content.c_str());

#ifdef PLATFORM_WEB
    if (success) {
        // Auto-sync to IndexedDB for persistence
        EM_ASM({
            FS.syncfs(false, function(err) {
                if (err) console.error('FS.syncfs failed:', err);
            });
        });
    }
#endif

    return success;
}

bool write_bytes(const std::string& path, const std::vector<uint8_t>& data, Root root) {
    // Reject writes to assets root
    if (root == Root::Assets) {
        return false;
    }

    std::string resolved = resolve_path(path, root);

    // Ensure parent directory exists
    if (!ensure_directory(resolved)) {
        return false;
    }

    // Use raylib's cross-platform SaveFileData
    bool success = SaveFileData(resolved.c_str(),
                                const_cast<void*>(reinterpret_cast<const void*>(data.data())),
                                static_cast<int>(data.size()));

#ifdef PLATFORM_WEB
    if (success) {
        // Auto-sync to IndexedDB for persistence
        EM_ASM({
            FS.syncfs(false, function(err) {
                if (err) console.error('FS.syncfs failed:', err);
            });
        });
    }
#endif

    return success;
}

bool exists(const std::string& path, Root root) {
    std::string resolved = resolve_path(path, root);

    // Use raylib's cross-platform FileExists
    return FileExists(resolved.c_str());
}

std::vector<std::string> list_files(const std::string& directory, Root root) {
#ifdef PLATFORM_WEB
    // Web: Not supported, return empty vector
    return {};
#else
    // Native: Use C++17 filesystem
    std::vector<std::string> files;

    try {
        std::string resolved = resolve_path(directory, root);

        if (!fs::exists(resolved) || !fs::is_directory(resolved)) {
            return files;
        }

        for (const auto& entry : fs::directory_iterator(resolved)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (...) {
        // Return empty vector on error
        return files;
    }

    return files;
#endif
}

}  // namespace filesystem
}  // namespace gmr
