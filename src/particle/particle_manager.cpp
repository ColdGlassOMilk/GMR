#include "gmr/particle/particle_manager.hpp"
#include "gmr/particle/config_loader.hpp"
#include "gmr/transform.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/animation/easing.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/camera.hpp"

#include <raylib.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace gmr {
namespace particle {

ParticleManager& ParticleManager::instance() {
    static ParticleManager inst;
    return inst;
}

// --- Config Management ---

std::string ParticleManager::load_config(const std::string& path) {
    // Check if already loaded
    auto it = path_to_name_.find(path);
    if (it != path_to_name_.end()) {
        TraceLog(LOG_DEBUG, "PARTICLE: Config already loaded: %s -> %s", path.c_str(), it->second.c_str());
        return it->second;
    }

    // Load from file
    auto config = load_config_from_file(path);
    if (!config) {
        TraceLog(LOG_WARNING, "PARTICLE: Failed to load config from: %s", path.c_str());
        return "";
    }

    std::string name = config->name;
    configs_[name] = std::move(*config);
    path_to_name_[path] = name;

    // Load texture if specified
    EmitterConfig& cfg = configs_[name];
    if (!cfg.texture_path.empty()) {
        cfg.texture = TextureManager::instance().load(cfg.texture_path);
        if (cfg.texture != INVALID_HANDLE) {
            // Auto-detect frame dimensions if not specified
            if (cfg.frame_width == 0 && cfg.spritesheet_cols > 0) {
                int tex_width = TextureManager::instance().get_width(cfg.texture);
                cfg.frame_width = static_cast<float>(tex_width) / cfg.spritesheet_cols;
            }
            if (cfg.frame_height == 0 && cfg.spritesheet_rows > 0) {
                int tex_height = TextureManager::instance().get_height(cfg.texture);
                cfg.frame_height = static_cast<float>(tex_height) / cfg.spritesheet_rows;
            }
            TraceLog(LOG_INFO, "PARTICLE: Loaded texture '%s' (%dx%d frames, %.0fx%.0f px each)",
                     cfg.texture_path.c_str(), cfg.spritesheet_cols, cfg.spritesheet_rows,
                     cfg.frame_width, cfg.frame_height);
        }
    }

    TraceLog(LOG_INFO, "PARTICLE: Loaded config '%s' from %s (spawn_rate=%.1f, max=%zu)",
             name.c_str(), path.c_str(), cfg.spawn_rate, cfg.max_particles);
    return name;
}

const EmitterConfig* ParticleManager::get_config(const std::string& name) const {
    auto it = configs_.find(name);
    if (it != configs_.end()) {
        return &it->second;
    }
    return nullptr;
}

void ParticleManager::register_config(const std::string& name, EmitterConfig config) {
    config.name = name;
    configs_[name] = std::move(config);
}

// --- Emitter Lifecycle ---

EmitterHandle ParticleManager::create(const EmitterConfig* config) {
    if (!config) return INVALID_EMITTER_HANDLE;

    EmitterHandle handle = next_id_++;
    EmitterState& emitter = emitters_[handle];
    emitter.init(config);
    TraceLog(LOG_DEBUG, "PARTICLE: Created emitter %d with config '%s' (pool size: %zu)",
             handle, config->name.c_str(), emitter.particles.size());
    return handle;
}

void ParticleManager::destroy(EmitterHandle handle) {
    auto it = emitters_.find(handle);
    if (it != emitters_.end()) {
        // Mark for deferred removal
        it->second.active = false;
        emitters_to_remove_.push_back(handle);
    }
}

EmitterState* ParticleManager::get(EmitterHandle handle) {
    auto it = emitters_.find(handle);
    if (it != emitters_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ParticleManager::valid(EmitterHandle handle) const {
    return emitters_.find(handle) != emitters_.end();
}

// --- Convenience API ---

EmitterHandle ParticleManager::emit(const std::string& effect_name, Vec2 position, bool one_shot) {
    const EmitterConfig* config = get_config(effect_name);
    if (!config) {
        // Try loading it
        std::string loaded_name = load_config(effect_name);
        config = get_config(loaded_name);
        if (!config) return INVALID_EMITTER_HANDLE;
    }

    EmitterHandle handle = create(config);
    EmitterState* emitter = get(handle);
    if (emitter) {
        emitter->position = position;
        emitter->last_position = position;
        emitter->one_shot = one_shot;
        emitter->emitting = true;

        // If burst mode, trigger immediate burst
        if (config->spawn_rate <= 0 && config->burst_count > 0) {
            burst(handle, config->burst_count);
            emitter->emitting = false;
        }
    }
    return handle;
}

EmitterHandle ParticleManager::emit(const std::string& effect_name, TransformHandle transform, bool one_shot) {
    const EmitterConfig* config = get_config(effect_name);
    if (!config) {
        std::string loaded_name = load_config(effect_name);
        config = get_config(loaded_name);
        if (!config) return INVALID_EMITTER_HANDLE;
    }

    EmitterHandle handle = create(config);
    EmitterState* emitter = get(handle);
    if (emitter) {
        emitter->attached_transform = transform;
        emitter->one_shot = one_shot;
        emitter->emitting = true;

        // Get initial position from transform
        if (transform != INVALID_HANDLE) {
            emitter->position = TransformManager::instance().get_world_position(transform);
            emitter->last_position = emitter->position;
        }

        // If burst mode, trigger immediate burst
        if (config->spawn_rate <= 0 && config->burst_count > 0) {
            burst(handle, config->burst_count);
            emitter->emitting = false;
        }
    }
    return handle;
}

// --- Emitter Control ---

void ParticleManager::start(EmitterHandle handle) {
    EmitterState* emitter = get(handle);
    if (emitter) {
        emitter->emitting = true;
        emitter->active = true;
        TraceLog(LOG_INFO, "PARTICLE: Started emitter %d at pos (%.2f, %.2f), has_texture=%d",
                 handle, emitter->position.x, emitter->position.y,
                 emitter->config ? (emitter->config->texture != INVALID_HANDLE) : -1);
    }
}

void ParticleManager::stop(EmitterHandle handle) {
    EmitterState* emitter = get(handle);
    if (emitter) {
        emitter->emitting = false;
    }
}

void ParticleManager::pause(EmitterHandle handle) {
    EmitterState* emitter = get(handle);
    if (emitter) {
        emitter->active = false;
    }
}

void ParticleManager::resume(EmitterHandle handle) {
    EmitterState* emitter = get(handle);
    if (emitter) {
        emitter->active = true;
    }
}

void ParticleManager::burst(EmitterHandle handle, int count) {
    EmitterState* emitter = get(handle);
    if (!emitter || !emitter->config) return;

    int to_spawn = (count < 0) ? emitter->config->burst_count : count;
    for (int i = 0; i < to_spawn; ++i) {
        ParticleState* p = emitter->spawn_particle();
        if (p) {
            initialize_particle(*p, *emitter);
        }
    }
}

// --- Frame Update ---

void ParticleManager::update(mrb_state* mrb, float dt_scaled, float dt_unscaled) {
    // Build list of handles to update (avoid iterator invalidation)
    std::vector<EmitterHandle> handles;
    handles.reserve(emitters_.size());
    for (auto& [handle, emitter] : emitters_) {
        if (emitter.active) {
            handles.push_back(handle);
        }
    }

    // Update each emitter
    for (EmitterHandle handle : handles) {
        EmitterState* emitter = get(handle);
        if (!emitter || !emitter->active || !emitter->config) continue;

        float dt = emitter->config->scaled ? dt_scaled : dt_unscaled;
        update_emitter(mrb, *emitter, dt);
    }

    // Cleanup finished emitters
    cleanup_finished(mrb);
}

void ParticleManager::update_emitter(mrb_state* mrb, EmitterState& emitter, float dt) {
    const EmitterConfig& cfg = *emitter.config;

    // Update position from attached transform
    if (emitter.attached_transform != INVALID_HANDLE) {
        auto& tm = TransformManager::instance();
        if (tm.valid(emitter.attached_transform)) {
            emitter.last_position = emitter.position;
            emitter.position = tm.get_world_position(emitter.attached_transform);
        }
    }

    // Spawn new particles (continuous emission)
    if (emitter.emitting && cfg.spawn_rate > 0) {
        spawn_particles(emitter, dt);
    }

    // Handle burst interval
    if (emitter.emitting && cfg.burst_interval > 0 && cfg.burst_count > 0) {
        emitter.burst_timer += dt;
        if (emitter.burst_timer >= cfg.burst_interval) {
            emitter.burst_timer -= cfg.burst_interval;
            for (int i = 0; i < cfg.burst_count; ++i) {
                ParticleState* p = emitter.spawn_particle();
                if (p) {
                    initialize_particle(*p, emitter);
                }
            }
        }
    }

    // Update existing particles
    size_t alive_count = 0;
    for (auto& p : emitter.particles) {
        if (!p.alive) continue;

        update_particle(p, cfg, dt);

        if (p.lifetime <= 0.0f) {
            p.alive = false;
        } else {
            alive_count++;
        }
    }
    emitter.alive_count = alive_count;

    // Check for completion (one-shot with no particles)
    if (emitter.one_shot && !emitter.emitting && alive_count == 0) {
        // Fire callback
        if (!mrb_nil_p(emitter.on_complete)) {
            mrb_funcall(mrb, emitter.on_complete, "call", 0);
            if (mrb->exc) {
                mrb->exc = nullptr; // Clear exception
            }
        }
        // Mark for removal
        emitter.active = false;
        emitters_to_remove_.push_back(
            [&]() -> EmitterHandle {
                for (auto& [h, e] : emitters_) {
                    if (&e == &emitter) return h;
                }
                return INVALID_EMITTER_HANDLE;
            }()
        );
    }
}

void ParticleManager::update_particle(ParticleState& p, const EmitterConfig& cfg, float dt) {
    // Lifetime
    p.lifetime -= dt;
    float t = 1.0f - (p.lifetime / p.max_lifetime); // 0 at birth, 1 at death
    t = std::clamp(t, 0.0f, 1.0f);

    // Apply eased interpolation for size
    float size_t = animation::apply_easing(cfg.size_easing, t);
    p.size = p.start_size + (p.end_size - p.start_size) * size_t;

    // Apply eased interpolation for color
    float color_t = animation::apply_easing(cfg.color_easing, t);
    p.color.r = static_cast<uint8_t>(p.start_color.r + (static_cast<int>(p.end_color.r) - static_cast<int>(p.start_color.r)) * color_t);
    p.color.g = static_cast<uint8_t>(p.start_color.g + (static_cast<int>(p.end_color.g) - static_cast<int>(p.start_color.g)) * color_t);
    p.color.b = static_cast<uint8_t>(p.start_color.b + (static_cast<int>(p.end_color.b) - static_cast<int>(p.start_color.b)) * color_t);
    p.color.a = static_cast<uint8_t>(p.start_color.a + (static_cast<int>(p.end_color.a) - static_cast<int>(p.start_color.a)) * color_t);

    // Apply gravity
    p.velocity.x += cfg.gravity.x * dt;
    p.velocity.y += cfg.gravity.y * dt;

    // Apply acceleration
    p.velocity.x += p.acceleration.x * dt;
    p.velocity.y += p.acceleration.y * dt;

    // Apply drag
    if (cfg.drag > 0.0f) {
        float damping = std::pow(1.0f - std::min(cfg.drag, 0.99f), dt);
        p.velocity.x *= damping;
        p.velocity.y *= damping;
    }

    // Update position
    p.position.x += p.velocity.x * dt;
    p.position.y += p.velocity.y * dt;

    // Update rotation
    p.rotation += p.angular_velocity * dt;
}

void ParticleManager::spawn_particles(EmitterState& emitter, float dt) {
    const EmitterConfig& cfg = *emitter.config;

    emitter.spawn_accumulator += cfg.spawn_rate * dt;

    // Spawn whole particles
    while (emitter.spawn_accumulator >= 1.0f) {
        emitter.spawn_accumulator -= 1.0f;

        ParticleState* p = emitter.spawn_particle();
        if (p) {
            initialize_particle(*p, emitter);
        }
    }
}

void ParticleManager::initialize_particle(ParticleState& p, EmitterState& emitter) {
    const EmitterConfig& cfg = *emitter.config;

    // Calculate spawn position
    Vec2 spawn_offset = calculate_spawn_position(emitter);
    p.position.x = emitter.position.x + spawn_offset.x;
    p.position.y = emitter.position.y + spawn_offset.y;

    // Calculate initial velocity
    p.velocity = calculate_initial_velocity(cfg, spawn_offset);

    // Initialize acceleration (radial/tangent will be applied during update if needed)
    p.acceleration = {0, 0};

    // Lifetime
    p.lifetime = cfg.lifetime.random();
    p.max_lifetime = p.lifetime;

    // Size
    p.start_size = cfg.start_size.random();
    p.end_size = cfg.end_size.random();
    p.size = p.start_size;

    // Rotation
    p.rotation = cfg.start_rotation.random();
    p.angular_velocity = cfg.angular_velocity.random();

    // Color
    p.start_color = cfg.start_color.random();
    p.end_color = cfg.end_color.random();
    p.color = p.start_color;

    // Assign random frame from spritesheet
    int total_frames = cfg.spritesheet_cols * cfg.spritesheet_rows;
    if (total_frames > 1) {
        p.frame_index = static_cast<uint32_t>(std::rand() % total_frames);
    } else {
        p.frame_index = 0;
    }

    // Custom data (zeroed by default)
    for (int i = 0; i < 4; ++i) {
        p.custom[i] = 0.0f;
    }
}

Vec2 ParticleManager::calculate_spawn_position(const EmitterState& emitter) {
    const EmitterConfig& cfg = *emitter.config;
    Vec2 offset{0, 0};

    switch (cfg.shape) {
        case EmissionShape::POINT:
            // No offset
            break;

        case EmissionShape::CIRCLE: {
            float angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * static_cast<float>(M_PI);
            float radius = static_cast<float>(std::rand()) / RAND_MAX * cfg.shape_radius;
            offset.x = std::cos(angle) * radius;
            offset.y = std::sin(angle) * radius;
            break;
        }

        case EmissionShape::CIRCLE_EDGE: {
            float angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * static_cast<float>(M_PI);
            offset.x = std::cos(angle) * cfg.shape_radius;
            offset.y = std::sin(angle) * cfg.shape_radius;
            break;
        }

        case EmissionShape::RECTANGLE: {
            offset.x = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * cfg.shape_size.x;
            offset.y = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * cfg.shape_size.y;
            break;
        }

        case EmissionShape::RECTANGLE_EDGE: {
            int edge = std::rand() % 4;
            float t = static_cast<float>(std::rand()) / RAND_MAX;
            switch (edge) {
                case 0: // Top
                    offset.x = (t - 0.5f) * cfg.shape_size.x;
                    offset.y = -cfg.shape_size.y * 0.5f;
                    break;
                case 1: // Bottom
                    offset.x = (t - 0.5f) * cfg.shape_size.x;
                    offset.y = cfg.shape_size.y * 0.5f;
                    break;
                case 2: // Left
                    offset.x = -cfg.shape_size.x * 0.5f;
                    offset.y = (t - 0.5f) * cfg.shape_size.y;
                    break;
                case 3: // Right
                    offset.x = cfg.shape_size.x * 0.5f;
                    offset.y = (t - 0.5f) * cfg.shape_size.y;
                    break;
            }
            break;
        }

        case EmissionShape::LINE: {
            float t = static_cast<float>(std::rand()) / RAND_MAX;
            offset.x = (t - 0.5f) * cfg.shape_size.x;
            offset.y = 0;
            break;
        }
    }

    return offset;
}

Vec2 ParticleManager::calculate_initial_velocity(const EmitterConfig& cfg, const Vec2& spawn_offset) {
    Vec2 velocity{0, 0};
    float speed = cfg.speed.random();
    float angle = 0;

    switch (cfg.velocity_mode) {
        case VelocityMode::DIRECTIONAL: {
            float spread = cfg.spread.random();
            angle = cfg.direction + spread;
            break;
        }

        case VelocityMode::RADIAL: {
            // Away from center
            if (spawn_offset.x != 0 || spawn_offset.y != 0) {
                angle = std::atan2(spawn_offset.y, spawn_offset.x);
            } else {
                angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * static_cast<float>(M_PI);
            }
            float spread = cfg.spread.random();
            angle += spread;
            break;
        }

        case VelocityMode::TANGENTIAL: {
            // Perpendicular to radial
            if (spawn_offset.x != 0 || spawn_offset.y != 0) {
                angle = std::atan2(spawn_offset.y, spawn_offset.x) + static_cast<float>(M_PI) * 0.5f;
            } else {
                angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * static_cast<float>(M_PI);
            }
            float spread = cfg.spread.random();
            angle += spread;
            break;
        }

        case VelocityMode::RANDOM: {
            angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * static_cast<float>(M_PI);
            break;
        }
    }

    velocity.x = std::cos(angle) * speed;
    velocity.y = std::sin(angle) * speed;
    return velocity;
}

// --- Rendering ---

void ParticleManager::queue_draw_emitter(EmitterHandle handle) {
    EmitterState* emitter = get(handle);
    if (!emitter || !emitter->active || !emitter->config) return;

    // Mark as manually drawn this frame
    emitter->drawn_this_frame = true;

    auto& draw_queue = DrawQueue::instance();
    const auto& cfg = *emitter->config;
    bool has_texture = (cfg.texture != INVALID_HANDLE);

    // For circles, we need camera info
    Camera2DState* cam = nullptr;
    float scale = 24.0f;
    if (!has_texture) {
        CameraHandle cam_handle = draw_queue.get_queued_camera();
        cam = (cam_handle != INVALID_CAMERA_HANDLE)
            ? CameraManager::instance().get(cam_handle) : nullptr;
        scale = cam ? cam->get_effective_scale() : 24.0f;
    }

    for (const auto& p : emitter->particles) {
        if (!p.alive) continue;

        if (has_texture) {
            int col = static_cast<int>(p.frame_index) % cfg.spritesheet_cols;
            int row = static_cast<int>(p.frame_index) / cfg.spritesheet_cols;
            float src_x = col * cfg.frame_width;
            float src_y = row * cfg.frame_height;

            DrawColor color{p.color.r, p.color.g, p.color.b, p.color.a};

            draw_queue.queue_texture_quad(
                cfg.texture,
                src_x, src_y, cfg.frame_width, cfg.frame_height,
                p.position.x, p.position.y,
                p.size,
                p.rotation,
                color,
                cfg.layer,
                cfg.z
            );
        } else {
            if (!cam) continue;
            Vec2 screen_pos = cam->world_to_screen(p.position);
            float screen_radius = p.size * 0.5f * scale;

            DrawColor color{p.color.r, p.color.g, p.color.b, p.color.a};
            draw_queue.queue_circle(
                screen_pos.x,
                screen_pos.y,
                screen_radius,
                color,
                true,
                cfg.layer,
                cfg.z
            );
        }
    }
}

void ParticleManager::queue_draw() {
    // Draw any emitters that weren't manually drawn via queue_draw_emitter
    for (auto& [handle, emitter] : emitters_) {
        if (emitter.drawn_this_frame) {
            // Reset flag for next frame, skip drawing
            emitter.drawn_this_frame = false;
            continue;
        }
        queue_draw_emitter(handle);
        emitter.drawn_this_frame = false; // Reset after drawing
    }
}

void ParticleManager::render_textured_particle(
    Texture2D& texture,
    const EmitterConfig& cfg,
    const ParticleState& p,
    Vec2 screen_pos,
    float scale,
    ::Color tint
) {
    // Calculate source rect from frame index
    int col = static_cast<int>(p.frame_index) % cfg.spritesheet_cols;
    int row = static_cast<int>(p.frame_index) / cfg.spritesheet_cols;

    // Apply texel inset to prevent texture bleeding
    constexpr float TEXEL_INSET = 0.5f;
    Rectangle source{
        col * cfg.frame_width + TEXEL_INSET,
        row * cfg.frame_height + TEXEL_INSET,
        cfg.frame_width - TEXEL_INSET * 2.0f,
        cfg.frame_height - TEXEL_INSET * 2.0f
    };

    // Destination size in screen pixels
    // p.size is in world units, scale converts to screen pixels
    float dest_size = p.size * scale;

    // Position at center, rotate around center
    Rectangle dest{
        screen_pos.x,
        screen_pos.y,
        dest_size,
        dest_size
    };

    Vector2 origin{dest_size * 0.5f, dest_size * 0.5f};
    float rotation_degrees = p.rotation * (180.0f / static_cast<float>(M_PI));

    DrawTexturePro(texture, source, dest, origin, rotation_degrees, tint);
}

// --- Cleanup ---

void ParticleManager::cleanup_finished(mrb_state* mrb) {
    for (EmitterHandle handle : emitters_to_remove_) {
        auto it = emitters_.find(handle);
        if (it != emitters_.end()) {
            // Unregister from GC if needed
            if (mrb && !mrb_nil_p(it->second.ruby_emitter_obj)) {
                mrb_gc_unregister(mrb, it->second.ruby_emitter_obj);
            }
            emitters_.erase(it);
        }
    }
    emitters_to_remove_.clear();
}

void ParticleManager::clear(mrb_state* mrb) {
    // Unregister all Ruby objects from GC
    if (mrb) {
        for (auto& [handle, emitter] : emitters_) {
            if (!mrb_nil_p(emitter.ruby_emitter_obj)) {
                mrb_gc_unregister(mrb, emitter.ruby_emitter_obj);
            }
            if (!mrb_nil_p(emitter.on_complete)) {
                mrb_gc_unregister(mrb, emitter.on_complete);
            }
        }
    }

    emitters_.clear();
    emitters_to_remove_.clear();
    configs_.clear();
    path_to_name_.clear();
    next_id_ = 0;
}

// --- Debug ---

size_t ParticleManager::total_particle_count() const {
    size_t total = 0;
    for (const auto& [handle, emitter] : emitters_) {
        total += emitter.particles.size();
    }
    return total;
}

size_t ParticleManager::alive_particle_count() const {
    size_t total = 0;
    for (const auto& [handle, emitter] : emitters_) {
        total += emitter.alive_count;
    }
    return total;
}

} // namespace particle
} // namespace gmr
