#include "gmr/scripting/script_error.hpp"
#include <sstream>

namespace gmr {
namespace scripting {

std::string ScriptError::dedup_key() const {
    return exception_class + "|" + message + "|" + file + "|" + std::to_string(line);
}

std::string ScriptError::format() const {
    std::ostringstream ss;
    ss << exception_class << ": " << message << "\n";
    ss << "  at " << file << ":" << line << "\n";

    if (!backtrace.empty()) {
        ss << "Backtrace:\n";
        for (const auto& entry : backtrace) {
            ss << "  " << entry << "\n";
        }
    }

    return ss.str();
}

std::string ScriptError::format_for_ide() const {
    std::ostringstream ss;

    // Escape quotes in strings for JSON
    auto escape = [](const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    };

    ss << "{"
       << "\"type\":\"script_error\","
       << "\"exception\":\"" << escape(exception_class) << "\","
       << "\"message\":\"" << escape(message) << "\","
       << "\"file\":\"" << escape(file) << "\","
       << "\"line\":" << line << ","
       << "\"backtrace\":[";

    for (size_t i = 0; i < backtrace.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "\"" << escape(backtrace[i]) << "\"";
    }
    ss << "]}";

    return ss.str();
}

} // namespace scripting
} // namespace gmr
