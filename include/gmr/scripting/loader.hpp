#ifndef GMR_SCRIPTING_LOADER_HPP
#define GMR_SCRIPTING_LOADER_HPP

#include <mruby.h>
#include <string>
#include <set>
#include <filesystem>

namespace gmr {
namespace scripting {

namespace fs = std::filesystem;

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
    void load_directory(const fs::path& dir_path);
    void load_file(const fs::path& path);
    void load_from_bytecode();
    void load_bytecode(const char* path, const uint8_t* bytecode);

    /* Raw filesystem timestamp (no conversion, no truncation) */
    using ScriptTime = fs::file_time_type;
    ScriptTime get_newest_mod_time(const fs::path& dir_path);

    mrb_state* mrb_ = nullptr;
    fs::path script_dir_;
    std::set<fs::path> loaded_files_;

    ScriptTime last_mod_time_{};
    ScriptTime pending_mod_time_{};

    double last_check_time_ = 0.0;  // Throttle reload checks
};

} // namespace scripting
} // namespace gmr

#endif // GMR_SCRIPTING_LOADER_HPP
