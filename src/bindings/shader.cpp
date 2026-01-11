#include "gmr/bindings/shader.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/resources/shader_manager.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/scripting/helpers.hpp"
#include "raylib.h"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <cstring>

namespace gmr {
namespace bindings {

/// @class Shader
/// @description A GPU shader for custom rendering effects.
///   Load shaders from GLSL files or source strings, set uniform values,
///   and apply them to any draws using the `use { }` block syntax.
/// @example # Basic post-processing effect
///   def init
///     @shader = GMR::Shader.load(fragment: "shaders/grayscale.fs")
///     @sprite = Sprite.new("textures/player.png")
///   end
///
///   def draw
///     @shader.set(:intensity, 0.5)
///     @shader.use do
///       @sprite.draw
///     end
///   end
/// @example # Animated shader with time uniform
///   def init
///     @shader = GMR::Shader.load(fragment: "shaders/wave.fs")
///   end
///
///   def draw
///     @shader.set(:time, GMR::Time.elapsed)
///     @shader.set(:resolution, Window.width.to_f, Window.height.to_f)
///     @shader.use do
///       @level.draw
///     end
///   end
/// @example # Shader from source code
///   fragment_code = <<~GLSL
///     #version 330
///     in vec2 fragTexCoord;
///     in vec4 fragColor;
///     uniform sampler2D texture0;
///     uniform float intensity;
///     out vec4 finalColor;
///
///     void main() {
///       vec4 texel = texture(texture0, fragTexCoord);
///       float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
///       finalColor = vec4(mix(texel.rgb, vec3(gray), intensity), texel.a) * fragColor;
///     }
///   GLSL
///
///   @shader = GMR::Shader.from_source(fragment: fragment_code)

// ============================================================================
// Shader Binding Data
// ============================================================================

struct ShaderData {
    ShaderHandle handle;
};

static void shader_free(mrb_state* mrb, void* ptr) {
    ShaderData* data = static_cast<ShaderData*>(ptr);
    if (data) {
        ShaderManager::instance().release(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type shader_data_type = {
    "Shader", shader_free
};

static ShaderData* get_shader_data(mrb_state* mrb, mrb_value self) {
    return static_cast<ShaderData*>(mrb_data_get_ptr(mrb, self, &shader_data_type));
}

// External texture data type for sampler2d uniforms
extern const mrb_data_type texture_data_type;

// Helper struct matching graphics.cpp's TextureData
struct TextureBindingData {
    TextureHandle handle;
};

// ============================================================================
// Class Methods
// ============================================================================

/// @classmethod load
/// @description Load a shader from file paths. The fragment shader is required;
///   vertex shader is optional (uses raylib default if not provided).
/// @param fragment [String] Path to fragment shader file (required)
/// @param vertex [String] Path to vertex shader file (optional)
/// @returns [Shader] The loaded shader
/// @raises [RuntimeError] if shader compilation fails
/// @example shader = GMR::Shader.load(fragment: "shaders/blur.fs")
/// @example shader = GMR::Shader.load(vertex: "shaders/custom.vs", fragment: "shaders/custom.fs")
static mrb_value mrb_shader_load(mrb_state* mrb, mrb_value klass) {
    mrb_value kwargs;
    mrb_get_args(mrb, "H", &kwargs);

    const char* vertex_path = nullptr;
    const char* fragment_path = nullptr;

    // Parse keyword arguments
    mrb_value keys = mrb_hash_keys(mrb, kwargs);
    mrb_int len = RARRAY_LEN(keys);

    for (mrb_int i = 0; i < len; i++) {
        mrb_value key = mrb_ary_ref(mrb, keys, i);
        mrb_value val = mrb_hash_get(mrb, kwargs, key);

        const char* key_name = nullptr;
        if (mrb_symbol_p(key)) {
            key_name = mrb_sym_name(mrb, mrb_symbol(key));
        } else if (mrb_string_p(key)) {
            key_name = mrb_string_cstr(mrb, key);
        }

        if (key_name) {
            if (strcmp(key_name, "vertex") == 0 && !mrb_nil_p(val)) {
                vertex_path = mrb_string_cstr(mrb, val);
            } else if (strcmp(key_name, "fragment") == 0 && !mrb_nil_p(val)) {
                fragment_path = mrb_string_cstr(mrb, val);
            }
        }
    }

    if (!fragment_path) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "fragment: keyword argument is required");
        return mrb_nil_value();
    }

    ShaderHandle handle = ShaderManager::instance().load(
        fragment_path,
        vertex_path ? vertex_path : ""
    );

    if (handle == INVALID_HANDLE) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "Failed to load shader '%s'. Check GLSL compilation errors above.",
            fragment_path);
        return mrb_nil_value();
    }

    // Create Ruby object
    RClass* shader_class = mrb_class_ptr(klass);
    mrb_value obj = mrb_obj_new(mrb, shader_class, 0, nullptr);

    ShaderData* data = static_cast<ShaderData*>(mrb_malloc(mrb, sizeof(ShaderData)));
    data->handle = handle;
    mrb_data_init(obj, data, &shader_data_type);

    return obj;
}

/// @classmethod from_source
/// @description Load a shader from GLSL source code strings.
///   The fragment shader is required; vertex shader is optional.
/// @param fragment [String] Fragment shader source code (required)
/// @param vertex [String] Vertex shader source code (optional)
/// @returns [Shader] The compiled shader
/// @raises [RuntimeError] if shader compilation fails
/// @example shader = GMR::Shader.from_source(fragment: glsl_code)
static mrb_value mrb_shader_from_source(mrb_state* mrb, mrb_value klass) {
    mrb_value kwargs;
    mrb_get_args(mrb, "H", &kwargs);

    const char* vertex_code = nullptr;
    const char* fragment_code = nullptr;

    // Parse keyword arguments
    mrb_value keys = mrb_hash_keys(mrb, kwargs);
    mrb_int len = RARRAY_LEN(keys);

    for (mrb_int i = 0; i < len; i++) {
        mrb_value key = mrb_ary_ref(mrb, keys, i);
        mrb_value val = mrb_hash_get(mrb, kwargs, key);

        const char* key_name = nullptr;
        if (mrb_symbol_p(key)) {
            key_name = mrb_sym_name(mrb, mrb_symbol(key));
        } else if (mrb_string_p(key)) {
            key_name = mrb_string_cstr(mrb, key);
        }

        if (key_name) {
            if (strcmp(key_name, "vertex") == 0 && !mrb_nil_p(val)) {
                vertex_code = mrb_string_cstr(mrb, val);
            } else if (strcmp(key_name, "fragment") == 0 && !mrb_nil_p(val)) {
                fragment_code = mrb_string_cstr(mrb, val);
            }
        }
    }

    if (!fragment_code) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "fragment: keyword argument is required");
        return mrb_nil_value();
    }

    ShaderHandle handle = ShaderManager::instance().load_from_memory(
        fragment_code,
        vertex_code ? vertex_code : ""
    );

    if (handle == INVALID_HANDLE) {
        mrb_raise(mrb, E_RUNTIME_ERROR,
            "Failed to compile shader from source. Check GLSL errors above.");
        return mrb_nil_value();
    }

    // Create Ruby object
    RClass* shader_class = mrb_class_ptr(klass);
    mrb_value obj = mrb_obj_new(mrb, shader_class, 0, nullptr);

    ShaderData* data = static_cast<ShaderData*>(mrb_malloc(mrb, sizeof(ShaderData)));
    data->handle = handle;
    mrb_data_init(obj, data, &shader_data_type);

    return obj;
}

// ============================================================================
// Instance Methods
// ============================================================================

/// @method set
/// @description Set a uniform value. Type is automatically inferred from arguments.
/// @param name [Symbol, String] The uniform name in the shader
/// @param values [Float, Integer, Texture, ...] Value(s) to set
/// @returns [Shader] self for chaining
/// @raises [ArgumentError] if uniform name doesn't exist in shader
/// @example shader.set(:time, 1.5)                      # float
/// @example shader.set(:resolution, 800.0, 600.0)       # vec2
/// @example shader.set(:color, 1.0, 0.5, 0.0)           # vec3
/// @example shader.set(:tint, 1.0, 0.5, 0.0, 1.0)       # vec4
/// @example shader.set(:count, 5)                       # int
/// @example shader.set(:grid, 4, 4)                     # ivec2
/// @example shader.set(:noise_tex, noise_texture)       # sampler2D
static mrb_value mrb_shader_set(mrb_state* mrb, mrb_value self) {
    mrb_value name_val;
    mrb_value* args;
    mrb_int argc;
    mrb_get_args(mrb, "o*", &name_val, &args, &argc);

    ShaderData* data = get_shader_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Shader object");
        return mrb_nil_value();
    }

    ShaderState* shader = ShaderManager::instance().get(data->handle);
    if (!shader) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Shader has been released");
        return mrb_nil_value();
    }

    // Get uniform name as string
    const char* name;
    if (mrb_symbol_p(name_val)) {
        name = mrb_sym_name(mrb, mrb_symbol(name_val));
    } else if (mrb_string_p(name_val)) {
        name = mrb_string_cstr(mrb, name_val);
    } else {
        mrb_raise(mrb, E_TYPE_ERROR, "Uniform name must be Symbol or String");
        return mrb_nil_value();
    }

    // Get uniform location (cached internally)
    int location = shader->get_location(name);
    if (location == -1) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "Unknown uniform '%s'. Check spelling and ensure it's used in shader.", name);
        return mrb_nil_value();
    }

    // Type inference based on argument count and types
    if (argc == 1) {
        mrb_value val = args[0];

        // Check if Texture object
        if (mrb_data_p(val)) {
            void* ptr = mrb_data_check_get_ptr(mrb, val, &texture_data_type);
            if (ptr) {
                TextureBindingData* tex_data = static_cast<TextureBindingData*>(ptr);
                Texture2D* texture = TextureManager::instance().get(tex_data->handle);
                if (texture) {
                    SetShaderValueTexture(shader->raylib_shader, location, *texture);
                }
                return self;
            }
        }

        // Float or Int
        if (mrb_integer_p(val)) {
            int ival = static_cast<int>(mrb_integer(val));
            SetShaderValue(shader->raylib_shader, location, &ival, SHADER_UNIFORM_INT);
        } else {
            float fval = static_cast<float>(mrb_as_float(mrb, val));
            SetShaderValue(shader->raylib_shader, location, &fval, SHADER_UNIFORM_FLOAT);
        }
    } else if (argc == 2) {
        // vec2 or ivec2
        if (mrb_integer_p(args[0]) && mrb_integer_p(args[1])) {
            int ivec[2] = {
                static_cast<int>(mrb_integer(args[0])),
                static_cast<int>(mrb_integer(args[1]))
            };
            SetShaderValue(shader->raylib_shader, location, ivec, SHADER_UNIFORM_IVEC2);
        } else {
            float vec[2] = {
                static_cast<float>(mrb_as_float(mrb, args[0])),
                static_cast<float>(mrb_as_float(mrb, args[1]))
            };
            SetShaderValue(shader->raylib_shader, location, vec, SHADER_UNIFORM_VEC2);
        }
    } else if (argc == 3) {
        // vec3 or ivec3
        if (mrb_integer_p(args[0]) && mrb_integer_p(args[1]) && mrb_integer_p(args[2])) {
            int ivec[3] = {
                static_cast<int>(mrb_integer(args[0])),
                static_cast<int>(mrb_integer(args[1])),
                static_cast<int>(mrb_integer(args[2]))
            };
            SetShaderValue(shader->raylib_shader, location, ivec, SHADER_UNIFORM_IVEC3);
        } else {
            float vec[3] = {
                static_cast<float>(mrb_as_float(mrb, args[0])),
                static_cast<float>(mrb_as_float(mrb, args[1])),
                static_cast<float>(mrb_as_float(mrb, args[2]))
            };
            SetShaderValue(shader->raylib_shader, location, vec, SHADER_UNIFORM_VEC3);
        }
    } else if (argc == 4) {
        // vec4 or ivec4
        bool all_int = mrb_integer_p(args[0]) && mrb_integer_p(args[1]) &&
                       mrb_integer_p(args[2]) && mrb_integer_p(args[3]);
        if (all_int) {
            int ivec[4] = {
                static_cast<int>(mrb_integer(args[0])),
                static_cast<int>(mrb_integer(args[1])),
                static_cast<int>(mrb_integer(args[2])),
                static_cast<int>(mrb_integer(args[3]))
            };
            SetShaderValue(shader->raylib_shader, location, ivec, SHADER_UNIFORM_IVEC4);
        } else {
            float vec[4] = {
                static_cast<float>(mrb_as_float(mrb, args[0])),
                static_cast<float>(mrb_as_float(mrb, args[1])),
                static_cast<float>(mrb_as_float(mrb, args[2])),
                static_cast<float>(mrb_as_float(mrb, args[3]))
            };
            SetShaderValue(shader->raylib_shader, location, vec, SHADER_UNIFORM_VEC4);
        }
    } else if (argc == 0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "No value provided for uniform");
        return mrb_nil_value();
    } else {
        mrb_raise(mrb, E_ARGUMENT_ERROR,
            "Uniform set expects 1-4 values (float/int/vec2/vec3/vec4) or 1 Texture");
        return mrb_nil_value();
    }

    return self;
}

/// @method use
/// @description Execute a block with this shader active. All draws within
///   the block will use this shader. Shader is automatically disabled
///   when the block exits.
/// @yields The block to execute with shader active
/// @returns The block's return value
/// @example shader.use do
///   sprite.draw
///   Graphics.draw_rect(10, 10, 100, 100, :red)
/// end
static mrb_value mrb_shader_use(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Block required for shader.use");
        return mrb_nil_value();
    }

    ShaderData* data = get_shader_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Shader object in use block");
        return mrb_nil_value();
    }

    // Verify shader is still valid before queuing
    if (!ShaderManager::instance().valid(data->handle)) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Shader has been released, cannot use");
        return mrb_nil_value();
    }

    // Queue shader begin command (deferred - applied during DrawQueue::flush)
    DrawQueue::instance().queue_shader_begin(data->handle);

    // Yield to block - draws here will be queued between shader begin/end
    mrb_value result = scripting::safe_yield(mrb, block, mrb_nil_value());

    // Queue shader end command (deferred)
    DrawQueue::instance().queue_shader_end();

    return result;
}

/// @method begin
/// @description Begin shader mode. All subsequent draws will use this shader
///   until `end` is called. Prefer `use { }` block syntax when possible.
/// @returns [Shader] self for chaining
/// @example shader.begin
///   @sprites.each(&:draw)
///   shader.end
static mrb_value mrb_shader_begin(mrb_state* mrb, mrb_value self) {
    ShaderData* data = get_shader_data(mrb, self);
    if (!data) return mrb_nil_value();

    DrawQueue::instance().queue_shader_begin(data->handle);
    return self;
}

/// @method end
/// @description End shader mode. Should be called after `begin`.
/// @returns [Shader] self for chaining
static mrb_value mrb_shader_end(mrb_state* mrb, mrb_value self) {
    (void)self;  // Unused but kept for consistency
    DrawQueue::instance().queue_shader_end();
    return self;
}

/// @method valid?
/// @description Check if shader is still valid (not released).
/// @returns [Boolean] true if shader can be used
static mrb_value mrb_shader_valid(mrb_state* mrb, mrb_value self) {
    ShaderData* data = get_shader_data(mrb, self);
    if (!data) return mrb_false_value();

    return mrb_bool_value(ShaderManager::instance().valid(data->handle));
}

/// @method release
/// @description Explicitly release shader resources. After calling this,
///   the shader is invalid and cannot be used.
/// @returns [nil]
static mrb_value mrb_shader_release(mrb_state* mrb, mrb_value self) {
    ShaderData* data = get_shader_data(mrb, self);
    if (data && ShaderManager::instance().valid(data->handle)) {
        ShaderManager::instance().release(data->handle);
        data->handle = INVALID_HANDLE;
    }
    return mrb_nil_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_shader(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* graphics = mrb_module_get_under(mrb, gmr, "Graphics");

    RClass* shader_class = mrb_define_class_under(mrb, graphics, "Shader", mrb->object_class);
    MRB_SET_INSTANCE_TT(shader_class, MRB_TT_CDATA);

    // Class methods
    mrb_define_class_method(mrb, shader_class, "load", mrb_shader_load, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, shader_class, "from_source", mrb_shader_from_source, MRB_ARGS_REQ(1));

    // Instance methods
    mrb_define_method(mrb, shader_class, "set", mrb_shader_set, MRB_ARGS_REQ(1) | MRB_ARGS_REST());
    mrb_define_method(mrb, shader_class, "use", mrb_shader_use, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, shader_class, "begin", mrb_shader_begin, MRB_ARGS_NONE());
    mrb_define_method(mrb, shader_class, "end", mrb_shader_end, MRB_ARGS_NONE());
    mrb_define_method(mrb, shader_class, "valid?", mrb_shader_valid, MRB_ARGS_NONE());
    mrb_define_method(mrb, shader_class, "release", mrb_shader_release, MRB_ARGS_NONE());

    // Top-level alias: GMR::Shader
    mrb_define_const(mrb, gmr, "Shader", mrb_obj_value(shader_class));
}

} // namespace bindings
} // namespace gmr
