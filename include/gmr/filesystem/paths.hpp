#pragma once

#include <string>

namespace gmr {
namespace filesystem {

// Logical root directories for file access
enum class Root {
    Assets,  // Read-only game assets (game/assets/ or /assets/)
    Data     // Writable runtime storage (game/data/ or /data/)
};

// Convert logical root + relative path to platform-specific absolute path
// Native: prepends "game/assets/" or "game/data/"
// Web: prepends "/assets/" or "/data/"
std::string resolve_path(const std::string& path, Root root);

// Validate that path is safe (no directory traversal, no absolute paths)
// Returns true if path is valid, false otherwise
bool is_valid_path(const std::string& path);

// Ensure directory exists (creates if needed on native, no-op on web)
// Returns true if directory exists or was created, false on failure
bool ensure_directory(const std::string& path);

}  // namespace filesystem
}  // namespace gmr
