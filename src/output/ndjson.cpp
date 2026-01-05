#include "gmr/output/ndjson.hpp"
#include <cstdio>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace gmr {
namespace output {

// Escape a string for JSON output
static std::string json_escape(const std::string& str) {
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

// Get current time as ISO 8601 timestamp
static std::string get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_buf;
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t_now);
#else
    gmtime_r(&time_t_now, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

void emit_hot_reload_event(const char* reload_type,
                           const std::vector<std::string>& files,
                           bool state_preserved,
                           bool init_changed) {
    std::ostringstream oss;

    // Build NDJSON envelope matching CLI format
    oss << "{";
    oss << "\"protocol\":\"gmr/1.0\",";
    oss << "\"type\":\"event\",";
    oss << "\"timestamp\":\"" << get_iso_timestamp() << "\",";
    oss << "\"event\":{";
    oss << "\"type\":\"hot_reload\",";
    oss << "\"reloadType\":\"" << json_escape(reload_type ? reload_type : "unknown") << "\",";

    // Files array
    oss << "\"files\":[";
    for (size_t i = 0; i < files.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << json_escape(files[i]) << "\"";
    }
    oss << "],";

    oss << "\"statePreserved\":" << (state_preserved ? "true" : "false") << ",";
    oss << "\"initChanged\":" << (init_changed ? "true" : "false");
    oss << "}";
    oss << "}";

    // Print as single line (NDJSON) and flush immediately
    printf("%s\n", oss.str().c_str());
    fflush(stdout);
}

} // namespace output
} // namespace gmr
