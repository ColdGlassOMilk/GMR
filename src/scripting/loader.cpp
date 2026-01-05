#include "gmr/scripting/loader.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/output/ndjson.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/bindings/graphics.hpp"
#include "gmr/bindings/input.hpp"
#include "gmr/bindings/audio.hpp"
#include "gmr/bindings/window.hpp"
#include "gmr/bindings/util.hpp"
#include "gmr/bindings/console.hpp"
#include "gmr/bindings/collision.hpp"
#include "gmr/bindings/math.hpp"
#include "gmr/bindings/camera.hpp"
#include "gmr/bindings/transform.hpp"
#include "gmr/bindings/sprite.hpp"
#include "gmr/bindings/node.hpp"
#include "gmr/bindings/scene.hpp"
#include "gmr/scene.hpp"
#include "gmr/console/console_module.hpp"
#include <mruby/compile.h>
#include <mruby/irep.h>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>
#include "raylib.h"

#if defined(GMR_DEBUG_ENABLED)
#include "gmr/debug/debug_hooks.hpp"
#endif

// Include compiled scripts only in release builds
#if !defined(DEBUG) && defined(GMR_USE_COMPILED_SCRIPTS)
#include "gmr/scripting/compiled_scripts.hpp"
#endif

namespace gmr {
namespace scripting {

Loader& Loader::instance() {
    static Loader instance;
    return instance;
}

Loader::~Loader() {
    if (mrb_) {
        mrb_close(mrb_);
    }
}

void Loader::register_all_bindings() {
    // Initialize GMR module hierarchy first
    bindings::init_gmr_modules(mrb_);

    // Register math types first (Vec2, Vec3, Rect) - other bindings may depend on these
    bindings::register_math(mrb_);

    // Register all bindings (they add to the modules created above)
    bindings::register_graphics(mrb_);
    bindings::register_input(mrb_);
    bindings::register_audio(mrb_);
    bindings::register_window(mrb_);
    bindings::register_util(mrb_);
    bindings::register_console(mrb_);
    bindings::register_collision(mrb_);

    // Register Camera2D (depends on math types)
    bindings::register_camera(mrb_);

    // Register Transform2D and Sprite (depend on math types)
    bindings::register_transform(mrb_);
    bindings::register_sprite(mrb_);

    // Register Node (scene graph)
    bindings::register_node(mrb_);

    // Register Scene and SceneManager
    bindings::register_scene(mrb_);

    // Register built-in console module (GMR::Console)
    console::register_console_module(mrb_);
}

void Loader::load_file(const fs::path& path) {
    fs::path canonical = fs::weakly_canonical(path);

    if (loaded_files_.count(canonical)) {
        return;
    }

    FILE* fp = fopen(path.string().c_str(), "r");
    if (!fp) {
        fprintf(stderr, "Could not open: %s\n", path.string().c_str());
        return;
    }

    std::string path_str = path.string();
    std::replace(path_str.begin(), path_str.end(), '\\', '/');
    printf("  Loading: %s\n", path_str.c_str());

    // Create compile context with filename for proper error reporting
    mrbc_context* ctx = mrbc_context_new(mrb_);
    mrbc_filename(mrb_, ctx, path.filename().string().c_str());

    mrb_load_file_cxt(mrb_, fp, ctx);

    mrbc_context_free(mrb_, ctx);
    fclose(fp);

    loaded_files_.insert(canonical);

    if (mrb_->exc) {
        handle_exception(mrb_, path.string().c_str());
    }
}

void Loader::load_directory(const fs::path& dir_path) {
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        fprintf(stderr, "Could not open directory: %s\n", dir_path.string().c_str());
        return;
    }

    std::vector<fs::path> rb_files;
    std::vector<fs::path> subdirs;

    for (const auto& entry : fs::directory_iterator(dir_path)) {
        const auto& p = entry.path();

        if (p.filename().string()[0] == '.') {
            continue;
        }

        if (entry.is_directory()) {
            subdirs.push_back(p);
        } else if (entry.is_regular_file() && p.extension() == ".rb") {
            if (p.filename() != "main.rb") {
                rb_files.push_back(p);
            }
        }
    }

    std::sort(rb_files.begin(), rb_files.end());
    std::sort(subdirs.begin(), subdirs.end());

    for (const auto& file : rb_files) {
        load_file(file);
    }

    for (const auto& subdir : subdirs) {
        load_directory(subdir);
    }
}

/* Stable, raw filesystem timestamp */
Loader::ScriptTime Loader::get_newest_mod_time(const fs::path& dir_path) {
    ScriptTime newest = ScriptTime::min();

    if (!fs::exists(dir_path)) {
        return newest;
    }

    for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".rb") continue;

        auto t = entry.last_write_time();
        if (t > newest) {
            newest = t;
        }
    }

    return newest;
}

std::vector<fs::path> Loader::get_changed_files() {
    std::vector<fs::path> changed;

    if (!fs::exists(script_dir_)) {
        return changed;
    }

    for (const auto& entry : fs::recursive_directory_iterator(script_dir_)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".rb") continue;

        fs::path canonical = fs::weakly_canonical(entry.path());
        auto mod_time = entry.last_write_time();

        auto it = file_states_.find(canonical);
        if (it == file_states_.end()) {
            // New file detected
            changed.push_back(canonical);
            file_states_[canonical] = {mod_time};
        } else if (mod_time > it->second.last_modified) {
            // File was modified
            changed.push_back(canonical);
            it->second.last_modified = mod_time;
        }
    }

    return changed;
}

void Loader::reload_file(const fs::path& path) {
    if (!mrb_) return;

    FILE* fp = fopen(path.string().c_str(), "r");
    if (!fp) {
        fprintf(stderr, "Could not open: %s\n", path.string().c_str());
        return;
    }

    mrbc_context* ctx = mrbc_context_new(mrb_);
    mrbc_filename(mrb_, ctx, path.filename().string().c_str());

    // This redefines functions in the file - globals untouched
    mrb_load_file_cxt(mrb_, fp, ctx);

    mrbc_context_free(mrb_, ctx);
    fclose(fp);

    if (mrb_->exc) {
        handle_exception(mrb_, path.string().c_str());
    }
}

std::string Loader::extract_init_content() {
    fs::path main_path = script_dir_ / "main.rb";
    if (!fs::exists(main_path)) return "";

    std::ifstream file(main_path);
    if (!file) return "";

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    // Find "def init" block
    size_t start = content.find("def init");
    if (start == std::string::npos) return "";

    // Find matching "end" by tracking depth
    size_t pos = start;
    int depth = 0;
    bool found_def = false;

    while (pos < content.size()) {
        // Check for "def" keyword (not part of another identifier)
        if (content.compare(pos, 3, "def") == 0 &&
            (pos == 0 || !std::isalnum(static_cast<unsigned char>(content[pos-1]))) &&
            (pos + 3 >= content.size() || !std::isalnum(static_cast<unsigned char>(content[pos+3])))) {
            depth++;
            found_def = true;
            pos += 3;
            continue;
        }

        // Check for "end" keyword
        if (content.compare(pos, 3, "end") == 0 &&
            (pos == 0 || !std::isalnum(static_cast<unsigned char>(content[pos-1]))) &&
            (pos + 3 >= content.size() || !std::isalnum(static_cast<unsigned char>(content[pos+3])))) {
            depth--;
            if (found_def && depth == 0) {
                return content.substr(start, pos + 3 - start);
            }
            pos += 3;
            continue;
        }

        pos++;
    }

    return "";
}

void Loader::load_bytecode(const char* path, const uint8_t* bytecode) {
    printf("  Loading: %s (compiled)\n", path);

    mrb_load_irep(mrb_, bytecode);

    if (mrb_->exc) {
        handle_exception(mrb_, path);
    }
}

void Loader::load_from_bytecode() {
#if !defined(DEBUG) && defined(GMR_USE_COMPILED_SCRIPTS)
    printf("--- Loading compiled scripts ---\n");

    // Load all scripts except main.rb first
    for (size_t i = 0; i < compiled_scripts::SCRIPT_COUNT; ++i) {
        const auto& script = compiled_scripts::SCRIPTS[i];
        std::string path_str(script.path);

        // Skip main.rb for now
        if (path_str.find("main.rb") != std::string::npos) {
            continue;
        }

        load_bytecode(script.path, script.bytecode);
    }

    // Load main.rb last
    for (size_t i = 0; i < compiled_scripts::SCRIPT_COUNT; ++i) {
        const auto& script = compiled_scripts::SCRIPTS[i];
        std::string path_str(script.path);

        if (path_str.find("main.rb") != std::string::npos) {
            printf("  Loading: %s (entry point, compiled)\n", script.path);
            load_bytecode(script.path, script.bytecode);
            break;
        }
    }

    printf("--- Loaded %zu compiled script(s) ---\n", compiled_scripts::SCRIPT_COUNT);
#else
    fprintf(stderr, "Compiled scripts not available in this build\n");
#endif
}

void Loader::load(const std::string& script_dir) {
    if (mrb_) {
        // Clear scene stack before closing mruby state
        SceneManager::instance().clear(mrb_);
        mrb_close(mrb_);
        mrb_ = nullptr;
    }

    loaded_files_.clear();
    file_states_.clear();
    clear_error_state();

    mrb_ = mrb_open();
    if (!mrb_) {
        fprintf(stderr, "Failed to open mruby state\n");
        return;
    }

#if defined(GMR_DEBUG_ENABLED)
    // Install debug hooks for breakpoints and stepping
    gmr::debug::install_hooks(mrb_);
#endif

    script_dir_ = script_dir;
    register_all_bindings();

#if !defined(DEBUG) && defined(GMR_USE_COMPILED_SCRIPTS)
    // Release build: use compiled bytecode
    load_from_bytecode();
#else
    // Debug build: load from files for hot reload
    printf("--- Loading scripts from '%s' ---\n", script_dir.c_str());

    load_directory(script_dir_);

    fs::path main_path = script_dir_ / "main.rb";
    if (fs::exists(main_path)) {
        std::string main_path_str = main_path.string();
        std::replace(main_path_str.begin(), main_path_str.end(), '\\', '/');
        printf("  Loading: %s (entry point)\n", main_path_str.c_str());
        loaded_files_.insert(fs::weakly_canonical(main_path));

        FILE* fp = fopen(main_path.string().c_str(), "r");
        if (fp) {
            // Create compile context with filename for proper error reporting
            mrbc_context* ctx = mrbc_context_new(mrb_);
            mrbc_filename(mrb_, ctx, "main.rb");

            mrb_load_file_cxt(mrb_, fp, ctx);

            mrbc_context_free(mrb_, ctx);
            fclose(fp);

            if (mrb_->exc) {
                handle_exception(mrb_, "main.rb");
            }
        }
    }

    printf("--- Loaded %zu script(s) ---\n", loaded_files_.size());

    // Initialize per-file tracking for selective hot reload
    for (const auto& entry : fs::recursive_directory_iterator(script_dir_)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".rb") continue;
        fs::path canonical = fs::weakly_canonical(entry.path());
        file_states_[canonical] = {entry.last_write_time()};
    }

    // Capture initial init content for change detection
    last_init_content_ = extract_init_content();
#endif

    safe_call(mrb_, "init");

    last_mod_time_    = get_newest_mod_time(script_dir_);
    pending_mod_time_ = last_mod_time_;
    last_check_time_  = GetTime();
}

void Loader::reload_if_changed() {
#if defined(DEBUG)  // Only enable hot reload in debug builds
    double now = GetTime();
    if (now - last_check_time_ < 0.5) {
        return;
    }
    last_check_time_ = now;

    auto changed_files = get_changed_files();
    if (changed_files.empty()) {
        return;
    }

    // Collect file names for NDJSON event
    std::vector<std::string> file_names;
    for (const auto& file : changed_files) {
        file_names.push_back(file.filename().string());
    }

    // Check for new files that weren't loaded yet
    bool has_new_files = false;
    for (const auto& file : changed_files) {
        if (loaded_files_.find(file) == loaded_files_.end()) {
            has_new_files = true;
            break;
        }
    }

    // New files require full reload (dependency order matters)
    if (has_new_files) {
        // Emit NDJSON event for full reload (state not preserved, init will run)
        output::emit_hot_reload_event("full", file_names, false, true);
        load(script_dir_.string());
        return;
    }

    // Selective reload - keep VM alive, only reload changed files
    // Capture init content BEFORE reload
    std::string old_init = extract_init_content();

    clear_error_state();

    // Reload only changed files
    for (const auto& file : changed_files) {
        reload_file(file);
    }

    // Check if init changed
    std::string new_init = extract_init_content();
    bool init_changed = (old_init != new_init && !new_init.empty());

    if (init_changed) {
        safe_call(mrb_, "init");
        last_init_content_ = new_init;
    }

    // Emit NDJSON event for selective reload
    output::emit_hot_reload_event("selective", file_names, !init_changed, init_changed);
#else
    // Hot reload disabled in release or web builds
    (void)last_check_time_; // suppress unused variable warning
    (void)pending_mod_time_;
#endif
}

void Loader::clear_error_state() {
    in_error_state_ = false;
    last_error_.reset();
    reported_errors_.clear();
}

bool Loader::handle_exception(mrb_state* mrb, const char* context) {
    auto error_opt = capture_exception(mrb);
    if (!error_opt) {
        return false;
    }

    // Clear the exception
    mrb->exc = nullptr;

    ScriptError error = std::move(*error_opt);
    std::string key = error.dedup_key();

    // Check if already reported (deduplication)
    if (reported_errors_.count(key)) {
        return true;  // Error exists but suppressed
    }

    // Record this error
    reported_errors_.insert(key);
    last_error_ = error;
    in_error_state_ = true;

    // Print formatted error (replaces mrb_print_error spam)
    fprintf(stderr, "\n=== Script Error ===\n");
    if (context) {
        fprintf(stderr, "Context: %s\n", context);
    }
    fprintf(stderr, "%s: %s\n", error.exception_class.c_str(), error.message.c_str());
    fprintf(stderr, "  at %s:%d\n", error.file.c_str(), error.line);

    if (!error.backtrace.empty()) {
        fprintf(stderr, "Backtrace:\n");
        for (const auto& line : error.backtrace) {
            fprintf(stderr, "  %s\n", line.c_str());
        }
    }
    fprintf(stderr, "====================\n\n");

    return true;
}

} // namespace scripting
} // namespace gmr