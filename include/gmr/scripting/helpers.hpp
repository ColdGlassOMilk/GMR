#ifndef GMR_SCRIPTING_HELPERS_HPP
#define GMR_SCRIPTING_HELPERS_HPP

#include <mruby.h>
#include <string>
#include <vector>
#include <optional>
#include "gmr/scripting/script_error.hpp"

// =============================================================================
// GMR_UNSAFE_MRUBY_CALL Macro
// =============================================================================
// Mark intentionally unsafe mruby calls that bypass safe wrappers.
// Usage: GMR_UNSAFE_MRUBY_CALL("reason for bypassing safe wrapper")
//
// This does nothing at runtime but documents the justification and makes
// code review easier. Use `grep -r GMR_UNSAFE_MRUBY_CALL` to audit all
// intentionally unsafe calls in the codebase.
//
// Example:
//   GMR_UNSAFE_MRUBY_CALL("REPL to_s failure returns empty string, not raised")
//   mrb_value str = mrb_funcall(mrb, val, "to_s", 0);
//
#define GMR_UNSAFE_MRUBY_CALL(reason) /* intentional unsafe mruby call: reason */

namespace gmr {
namespace scripting {

// =============================================================================
// Safe Function Calls (top-level)
// =============================================================================

// Safe function call with error handling (calls top-level functions)
void safe_call(mrb_state* mrb, const std::string& func);
void safe_call(mrb_state* mrb, const std::string& func, mrb_value arg);
void safe_call(mrb_state* mrb, const std::string& func, const std::vector<mrb_value>& args);

// =============================================================================
// Safe Method Calls (on objects)
// =============================================================================

// Safe method call with error handling (calls methods on objects)
// Returns mrb_nil_value() if an exception occurs
mrb_value safe_method_call(mrb_state* mrb, mrb_value obj, const char* method, const std::vector<mrb_value>& args = {});

// =============================================================================
// Safe Block Yields
// =============================================================================

// Safe block yield with error handling
// Returns mrb_nil_value() if an exception occurs
mrb_value safe_yield(mrb_state* mrb, mrb_value block, mrb_value arg);
mrb_value safe_yield(mrb_state* mrb, mrb_value block, const std::vector<mrb_value>& args);

// =============================================================================
// Safe Instance Exec
// =============================================================================

// Safe instance_exec - executes a block with receiver as 'self'
// Uses mrb_funcall_with_block internally to properly pass block context
// Returns mrb_nil_value() if an exception occurs
mrb_value safe_instance_exec(mrb_state* mrb, mrb_value receiver, mrb_value block);

// =============================================================================
// Exception Handling
// =============================================================================

// Extract structured error info from mrb->exc
// Returns nullopt if no exception is pending
std::optional<ScriptError> capture_exception(mrb_state* mrb);

// Check and handle exception with deduplication
// Returns true if an error occurred (even if deduplicated/suppressed)
bool check_error(mrb_state* mrb, const char* context = nullptr);

// Clear exception with optional debug logging
// Use this instead of direct `mrb->exc = nullptr` to maintain audit trail.
// In debug builds, logs the exception at debug level before clearing.
// In release builds, clears silently for better UX.
void safe_clear_exception(mrb_state* mrb, const char* context = nullptr);

} // namespace scripting
} // namespace gmr

#endif
