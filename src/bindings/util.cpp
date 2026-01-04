#include "gmr/bindings/util.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/scripting/loader.hpp"
#include <mruby/hash.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include "raylib.h"
#include "rlgl.h"
#include <cstdlib>

// OpenGL constants for glGetString (avoid including gl.h due to raylib conflicts)
#if !defined(PLATFORM_WEB)
    #define GL_VENDOR                     0x1F00
    #define GL_RENDERER                   0x1F01
    #define GL_VERSION                    0x1F02
    #define GL_SHADING_LANGUAGE_VERSION   0x8B8C

    typedef const unsigned char* (*GlGetStringFunc)(unsigned int name);
    static GlGetStringFunc glGetStringPtr = nullptr;

    static const char* getGLString(unsigned int name) {
        if (!glGetStringPtr) {
            glGetStringPtr = (GlGetStringFunc)rlGetProcAddress("glGetString");
        }
        if (glGetStringPtr) {
            return reinterpret_cast<const char*>(glGetStringPtr(name));
        }
        return nullptr;
    }
#endif

namespace gmr {
namespace bindings {

// ============================================================================
// GMR::System Module Functions
// ============================================================================

// GMR::System.random_int(min, max)
static mrb_value mrb_system_random_int(mrb_state* mrb, mrb_value) {
    mrb_int min, max;
    mrb_get_args(mrb, "ii", &min, &max);
    return mrb_fixnum_value(GetRandomValue(min, max));
}

// GMR::System.random_float
static mrb_value mrb_system_random_float(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, static_cast<double>(GetRandomValue(0, RAND_MAX)) / RAND_MAX);
}

// GMR::System.quit
static mrb_value mrb_system_quit(mrb_state*, mrb_value) {
    CloseWindow();
    exit(0);
    return mrb_nil_value();
}

// GMR::System.platform
static mrb_value mrb_system_platform(mrb_state* mrb, mrb_value) {
#if defined(PLATFORM_WEB)
    return mrb_str_new_cstr(mrb, "web");
#elif defined(_WIN32)
    return mrb_str_new_cstr(mrb, "windows");
#elif defined(__APPLE__)
    return mrb_str_new_cstr(mrb, "macos");
#elif defined(__linux__)
    return mrb_str_new_cstr(mrb, "linux");
#else
    return mrb_str_new_cstr(mrb, "unknown");
#endif
}

// GMR::System.build_type
static mrb_value mrb_system_build_type(mrb_state* mrb, mrb_value) {
#if defined(DEBUG)
    return mrb_str_new_cstr(mrb, "debug");
#elif defined(NDEBUG)
    return mrb_str_new_cstr(mrb, "release");
#else
    return mrb_str_new_cstr(mrb, "unknown");
#endif
}

// GMR::System.compiled_scripts?
static mrb_value mrb_system_compiled_scripts(mrb_state* mrb, mrb_value) {
#if defined(GMR_USE_COMPILED_SCRIPTS)
    return mrb_true_value();
#else
    return mrb_false_value();
#endif
}

// GMR::System.raylib_version
static mrb_value mrb_system_raylib_version(mrb_state* mrb, mrb_value) {
    return mrb_str_new_cstr(mrb, RAYLIB_VERSION);
}

// GMR::System.gpu_vendor
static mrb_value mrb_system_gpu_vendor(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* vendor = getGLString(GL_VENDOR);
    if (vendor) {
        return mrb_str_new_cstr(mrb, vendor);
    }
#endif
    return mrb_str_new_cstr(mrb, "unknown");
}

// GMR::System.gpu_renderer
static mrb_value mrb_system_gpu_renderer(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* renderer = getGLString(GL_RENDERER);
    if (renderer) {
        return mrb_str_new_cstr(mrb, renderer);
    }
#endif
    return mrb_str_new_cstr(mrb, "WebGL");
}

// GMR::System.gl_version
static mrb_value mrb_system_gl_version(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* version = getGLString(GL_VERSION);
    if (version) {
        return mrb_str_new_cstr(mrb, version);
    }
#endif
    return mrb_str_new_cstr(mrb, "WebGL 2.0");
}

// GMR::System.glsl_version
static mrb_value mrb_system_glsl_version(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* version = getGLString(GL_SHADING_LANGUAGE_VERSION);
    if (version) {
        return mrb_str_new_cstr(mrb, version);
    }
#endif
    return mrb_str_new_cstr(mrb, "GLSL ES 3.00");
}

// GMR::System.last_error
// Returns the last script error as a hash, or nil if no error
static mrb_value mrb_system_last_error(mrb_state* mrb, mrb_value) {
    auto& loader = gmr::scripting::Loader::instance();
    const auto& error = loader.last_error();

    if (!error) {
        return mrb_nil_value();
    }

    mrb_value hash = mrb_hash_new(mrb);

    // Set :class
    mrb_hash_set(mrb, hash,
        mrb_symbol_value(mrb_intern_cstr(mrb, "class")),
        mrb_str_new_cstr(mrb, error->exception_class.c_str()));

    // Set :message
    mrb_hash_set(mrb, hash,
        mrb_symbol_value(mrb_intern_cstr(mrb, "message")),
        mrb_str_new_cstr(mrb, error->message.c_str()));

    // Set :file
    mrb_hash_set(mrb, hash,
        mrb_symbol_value(mrb_intern_cstr(mrb, "file")),
        mrb_str_new_cstr(mrb, error->file.c_str()));

    // Set :line
    mrb_hash_set(mrb, hash,
        mrb_symbol_value(mrb_intern_cstr(mrb, "line")),
        mrb_fixnum_value(error->line));

    // Set :backtrace as array of strings
    mrb_value bt = mrb_ary_new_capa(mrb, static_cast<mrb_int>(error->backtrace.size()));
    for (const auto& entry : error->backtrace) {
        mrb_ary_push(mrb, bt, mrb_str_new_cstr(mrb, entry.c_str()));
    }
    mrb_hash_set(mrb, hash,
        mrb_symbol_value(mrb_intern_cstr(mrb, "backtrace")),
        bt);

    return hash;
}

// GMR::System.in_error_state?
static mrb_value mrb_system_in_error_state(mrb_state*, mrb_value) {
    auto& loader = gmr::scripting::Loader::instance();
    return loader.in_error_state() ? mrb_true_value() : mrb_false_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_util(mrb_state* mrb) {
    RClass* system = get_gmr_submodule(mrb, "System");

    mrb_define_module_function(mrb, system, "random_int", mrb_system_random_int, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, system, "random_float", mrb_system_random_float, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "quit", mrb_system_quit, MRB_ARGS_NONE());

    // Build info
    mrb_define_module_function(mrb, system, "platform", mrb_system_platform, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "build_type", mrb_system_build_type, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "compiled_scripts?", mrb_system_compiled_scripts, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "raylib_version", mrb_system_raylib_version, MRB_ARGS_NONE());

    // GPU info
    mrb_define_module_function(mrb, system, "gpu_vendor", mrb_system_gpu_vendor, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "gpu_renderer", mrb_system_gpu_renderer, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "gl_version", mrb_system_gl_version, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "glsl_version", mrb_system_glsl_version, MRB_ARGS_NONE());

    // Error state
    mrb_define_module_function(mrb, system, "last_error", mrb_system_last_error, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, system, "in_error_state?", mrb_system_in_error_state, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
