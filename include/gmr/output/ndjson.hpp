#ifndef GMR_OUTPUT_NDJSON_HPP
#define GMR_OUTPUT_NDJSON_HPP

#include <string>
#include <vector>

namespace gmr {

// Forward declaration
namespace scripting {
struct ScriptError;
}

namespace output {

// Emit a hot_reload event as NDJSON to stdout
// reload_type: "full" or "selective"
// files: list of reloaded file names
// state_preserved: true if game state was preserved (selective reload)
// init_changed: true if init() function changed and was re-run
void emit_hot_reload_event(const char* reload_type,
                           const std::vector<std::string>& files,
                           bool state_preserved,
                           bool init_changed);

// Emit a script_error event as NDJSON to stdout
// Sends structured error information for IDE consumption
void emit_script_error_event(const scripting::ScriptError& error);

} // namespace output
} // namespace gmr

#endif // GMR_OUTPUT_NDJSON_HPP
