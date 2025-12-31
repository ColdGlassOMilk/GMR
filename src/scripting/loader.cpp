#include "gmr/scripting/loader.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/bindings/graphics.hpp"
#include "gmr/bindings/input.hpp"
#include "gmr/bindings/audio.hpp"
#include "gmr/bindings/window.hpp"
#include "gmr/bindings/util.hpp"
#include "gmr/bindings/console.hpp"
#include <mruby/compile.h>
#include <cstdio>
#include <algorithm>
#include "raylib.h"

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
    bindings::register_graphics(mrb_);
    bindings::register_input(mrb_);
    bindings::register_audio(mrb_);
    bindings::register_window(mrb_);
    bindings::register_util(mrb_);
    bindings::register_console(mrb_);
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

    printf("  Loading: %s\n", path.string().c_str());
    mrb_load_file(mrb_, fp);
    fclose(fp);

    loaded_files_.insert(canonical);

    if (mrb_->exc) {
        mrb_print_error(mrb_);
        mrb_->exc = nullptr;
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

void Loader::load(const std::string& script_dir) {
    if (mrb_) {
        mrb_close(mrb_);
        mrb_ = nullptr;
    }

    loaded_files_.clear();

    mrb_ = mrb_open();
    if (!mrb_) {
        fprintf(stderr, "Failed to open mruby state\n");
        return;
    }

    script_dir_ = script_dir;
    register_all_bindings();

    printf("--- Loading scripts from '%s' ---\n", script_dir.c_str());

    load_directory(script_dir_);

    fs::path main_path = script_dir_ / "main.rb";
    if (fs::exists(main_path)) {
        printf("  Loading: %s (entry point)\n", main_path.string().c_str());
        loaded_files_.insert(fs::weakly_canonical(main_path));

        FILE* fp = fopen(main_path.string().c_str(), "r");
        if (fp) {
            mrb_load_file(mrb_, fp);
            fclose(fp);

            if (mrb_->exc) {
                mrb_print_error(mrb_);
                mrb_->exc = nullptr;
            }
        }
    }

    printf("--- Loaded %zu script(s) ---\n", loaded_files_.size());

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

    auto newest = get_newest_mod_time(script_dir_);

    if (newest > last_mod_time_) {
        if (newest == pending_mod_time_) {
            printf("\n*** Hot reload triggered ***\n");
            load(script_dir_.string());
        } else {
            pending_mod_time_ = newest;
        }
    }
#else
    // Hot reload disabled in release or web builds
    (void)last_check_time_; // suppress unused variable warning
    (void)pending_mod_time_;
#endif
}

} // namespace scripting
} // namespace gmr