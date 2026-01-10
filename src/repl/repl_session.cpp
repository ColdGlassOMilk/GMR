#include "gmr/repl/repl_session.hpp"
#include "gmr/repl/output_capture.hpp"
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/error.h>
#include <sstream>
#include <ctime>
#include <iomanip>

#if defined(GMR_DEBUG_ENABLED)
#include "gmr/repl/multiline_detector.hpp"
#endif

namespace gmr {
namespace repl {

ReplSession& ReplSession::instance() {
    static ReplSession session;
    return session;
}

ReplSession::ReplSession() = default;

ReplSession::~ReplSession() {
    shutdown();
}

bool ReplSession::init(mrb_state* mrb) {
    if (!mrb) {
        return false;
    }

    mrb_ = mrb;
    initialized_ = true;
    eval_count_ = 0;
    input_buffer_.clear();

    // Install output capture hooks
    install_output_hooks(mrb_);

    // Register built-in commands
    register_builtin_commands();

#if !defined(GMR_DEBUG_ENABLED)
    // Release mode: enable logging by default
    logging_enabled_ = true;
#endif

    return true;
}

void ReplSession::shutdown() {
    mrb_ = nullptr;
    initialized_ = false;
    input_buffer_.clear();
    commands_.clear();
    command_log_.clear();
}

bool ReplSession::is_ready() const {
    return initialized_ && mrb_ != nullptr;
}

bool ReplSession::is_evaluating() const {
    return evaluating_.load();
}

void ReplSession::register_builtin_commands() {
    register_command("help", "List available commands", [this](const std::vector<std::string>&) {
        return list_commands_formatted();
    });

    register_command("clear", "Clear console output", [](const std::vector<std::string>&) {
        return "__clear__";  // Special signal for console to clear
    });

    register_command("version", "Show engine version", [](const std::vector<std::string>&) {
        return "GMR Engine v" GMR_VERSION;
    });

    register_command("history", "Show command history", [this](const std::vector<std::string>&) {
        if (command_log_.empty()) {
            return std::string("No commands in history");
        }
        std::ostringstream oss;
        size_t start = command_log_.size() > 20 ? command_log_.size() - 20 : 0;
        for (size_t i = start; i < command_log_.size(); i++) {
            oss << (i + 1) << ". " << command_log_[i] << "\n";
        }
        return oss.str();
    });
}

void ReplSession::register_command(const std::string& name,
                                   const std::string& description,
                                   CommandHandler handler) {
    commands_[name] = Command{name, description, std::move(handler)};
}

bool ReplSession::has_command(const std::string& name) const {
    return commands_.find(name) != commands_.end();
}

std::string ReplSession::list_commands_formatted() const {
    std::ostringstream oss;
    oss << "Available commands:\n";
    for (const auto& [name, cmd] : commands_) {
        oss << "  " << name << " - " << cmd.description << "\n";
    }

#if defined(GMR_DEBUG_ENABLED)
    oss << "\n(Debug mode: arbitrary Ruby code is also allowed)\n";
#endif

    return oss.str();
}

std::pair<std::string, std::vector<std::string>> ReplSession::parse_command_line(
    const std::string& input) const {

    std::string name;
    std::vector<std::string> args;

    // Trim leading whitespace
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        start++;
    }

    // Extract command name (first word)
    size_t end = start;
    while (end < input.size() && !std::isspace(static_cast<unsigned char>(input[end]))) {
        end++;
    }

    if (end > start) {
        name = input.substr(start, end - start);
    }

    // Extract arguments
    start = end;
    while (start < input.size()) {
        // Skip whitespace
        while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
            start++;
        }

        if (start >= input.size()) {
            break;
        }

        // Check for quoted argument
        if (input[start] == '"' || input[start] == '\'') {
            char quote = input[start];
            start++;
            end = start;
            while (end < input.size() && input[end] != quote) {
                if (input[end] == '\\' && end + 1 < input.size()) {
                    end++;  // Skip escaped character
                }
                end++;
            }
            if (end > start) {
                args.push_back(input.substr(start, end - start));
            }
            if (end < input.size()) {
                end++;  // Skip closing quote
            }
            start = end;
        } else {
            // Unquoted argument
            end = start;
            while (end < input.size() && !std::isspace(static_cast<unsigned char>(input[end]))) {
                end++;
            }
            if (end > start) {
                args.push_back(input.substr(start, end - start));
            }
            start = end;
        }
    }

    return {name, args};
}

EvalResult ReplSession::evaluate(const std::string& input) {
    if (!is_ready()) {
        EvalResult result;
        result.status = EvalStatus::EVAL_ERROR;
        result.exception_message = "REPL session not initialized";
        return result;
    }

    // Reentrancy guard
    bool expected = false;
    if (!evaluating_.compare_exchange_strong(expected, true)) {
        EvalResult result;
        result.status = EvalStatus::REENTRANCY_BLOCKED;
        result.exception_message = "Nested evaluation not allowed";
        return result;
    }

    // RAII cleanup
    struct Guard {
        std::atomic<bool>& flag;
        ~Guard() { flag.store(false); }
    } guard{evaluating_};

    // Log command if enabled
    if (logging_enabled_) {
        log_command(input);
    }

    // Trim input
    std::string trimmed = input;
    size_t trim_start = 0;
    size_t trim_end = trimmed.size();
    while (trim_start < trim_end && std::isspace(static_cast<unsigned char>(trimmed[trim_start]))) {
        trim_start++;
    }
    while (trim_end > trim_start && std::isspace(static_cast<unsigned char>(trimmed[trim_end - 1]))) {
        trim_end--;
    }
    trimmed = trimmed.substr(trim_start, trim_end - trim_start);

    if (trimmed.empty()) {
        EvalResult result;
        result.status = EvalStatus::SUCCESS;
        return result;
    }

    // Parse command line
    auto [cmd_name, args] = parse_command_line(trimmed);

    // Check for registered command first
    if (has_command(cmd_name)) {
        return execute_command(cmd_name, args);
    }

#if defined(GMR_DEBUG_ENABLED)
    // Debug: Fall through to Ruby evaluation
    return evaluate_ruby(trimmed);
#else
    // Release: Command not found
    EvalResult result;
    result.status = EvalStatus::COMMAND_NOT_FOUND;
    result.result = "Unknown command: " + cmd_name + ". Type 'help' for available commands.";
    return result;
#endif
}

EvalResult ReplSession::execute_command(const std::string& name,
                                         const std::vector<std::string>& args) {
    EvalResult result;

    auto it = commands_.find(name);
    if (it == commands_.end()) {
        result.status = EvalStatus::COMMAND_NOT_FOUND;
        result.result = "Unknown command: " + name;
        return result;
    }

    auto start = std::chrono::high_resolution_clock::now();

    begin_capture();

    try {
        result.result = it->second.handler(args);
        result.status = EvalStatus::SUCCESS;
    } catch (const std::exception& e) {
        result.status = EvalStatus::EVAL_ERROR;
        result.exception_class = "std::exception";
        result.exception_message = e.what();
    } catch (...) {
        result.status = EvalStatus::EVAL_ERROR;
        result.exception_class = "unknown";
        result.exception_message = "Unknown exception in command handler";
    }

    end_capture(result.stdout_capture, result.stderr_capture);

    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    eval_count_++;
    return result;
}

#if defined(GMR_DEBUG_ENABLED)

EvalResult ReplSession::evaluate_ruby(const std::string& code) {
    auto start = std::chrono::high_resolution_clock::now();

    // Create compile context
    mrbc_context* ctx = mrbc_context_new(mrb_);
    mrbc_filename(mrb_, ctx, "(repl)");

    begin_capture();

    // Evaluate the code
    mrb_value result_val = mrb_load_string_cxt(mrb_, code.c_str(), ctx);

    mrbc_context_free(mrb_, ctx);

    auto end_time = std::chrono::high_resolution_clock::now();
    double exec_time = std::chrono::duration<double, std::milli>(end_time - start).count();

    EvalResult result;

    if (mrb_->exc) {
        result = build_error_result(exec_time);
    } else {
        result = build_success_result(result_val, exec_time);
    }

    eval_count_++;
    return result;
}

#else

EvalResult ReplSession::evaluate_ruby(const std::string&) {
    // Release mode: Ruby eval disabled
    EvalResult result;
    result.status = EvalStatus::EVAL_ERROR;
    result.exception_message = "Ruby evaluation not available in Release mode";
    return result;
}

#endif

EvalResult ReplSession::build_success_result(mrb_value result_val, double exec_time) {
    EvalResult result;
    result.status = EvalStatus::SUCCESS;
    result.execution_time_ms = exec_time;

    // Get captured output
    end_capture(result.stdout_capture, result.stderr_capture);

    // Inspect result value
    if (!mrb_nil_p(result_val)) {
        mrb_value inspected = mrb_inspect(mrb_, result_val);
        if (!mrb_->exc && mrb_string_p(inspected)) {
            result.result = std::string(RSTRING_PTR(inspected), RSTRING_LEN(inspected));
        }
    } else {
        result.result = "nil";
    }

    return result;
}

EvalResult ReplSession::build_error_result(double exec_time) {
    EvalResult result;
    result.status = EvalStatus::EVAL_ERROR;
    result.execution_time_ms = exec_time;

    // Get captured output
    end_capture(result.stdout_capture, result.stderr_capture);

    if (mrb_->exc) {
        mrb_value exc = mrb_obj_value(mrb_->exc);

        // Get exception class name
        result.exception_class = mrb_obj_classname(mrb_, exc);

        // Get message
        mrb_value msg = mrb_funcall(mrb_, exc, "message", 0);
        if (!mrb_->exc && mrb_string_p(msg)) {
            result.exception_message = std::string(RSTRING_PTR(msg), RSTRING_LEN(msg));
        }

        // Clear inner exception from message call
        mrb_->exc = nullptr;

        // Get backtrace
        mrb_value bt = mrb_funcall(mrb_, exc, "backtrace", 0);
        if (!mrb_->exc && mrb_array_p(bt)) {
            mrb_int len = RARRAY_LEN(bt);
            for (mrb_int i = 0; i < len && i < 20; i++) {
                mrb_value frame = mrb_ary_ref(mrb_, bt, i);
                if (mrb_string_p(frame)) {
                    result.backtrace.push_back(
                        std::string(RSTRING_PTR(frame), RSTRING_LEN(frame)));
                }
            }
        }

        // Clear exception
        mrb_->exc = nullptr;

        // Extract source file and line from first backtrace entry
        if (!result.backtrace.empty()) {
            const std::string& first = result.backtrace[0];
            size_t colon1 = first.find(':');
            if (colon1 != std::string::npos) {
                result.source_file = first.substr(0, colon1);
                size_t colon2 = first.find(':', colon1 + 1);
                if (colon2 != std::string::npos) {
                    std::string line_str = first.substr(colon1 + 1, colon2 - colon1 - 1);
                    result.source_line = std::atoi(line_str.c_str());
                }
            }
        }
    }

    return result;
}

// Multi-line support (Debug only)
#if defined(GMR_DEBUG_ENABLED)

bool ReplSession::is_input_complete(const std::string& code) const {
    return MultilineDetector::is_complete(code);
}

#else

bool ReplSession::is_input_complete(const std::string&) const {
    return true;  // Release mode doesn't support multi-line
}

#endif

void ReplSession::append_to_buffer(const std::string& line) {
    if (!input_buffer_.empty()) {
        input_buffer_ += "\n";
    }
    input_buffer_ += line;
}

void ReplSession::clear_buffer() {
    input_buffer_.clear();
}

bool ReplSession::has_pending_input() const {
    return !input_buffer_.empty();
}

const std::string& ReplSession::get_buffer() const {
    return input_buffer_;
}

// Audit logging
void ReplSession::enable_logging(bool enable) {
    logging_enabled_ = enable;
}

void ReplSession::clear_log() {
    command_log_.clear();
}

void ReplSession::log_command(const std::string& input) {
    // Format: [timestamp] command
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "[%Y-%m-%d %H:%M:%S] ");
    oss << input;

    command_log_.push_back(oss.str());

    // Limit log size
    while (command_log_.size() > 1000) {
        command_log_.erase(command_log_.begin());
    }
}

} // namespace repl
} // namespace gmr
