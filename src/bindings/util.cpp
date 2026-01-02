#include "gmr/bindings/util.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "raylib.h"
#include "rlgl.h"
#include <cstdlib>

namespace gmr {
namespace bindings {

static mrb_value mrb_random_int(mrb_state* mrb, mrb_value) {
    mrb_int min, max;
    mrb_get_args(mrb, "ii", &min, &max);
    return mrb_fixnum_value(GetRandomValue(min, max));
}

static mrb_value mrb_random_float(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, static_cast<double>(GetRandomValue(0, RAND_MAX)) / RAND_MAX);
}

static mrb_value mrb_quit(mrb_state*, mrb_value) {
    CloseWindow();
    exit(0);
    return mrb_nil_value();
}

// Build info functions
static mrb_value mrb_build_platform(mrb_state* mrb, mrb_value) {
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

static mrb_value mrb_build_type(mrb_state* mrb, mrb_value) {
#if defined(DEBUG)
    return mrb_str_new_cstr(mrb, "debug");
#elif defined(NDEBUG)
    return mrb_str_new_cstr(mrb, "release");
#else
    return mrb_str_new_cstr(mrb, "unknown");
#endif
}

static mrb_value mrb_build_compiled_scripts(mrb_state* mrb, mrb_value) {
#if defined(GMR_USE_COMPILED_SCRIPTS)
    return mrb_true_value();
#else
    return mrb_false_value();
#endif
}

static mrb_value mrb_raylib_version(mrb_state* mrb, mrb_value) {
    return mrb_str_new_cstr(mrb, RAYLIB_VERSION);
}

// GPU info functions (using OpenGL queries via rlgl)
static mrb_value mrb_gpu_vendor(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    if (vendor) {
        return mrb_str_new_cstr(mrb, vendor);
    }
#endif
    return mrb_str_new_cstr(mrb, "unknown");
}

static mrb_value mrb_gpu_renderer(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (renderer) {
        return mrb_str_new_cstr(mrb, renderer);
    }
#endif
    return mrb_str_new_cstr(mrb, "WebGL");
}

static mrb_value mrb_gl_version(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (version) {
        return mrb_str_new_cstr(mrb, version);
    }
#endif
    return mrb_str_new_cstr(mrb, "WebGL 2.0");
}

static mrb_value mrb_glsl_version(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* version = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (version) {
        return mrb_str_new_cstr(mrb, version);
    }
#endif
    return mrb_str_new_cstr(mrb, "GLSL ES 3.00");
}

void register_util(mrb_state* mrb) {
    define_method(mrb, "random_int", mrb_random_int, MRB_ARGS_REQ(2));
    define_method(mrb, "random_float", mrb_random_float, MRB_ARGS_NONE());
    define_method(mrb, "quit", mrb_quit, MRB_ARGS_NONE());

    // Build info
    define_method(mrb, "build_platform", mrb_build_platform, MRB_ARGS_NONE());
    define_method(mrb, "build_type", mrb_build_type, MRB_ARGS_NONE());
    define_method(mrb, "compiled_scripts?", mrb_build_compiled_scripts, MRB_ARGS_NONE());
    define_method(mrb, "raylib_version", mrb_raylib_version, MRB_ARGS_NONE());

    // GPU info
    define_method(mrb, "gpu_vendor", mrb_gpu_vendor, MRB_ARGS_NONE());
    define_method(mrb, "gpu_renderer", mrb_gpu_renderer, MRB_ARGS_NONE());
    define_method(mrb, "gl_version", mrb_gl_version, MRB_ARGS_NONE());
    define_method(mrb, "glsl_version", mrb_glsl_version, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
