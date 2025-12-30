#include "gmr/scripting/loader.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/bindings/graphics.hpp"
#include "gmr/bindings/input.hpp"
#include "gmr/bindings/audio.hpp"
#include "gmr/bindings/window.hpp"
#include "gmr/bindings/util.hpp"
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
}

void Loader::load_file(const fs::path& path) {
    // Normalize path for consistent comparison
    fs::path canonical = fs::weakly_canonical(path);
    
    // Check if already loaded
    if (loaded_files_.find(canonical) != loaded_files_.end()) {
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
    
    // Mark as loaded
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
    
    // Collect all .rb files (except main.rb)
    std::vector<fs::path> rb_files;
    std::vector<fs::path> subdirs;
    
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        const auto& p = entry.path();
        
        if (p.filename().string()[0] == '.') {
            continue;  // Skip hidden files/dirs
        }
        
        if (entry.is_directory()) {
            subdirs.push_back(p);
        } else if (entry.is_regular_file() && p.extension() == ".rb") {
            // Skip main.rb - it's loaded last
            if (p.filename() != "main.rb") {
                rb_files.push_back(p);
            }
        }
    }
    
    // Sort for consistent load order
    std::sort(rb_files.begin(), rb_files.end());
    std::sort(subdirs.begin(), subdirs.end());
    
    // Load .rb files first
    for (const auto& file : rb_files) {
        load_file(file);
    }
    
    // Then recurse into subdirectories
    for (const auto& subdir : subdirs) {
        load_directory(subdir);
    }
}

fs::file_time_type Loader::get_newest_mod_time(const fs::path& dir_path) {
    fs::file_time_type newest{};
    
    if (!fs::exists(dir_path)) {
        return newest;
    }
    
    for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".rb") {
            auto mod_time = entry.last_write_time();
            if (mod_time > newest) {
                newest = mod_time;
            }
        }
    }
    
    return newest;
}

void Loader::load(const std::string& script_dir) {
    // Clean up previous state
    if (mrb_) {
        mrb_close(mrb_);
        mrb_ = nullptr;
    }
    
    // Clear loaded files set for fresh load
    loaded_files_.clear();
    
    mrb_ = mrb_open();
    if (!mrb_) {
        fprintf(stderr, "Failed to open mruby state\n");
        return;
    }
    
    script_dir_ = script_dir;
    register_all_bindings();
    
    printf("--- Loading scripts from '%s' ---\n", script_dir.c_str());
    
    // Load all .rb files (except main.rb) from scripts directory and subdirectories
    load_directory(script_dir_);
    
    // Load main.rb last (entry point)
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
    } else {
        fprintf(stderr, "Warning: Could not find %s\n", main_path.string().c_str());
    }
    
    printf("--- Loaded %zu script(s) ---\n", loaded_files_.size());
    
    // Call init if it exists
    safe_call(mrb_, "init");
    
    // Record mod time AFTER successful load
    last_mod_time_ = get_newest_mod_time(script_dir_);
    last_check_time_ = GetTime();
}

void Loader::reload_if_changed() {
    // Throttle file system checks to every 0.5 seconds for performance
    double current_time = GetTime();
    if (current_time - last_check_time_ < 0.5) {
        return;
    }
    last_check_time_ = current_time;
    
    fs::file_time_type newest = get_newest_mod_time(script_dir_);
    
    if (newest > last_mod_time_) {
        printf("\n*** Hot reload triggered ***\n");
        load(script_dir_.string());
    }
}

} // namespace scripting
} // namespace gmr
