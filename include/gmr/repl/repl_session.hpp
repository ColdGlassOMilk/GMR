#ifndef GMR_REPL_SESSION_HPP
#define GMR_REPL_SESSION_HPP

#include <mruby.h>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <chrono>

namespace gmr {
namespace repl {

// Result status of an evaluation
enum class EvalStatus {
    SUCCESS,              // Code executed successfully
    EVAL_ERROR,           // Runtime error occurred (named EVAL_ERROR to avoid Windows ERROR macro)
    INCOMPLETE,           // More input needed (multi-line, Debug only)
    COMMAND_NOT_FOUND,    // Unknown command (Release mode)
    REENTRANCY_BLOCKED    // Nested eval attempt blocked
};

// Structured result from evaluation
struct EvalResult {
    EvalStatus status = EvalStatus::SUCCESS;
    std::string result;             // Inspected result value or command output
    std::string stdout_capture;     // Captured stdout (puts/print)
    std::string stderr_capture;     // Captured stderr (warn)

    // Exception details (populated when status == ERROR)
    std::string exception_class;
    std::string exception_message;
    std::vector<std::string> backtrace;
    std::string source_file;
    int source_line = 0;

    // Timing
    double execution_time_ms = 0.0;
};

// Registered command structure
struct Command {
    std::string name;
    std::string description;
    std::function<std::string(const std::vector<std::string>&)> handler;
};

// REPL session - manages code evaluation with reentrancy protection
class ReplSession {
public:
    // Singleton access
    static ReplSession& instance();

    // Lifecycle
    bool init(mrb_state* mrb);
    void shutdown();
    bool is_ready() const;

    // Main evaluation entry point
    // - Debug: Evaluates Ruby code or registered command
    // - Release: Only executes registered commands
    EvalResult evaluate(const std::string& input);

    // Multi-line support (Debug only)
    bool is_input_complete(const std::string& code) const;
    void append_to_buffer(const std::string& line);
    void clear_buffer();
    bool has_pending_input() const;
    const std::string& get_buffer() const;

    // Command registration (both Debug and Release)
    using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;
    void register_command(const std::string& name,
                         const std::string& description,
                         CommandHandler handler);
    bool has_command(const std::string& name) const;
    std::string list_commands_formatted() const;

    // State access
    bool is_evaluating() const;
    mrb_state* mrb() const { return mrb_; }
    size_t eval_count() const { return eval_count_; }

    // Audit logging (Release mode)
    void enable_logging(bool enable);
    const std::vector<std::string>& command_log() const { return command_log_; }
    void clear_log();

    // Delete copy/move
    ReplSession(const ReplSession&) = delete;
    ReplSession& operator=(const ReplSession&) = delete;

private:
    ReplSession();
    ~ReplSession();

    // Internal evaluation methods
    EvalResult evaluate_ruby(const std::string& code);
    EvalResult execute_command(const std::string& name,
                              const std::vector<std::string>& args);

    // Parse command line into name and arguments
    std::pair<std::string, std::vector<std::string>> parse_command_line(
        const std::string& input) const;

    // Build results
    EvalResult build_success_result(mrb_value result, double exec_time);
    EvalResult build_error_result(double exec_time);

    // Register built-in commands
    void register_builtin_commands();

    // Audit logging
    void log_command(const std::string& input);

    // State
    mrb_state* mrb_ = nullptr;
    bool initialized_ = false;

    // Reentrancy guard
    std::atomic<bool> evaluating_{false};

    // Multi-line buffer (Debug only)
    std::string input_buffer_;

    // Command registry
    std::unordered_map<std::string, Command> commands_;

    // Statistics
    size_t eval_count_ = 0;

    // Audit log
    bool logging_enabled_ = false;
    std::vector<std::string> command_log_;
};

} // namespace repl
} // namespace gmr

#endif // GMR_REPL_SESSION_HPP
