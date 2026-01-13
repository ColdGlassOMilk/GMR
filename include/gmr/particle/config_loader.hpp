#ifndef GMR_PARTICLE_CONFIG_LOADER_HPP
#define GMR_PARTICLE_CONFIG_LOADER_HPP

#include "gmr/particle/emitter_config.hpp"
#include <string>
#include <optional>

namespace gmr {
namespace particle {

// Load an emitter configuration from a JSON file
// Returns the loaded config, or std::nullopt on failure
std::optional<EmitterConfig> load_config_from_file(const std::string& path);

// Parse an emitter configuration from a JSON string
std::optional<EmitterConfig> parse_config_json(const std::string& json_str);

} // namespace particle
} // namespace gmr

#endif // GMR_PARTICLE_CONFIG_LOADER_HPP
