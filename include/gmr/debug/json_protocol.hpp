#ifndef GMR_DEBUG_JSON_PROTOCOL_HPP
#define GMR_DEBUG_JSON_PROTOCOL_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <string>
#include <cstdint>

namespace gmr {
namespace debug {

// Command types received from IDE
enum class CommandType {
    UNKNOWN,
    SET_BREAKPOINT,
    CLEAR_BREAKPOINT,
    CONTINUE,
    STEP_OVER,
    STEP_INTO,
    STEP_OUT,
    PAUSE,
    EVALUATE
};

// Parsed debug command
struct DebugCommand {
    CommandType type = CommandType::UNKNOWN;
    std::string file;
    int32_t line = 0;
    std::string expression;
    int frame_id = 0;
};

// Parse a JSON command string into DebugCommand
DebugCommand parse_command(const std::string& json);

// Generate JSON for paused event
std::string make_paused_event(const char* reason, const char* file, int32_t line,
                               const std::string& stack_json,
                               const std::string& locals_json);

// Generate JSON for exception event
std::string make_exception_event(const char* exception_class, const char* message,
                                  const char* file, int32_t line,
                                  const std::string& stack_json);

// Generate JSON for evaluate response
std::string make_evaluate_response(bool success, const std::string& result);

// Escape a string for JSON
std::string json_escape(const std::string& str);

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_DEBUG_JSON_PROTOCOL_HPP
