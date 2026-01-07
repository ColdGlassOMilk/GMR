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

/// @module GMR::System
/// @description System utilities and build information. Provides platform detection,
///   GPU information, and error state queries.
/// @example # Platform-specific features
///   class Game
///     def init
///       case GMR::System.platform
///       when "web"
///         # Disable features not available on web
///         @allow_save_to_disk = false
///         @fullscreen_key = nil  # Browser handles this
///         puts "Running in browser mode"
///       when "windows", "macos", "linux"
///         @allow_save_to_disk = true
///         @fullscreen_key = :f11
///         puts "Running desktop version"
///       end
///
///       # Debug features only in debug builds
///       if GMR::System.build_type == "debug"
///         @show_debug_overlay = true
///         @invincible_mode = false
///         puts "Debug build - press F3 for debug overlay"
///       end
///     end
///   end
/// @example # Error handling and recovery
///   class ErrorScreen
///     def init
///       @error = GMR::System.last_error
///     end
///
///     def update(dt)
///       # Allow retry or quit
///       if GMR::Input.key_pressed?(:r)
///         GMR::SceneManager.load(TitleScene.new)  # Try to restart
///       elsif GMR::Input.key_pressed?(:escape)
///         GMR::System.quit
///       end
///     end
///
///     def draw
///       GMR::Graphics.draw_rect(0, 0, 800, 600, [40, 0, 0])
///       GMR::Graphics.draw_text("An error occurred!", 100, 50, 32, [255, 100, 100])
///
///       if @error
///         GMR::Graphics.draw_text("#{@error[:class]}: #{@error[:message]}", 100, 120, 18, [255, 200, 200])
///         GMR::Graphics.draw_text("at #{@error[:file]}:#{@error[:line]}", 100, 145, 14, [200, 200, 200])
///
///         y = 180
///         @error[:backtrace].first(5).each do |line|
///           GMR::Graphics.draw_text(line, 100, y, 12, [180, 180, 180])
///           y += 16
///         end
///       end
///
///       GMR::Graphics.draw_text("[R] Retry   [ESC] Quit", 100, 500, 20, [200, 200, 200])
///     end
///   end
/// @example # System info display for about screen
///   class AboutScene < GMR::Scene
///     def draw
///       GMR::Graphics.draw_text("System Information", 100, 50, 28, [255, 255, 255])
///
///       y = 100
///       info = [
///         ["Platform", GMR::System.platform],
///         ["Build", GMR::System.build_type],
///         ["Raylib", GMR::System.raylib_version],
///         ["GPU", GMR::System.gpu_renderer],
///         ["OpenGL", GMR::System.gl_version],
///         ["GLSL", GMR::System.glsl_version]
///       ]
///
///       info.each do |label, value|
///         GMR::Graphics.draw_text("#{label}:", 100, y, 16, [180, 180, 180])
///         GMR::Graphics.draw_text(value, 200, y, 16, [255, 255, 255])
///         y += 25
///       end
///     end
///   end

/// @function quit
/// @description Immediately exit the application. Closes the window and terminates
///   the process.
/// @returns [nil]
/// @example GMR::System.quit  # Exit the game
static mrb_value mrb_system_quit(mrb_state*, mrb_value) {
    CloseWindow();
    exit(0);
    return mrb_nil_value();
}

/// @function platform
/// @description Get the current platform identifier.
/// @returns [String] Platform name: "windows", "macos", "linux", "web", or "unknown"
/// @example if GMR::System.platform == "web"
///   # Disable desktop-only features
/// end
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

/// @function build_type
/// @description Get the build configuration type.
/// @returns [String] Build type: "debug", "release", or "unknown"
/// @example if GMR::System.build_type == "debug"
///   enable_debug_overlay
/// end
static mrb_value mrb_system_build_type(mrb_state* mrb, mrb_value) {
#if defined(DEBUG)
    return mrb_str_new_cstr(mrb, "debug");
#elif defined(NDEBUG)
    return mrb_str_new_cstr(mrb, "release");
#else
    return mrb_str_new_cstr(mrb, "unknown");
#endif
}

/// @function compiled_scripts?
/// @description Check if scripts were precompiled into the binary.
/// @returns [Boolean] true if scripts are compiled in, false if loading from files
/// @example puts "Scripts compiled: #{GMR::System.compiled_scripts?}"
static mrb_value mrb_system_compiled_scripts(mrb_state* mrb, mrb_value) {
#if defined(GMR_USE_COMPILED_SCRIPTS)
    return mrb_true_value();
#else
    return mrb_false_value();
#endif
}

/// @function raylib_version
/// @description Get the version of the underlying Raylib graphics library.
/// @returns [String] Raylib version string (e.g., "5.0")
/// @example puts "Raylib: #{GMR::System.raylib_version}"
static mrb_value mrb_system_raylib_version(mrb_state* mrb, mrb_value) {
    return mrb_str_new_cstr(mrb, RAYLIB_VERSION);
}

/// @function gpu_vendor
/// @description Get the GPU vendor name from OpenGL.
/// @returns [String] GPU vendor name (e.g., "NVIDIA Corporation") or "unknown"
/// @example puts "GPU Vendor: #{GMR::System.gpu_vendor}"
static mrb_value mrb_system_gpu_vendor(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* vendor = getGLString(GL_VENDOR);
    if (vendor) {
        return mrb_str_new_cstr(mrb, vendor);
    }
#endif
    return mrb_str_new_cstr(mrb, "unknown");
}

/// @function gpu_renderer
/// @description Get the GPU renderer name from OpenGL.
/// @returns [String] GPU renderer name (e.g., "GeForce RTX 3080") or "WebGL"
/// @example puts "GPU: #{GMR::System.gpu_renderer}"
static mrb_value mrb_system_gpu_renderer(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* renderer = getGLString(GL_RENDERER);
    if (renderer) {
        return mrb_str_new_cstr(mrb, renderer);
    }
#endif
    return mrb_str_new_cstr(mrb, "WebGL");
}

/// @function gl_version
/// @description Get the OpenGL version string.
/// @returns [String] OpenGL version (e.g., "4.6.0") or "WebGL 2.0"
/// @example puts "OpenGL: #{GMR::System.gl_version}"
static mrb_value mrb_system_gl_version(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* version = getGLString(GL_VERSION);
    if (version) {
        return mrb_str_new_cstr(mrb, version);
    }
#endif
    return mrb_str_new_cstr(mrb, "WebGL 2.0");
}

/// @function glsl_version
/// @description Get the GLSL (shader language) version string.
/// @returns [String] GLSL version (e.g., "4.60") or "GLSL ES 3.00"
/// @example puts "GLSL: #{GMR::System.glsl_version}"
static mrb_value mrb_system_glsl_version(mrb_state* mrb, mrb_value) {
#if !defined(PLATFORM_WEB)
    const char* version = getGLString(GL_SHADING_LANGUAGE_VERSION);
    if (version) {
        return mrb_str_new_cstr(mrb, version);
    }
#endif
    return mrb_str_new_cstr(mrb, "GLSL ES 3.00");
}

/// @function last_error
/// @description Get details about the last script error. Returns nil if no error occurred.
/// @returns [Hash, nil] Error hash with keys :class, :message, :file, :line, :backtrace, or nil
/// @example error = GMR::System.last_error
///   if error
///     puts "#{error[:class]}: #{error[:message]}"
///     puts "  at #{error[:file]}:#{error[:line]}"
///     error[:backtrace].each { |line| puts "    #{line}" }
///   end
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

/// @function in_error_state?
/// @description Check if the scripting engine is currently in an error state.
/// @returns [Boolean] true if an unhandled error has occurred
/// @example if GMR::System.in_error_state?
///   show_error_screen
/// end
static mrb_value mrb_system_in_error_state(mrb_state*, mrb_value) {
    auto& loader = gmr::scripting::Loader::instance();
    return loader.in_error_state() ? mrb_true_value() : mrb_false_value();
}

// ============================================================================
// Registration
// ============================================================================

void register_util(mrb_state* mrb) {
    RClass* system = get_gmr_submodule(mrb, "System");

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
