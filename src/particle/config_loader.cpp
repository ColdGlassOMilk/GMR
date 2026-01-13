#include "gmr/particle/config_loader.hpp"
#include "gmr/filesystem/operations.hpp"
#include "gmr/paths.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace gmr {
namespace particle {

// Simple JSON tokenizer and parser for particle configs
// Not a full JSON parser - handles the specific structure we need

namespace {

// Skip whitespace
const char* skip_ws(const char* p) {
    while (*p && std::isspace(*p)) ++p;
    return p;
}

// Parse a string (assumes p points to opening quote)
const char* parse_string(const char* p, std::string& out) {
    if (*p != '"') return nullptr;
    ++p;
    out.clear();
    while (*p && *p != '"') {
        if (*p == '\\' && *(p + 1)) {
            ++p;
            switch (*p) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                default: out += *p; break;
            }
        } else {
            out += *p;
        }
        ++p;
    }
    if (*p == '"') ++p;
    return p;
}

// Parse a number
const char* parse_number(const char* p, float& out) {
    char* end;
    out = std::strtof(p, &end);
    return end;
}

// Parse a boolean
const char* parse_bool(const char* p, bool& out) {
    if (std::strncmp(p, "true", 4) == 0) {
        out = true;
        return p + 4;
    } else if (std::strncmp(p, "false", 5) == 0) {
        out = false;
        return p + 5;
    }
    return nullptr;
}

// Parse a FloatRange: either a number or {"min": x, "max": y}
const char* parse_float_range(const char* p, FloatRange& out) {
    p = skip_ws(p);
    if (*p == '{') {
        // Object form
        ++p;
        float min_val = 0, max_val = 0;
        while (*p && *p != '}') {
            p = skip_ws(p);
            std::string key;
            p = parse_string(p, key);
            if (!p) return nullptr;
            p = skip_ws(p);
            if (*p == ':') ++p;
            p = skip_ws(p);
            float val;
            p = parse_number(p, val);
            if (key == "min") min_val = val;
            else if (key == "max") max_val = val;
            p = skip_ws(p);
            if (*p == ',') ++p;
        }
        if (*p == '}') ++p;
        out = FloatRange(min_val, max_val);
    } else {
        // Single number form
        float val;
        p = parse_number(p, val);
        out = FloatRange(val);
    }
    return p;
}

// Parse a Vec2: {"x": x, "y": y}
const char* parse_vec2(const char* p, Vec2& out) {
    p = skip_ws(p);
    if (*p != '{') return nullptr;
    ++p;
    out = {0, 0};
    while (*p && *p != '}') {
        p = skip_ws(p);
        std::string key;
        p = parse_string(p, key);
        if (!p) return nullptr;
        p = skip_ws(p);
        if (*p == ':') ++p;
        p = skip_ws(p);
        float val;
        p = parse_number(p, val);
        if (key == "x") out.x = val;
        else if (key == "y") out.y = val;
        p = skip_ws(p);
        if (*p == ',') ++p;
    }
    if (*p == '}') ++p;
    return p;
}

// Parse a Color: {"r": r, "g": g, "b": b, "a": a}
const char* parse_color(const char* p, Color& out) {
    p = skip_ws(p);
    if (*p != '{') return nullptr;
    ++p;
    out = {255, 255, 255, 255};
    while (*p && *p != '}') {
        p = skip_ws(p);
        std::string key;
        p = parse_string(p, key);
        if (!p) return nullptr;
        p = skip_ws(p);
        if (*p == ':') ++p;
        p = skip_ws(p);
        float val;
        p = parse_number(p, val);
        if (key == "r") out.r = static_cast<uint8_t>(val);
        else if (key == "g") out.g = static_cast<uint8_t>(val);
        else if (key == "b") out.b = static_cast<uint8_t>(val);
        else if (key == "a") out.a = static_cast<uint8_t>(val);
        p = skip_ws(p);
        if (*p == ',') ++p;
    }
    if (*p == '}') ++p;
    return p;
}

// Parse a ColorRange: either a Color or {"min": Color, "max": Color}
const char* parse_color_range(const char* p, ColorRange& out) {
    p = skip_ws(p);
    if (*p != '{') return nullptr;

    // Peek ahead to see if this is a single color or a range
    const char* peek = p + 1;
    peek = skip_ws(peek);
    std::string first_key;
    const char* after_key = parse_string(peek, first_key);

    if (first_key == "min" || first_key == "max") {
        // Range form: {"min": {...}, "max": {...}}
        ++p; // skip '{'
        Color min_col{255, 255, 255, 255}, max_col{255, 255, 255, 255};
        while (*p && *p != '}') {
            p = skip_ws(p);
            std::string key;
            p = parse_string(p, key);
            if (!p) return nullptr;
            p = skip_ws(p);
            if (*p == ':') ++p;
            p = skip_ws(p);
            Color col;
            p = parse_color(p, col);
            if (!p) return nullptr;
            if (key == "min") min_col = col;
            else if (key == "max") max_col = col;
            p = skip_ws(p);
            if (*p == ',') ++p;
        }
        if (*p == '}') ++p;
        out = ColorRange(min_col, max_col);
    } else {
        // Single color form: {"r": ..., "g": ...}
        Color col;
        p = parse_color(p, col);
        if (!p) return nullptr;
        out = ColorRange(col);
    }
    return p;
}

// Parse a Rect: {"x": x, "y": y, "w": w, "h": h}
const char* parse_rect(const char* p, Rect& out) {
    p = skip_ws(p);
    if (*p != '{') return nullptr;
    ++p;
    out = {0, 0, 0, 0};
    while (*p && *p != '}') {
        p = skip_ws(p);
        std::string key;
        p = parse_string(p, key);
        if (!p) return nullptr;
        p = skip_ws(p);
        if (*p == ':') ++p;
        p = skip_ws(p);
        float val;
        p = parse_number(p, val);
        if (key == "x") out.x = val;
        else if (key == "y") out.y = val;
        else if (key == "w" || key == "width") out.width = val;
        else if (key == "h" || key == "height") out.height = val;
        p = skip_ws(p);
        if (*p == ',') ++p;
    }
    if (*p == '}') ++p;
    return p;
}

// Parse EmissionShape from string
EmissionShape parse_emission_shape(const std::string& s) {
    if (s == "point") return EmissionShape::POINT;
    if (s == "circle") return EmissionShape::CIRCLE;
    if (s == "circle_edge") return EmissionShape::CIRCLE_EDGE;
    if (s == "rectangle" || s == "rect") return EmissionShape::RECTANGLE;
    if (s == "rectangle_edge" || s == "rect_edge") return EmissionShape::RECTANGLE_EDGE;
    if (s == "line") return EmissionShape::LINE;
    return EmissionShape::POINT;
}

// Parse VelocityMode from string
VelocityMode parse_velocity_mode(const std::string& s) {
    if (s == "directional") return VelocityMode::DIRECTIONAL;
    if (s == "radial") return VelocityMode::RADIAL;
    if (s == "tangential") return VelocityMode::TANGENTIAL;
    if (s == "random") return VelocityMode::RANDOM;
    return VelocityMode::DIRECTIONAL;
}

// Parse EasingType from string
animation::EasingType parse_easing(const std::string& s) {
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
    if (s == "in_expo") return animation::EasingType::IN_EXPO;
    if (s == "out_expo") return animation::EasingType::OUT_EXPO;
    if (s == "in_out_expo") return animation::EasingType::IN_OUT_EXPO;
    if (s == "in_back") return animation::EasingType::IN_BACK;
    if (s == "out_back") return animation::EasingType::OUT_BACK;
    if (s == "in_out_back") return animation::EasingType::IN_OUT_BACK;
    if (s == "in_elastic") return animation::EasingType::IN_ELASTIC;
    if (s == "out_elastic") return animation::EasingType::OUT_ELASTIC;
    if (s == "in_out_elastic") return animation::EasingType::IN_OUT_ELASTIC;
    if (s == "in_bounce") return animation::EasingType::IN_BOUNCE;
    if (s == "out_bounce") return animation::EasingType::OUT_BOUNCE;
    if (s == "in_out_bounce") return animation::EasingType::IN_OUT_BOUNCE;
    return animation::EasingType::LINEAR;
}

} // anonymous namespace

std::optional<EmitterConfig> parse_config_json(const std::string& json_str) {
    EmitterConfig config;
    const char* p = json_str.c_str();

    p = skip_ws(p);
    if (*p != '{') return std::nullopt;
    ++p;

    while (*p && *p != '}') {
        p = skip_ws(p);
        if (*p == '}') break;

        // Parse key
        std::string key;
        p = parse_string(p, key);
        if (!p) return std::nullopt;

        p = skip_ws(p);
        if (*p == ':') ++p;
        p = skip_ws(p);

        // Parse value based on key
        if (key == "name") {
            p = parse_string(p, config.name);
        } else if (key == "texture" || key == "texture_path") {
            p = parse_string(p, config.texture_path);
        } else if (key == "source_rect") {
            p = parse_rect(p, config.source_rect);
        } else if (key == "spritesheet_cols" || key == "columns") {
            float val;
            p = parse_number(p, val);
            config.spritesheet_cols = static_cast<int>(val);
        } else if (key == "spritesheet_rows" || key == "rows") {
            float val;
            p = parse_number(p, val);
            config.spritesheet_rows = static_cast<int>(val);
        } else if (key == "frame_width") {
            p = parse_number(p, config.frame_width);
        } else if (key == "frame_height") {
            p = parse_number(p, config.frame_height);
        } else if (key == "spawn_rate") {
            p = parse_number(p, config.spawn_rate);
        } else if (key == "burst_count") {
            float val;
            p = parse_number(p, val);
            config.burst_count = static_cast<int>(val);
        } else if (key == "burst_interval") {
            p = parse_number(p, config.burst_interval);
        } else if (key == "max_particles") {
            float val;
            p = parse_number(p, val);
            config.max_particles = static_cast<size_t>(val);
        } else if (key == "lifetime") {
            p = parse_float_range(p, config.lifetime);
        } else if (key == "shape") {
            std::string shape_str;
            p = parse_string(p, shape_str);
            config.shape = parse_emission_shape(shape_str);
        } else if (key == "shape_radius") {
            p = parse_number(p, config.shape_radius);
        } else if (key == "shape_size") {
            p = parse_vec2(p, config.shape_size);
        } else if (key == "velocity_mode") {
            std::string mode_str;
            p = parse_string(p, mode_str);
            config.velocity_mode = parse_velocity_mode(mode_str);
        } else if (key == "direction") {
            p = parse_number(p, config.direction);
        } else if (key == "spread") {
            p = parse_float_range(p, config.spread);
        } else if (key == "speed") {
            p = parse_float_range(p, config.speed);
        } else if (key == "gravity") {
            p = parse_vec2(p, config.gravity);
        } else if (key == "radial_accel") {
            p = parse_number(p, config.radial_accel);
        } else if (key == "tangent_accel") {
            p = parse_number(p, config.tangent_accel);
        } else if (key == "drag") {
            p = parse_number(p, config.drag);
        } else if (key == "start_size") {
            p = parse_float_range(p, config.start_size);
        } else if (key == "end_size") {
            p = parse_float_range(p, config.end_size);
        } else if (key == "size_easing") {
            std::string easing_str;
            p = parse_string(p, easing_str);
            config.size_easing = parse_easing(easing_str);
        } else if (key == "start_rotation") {
            p = parse_float_range(p, config.start_rotation);
        } else if (key == "angular_velocity") {
            p = parse_float_range(p, config.angular_velocity);
        } else if (key == "start_color") {
            p = parse_color_range(p, config.start_color);
        } else if (key == "end_color") {
            p = parse_color_range(p, config.end_color);
        } else if (key == "color_easing") {
            std::string easing_str;
            p = parse_string(p, easing_str);
            config.color_easing = parse_easing(easing_str);
        } else if (key == "layer") {
            float val;
            p = parse_number(p, val);
            config.layer = static_cast<uint8_t>(val);
        } else if (key == "z") {
            p = parse_number(p, config.z);
        } else if (key == "additive_blend") {
            p = parse_bool(p, config.additive_blend);
        } else if (key == "world_space") {
            p = parse_bool(p, config.world_space);
        } else if (key == "scaled") {
            p = parse_bool(p, config.scaled);
        } else {
            // Skip unknown value
            int depth = 0;
            bool in_string = false;
            while (*p) {
                if (*p == '"' && (p == json_str.c_str() || *(p - 1) != '\\')) {
                    in_string = !in_string;
                } else if (!in_string) {
                    if (*p == '{' || *p == '[') depth++;
                    else if (*p == '}' || *p == ']') {
                        if (depth == 0) break;
                        depth--;
                    } else if (*p == ',' && depth == 0) break;
                }
                ++p;
            }
        }

        if (!p) return std::nullopt;
        p = skip_ws(p);
        if (*p == ',') ++p;
    }

    return config;
}

std::optional<EmitterConfig> load_config_from_file(const std::string& path) {
    // Try loading from assets
    auto content = filesystem::read_text(path, filesystem::Root::Assets);
    if (!content) {
        // Try with "data/" prefix if not found
        content = filesystem::read_text("data/" + path, filesystem::Root::Assets);
    }

    if (!content) {
        return std::nullopt;
    }

    auto config = parse_config_json(*content);
    if (config && config->name.empty()) {
        // Extract name from filename if not specified
        size_t last_slash = path.find_last_of("/\\");
        size_t start = (last_slash == std::string::npos) ? 0 : last_slash + 1;
        size_t last_dot = path.rfind('.');
        if (last_dot != std::string::npos && last_dot > start) {
            config->name = path.substr(start, last_dot - start);
        } else {
            config->name = path.substr(start);
        }
    }

    return config;
}

} // namespace particle
} // namespace gmr
