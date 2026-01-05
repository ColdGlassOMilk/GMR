#ifndef GMR_DEBUG_JSON_PROTOCOL_HPP
#define GMR_DEBUG_JSON_PROTOCOL_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <string>
#include <cstdint>

// Forward declaration in correct namespace
namespace gmr {
namespace repl {
    struct EvalResult;
}
}

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
    EVALUATE,
    // REPL commands
    REPL_EVAL,
    REPL_CHECK_COMPLETE,
    REPL_CLEAR_BUFFER,
    REPL_LIST_COMMANDS
};

// Parsed debug command
struct DebugCommand {
    CommandType type = CommandType::UNKNOWN;
    std::string file;
    int32_t line = 0;
    std::string expression;  // Also used for REPL code
    int frame_id = 0;
    int32_t id = 0;          // Request ID for REPL responses
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

// Generate JSON for continued event (execution resumed)
std::string make_continued_event();

// Escape a string for JSON
std::string json_escape(const std::string& str);

// Generate JSON for REPL result response
std::string make_repl_result_response(int32_t id, const gmr::repl::EvalResult& result);

// Generate JSON for REPL incomplete response
std::string make_repl_incomplete_response(int32_t id, const std::string& buffer);

// Generate JSON for REPL complete check response
std::string make_repl_complete_check_response(int32_t id, bool complete);

// Generate JSON for REPL command list response
std::string make_repl_commands_response(int32_t id, const std::string& commands);

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_DEBUG_JSON_PROTOCOL_HPP
