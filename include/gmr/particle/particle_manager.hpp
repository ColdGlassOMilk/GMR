#ifndef GMR_PARTICLE_MANAGER_HPP
#define GMR_PARTICLE_MANAGER_HPP

#include "gmr/particle/particle_types.hpp"
#include "gmr/particle/emitter_config.hpp"
#include "gmr/particle/emitter_state.hpp"
#include "gmr/types.hpp"
#include <mruby.h>
#include <raylib.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace gmr {
namespace particle {

class ParticleManager {
public:
    static ParticleManager& instance();

    // --- Config Management ---

    // Load config from JSON file, returns config name for lookup
    // Caches the config; subsequent loads of same path return cached name
    std::string load_config(const std::string& path);

    // Get config by name (returns nullptr if not found)
    const EmitterConfig* get_config(const std::string& name) const;

    // Register config programmatically
    void register_config(const std::string& name, EmitterConfig config);

    // --- Emitter Lifecycle ---

    // Create emitter from config (config must remain valid)
    EmitterHandle create(const EmitterConfig* config);

    // Destroy emitter
    void destroy(EmitterHandle handle);

    // Get emitter state (returns nullptr if invalid)
    EmitterState* get(EmitterHandle handle);

    // Check if handle is valid
    bool valid(EmitterHandle handle) const;

    // --- Convenience API (fire-and-forget) ---

    // Emit particles at a position (auto-destroys when complete)
    EmitterHandle emit(const std::string& effect_name, Vec2 position, bool one_shot = true);

    // Emit particles attached to a transform
    EmitterHandle emit(const std::string& effect_name, TransformHandle transform, bool one_shot = true);

    // --- Emitter Control ---

    // Start continuous emission
    void start(EmitterHandle handle);

    // Stop emission (particles continue to live out)
    void stop(EmitterHandle handle);

    // Pause all updates
    void pause(EmitterHandle handle);

    // Resume updates
    void resume(EmitterHandle handle);

    // Emit a burst of particles immediately
    // count = -1 uses config's burst_count
    void burst(EmitterHandle handle, int count = -1);

    // --- Frame Update ---

    // Update all emitters and particles (called from main loop)
    void update(mrb_state* mrb, float dt_scaled, float dt_unscaled);

    // Queue all particles for rendering (called from draw phase)
    void queue_draw();

    // Queue a single emitter's particles for rendering (for Ruby draw control)
    void queue_draw_emitter(EmitterHandle handle);

    // --- Cleanup ---

    // Clear all emitters and configs (for hot reload / shutdown)
    void clear(mrb_state* mrb);

    // --- Debug ---

    size_t emitter_count() const { return emitters_.size(); }
    size_t config_count() const { return configs_.size(); }
    size_t total_particle_count() const;
    size_t alive_particle_count() const;

    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

private:
    ParticleManager() = default;

    // Update a single emitter
    void update_emitter(mrb_state* mrb, EmitterState& emitter, float dt);

    // Update a single particle
    void update_particle(ParticleState& p, const EmitterConfig& cfg, float dt);

    // Spawn particles for an emitter based on spawn rate
    void spawn_particles(EmitterState& emitter, float dt);

    // Initialize a newly spawned particle
    void initialize_particle(ParticleState& p, EmitterState& emitter);

    // Calculate spawn position based on emission shape
    Vec2 calculate_spawn_position(const EmitterState& emitter);

    // Calculate initial velocity based on velocity mode
    Vec2 calculate_initial_velocity(const EmitterConfig& cfg, const Vec2& spawn_offset);

    // Render a textured particle
    void render_textured_particle(Texture2D& texture, const EmitterConfig& cfg,
                                  const ParticleState& p, Vec2 screen_pos,
                                  float scale, ::Color tint);

    // Cleanup finished emitters
    void cleanup_finished(mrb_state* mrb);

    // Config storage (name -> config)
    std::unordered_map<std::string, EmitterConfig> configs_;

    // Path to name mapping (for caching loaded configs)
    std::unordered_map<std::string, std::string> path_to_name_;

    // Emitter storage (handle -> state)
    std::unordered_map<EmitterHandle, EmitterState> emitters_;
    EmitterHandle next_id_{0};

    // Deferred removal list
    std::vector<EmitterHandle> emitters_to_remove_;
};

} // namespace particle
} // namespace gmr

#endif // GMR_PARTICLE_MANAGER_HPP
