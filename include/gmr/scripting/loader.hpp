#ifndef GMR_SCRIPTING_LOADER_HPP
#define GMR_SCRIPTING_LOADER_HPP

#include <mruby.h>
#include <string>
#include <ctime>

namespace gmr {
namespace scripting {

class Loader {
public:
    static Loader& instance();
    
    void load(const std::string& script_dir);
    void reload_if_changed();
    
    mrb_state* mrb() { return mrb_; }
    bool has_error() const { return mrb_ == nullptr; }
    
    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;
    
private:
    Loader() = default;
    ~Loader();
    
    void register_all_bindings();
    void load_directory(const std::string& dir_path);
    time_t get_newest_mod_time(const std::string& dir_path);
    
    mrb_state* mrb_ = nullptr;
    std::string script_dir_;
    time_t last_mod_time_ = 0;
};

} // namespace scripting
} // namespace gmr

#endif
