#include "gmr/resources/shader_manager.hpp"
#include "gmr/paths.hpp"
#include "raylib.h"

namespace gmr {

// Platform-specific shader path adjustment
// On native Mac/Linux, use GLSL 330 shaders; on Web use GLSL ES 100
static std::string adjust_shader_path_for_platform(const std::string& path) {
#if defined(PLATFORM_WEB)
    // Web uses GLSL ES 100 shaders (the default ones in shaders/)
    return path;
#elif defined(__APPLE__) || defined(__linux__)
    // Mac and Linux use OpenGL 3.3 core profile which requires GLSL 330
    // Redirect shaders/foo.fs -> shaders/glsl330/foo.fs
    const std::string shaders_prefix = "shaders/";
    if (path.rfind(shaders_prefix, 0) == 0 && path.find("glsl330/") == std::string::npos) {
        // Extract filename after "shaders/"
        std::string filename = path.substr(shaders_prefix.length());
        return shaders_prefix + "glsl330/" + filename;
    }
    return path;
#else
    // Windows can use either, but GLSL ES 100 works in compatibility mode
    return path;
#endif
}

// ShaderState implementation

int ShaderState::get_location(const std::string& name) {
    // Check cache first
    auto it = uniform_locations.find(name);
    if (it != uniform_locations.end()) {
        return it->second;
    }

    // Look up and cache (even if -1, to avoid repeated lookups)
    int location = GetShaderLocation(raylib_shader, name.c_str());
    uniform_locations[name] = location;
    return location;
}

// ShaderManager implementation

ShaderManager& ShaderManager::instance() {
    static ShaderManager instance;
    return instance;
}

std::string ShaderManager::make_cache_key(const std::string& vertex_path,
                                          const std::string& fragment_path) const {
    // Combine paths with separator that can't appear in file paths
    return vertex_path + "|" + fragment_path;
}

ShaderHandle ShaderManager::load(const std::string& fragment_path,
                                 const std::string& vertex_path) {
    // Check cache
    std::string cache_key = make_cache_key(vertex_path, fragment_path);
    auto it = path_to_handle_.find(cache_key);
    if (it != path_to_handle_.end()) {
        ShaderHandle handle = it->second;
        if (handle >= 0 && handle < static_cast<ShaderHandle>(ref_counts_.size())) {
            ref_counts_[handle]++;
        }
        return handle;
    }

    // Resolve paths (with platform-specific shader directory adjustment)
    std::string adjusted_frag = adjust_shader_path_for_platform(fragment_path);
    std::string resolved_frag = resolve_asset_path(adjusted_frag);
    const char* vert_ptr = nullptr;
    std::string resolved_vert;
    if (!vertex_path.empty()) {
        std::string adjusted_vert = adjust_shader_path_for_platform(vertex_path);
        resolved_vert = resolve_asset_path(adjusted_vert);
        vert_ptr = resolved_vert.c_str();
    }

    // Load shader
    Shader shader = LoadShader(vert_ptr, resolved_frag.c_str());

    if (!IsShaderValid(shader)) {
        TraceLog(LOG_ERROR, "SHADER: Failed to load shader - fragment: %s, vertex: %s",
                 fragment_path.c_str(),
                 vertex_path.empty() ? "(default)" : vertex_path.c_str());
        return INVALID_HANDLE;
    }

    // Create state
    ShaderHandle handle = static_cast<ShaderHandle>(shaders_.size());
    ShaderState state;
    state.raylib_shader = shader;
    state.vertex_path = vertex_path;
    state.fragment_path = fragment_path;
    state.from_memory = false;
    state.valid = true;

    shaders_.push_back(std::move(state));
    ref_counts_.push_back(1);
    path_to_handle_[cache_key] = handle;

    TraceLog(LOG_INFO, "SHADER: Loaded shader [ID %d] - fragment: %s, vertex: %s",
             handle, fragment_path.c_str(),
             vertex_path.empty() ? "(default)" : vertex_path.c_str());

    return handle;
}

ShaderHandle ShaderManager::load_from_memory(const std::string& fragment_code,
                                             const std::string& vertex_code) {
    const char* vert_ptr = vertex_code.empty() ? nullptr : vertex_code.c_str();
    const char* frag_ptr = fragment_code.c_str();

    Shader shader = LoadShaderFromMemory(vert_ptr, frag_ptr);

    if (!IsShaderValid(shader)) {
        TraceLog(LOG_ERROR, "SHADER: Failed to compile shader from source");
        return INVALID_HANDLE;
    }

    ShaderHandle handle = static_cast<ShaderHandle>(shaders_.size());
    ShaderState state;
    state.raylib_shader = shader;
    state.from_memory = true;
    state.valid = true;

    shaders_.push_back(std::move(state));
    ref_counts_.push_back(1);

    TraceLog(LOG_INFO, "SHADER: Compiled shader from memory [ID %d]", handle);

    return handle;
}

ShaderState* ShaderManager::get(ShaderHandle handle) {
    if (handle < 0 || handle >= static_cast<ShaderHandle>(shaders_.size())) {
        return nullptr;
    }
    if (!shaders_[handle].valid) {
        return nullptr;
    }
    return &shaders_[handle];
}

const ShaderState* ShaderManager::get(ShaderHandle handle) const {
    if (handle < 0 || handle >= static_cast<ShaderHandle>(shaders_.size())) {
        return nullptr;
    }
    if (!shaders_[handle].valid) {
        return nullptr;
    }
    return &shaders_[handle];
}

bool ShaderManager::valid(ShaderHandle handle) const {
    if (handle < 0 || handle >= static_cast<ShaderHandle>(shaders_.size())) {
        return false;
    }
    return shaders_[handle].valid && ref_counts_[handle] > 0;
}

bool ShaderManager::requires_surface_continuity(ShaderHandle handle) const {
    const ShaderState* shader = get(handle);
    return shader && has_flag(shader->flags, ShaderFlags::SURFACE_CONTINUITY);
}

void ShaderManager::release(ShaderHandle handle) {
    if (handle < 0 || handle >= static_cast<ShaderHandle>(ref_counts_.size())) {
        return;
    }

    if (ref_counts_[handle] > 0) {
        ref_counts_[handle]--;

        if (ref_counts_[handle] == 0) {
            ShaderState& state = shaders_[handle];
            if (state.valid) {
                UnloadShader(state.raylib_shader);
                state.valid = false;

                TraceLog(LOG_INFO, "SHADER: Unloaded shader [ID %d]", handle);

                // Remove from path cache if file-based
                if (!state.from_memory) {
                    std::string cache_key = make_cache_key(state.vertex_path, state.fragment_path);
                    path_to_handle_.erase(cache_key);
                }
            }
        }
    }
}

void ShaderManager::add_ref(ShaderHandle handle) {
    if (handle >= 0 && handle < static_cast<ShaderHandle>(ref_counts_.size())) {
        ref_counts_[handle]++;
    }
}

int ShaderManager::get_ref_count(ShaderHandle handle) const {
    if (handle >= 0 && handle < static_cast<ShaderHandle>(ref_counts_.size())) {
        return ref_counts_[handle];
    }
    return 0;
}

void ShaderManager::clear() {
    for (size_t i = 0; i < shaders_.size(); ++i) {
        if (shaders_[i].valid && ref_counts_[i] > 0) {
            UnloadShader(shaders_[i].raylib_shader);
            shaders_[i].valid = false;
        }
    }
    shaders_.clear();
    ref_counts_.clear();
    path_to_handle_.clear();
}

} // namespace gmr
