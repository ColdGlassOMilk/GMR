#if defined(GMR_DEBUG_ENABLED)

#include "gmr/debug/json_protocol.hpp"
#include "gmr/repl/repl_session.hpp"
#include <sstream>
#include <cctype>

namespace gmr {
namespace debug {

// Simple JSON parser - finds a string value for a given key
static std::string find_string_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t key_pos = json.find(search);
    if (key_pos == std::string::npos) return "";

    // Find the colon after the key
    size_t colon_pos = json.find(':', key_pos + search.length());
    if (colon_pos == std::string::npos) return "";

    // Find the opening quote
    size_t start = json.find('"', colon_pos + 1);
    if (start == std::string::npos) return "";
    start++;

    // Find the closing quote (handle escaped quotes)
    size_t end = start;
    while (end < json.length()) {
        if (json[end] == '"' && json[end - 1] != '\\') {
            break;
        }
        end++;
    }

    return json.substr(start, end - start);
}

// Simple JSON parser - finds an integer value for a given key
static int32_t find_int_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t key_pos = json.find(search);
    if (key_pos == std::string::npos) return 0;

    // Find the colon after the key
    size_t colon_pos = json.find(':', key_pos + search.length());
    if (colon_pos == std::string::npos) return 0;

    // Skip whitespace
    size_t start = colon_pos + 1;
    while (start < json.length() && std::isspace(json[start])) {
        start++;
    }

    // Parse the number
    int32_t result = 0;
    bool negative = false;
    if (start < json.length() && json[start] == '-') {
        negative = true;
        start++;
    }
    while (start < json.length() && std::isdigit(json[start])) {
        result = result * 10 + (json[start] - '0');
        start++;
    }

    return negative ? -result : result;
}

DebugCommand parse_command(const std::string& json) {
    DebugCommand cmd;

    std::string type = find_string_value(json, "type");

    if (type == "set_breakpoint") {
        cmd.type = CommandType::SET_BREAKPOINT;
        cmd.file = find_string_value(json, "file");
        cmd.line = find_int_value(json, "line");
    } else if (type == "clear_breakpoint") {
        cmd.type = CommandType::CLEAR_BREAKPOINT;
        cmd.file = find_string_value(json, "file");
        cmd.line = find_int_value(json, "line");
    } else if (type == "continue") {
        cmd.type = CommandType::CONTINUE;
    } else if (type == "step_over") {
        cmd.type = CommandType::STEP_OVER;
    } else if (type == "step_into") {
        cmd.type = CommandType::STEP_INTO;
    } else if (type == "step_out") {
        cmd.type = CommandType::STEP_OUT;
    } else if (type == "pause") {
        cmd.type = CommandType::PAUSE;
    } else if (type == "evaluate") {
        cmd.type = CommandType::EVALUATE;
        cmd.expression = find_string_value(json, "expression");
        cmd.frame_id = find_int_value(json, "frame_id");
    } else if (type == "repl_eval") {
        cmd.type = CommandType::REPL_EVAL;
        cmd.expression = find_string_value(json, "code");
        cmd.id = find_int_value(json, "id");
    } else if (type == "repl_check_complete") {
        cmd.type = CommandType::REPL_CHECK_COMPLETE;
        cmd.expression = find_string_value(json, "code");
        cmd.id = find_int_value(json, "id");
    } else if (type == "repl_clear_buffer") {
        cmd.type = CommandType::REPL_CLEAR_BUFFER;
        cmd.id = find_int_value(json, "id");
    } else if (type == "repl_list_commands") {
        cmd.type = CommandType::REPL_LIST_COMMANDS;
        cmd.id = find_int_value(json, "id");
    }

    return cmd;
}

std::string json_escape(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Control character - use hex escape
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    oss << buf;
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

std::string make_paused_event(const char* reason, const char* file, int32_t line,
                               const std::string& stack_json,
                               const std::string& locals_json) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"paused\",";
    oss << "\"reason\":\"" << json_escape(reason ? reason : "unknown") << "\",";
    oss << "\"file\":\"" << json_escape(file ? file : "(unknown)") << "\",";
    oss << "\"line\":" << line << ",";
    oss << "\"stack\":" << stack_json << ",";
    oss << "\"locals\":" << locals_json;
    oss << "}";
    return oss.str();
}

std::string make_exception_event(const char* exception_class, const char* message,
                                  const char* file, int32_t line,
                                  const std::string& stack_json) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"exception\",";
    oss << "\"exception_class\":\"" << json_escape(exception_class ? exception_class : "Exception") << "\",";
    oss << "\"message\":\"" << json_escape(message ? message : "") << "\",";
    oss << "\"file\":\"" << json_escape(file ? file : "(unknown)") << "\",";
    oss << "\"line\":" << line << ",";
    oss << "\"stack\":" << stack_json;
    oss << "}";
    return oss.str();
}

std::string make_evaluate_response(bool success, const std::string& result) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"evaluate_response\",";
    oss << "\"success\":" << (success ? "true" : "false") << ",";
    oss << "\"result\":" << result;
    oss << "}";
    return oss.str();
}

std::string make_continued_event() {
    return "{\"type\":\"continued\"}";
}

static const char* eval_status_to_string(gmr::repl::EvalStatus status) {
    switch (status) {
        case gmr::repl::EvalStatus::SUCCESS: return "success";
        case gmr::repl::EvalStatus::EVAL_ERROR: return "error";
        case gmr::repl::EvalStatus::INCOMPLETE: return "incomplete";
        case gmr::repl::EvalStatus::COMMAND_NOT_FOUND: return "command_not_found";
        case gmr::repl::EvalStatus::REENTRANCY_BLOCKED: return "reentrancy_blocked";
        default: return "unknown";
    }
}

std::string make_repl_result_response(int32_t id, const gmr::repl::EvalResult& result) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"repl_result\",";
    oss << "\"id\":" << id << ",";
    oss << "\"status\":\"" << eval_status_to_string(result.status) << "\",";

    // Result value
    if (!result.result.empty()) {
        oss << "\"result\":\"" << json_escape(result.result) << "\",";
    } else {
        oss << "\"result\":null,";
    }

    // Captured output
    oss << "\"stdout\":\"" << json_escape(result.stdout_capture) << "\",";
    oss << "\"stderr\":\"" << json_escape(result.stderr_capture) << "\",";

    // Execution time
    oss << "\"execution_time_ms\":" << result.execution_time_ms << ",";

    // Exception details
    oss << "\"exception\":{";
    if (result.status == gmr::repl::EvalStatus::EVAL_ERROR) {
        oss << "\"class\":\"" << json_escape(result.exception_class) << "\",";
        oss << "\"message\":\"" << json_escape(result.exception_message) << "\",";
        oss << "\"file\":\"" << json_escape(result.source_file) << "\",";
        oss << "\"line\":" << result.source_line << ",";

        oss << "\"backtrace\":[";
        for (size_t i = 0; i < result.backtrace.size(); i++) {
            if (i > 0) oss << ",";
            oss << "\"" << json_escape(result.backtrace[i]) << "\"";
        }
        oss << "]";
    }
    oss << "}";

    oss << "}";
    return oss.str();
}

std::string make_repl_incomplete_response(int32_t id, const std::string& buffer) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"repl_result\",";
    oss << "\"id\":" << id << ",";
    oss << "\"status\":\"incomplete\",";
    oss << "\"buffer\":\"" << json_escape(buffer) << "\"";
    oss << "}";
    return oss.str();
}

std::string make_repl_complete_check_response(int32_t id, bool complete) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"repl_complete_check\",";
    oss << "\"id\":" << id << ",";
    oss << "\"complete\":" << (complete ? "true" : "false");
    oss << "}";
    return oss.str();
}

std::string make_repl_commands_response(int32_t id, const std::string& commands) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"type\":\"repl_commands\",";
    oss << "\"id\":" << id << ",";
    oss << "\"commands\":\"" << json_escape(commands) << "\"";
    oss << "}";
    return oss.str();
}

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
