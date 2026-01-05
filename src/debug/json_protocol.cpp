#if defined(GMR_DEBUG_ENABLED)

#include "gmr/debug/json_protocol.hpp"
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

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
