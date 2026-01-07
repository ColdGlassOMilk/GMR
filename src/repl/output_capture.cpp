#include "gmr/repl/output_capture.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/string.h>
#include <mruby/array.h>
#include <cstdio>

namespace gmr {
namespace repl {

// Thread-local capture context
static thread_local OutputCapture g_capture;

OutputCapture& get_capture_context() {
    return g_capture;
}

void begin_capture() {
    g_capture.active = true;
    g_capture.stdout_buffer.clear();
    g_capture.stderr_buffer.clear();
}

void end_capture(std::string& out_stdout, std::string& out_stderr) {
    g_capture.active = false;
    out_stdout = std::move(g_capture.stdout_buffer);
    out_stderr = std::move(g_capture.stderr_buffer);
    g_capture.stdout_buffer.clear();
    g_capture.stderr_buffer.clear();
}

bool is_capturing() {
    return g_capture.active;
}

// Helper to convert mrb_value to string
static std::string value_to_string(mrb_state* mrb, mrb_value val) {
    if (mrb_nil_p(val)) {
        return "";
    }
    // GMR_UNSAFE_MRUBY_CALL: REPL output capture - to_s failure returns empty string.
    // This is intentional: output capture should never raise to user, and silently
    // returning empty is acceptable for display-only purposes.
    GMR_UNSAFE_MRUBY_CALL("REPL output to_s - failure returns empty, not raised to user")
    mrb_value str = mrb_funcall(mrb, val, "to_s", 0);
    if (mrb->exc) {
        scripting::safe_clear_exception(mrb, "output_capture to_s");
        return "";
    }
    if (mrb_string_p(str)) {
        return std::string(RSTRING_PTR(str), RSTRING_LEN(str));
    }
    return "";
}

// Captured print implementation
static mrb_value captured_print(mrb_state* mrb, mrb_value self) {
    mrb_int argc;
    mrb_value* argv;
    mrb_get_args(mrb, "*", &argv, &argc);

    std::string output;
    for (mrb_int i = 0; i < argc; i++) {
        output += value_to_string(mrb, argv[i]);
    }

    if (g_capture.active) {
        g_capture.stdout_buffer += output;
    } else {
        printf("%s", output.c_str());
        fflush(stdout);
    }

    return mrb_nil_value();
}

// Captured puts implementation
static mrb_value captured_puts(mrb_state* mrb, mrb_value self) {
    mrb_int argc;
    mrb_value* argv;
    mrb_get_args(mrb, "*", &argv, &argc);

    if (argc == 0) {
        if (g_capture.active) {
            g_capture.stdout_buffer += "\n";
        } else {
            printf("\n");
            fflush(stdout);
        }
        return mrb_nil_value();
    }

    for (mrb_int i = 0; i < argc; i++) {
        std::string output;

        if (mrb_array_p(argv[i])) {
            // For arrays, puts each element on its own line
            mrb_int len = RARRAY_LEN(argv[i]);
            for (mrb_int j = 0; j < len; j++) {
                mrb_value elem = mrb_ary_ref(mrb, argv[i], j);
                output += value_to_string(mrb, elem);
                output += "\n";
            }
        } else {
            output = value_to_string(mrb, argv[i]);
            output += "\n";
        }

        if (g_capture.active) {
            g_capture.stdout_buffer += output;
        } else {
            printf("%s", output.c_str());
        }
    }

    if (!g_capture.active) {
        fflush(stdout);
    }

    return mrb_nil_value();
}

// Captured p implementation (inspect output)
static mrb_value captured_p(mrb_state* mrb, mrb_value self) {
    mrb_int argc;
    mrb_value* argv;
    mrb_get_args(mrb, "*", &argv, &argc);

    for (mrb_int i = 0; i < argc; i++) {
        // GMR_UNSAFE_MRUBY_CALL: REPL p output - inspect failure skips value.
        // This is intentional: output capture should not raise to user.
        GMR_UNSAFE_MRUBY_CALL("REPL p inspect - failure skips value, not raised to user")
        mrb_value inspected = mrb_inspect(mrb, argv[i]);
        if (mrb->exc) {
            scripting::safe_clear_exception(mrb, "output_capture p inspect");
            continue;
        }

        std::string output;
        if (mrb_string_p(inspected)) {
            output = std::string(RSTRING_PTR(inspected), RSTRING_LEN(inspected));
        }
        output += "\n";

        if (g_capture.active) {
            g_capture.stdout_buffer += output;
        } else {
            printf("%s", output.c_str());
        }
    }

    if (!g_capture.active) {
        fflush(stdout);
    }

    // p returns its argument(s)
    if (argc == 0) {
        return mrb_nil_value();
    } else if (argc == 1) {
        return argv[0];
    } else {
        return mrb_ary_new_from_values(mrb, argc, argv);
    }
}

// Captured warn implementation
static mrb_value captured_warn(mrb_state* mrb, mrb_value self) {
    mrb_int argc;
    mrb_value* argv;
    mrb_get_args(mrb, "*", &argv, &argc);

    std::string output;
    for (mrb_int i = 0; i < argc; i++) {
        output += value_to_string(mrb, argv[i]);
    }
    output += "\n";

    if (g_capture.active) {
        g_capture.stderr_buffer += output;
    } else {
        fprintf(stderr, "%s", output.c_str());
        fflush(stderr);
    }

    return mrb_nil_value();
}

void install_output_hooks(mrb_state* mrb) {
    if (!mrb) return;

    // Override Kernel methods
    mrb_define_method(mrb, mrb->kernel_module, "print", captured_print, MRB_ARGS_ANY());
    mrb_define_method(mrb, mrb->kernel_module, "puts", captured_puts, MRB_ARGS_ANY());
    mrb_define_method(mrb, mrb->kernel_module, "p", captured_p, MRB_ARGS_ANY());
    mrb_define_method(mrb, mrb->kernel_module, "warn", captured_warn, MRB_ARGS_ANY());
}

} // namespace repl
} // namespace gmr
