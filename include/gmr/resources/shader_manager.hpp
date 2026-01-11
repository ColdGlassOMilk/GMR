#ifndef GMR_SHADER_MANAGER_HPP
#define GMR_SHADER_MANAGER_HPP

#include "gmr/types.hpp"
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <cstdint>

namespace gmr {

// Shader capability flags for classification
enum class ShaderFlags : uint32_t {
    NONE = 0,
    SURFACE_CONTINUITY = 1 << 0,  // Shader requires unified UV space (wave, CRT, distortion)
};

// Bitwise operators for ShaderFlags
inline ShaderFlags operator|(ShaderFlags a, ShaderFlags b) {
    return static_cast<ShaderFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ShaderFlags operator&(ShaderFlags a, ShaderFlags b) {
    return static_cast<ShaderFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline ShaderFlags& operator|=(ShaderFlags& a, ShaderFlags b) {
    return a = a | b;
}

inline ShaderFlags& operator&=(ShaderFlags& a, ShaderFlags b) {
    return a = a & b;
}

inline bool has_flag(ShaderFlags flags, ShaderFlags flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// Internal shader state with uniform location caching
struct ShaderState {
    Shader raylib_shader{};
    std::unordered_map<std::string, int> uniform_locations;
    std::string vertex_path;
    std::string fragment_path;
    bool from_memory = false;
    bool valid = false;
    ShaderFlags flags = ShaderFlags::NONE;  // Shader capability flags

    // Get or cache uniform location. Returns -1 for unknown uniforms.
    int get_location(const std::string& name);
};

class ShaderManager {
public:
    static ShaderManager& instance();

    // Load shader from files. Vertex is optional (nullptr = default vertex shader).
    ShaderHandle load(const std::string& fragment_path,
                      const std::string& vertex_path = "");

    // Load shader from source code strings. Vertex is optional.
    ShaderHandle load_from_memory(const std::string& fragment_code,
                                  const std::string& vertex_code = "");

    // Get shader state by handle
    ShaderState* get(ShaderHandle handle);
    const ShaderState* get(ShaderHandle handle) const;

    // Check if handle is valid
    bool valid(ShaderHandle handle) const;

    // Check if shader requires surface continuity (unified UV space)
    bool requires_surface_continuity(ShaderHandle handle) const;

    // Release a reference (decrements refcount, unloads at 0)
    void release(ShaderHandle handle);

    // Add a reference (for copying handles in Ruby)
    void add_ref(ShaderHandle handle);

    // Get current reference count
    int get_ref_count(ShaderHandle handle) const;

    // Clear all shaders
    void clear();

    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

private:
    ShaderManager() = default;

    // Generate cache key for file-based shaders
    std::string make_cache_key(const std::string& vertex_path,
                               const std::string& fragment_path) const;

    std::vector<ShaderState> shaders_;
    std::vector<int> ref_counts_;
    std::unordered_map<std::string, ShaderHandle> path_to_handle_;
};

} // namespace gmr

#endif
