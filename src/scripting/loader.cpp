#include "gmr/scripting/loader.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/bindings/graphics.hpp"
#include "gmr/bindings/input.hpp"
#include "gmr/bindings/audio.hpp"
#include "gmr/bindings/window.hpp"
#include "gmr/bindings/util.hpp"
#include <mruby/compile.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>

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

void Loader::load_directory(const std::string& dir_path) {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    
    // First pass: load .rb files
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        
        std::string full_path = dir_path + "/" + entry->d_name;
        const char* ext = strrchr(entry->d_name, '.');
        
        if (ext && strcmp(ext, ".rb") == 0) {
            if (strcmp(entry->d_name, "main.rb") == 0) continue;
            
            FILE* fp = fopen(full_path.c_str(), "r");
            if (fp) {
                printf("Loading: %s\n", full_path.c_str());
                mrb_load_file(mrb_, fp);
                fclose(fp);
                
                if (mrb_->exc) {
                    mrb_print_error(mrb_);
                    mrb_->exc = nullptr;
                }
            }
        }
    }
    
    // Second pass: recurse into subdirectories
    rewinddir(dir);
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        
        std::string full_path = dir_path + "/" + entry->d_name;
        
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            load_directory(full_path);
        }
    }
    
    closedir(dir);
}

time_t Loader::get_newest_mod_time(const std::string& dir_path) {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) return 0;
    
    time_t newest = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        
        std::string full_path = dir_path + "/" + entry->d_name;
        
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                time_t sub_newest = get_newest_mod_time(full_path);
                if (sub_newest > newest) newest = sub_newest;
            } else {
                const char* ext = strrchr(entry->d_name, '.');
                if (ext && strcmp(ext, ".rb") == 0) {
                    if (st.st_mtime > newest) newest = st.st_mtime;
                }
            }
        }
    }
    
    closedir(dir);
    return newest;
}

void Loader::load(const std::string& script_dir) {
    if (mrb_) {
        mrb_close(mrb_);
    }
    
    mrb_ = mrb_open();
    if (!mrb_) {
        fprintf(stderr, "Failed to open mruby state\n");
        return;
    }
    
    script_dir_ = script_dir;
    register_all_bindings();
    
    printf("--- Loading scripts ---\n");
    
    load_directory(script_dir);
    
    // Load main.rb last
    std::string main_path = script_dir + "/main.rb";
    FILE* fp = fopen(main_path.c_str(), "r");
    if (fp) {
        printf("Loading: %s\n", main_path.c_str());
        mrb_load_file(mrb_, fp);
        fclose(fp);
        
        if (mrb_->exc) {
            mrb_print_error(mrb_);
            mrb_->exc = nullptr;
            return;
        }
    } else {
        fprintf(stderr, "Could not find %s\n", main_path.c_str());
    }
    
    printf("--- Done ---\n");
    
    safe_call(mrb_, "init");
    last_mod_time_ = get_newest_mod_time(script_dir);
}

void Loader::reload_if_changed() {
    time_t newest = get_newest_mod_time(script_dir_);
    if (newest > last_mod_time_) {
        load(script_dir_);
        printf("Scripts reloaded!\n");
    }
}

} // namespace scripting
} // namespace gmr
