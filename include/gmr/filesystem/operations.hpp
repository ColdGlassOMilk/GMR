#pragma once

#include "gmr/filesystem/paths.hpp"

#include <optional>
#include <string>
#include <vector>
#include <cstdint>

namespace gmr {
namespace filesystem {

// === Read Operations ===

// Read entire file as text string
// Returns std::nullopt if file doesn't exist or read fails
std::optional<std::string> read_text(const std::string& path, Root root);

// Read entire file as byte array
// Returns std::nullopt if file doesn't exist or read fails
std::optional<std::vector<uint8_t>> read_bytes(const std::string& path, Root root);

// === Write Operations ===

// Write text string to file (overwrites existing file)
// Returns true on success, false on failure
bool write_text(const std::string& path, const std::string& content, Root root);

// Write byte array to file (overwrites existing file)
// Returns true on success, false on failure
bool write_bytes(const std::string& path, const std::vector<uint8_t>& data, Root root);

// === Query Operations ===

// Check if file exists
bool exists(const std::string& path, Root root);

// List all files in a directory (non-recursive)
// Returns empty vector on failure or if directory doesn't exist
// Web: Always returns empty vector (not supported)
std::vector<std::string> list_files(const std::string& directory, Root root);

}  // namespace filesystem
}  // namespace gmr
