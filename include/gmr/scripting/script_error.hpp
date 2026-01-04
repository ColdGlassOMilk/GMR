#ifndef GMR_SCRIPTING_SCRIPT_ERROR_HPP
#define GMR_SCRIPTING_SCRIPT_ERROR_HPP

#include <string>
#include <vector>

namespace gmr {
namespace scripting {

// Structured error information from mruby exceptions
struct ScriptError {
    std::string exception_class;   // e.g., "NoMethodError", "RuntimeError"
    std::string message;           // The error message
    std::string file;              // Source file (or "(unknown)")
    int line = 0;                  // Line number (or 0 if unknown)
    std::vector<std::string> backtrace;  // Stack trace entries

    // Create a unique key for deduplication (class + message + file + line)
    std::string dedup_key() const;

    // Format as a human-readable string
    std::string format() const;

    // Format for IDE consumption (JSON format)
    std::string format_for_ide() const;
};

} // namespace scripting
} // namespace gmr

#endif // GMR_SCRIPTING_SCRIPT_ERROR_HPP
