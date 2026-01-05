#include "gmr/bindings/console.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/repl/repl_session.hpp"
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>

namespace gmr {
namespace bindings {

// Legacy eval_ruby - returns [success, result_string] for backward compatibility
static mrb_value mrb_eval_ruby(mrb_state* mrb, mrb_value) {
    const char* code;
    mrb_get_args(mrb, "z", &code);

    // Create context for evaluation
    mrbc_context* ctx = mrbc_context_new(mrb);
    mrbc_filename(mrb, ctx, "(console)");

    // Evaluate the code
    mrb_value result = mrb_load_string_cxt(mrb, code, ctx);
    mrbc_context_free(mrb, ctx);

    // Check for errors
    if (mrb->exc) {
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb_value msg = mrb_inspect(mrb, exc);
        mrb->exc = nullptr;

        // Return [false, error_message]
        mrb_value arr = mrb_ary_new_capa(mrb, 2);
        mrb_ary_push(mrb, arr, mrb_false_value());
        mrb_ary_push(mrb, arr, msg);
        return arr;
    }

    // Return [true, result_inspect]
    mrb_value arr = mrb_ary_new_capa(mrb, 2);
    mrb_ary_push(mrb, arr, mrb_true_value());
    mrb_ary_push(mrb, arr, mrb_inspect(mrb, result));
    return arr;
}

// New REPL eval - returns a hash with structured result
// { status: 'success'/'error'/'incomplete'/'command_not_found',
//   result: string or nil,
//   stdout: string,
//   stderr: string,
//   exception: { class: string, message: string, backtrace: [string] } or nil }
static mrb_value mrb_repl_eval(mrb_state* mrb, mrb_value) {
    const char* code;
    mrb_get_args(mrb, "z", &code);

    auto& session = repl::ReplSession::instance();

    // Initialize session if needed
    if (!session.is_ready()) {
        session.init(mrb);
    }

    // Evaluate
    auto result = session.evaluate(code);

    // Build result hash
    mrb_value hash = mrb_hash_new(mrb);

    // Status
    const char* status_str = "unknown";
    switch (result.status) {
        case repl::EvalStatus::SUCCESS: status_str = "success"; break;
        case repl::EvalStatus::EVAL_ERROR: status_str = "error"; break;
        case repl::EvalStatus::INCOMPLETE: status_str = "incomplete"; break;
        case repl::EvalStatus::COMMAND_NOT_FOUND: status_str = "command_not_found"; break;
        case repl::EvalStatus::REENTRANCY_BLOCKED: status_str = "reentrancy_blocked"; break;
    }
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "status")),
                 mrb_str_new_cstr(mrb, status_str));

    // Result value
    if (!result.result.empty()) {
        mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "result")),
                     mrb_str_new_cstr(mrb, result.result.c_str()));
    } else {
        mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "result")),
                     mrb_nil_value());
    }

    // Captured output
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "stdout")),
                 mrb_str_new_cstr(mrb, result.stdout_capture.c_str()));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "stderr")),
                 mrb_str_new_cstr(mrb, result.stderr_capture.c_str()));

    // Exception details
    if (result.status == repl::EvalStatus::EVAL_ERROR && !result.exception_class.empty()) {
        mrb_value exc_hash = mrb_hash_new(mrb);
        mrb_hash_set(mrb, exc_hash, mrb_symbol_value(mrb_intern_cstr(mrb, "class")),
                     mrb_str_new_cstr(mrb, result.exception_class.c_str()));
        mrb_hash_set(mrb, exc_hash, mrb_symbol_value(mrb_intern_cstr(mrb, "message")),
                     mrb_str_new_cstr(mrb, result.exception_message.c_str()));

        // Backtrace array
        mrb_value bt_arr = mrb_ary_new_capa(mrb, static_cast<mrb_int>(result.backtrace.size()));
        for (const auto& frame : result.backtrace) {
            mrb_ary_push(mrb, bt_arr, mrb_str_new_cstr(mrb, frame.c_str()));
        }
        mrb_hash_set(mrb, exc_hash, mrb_symbol_value(mrb_intern_cstr(mrb, "backtrace")), bt_arr);

        mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "exception")), exc_hash);
    } else {
        mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "exception")),
                     mrb_nil_value());
    }

    return hash;
}

// Check if input is complete (for multi-line detection)
static mrb_value mrb_repl_is_complete(mrb_state* mrb, mrb_value) {
    const char* code;
    mrb_get_args(mrb, "z", &code);

    auto& session = repl::ReplSession::instance();
    if (!session.is_ready()) {
        session.init(mrb);
    }

    bool complete = session.is_input_complete(code);
    return mrb_bool_value(complete);
}

// Clear multi-line buffer
static mrb_value mrb_repl_clear_buffer(mrb_state* mrb, mrb_value) {
    auto& session = repl::ReplSession::instance();
    session.clear_buffer();
    return mrb_nil_value();
}

// Check if there's pending multi-line input
static mrb_value mrb_repl_has_pending(mrb_state* mrb, mrb_value) {
    auto& session = repl::ReplSession::instance();
    return mrb_bool_value(session.has_pending_input());
}

// List registered commands
static mrb_value mrb_repl_list_commands(mrb_state* mrb, mrb_value) {
    auto& session = repl::ReplSession::instance();
    if (!session.is_ready()) {
        session.init(mrb);
    }

    std::string commands = session.list_commands_formatted();
    return mrb_str_new_cstr(mrb, commands.c_str());
}

void register_console(mrb_state* mrb) {
    // Legacy eval_ruby for backward compatibility
    mrb_define_method(mrb, mrb->kernel_module, "eval_ruby", mrb_eval_ruby, MRB_ARGS_REQ(1));

    // New REPL bindings
    mrb_define_method(mrb, mrb->kernel_module, "__repl_eval", mrb_repl_eval, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, mrb->kernel_module, "__repl_is_complete", mrb_repl_is_complete, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, mrb->kernel_module, "__repl_clear_buffer", mrb_repl_clear_buffer, MRB_ARGS_NONE());
    mrb_define_method(mrb, mrb->kernel_module, "__repl_has_pending", mrb_repl_has_pending, MRB_ARGS_NONE());
    mrb_define_method(mrb, mrb->kernel_module, "__repl_list_commands", mrb_repl_list_commands, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr