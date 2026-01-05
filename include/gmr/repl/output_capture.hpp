#ifndef GMR_REPL_OUTPUT_CAPTURE_HPP
#define GMR_REPL_OUTPUT_CAPTURE_HPP

#include <mruby.h>
#include <string>

namespace gmr {
namespace repl {

// Thread-local output capture context
// Allows REPL to capture stdout/stderr during evaluation
// without affecting normal game output
struct OutputCapture {
    bool active = false;
    std::string stdout_buffer;
    std::string stderr_buffer;
};

// Get the current thread's capture context
OutputCapture& get_capture_context();

// Begin capturing output (clears buffers)
void begin_capture();

// End capturing and return captured output
void end_capture(std::string& out_stdout, std::string& out_stderr);

// Check if capture is currently active
bool is_capturing();

// Install output capture hooks into mruby
// Replaces print, puts, p, warn with capturing versions
void install_output_hooks(mrb_state* mrb);

} // namespace repl
} // namespace gmr

#endif // GMR_REPL_OUTPUT_CAPTURE_HPP
