#include "gmr/bindings/particle.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/particle/particle_manager.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/transform.hpp"
#include "gmr/scripting/helpers.hpp"

#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/variable.h>
#include <mruby/array.h>
#include <cstdlib>
#include <sstream>

namespace gmr {
namespace bindings {

// ============================================================================
// Vec2 Helper Functions
// ============================================================================

static mrb_value create_vec2(mrb_state* mrb, const Vec2& v) {
    RClass* gmr_mod = get_gmr_module(mrb);
    RClass* mathf = mrb_module_get_under(mrb, gmr_mod, "Mathf");
    RClass* vec2_class = mrb_class_get_under(mrb, mathf, "Vec2");
    mrb_value args[2] = {mrb_float_value(mrb, v.x), mrb_float_value(mrb, v.y)};
    return mrb_obj_new(mrb, vec2_class, 2, args);
}

static Vec2 extract_vec2(mrb_state* mrb, mrb_value val) {
    mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
    mrb_sym y_sym = mrb_intern_cstr(mrb, "y");

    if (mrb_respond_to(mrb, val, x_sym) && mrb_respond_to(mrb, val, y_sym)) {
        mrb_value x = scripting::safe_method_call(mrb, val, "x");
        mrb_value y = scripting::safe_method_call(mrb, val, "y");
        return {static_cast<float>(mrb_as_float(mrb, x)),
                static_cast<float>(mrb_as_float(mrb, y))};
    }

    mrb_raise(mrb, E_TYPE_ERROR, "Expected Vec2 or object with x/y methods");
    return {0.0f, 0.0f};
}

// ============================================================================
// Hash Config Parsing Helpers
// ============================================================================

// Get a symbol key from hash, returns nil if not found
static mrb_value hash_get_sym(mrb_state* mrb, mrb_value hash, const char* key) {
    return mrb_hash_get(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, key)));
}

// Extract float from mrb_value
static float extract_float(mrb_state* mrb, mrb_value val, float default_val = 0.0f) {
    if (mrb_nil_p(val)) return default_val;
    return static_cast<float>(mrb_as_float(mrb, val));
}

// Extract int from mrb_value
static int extract_int(mrb_state* mrb, mrb_value val, int default_val = 0) {
    if (mrb_nil_p(val)) return default_val;
    if (mrb_integer_p(val)) return static_cast<int>(mrb_integer(val));
    return static_cast<int>(mrb_as_float(mrb, val));
}

// Extract bool from mrb_value
static bool extract_bool(mrb_state* mrb, mrb_value val, bool default_val = false) {
    if (mrb_nil_p(val)) return default_val;
    return mrb_test(val);
}

// Extract string from mrb_value
static std::string extract_string(mrb_state* mrb, mrb_value val) {
    if (mrb_nil_p(val)) return "";
    if (mrb_string_p(val)) return std::string(RSTRING_PTR(val), RSTRING_LEN(val));
    if (mrb_symbol_p(val)) {
        mrb_int len;
        const char* str = mrb_sym_name_len(mrb, mrb_symbol(val), &len);
        return std::string(str, len);
    }
    return "";
}

// Extract FloatRange from mrb_value (accepts number or {min:, max:} hash)
static particle::FloatRange extract_float_range(mrb_state* mrb, mrb_value val) {
    if (mrb_nil_p(val)) return particle::FloatRange(0.0f);

    if (mrb_hash_p(val)) {
        float min_val = extract_float(mrb, hash_get_sym(mrb, val, "min"), 0.0f);
        float max_val = extract_float(mrb, hash_get_sym(mrb, val, "max"), min_val);
        return particle::FloatRange(min_val, max_val);
    }

    float v = static_cast<float>(mrb_as_float(mrb, val));
    return particle::FloatRange(v);
}

// Extract Color from mrb_value (accepts {r:, g:, b:, a:} hash)
static Color extract_color(mrb_state* mrb, mrb_value val) {
    if (mrb_nil_p(val)) return Color{255, 255, 255, 255};

    if (mrb_hash_p(val)) {
        return Color{
            static_cast<uint8_t>(extract_int(mrb, hash_get_sym(mrb, val, "r"), 255)),
            static_cast<uint8_t>(extract_int(mrb, hash_get_sym(mrb, val, "g"), 255)),
            static_cast<uint8_t>(extract_int(mrb, hash_get_sym(mrb, val, "b"), 255)),
            static_cast<uint8_t>(extract_int(mrb, hash_get_sym(mrb, val, "a"), 255))
        };
    }

    return Color{255, 255, 255, 255};
}

// Extract ColorRange from mrb_value
static particle::ColorRange extract_color_range(mrb_state* mrb, mrb_value val) {
    if (mrb_nil_p(val)) return particle::ColorRange();

    if (mrb_hash_p(val)) {
        // Check if it's a range {min:, max:} or a single color {r:, g:, b:}
        mrb_value min_val = hash_get_sym(mrb, val, "min");
        if (!mrb_nil_p(min_val)) {
            Color min_col = extract_color(mrb, min_val);
            Color max_col = extract_color(mrb, hash_get_sym(mrb, val, "max"));
            return particle::ColorRange(min_col, max_col);
        }

        // Single color
        Color col = extract_color(mrb, val);
        return particle::ColorRange(col);
    }

    return particle::ColorRange();
}

// Extract Vec2 from hash with x:, y: keys
static Vec2 extract_vec2_from_hash(mrb_state* mrb, mrb_value val) {
    if (mrb_nil_p(val)) return Vec2{0.0f, 0.0f};

    if (mrb_hash_p(val)) {
        return Vec2{
            extract_float(mrb, hash_get_sym(mrb, val, "x"), 0.0f),
            extract_float(mrb, hash_get_sym(mrb, val, "y"), 0.0f)
        };
    }

    // Try object with x/y methods
    mrb_sym x_sym = mrb_intern_cstr(mrb, "x");
    mrb_sym y_sym = mrb_intern_cstr(mrb, "y");
    if (mrb_respond_to(mrb, val, x_sym) && mrb_respond_to(mrb, val, y_sym)) {
        return extract_vec2(mrb, val);
    }

    return Vec2{0.0f, 0.0f};
}

// Parse EmissionShape from string
static particle::EmissionShape parse_shape_string(const std::string& s) {
    if (s == "point") return particle::EmissionShape::POINT;
    if (s == "circle") return particle::EmissionShape::CIRCLE;
    if (s == "circle_edge") return particle::EmissionShape::CIRCLE_EDGE;
    if (s == "rectangle" || s == "rect") return particle::EmissionShape::RECTANGLE;
    if (s == "rectangle_edge" || s == "rect_edge") return particle::EmissionShape::RECTANGLE_EDGE;
    if (s == "line") return particle::EmissionShape::LINE;
    return particle::EmissionShape::POINT;
}

// Parse VelocityMode from string
static particle::VelocityMode parse_velocity_mode_string(const std::string& s) {
    if (s == "directional") return particle::VelocityMode::DIRECTIONAL;
    if (s == "radial") return particle::VelocityMode::RADIAL;
    if (s == "tangential") return particle::VelocityMode::TANGENTIAL;
    if (s == "random") return particle::VelocityMode::RANDOM;
    return particle::VelocityMode::DIRECTIONAL;
}

// Parse EasingType from string
static animation::EasingType parse_easing_string(const std::string& s) {
    if (s == "linear") return animation::EasingType::LINEAR;
    if (s == "in_quad") return animation::EasingType::IN_QUAD;
    if (s == "out_quad") return animation::EasingType::OUT_QUAD;
    if (s == "in_out_quad") return animation::EasingType::IN_OUT_QUAD;
    if (s == "in_cubic") return animation::EasingType::IN_CUBIC;
    if (s == "out_cubic") return animation::EasingType::OUT_CUBIC;
    if (s == "in_out_cubic") return animation::EasingType::IN_OUT_CUBIC;
    if (s == "in_sine") return animation::EasingType::IN_SINE;
    if (s == "out_sine") return animation::EasingType::OUT_SINE;
    if (s == "in_out_sine") return animation::EasingType::IN_OUT_SINE;
    return animation::EasingType::LINEAR;
}

// Generate unique config name for Hash-based configs
static std::string generate_config_name() {
    static int counter = 0;
    std::ostringstream oss;
    oss << "_ruby_config_" << counter++;
    return oss.str();
}

// Parse a Ruby Hash into an EmitterConfig
static particle::EmitterConfig parse_config_from_hash(mrb_state* mrb, mrb_value hash) {
    particle::EmitterConfig cfg;

    // Name (optional, will be auto-generated if not provided)
    std::string name = extract_string(mrb, hash_get_sym(mrb, hash, "name"));
    cfg.name = name.empty() ? generate_config_name() : name;

    // Texture
    cfg.texture_path = extract_string(mrb, hash_get_sym(mrb, hash, "texture"));
    cfg.spritesheet_cols = extract_int(mrb, hash_get_sym(mrb, hash, "spritesheet_cols"), 1);
    cfg.spritesheet_rows = extract_int(mrb, hash_get_sym(mrb, hash, "spritesheet_rows"), 1);
    // Also accept shorter aliases
    if (cfg.spritesheet_cols == 1) {
        cfg.spritesheet_cols = extract_int(mrb, hash_get_sym(mrb, hash, "columns"), 1);
    }
    if (cfg.spritesheet_rows == 1) {
        cfg.spritesheet_rows = extract_int(mrb, hash_get_sym(mrb, hash, "rows"), 1);
    }
    cfg.frame_width = extract_float(mrb, hash_get_sym(mrb, hash, "frame_width"), 0);
    cfg.frame_height = extract_float(mrb, hash_get_sym(mrb, hash, "frame_height"), 0);

    // Emission
    cfg.spawn_rate = extract_float(mrb, hash_get_sym(mrb, hash, "spawn_rate"), 10.0f);
    cfg.burst_count = extract_int(mrb, hash_get_sym(mrb, hash, "burst_count"), 0);
    cfg.burst_interval = extract_float(mrb, hash_get_sym(mrb, hash, "burst_interval"), 0.0f);
    cfg.max_particles = static_cast<size_t>(extract_int(mrb, hash_get_sym(mrb, hash, "max_particles"), 100));

    // Lifetime
    cfg.lifetime = extract_float_range(mrb, hash_get_sym(mrb, hash, "lifetime"));
    if (cfg.lifetime.min == 0.0f && cfg.lifetime.max == 0.0f) {
        cfg.lifetime = particle::FloatRange(1.0f, 2.0f);
    }

    // Shape
    std::string shape_str = extract_string(mrb, hash_get_sym(mrb, hash, "shape"));
    cfg.shape = parse_shape_string(shape_str);
    cfg.shape_radius = extract_float(mrb, hash_get_sym(mrb, hash, "shape_radius"), 0.0f);
    cfg.shape_size = extract_vec2_from_hash(mrb, hash_get_sym(mrb, hash, "shape_size"));

    // Velocity
    std::string velocity_mode_str = extract_string(mrb, hash_get_sym(mrb, hash, "velocity_mode"));
    cfg.velocity_mode = parse_velocity_mode_string(velocity_mode_str);
    cfg.direction = extract_float(mrb, hash_get_sym(mrb, hash, "direction"), 0.0f);
    cfg.spread = extract_float_range(mrb, hash_get_sym(mrb, hash, "spread"));
    cfg.speed = extract_float_range(mrb, hash_get_sym(mrb, hash, "speed"));
    if (cfg.speed.min == 0.0f && cfg.speed.max == 0.0f) {
        cfg.speed = particle::FloatRange(1.0f, 2.0f);
    }

    // Forces
    cfg.gravity = extract_vec2_from_hash(mrb, hash_get_sym(mrb, hash, "gravity"));
    cfg.radial_accel = extract_float(mrb, hash_get_sym(mrb, hash, "radial_accel"), 0.0f);
    cfg.tangent_accel = extract_float(mrb, hash_get_sym(mrb, hash, "tangent_accel"), 0.0f);
    cfg.drag = extract_float(mrb, hash_get_sym(mrb, hash, "drag"), 0.0f);

    // Size
    cfg.start_size = extract_float_range(mrb, hash_get_sym(mrb, hash, "start_size"));
    if (cfg.start_size.min == 0.0f && cfg.start_size.max == 0.0f) {
        cfg.start_size = particle::FloatRange(0.5f, 1.0f);
    }
    cfg.end_size = extract_float_range(mrb, hash_get_sym(mrb, hash, "end_size"));
    std::string size_easing_str = extract_string(mrb, hash_get_sym(mrb, hash, "size_easing"));
    cfg.size_easing = parse_easing_string(size_easing_str);

    // Rotation
    cfg.start_rotation = extract_float_range(mrb, hash_get_sym(mrb, hash, "start_rotation"));
    cfg.angular_velocity = extract_float_range(mrb, hash_get_sym(mrb, hash, "angular_velocity"));

    // Color
    cfg.start_color = extract_color_range(mrb, hash_get_sym(mrb, hash, "start_color"));
    cfg.end_color = extract_color_range(mrb, hash_get_sym(mrb, hash, "end_color"));
    std::string color_easing_str = extract_string(mrb, hash_get_sym(mrb, hash, "color_easing"));
    cfg.color_easing = parse_easing_string(color_easing_str);

    // Rendering
    cfg.layer = static_cast<uint8_t>(extract_int(mrb, hash_get_sym(mrb, hash, "layer"), 150));
    cfg.z = extract_float(mrb, hash_get_sym(mrb, hash, "z"), 0.0f);
    cfg.additive_blend = extract_bool(mrb, hash_get_sym(mrb, hash, "additive_blend"), false);

    // Behavior
    cfg.world_space = extract_bool(mrb, hash_get_sym(mrb, hash, "world_space"), true);
    cfg.scaled = extract_bool(mrb, hash_get_sym(mrb, hash, "scaled"), true);

    return cfg;
}

// ============================================================================
// ParticleEmitter Class
// ============================================================================

/// @class GMR::ParticleEmitter
/// @description A particle emitter that spawns and manages particles.
///   Emitters can be attached to a Transform2D to follow game objects,
///   or positioned manually. Effects are defined in JSON config files.
/// @example # Fire-and-forget explosion
///   ParticleEmitter.emit("effects/explosion.json", position: Vec2.new(x, y))
///
/// @example # Continuous smoke trail attached to player
///   @smoke = ParticleEmitter.new("effects/smoke.json", @player.transform)
///   @smoke.start
///
/// @example # Burst on demand
///   @sparks = ParticleEmitter.new("effects/sparks.json")
///   @sparks.position = Vec2.new(x, y)
///   @sparks.burst(20)

struct ParticleEmitterData {
    particle::EmitterHandle handle;
};

static void particle_emitter_free(mrb_state* mrb, void* ptr) {
    auto* data = static_cast<ParticleEmitterData*>(ptr);
    if (data) {
        // Destroy emitter in manager
        particle::ParticleManager::instance().destroy(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type particle_emitter_data_type = {
    "ParticleEmitter", particle_emitter_free
};

static RClass* particle_emitter_class_ptr = nullptr;

static ParticleEmitterData* get_emitter_data(mrb_state* mrb, mrb_value self) {
    return static_cast<ParticleEmitterData*>(mrb_data_get_ptr(mrb, self, &particle_emitter_data_type));
}

// Helper to get TransformHandle from a Ruby Transform2D object
static TransformHandle get_transform_handle_from_mrb(mrb_state* mrb, mrb_value transform_val) {
    if (mrb_nil_p(transform_val)) return INVALID_HANDLE;

    // Check if it's a Transform2D object with a handle
    if (mrb_respond_to(mrb, transform_val, mrb_intern_cstr(mrb, "handle"))) {
        mrb_value handle_val = mrb_funcall(mrb, transform_val, "handle", 0);
        if (mrb_integer_p(handle_val)) {
            return static_cast<TransformHandle>(mrb_integer(handle_val));
        }
    }
    return INVALID_HANDLE;
}

// Create a Ruby ParticleEmitter object wrapping a handle
static mrb_value create_emitter_object(mrb_state* mrb, particle::EmitterHandle handle) {
    if (!particle_emitter_class_ptr) {
        RClass* gmr = get_gmr_module(mrb);
        particle_emitter_class_ptr = mrb_class_get_under(mrb, gmr, "ParticleEmitter");
    }

    auto* data = static_cast<ParticleEmitterData*>(mrb_malloc(mrb, sizeof(ParticleEmitterData)));
    data->handle = handle;
    mrb_value obj = mrb_obj_value(mrb_data_object_alloc(mrb, particle_emitter_class_ptr, data, &particle_emitter_data_type));

    // Register with GC to keep alive
    mrb_gc_register(mrb, obj);

    // Store reference back to Ruby object in emitter state
    particle::EmitterState* state = particle::ParticleManager::instance().get(handle);
    if (state) {
        state->ruby_emitter_obj = obj;
    }

    return obj;
}

// ============================================================================
// Class Methods
// ============================================================================

/// @function emit
/// @description Fire a one-shot particle effect at a position. The emitter
///   auto-destroys when all particles die.
/// @param config_path [String] Path to the effect JSON file
/// @param position: [Vec2] (optional) World position for the effect
/// @param transform: [Transform2D] (optional) Transform to attach to
/// @param block [Block] (optional) Callback when effect completes
/// @returns [ParticleEmitter] The emitter (auto-cleans when done)
/// @example ParticleEmitter.emit("effects/explosion.json", position: pos)
/// @example ParticleEmitter.emit("effects/dust.json", transform: @player.transform) { on_done }
static mrb_value mrb_particle_emit(mrb_state* mrb, mrb_value) {
    const char* config_path;
    mrb_value opts = mrb_nil_value();
    mrb_value block = mrb_nil_value();

    mrb_get_args(mrb, "z|H&", &config_path, &opts, &block);

    auto& manager = particle::ParticleManager::instance();

    // Load config
    std::string name = manager.load_config(config_path);
    if (name.empty()) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load particle config: %s", config_path);
        return mrb_nil_value();
    }

    const particle::EmitterConfig* config = manager.get_config(name);
    if (!config) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Particle config not found: %s", config_path);
        return mrb_nil_value();
    }

    // Parse options
    Vec2 position{0, 0};
    TransformHandle transform = INVALID_HANDLE;
    bool has_position = false;

    if (!mrb_nil_p(opts)) {
        // position:
        mrb_value pos_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "position")));
        if (!mrb_nil_p(pos_val)) {
            position = extract_vec2(mrb, pos_val);
            has_position = true;
        }

        // transform:
        mrb_value transform_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "transform")));
        if (!mrb_nil_p(transform_val)) {
            transform = get_transform_handle_from_mrb(mrb, transform_val);
        }
    }

    // Create emitter
    particle::EmitterHandle handle;
    if (transform != INVALID_HANDLE) {
        handle = manager.emit(name, transform, true);
    } else {
        handle = manager.emit(name, position, true);
    }

    if (handle == particle::INVALID_EMITTER_HANDLE) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create particle emitter");
        return mrb_nil_value();
    }

    // Set callback if provided
    if (!mrb_nil_p(block)) {
        particle::EmitterState* state = manager.get(handle);
        if (state) {
            mrb_gc_register(mrb, block);
            state->on_complete = block;
        }
    }

    return create_emitter_object(mrb, handle);
}

/// @function preload
/// @description Load a particle config file without creating an emitter.
///   Use this to avoid loading hitches when spawning effects.
/// @param config_path [String] Path to the effect JSON file
/// @returns [Boolean] true if loaded successfully
/// @example ParticleEmitter.preload("effects/explosion.json")
static mrb_value mrb_particle_preload(mrb_state* mrb, mrb_value) {
    const char* config_path;
    mrb_get_args(mrb, "z", &config_path);

    std::string name = particle::ParticleManager::instance().load_config(config_path);
    return to_mrb_bool(mrb, !name.empty());
}

/// @function total_count
/// @description Get the total number of allocated particles across all emitters.
/// @returns [Integer] Total particle count
/// @example puts "Particles: #{ParticleEmitter.total_count}"
static mrb_value mrb_particle_total_count(mrb_state* mrb, mrb_value) {
    return mrb_int_value(mrb, static_cast<mrb_int>(particle::ParticleManager::instance().total_particle_count()));
}

/// @function alive_count
/// @description Get the number of currently alive particles across all emitters.
/// @returns [Integer] Alive particle count
/// @example puts "Alive: #{ParticleEmitter.alive_count}"
static mrb_value mrb_particle_alive_count(mrb_state* mrb, mrb_value) {
    return mrb_int_value(mrb, static_cast<mrb_int>(particle::ParticleManager::instance().alive_particle_count()));
}

/// @function emitter_count
/// @description Get the number of active emitters.
/// @returns [Integer] Emitter count
static mrb_value mrb_particle_emitter_count(mrb_state* mrb, mrb_value) {
    return mrb_int_value(mrb, static_cast<mrb_int>(particle::ParticleManager::instance().emitter_count()));
}

// ============================================================================
// Constructor
// ============================================================================

/// @method initialize
/// @description Create a new particle emitter from a JSON config file or a Hash.
///   The emitter starts in a stopped state; call start() or burst() to emit.
/// @param config [String, Hash] Path to JSON config file, or a Hash with config options
/// @param transform [Transform2D] (optional) Transform to attach to
/// @returns [ParticleEmitter] The new emitter
/// @example # From JSON file
///   @emitter = ParticleEmitter.new("effects/smoke.json")
///   @trail = ParticleEmitter.new("effects/dust.json", @player.transform)
/// @example # From Hash (declarative)
///   @emitter = ParticleEmitter.new({
///     texture: "particles.jpg",
///     columns: 6, rows: 6,
///     spawn_rate: 10,
///     max_particles: 50,
///     lifetime: { min: 0.5, max: 1.0 },
///     start_size: { min: 0.3, max: 0.5 },
///     end_size: 0,
///     start_color: { r: 255, g: 200, b: 100, a: 255 },
///     end_color: { r: 255, g: 100, b: 0, a: 0 },
///     velocity_mode: "radial",
///     speed: { min: 1.0, max: 2.0 },
///     gravity: { x: 0, y: 2.0 }
///   })
static mrb_value mrb_particle_emitter_initialize(mrb_state* mrb, mrb_value self) {
    mrb_value config_val;
    mrb_value transform_val = mrb_nil_value();

    mrb_get_args(mrb, "o|o", &config_val, &transform_val);

    auto& manager = particle::ParticleManager::instance();
    const particle::EmitterConfig* config = nullptr;
    std::string config_name;

    if (mrb_string_p(config_val)) {
        // String path to JSON file
        const char* config_path = RSTRING_PTR(config_val);

        config_name = manager.load_config(config_path);
        if (config_name.empty()) {
            mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load particle config: %s", config_path);
            return mrb_nil_value();
        }

        config = manager.get_config(config_name);
        if (!config) {
            mrb_raisef(mrb, E_RUNTIME_ERROR, "Particle config not found: %s", config_path);
            return mrb_nil_value();
        }
    } else if (mrb_hash_p(config_val)) {
        // Hash with config options
        particle::EmitterConfig cfg = parse_config_from_hash(mrb, config_val);
        config_name = cfg.name;

        // Load texture if specified
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
            }
        }

        // Register the config
        manager.register_config(config_name, std::move(cfg));
        config = manager.get_config(config_name);
        if (!config) {
            mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to register particle config from Hash");
            return mrb_nil_value();
        }
    } else {
        mrb_raise(mrb, E_TYPE_ERROR, "ParticleEmitter.new expects a String (JSON path) or Hash (config)");
        return mrb_nil_value();
    }

    // Create emitter
    particle::EmitterHandle handle = manager.create(config);
    if (handle == particle::INVALID_EMITTER_HANDLE) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create particle emitter");
        return mrb_nil_value();
    }

    particle::EmitterState* state = manager.get(handle);
    if (!state) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to get emitter state");
        return mrb_nil_value();
    }

    // Attach to transform if provided
    if (!mrb_nil_p(transform_val)) {
        TransformHandle t_handle = get_transform_handle_from_mrb(mrb, transform_val);
        if (t_handle != INVALID_HANDLE) {
            state->attached_transform = t_handle;
            state->position = TransformManager::instance().get_world_position(t_handle);
            state->last_position = state->position;
        }
        // Store transform reference in instance variable
        mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@transform"), transform_val);
    }

    // Wrap handle in data
    auto* data = static_cast<ParticleEmitterData*>(mrb_malloc(mrb, sizeof(ParticleEmitterData)));
    data->handle = handle;
    mrb_data_init(self, data, &particle_emitter_data_type);

    // GC registration
    mrb_gc_register(mrb, self);
    state->ruby_emitter_obj = self;

    return self;
}

// ============================================================================
// Instance Methods - Control
// ============================================================================

/// @method start
/// @description Start continuous particle emission.
/// @returns [self]
/// @example @smoke.start
static mrb_value mrb_particle_emitter_start(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::ParticleManager::instance().start(data->handle);
    return self;
}

/// @method stop
/// @description Stop emitting new particles. Existing particles continue to live out.
/// @returns [self]
/// @example @smoke.stop
static mrb_value mrb_particle_emitter_stop(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::ParticleManager::instance().stop(data->handle);
    return self;
}

/// @method pause
/// @description Pause all particle updates (freeze in place).
/// @returns [self]
/// @example @smoke.pause
static mrb_value mrb_particle_emitter_pause(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::ParticleManager::instance().pause(data->handle);
    return self;
}

/// @method resume
/// @description Resume paused particle updates.
/// @returns [self]
/// @example @smoke.resume
static mrb_value mrb_particle_emitter_resume(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::ParticleManager::instance().resume(data->handle);
    return self;
}

/// @method reset
/// @description Clear all particles and reset to initial state.
/// @returns [self]
/// @example @emitter.reset
static mrb_value mrb_particle_emitter_reset(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (state) {
        state->reset();
    }
    return self;
}

/// @method burst
/// @description Emit a burst of particles immediately.
/// @param count [Integer] (optional) Number of particles to emit (default: config burst_count)
/// @returns [self]
/// @example @sparks.burst
/// @example @sparks.burst(50)
static mrb_value mrb_particle_emitter_burst(mrb_state* mrb, mrb_value self) {
    mrb_int count = -1;
    mrb_get_args(mrb, "|i", &count);

    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::ParticleManager::instance().burst(data->handle, static_cast<int>(count));
    return self;
}

// ============================================================================
// Instance Methods - Properties
// ============================================================================

/// @method position
/// @description Get the emitter's world position.
/// @returns [Vec2] Current position
/// @example pos = @emitter.position
static mrb_value mrb_particle_emitter_position(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        return mrb_nil_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (!state) {
        return mrb_nil_value();
    }

    return create_vec2(mrb, state->position);
}

/// @method position=
/// @description Set the emitter's world position (only works if not attached to transform).
/// @param pos [Vec2] New position
/// @returns [Vec2] The position
/// @example @emitter.position = Vec2.new(100, 200)
static mrb_value mrb_particle_emitter_set_position(mrb_state* mrb, mrb_value self) {
    mrb_value pos_val;
    mrb_get_args(mrb, "o", &pos_val);

    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (!state) {
        return mrb_nil_value();
    }

    Vec2 pos = extract_vec2(mrb, pos_val);
    state->last_position = state->position;
    state->position = pos;

    return pos_val;
}

/// @method transform
/// @description Get the attached transform.
/// @returns [Transform2D, nil] The attached transform or nil
static mrb_value mrb_particle_emitter_transform(mrb_state* mrb, mrb_value self) {
    return mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "@transform"));
}

/// @method draw
/// @description Draw this emitter's particles. Call this in your draw method
///   to control when particles are rendered relative to other objects.
///   If you don't call draw, particles render after all Ruby drawing.
/// @returns [self]
/// @example def draw
///            @level.draw
///            @dust_emitter.draw  # Dust behind player
///            @player.draw
///          end
static mrb_value mrb_particle_emitter_draw(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) return self;

    particle::ParticleManager::instance().queue_draw_emitter(data->handle);
    return self;
}

/// @method transform=
/// @description Attach the emitter to a transform, or detach (nil).
/// @param transform [Transform2D, nil] Transform to attach to
/// @returns [Transform2D, nil] The transform
/// @example @emitter.transform = @player.transform
/// @example @emitter.transform = nil  # Detach
static mrb_value mrb_particle_emitter_set_transform(mrb_state* mrb, mrb_value self) {
    mrb_value transform_val;
    mrb_get_args(mrb, "o", &transform_val);

    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (!state) {
        return mrb_nil_value();
    }

    if (mrb_nil_p(transform_val)) {
        state->attached_transform = INVALID_HANDLE;
    } else {
        TransformHandle t_handle = get_transform_handle_from_mrb(mrb, transform_val);
        state->attached_transform = t_handle;
        if (t_handle != INVALID_HANDLE) {
            state->position = TransformManager::instance().get_world_position(t_handle);
            state->last_position = state->position;
        }
    }

    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@transform"), transform_val);
    return transform_val;
}

// ============================================================================
// Instance Methods - State Queries
// ============================================================================

/// @method emitting?
/// @description Check if the emitter is currently spawning particles.
/// @returns [Boolean] true if emitting
static mrb_value mrb_particle_emitter_emitting(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        return mrb_false_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (!state) {
        return mrb_false_value();
    }

    return to_mrb_bool(mrb, state->emitting);
}

/// @method alive?
/// @description Check if the emitter has any alive particles.
/// @returns [Boolean] true if any particles are alive
static mrb_value mrb_particle_emitter_alive(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        return mrb_false_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (!state) {
        return mrb_false_value();
    }

    return to_mrb_bool(mrb, state->alive_count > 0);
}

/// @method active?
/// @description Check if the emitter is active (not paused or destroyed).
/// @returns [Boolean] true if active
static mrb_value mrb_particle_emitter_active(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        return mrb_false_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (!state) {
        return mrb_false_value();
    }

    return to_mrb_bool(mrb, state->active);
}

/// @method particle_count
/// @description Get the number of alive particles in this emitter.
/// @returns [Integer] Particle count
static mrb_value mrb_particle_emitter_particle_count(mrb_state* mrb, mrb_value self) {
    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        return mrb_int_value(mrb, 0);
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (!state) {
        return mrb_int_value(mrb, 0);
    }

    return mrb_int_value(mrb, static_cast<mrb_int>(state->alive_count));
}

/// @method on_complete
/// @description Set a callback to be called when all particles die (for one-shot emitters).
/// @param block [Block] The callback
/// @returns [self]
/// @example @emitter.on_complete { spawn_pickup }
static mrb_value mrb_particle_emitter_on_complete(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "on_complete requires a block");
        return mrb_nil_value();
    }

    ParticleEmitterData* data = get_emitter_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid ParticleEmitter");
        return mrb_nil_value();
    }

    particle::EmitterState* state = particle::ParticleManager::instance().get(data->handle);
    if (state) {
        // Unregister old callback if any
        if (!mrb_nil_p(state->on_complete)) {
            mrb_gc_unregister(mrb, state->on_complete);
        }
        mrb_gc_register(mrb, block);
        state->on_complete = block;
    }

    return self;
}

// ============================================================================
// Registration
// ============================================================================

void register_particle(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // Define ParticleEmitter class under GMR module
    particle_emitter_class_ptr = mrb_define_class_under(mrb, gmr, "ParticleEmitter", mrb->object_class);
    MRB_SET_INSTANCE_TT(particle_emitter_class_ptr, MRB_TT_CDATA);

    // Class methods
    mrb_define_class_method(mrb, particle_emitter_class_ptr, "emit", mrb_particle_emit,
                            MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_class_method(mrb, particle_emitter_class_ptr, "preload", mrb_particle_preload,
                            MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, particle_emitter_class_ptr, "total_count", mrb_particle_total_count,
                            MRB_ARGS_NONE());
    mrb_define_class_method(mrb, particle_emitter_class_ptr, "alive_count", mrb_particle_alive_count,
                            MRB_ARGS_NONE());
    mrb_define_class_method(mrb, particle_emitter_class_ptr, "emitter_count", mrb_particle_emitter_count,
                            MRB_ARGS_NONE());

    // Constructor
    mrb_define_method(mrb, particle_emitter_class_ptr, "initialize", mrb_particle_emitter_initialize,
                      MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));

    // Control methods
    mrb_define_method(mrb, particle_emitter_class_ptr, "start", mrb_particle_emitter_start,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "stop", mrb_particle_emitter_stop,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "pause", mrb_particle_emitter_pause,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "resume", mrb_particle_emitter_resume,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "reset", mrb_particle_emitter_reset,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "burst", mrb_particle_emitter_burst,
                      MRB_ARGS_OPT(1));

    // Property accessors
    mrb_define_method(mrb, particle_emitter_class_ptr, "position", mrb_particle_emitter_position,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "position=", mrb_particle_emitter_set_position,
                      MRB_ARGS_REQ(1));
    mrb_define_method(mrb, particle_emitter_class_ptr, "transform", mrb_particle_emitter_transform,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "transform=", mrb_particle_emitter_set_transform,
                      MRB_ARGS_REQ(1));

    // Drawing
    mrb_define_method(mrb, particle_emitter_class_ptr, "draw", mrb_particle_emitter_draw,
                      MRB_ARGS_NONE());

    // State queries
    mrb_define_method(mrb, particle_emitter_class_ptr, "emitting?", mrb_particle_emitter_emitting,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "alive?", mrb_particle_emitter_alive,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "active?", mrb_particle_emitter_active,
                      MRB_ARGS_NONE());
    mrb_define_method(mrb, particle_emitter_class_ptr, "particle_count", mrb_particle_emitter_particle_count,
                      MRB_ARGS_NONE());

    // Callbacks
    mrb_define_method(mrb, particle_emitter_class_ptr, "on_complete", mrb_particle_emitter_on_complete,
                      MRB_ARGS_BLOCK());
}

} // namespace bindings
} // namespace gmr
