#ifndef GMR_SCRIPTING_HELPERS_HPP
#define GMR_SCRIPTING_HELPERS_HPP

#include <mruby.h>
#include <string>
#include <vector>
#include <optional>
#include "gmr/scripting/script_error.hpp"

namespace gmr {
namespace scripting {

// Safe function call with error handling
void safe_call(mrb_state* mrb, const std::string& func);
void safe_call(mrb_state* mrb, const std::string& func, mrb_value arg);
void safe_call(mrb_state* mrb, const std::string& func, const std::vector<mrb_value>& args);

// Extract structured error info from mrb->exc
// Returns nullopt if no exception is pending
std::optional<ScriptError> capture_exception(mrb_state* mrb);

// Check and handle exception with deduplication
// Returns true if an error occurred (even if deduplicated/suppressed)
bool check_error(mrb_state* mrb, const char* context = nullptr);

} // namespace scripting
} // namespace gmr

#endif
