#ifndef GMR_SCRIPTING_LOADER_HPP
#define GMR_SCRIPTING_LOADER_HPP

#include <mruby.h>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <optional>
#include "gmr/scripting/script_error.hpp"

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

    // Error state queries
    bool in_error_state() const { return in_error_state_; }
    const std::optional<ScriptError>& last_error() const { return last_error_; }

    // Clear error state (called on hot reload)
    void clear_error_state();

    // Handle an exception - called by check_error()
    // Returns true if error was reported (not deduplicated)
    bool handle_exception(mrb_state* mrb, const char* context = nullptr);

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

    // Per-file tracking for selective hot reload
    struct FileState {
        ScriptTime last_modified{};
    };

    struct PathHasher {
        size_t operator()(const fs::path& p) const {
            return std::hash<std::string>{}(p.string());
        }
    };

    std::vector<fs::path> get_changed_files();
    void reload_file(const fs::path& path);
    std::string extract_init_content();

    mrb_state* mrb_ = nullptr;
    fs::path script_dir_;
    std::set<fs::path> loaded_files_;
    std::unordered_map<fs::path, FileState, PathHasher> file_states_;
    std::string last_init_content_;  // For detecting init() changes

    ScriptTime last_mod_time_{};
    ScriptTime pending_mod_time_{};

    double last_check_time_ = 0.0;  // Throttle reload checks

    // Error state
    bool in_error_state_ = false;
    std::optional<ScriptError> last_error_;
    std::unordered_set<std::string> reported_errors_;  // Dedup keys
};

} // namespace scripting
} // namespace gmr

#endif // GMR_SCRIPTING_LOADER_HPP
