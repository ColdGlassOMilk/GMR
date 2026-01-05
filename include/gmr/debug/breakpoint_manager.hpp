#ifndef GMR_DEBUG_BREAKPOINT_MANAGER_HPP
#define GMR_DEBUG_BREAKPOINT_MANAGER_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <string>
#include <unordered_set>
#include <cstdint>

namespace gmr {
namespace debug {

// Manages breakpoints with O(1) lookup
class BreakpointManager {
public:
    static BreakpointManager& instance();

    // Add a breakpoint at file:line
    void add(const std::string& file, int32_t line);

    // Remove a breakpoint at file:line
    void remove(const std::string& file, int32_t line);

    // Check if a breakpoint exists at file:line
    bool has(const char* file, int32_t line) const;

    // Check if any breakpoints are set
    bool empty() const { return breakpoints_.empty(); }

    // Clear all breakpoints
    void clear();

    // Get count of breakpoints
    size_t count() const { return breakpoints_.size(); }

    BreakpointManager(const BreakpointManager&) = delete;
    BreakpointManager& operator=(const BreakpointManager&) = delete;

private:
    BreakpointManager() = default;

    // Create key from file:line
    static std::string make_key(const char* file, int32_t line);
    static std::string make_key(const std::string& file, int32_t line);

    // Normalize file path for consistent matching
    static std::string normalize_path(const char* file);
    static std::string normalize_path(const std::string& file);

    std::unordered_set<std::string> breakpoints_;
};

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_DEBUG_BREAKPOINT_MANAGER_HPP
