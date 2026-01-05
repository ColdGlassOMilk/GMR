#ifndef GMR_DEBUG_VARIABLE_INSPECTOR_HPP
#define GMR_DEBUG_VARIABLE_INSPECTOR_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <mruby.h>
#include <string>
#include <unordered_set>

namespace gmr {
namespace debug {

// Context for serialization with cycle detection
struct SerializeContext {
    std::unordered_set<uintptr_t> visited;  // Object addresses for cycle detection
    int max_depth = 5;
    int current_depth = 0;

    void reset() {
        visited.clear();
        current_depth = 0;
    }
};

// Serialize an mrb_value to JSON string
std::string serialize_value(mrb_state* mrb, mrb_value val, SerializeContext& ctx);

// Get local variables for a stack frame as JSON object
std::string get_locals_json(mrb_state* mrb, int frame_index);

// Get instance variables of an object as JSON object
std::string get_instance_vars_json(mrb_state* mrb, mrb_value obj);

// Get the full stack trace as JSON array
std::string get_stack_trace_json(mrb_state* mrb);

// Evaluate an expression in the context of a frame and return result as JSON
std::string evaluate_expression(mrb_state* mrb, const std::string& expr, int frame_index);

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_DEBUG_VARIABLE_INSPECTOR_HPP
