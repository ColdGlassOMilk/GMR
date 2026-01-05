#if defined(GMR_DEBUG_ENABLED)

#include "gmr/debug/breakpoint_manager.hpp"
#include <algorithm>
#include <sstream>

namespace gmr {
namespace debug {

BreakpointManager& BreakpointManager::instance() {
    static BreakpointManager instance;
    return instance;
}

void BreakpointManager::add(const std::string& file, int32_t line) {
    breakpoints_.insert(make_key(file, line));
}

void BreakpointManager::remove(const std::string& file, int32_t line) {
    breakpoints_.erase(make_key(file, line));
}

bool BreakpointManager::has(const char* file, int32_t line) const {
    if (!file) return false;
    return breakpoints_.count(make_key(file, line)) > 0;
}

void BreakpointManager::clear() {
    breakpoints_.clear();
}

std::string BreakpointManager::make_key(const char* file, int32_t line) {
    std::ostringstream oss;
    oss << normalize_path(file) << ":" << line;
    return oss.str();
}

std::string BreakpointManager::make_key(const std::string& file, int32_t line) {
    std::ostringstream oss;
    oss << normalize_path(file) << ":" << line;
    return oss.str();
}

std::string BreakpointManager::normalize_path(const char* file) {
    if (!file) return "";
    return normalize_path(std::string(file));
}

std::string BreakpointManager::normalize_path(const std::string& file) {
    std::string result = file;

    // Convert backslashes to forward slashes
    std::replace(result.begin(), result.end(), '\\', '/');

    // Convert to lowercase for case-insensitive matching on Windows
#ifdef _WIN32
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
#endif

    // Extract just the filename for simpler matching
    // This matches how mruby reports filenames (usually just the basename)
    size_t last_slash = result.rfind('/');
    if (last_slash != std::string::npos) {
        result = result.substr(last_slash + 1);
    }

    return result;
}

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
