#include "gmr/scripting/helpers.hpp"
#include "gmr/scripting/loader.hpp"
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/error.h>
#include <mruby/internal.h>

namespace gmr {
namespace scripting {

std::optional<ScriptError> capture_exception(mrb_state* mrb) {
    if (!mrb || !mrb->exc) {
        return std::nullopt;
    }

    ScriptError error;

    mrb_value exc = mrb_obj_value(mrb->exc);

    // Get exception class name
    RClass* exc_class = mrb_obj_class(mrb, exc);
    error.exception_class = mrb_class_name(mrb, exc_class);

    // Get exception message
    mrb_value msg = mrb_funcall(mrb, exc, "message", 0);
    if (mrb_string_p(msg)) {
        error.message = std::string(RSTRING_PTR(msg), RSTRING_LEN(msg));
    }

    // Get backtrace by calling the backtrace method on the exception
    mrb_value bt = mrb_funcall(mrb, exc, "backtrace", 0);
    if (mrb_array_p(bt)) {
        mrb_int len = RARRAY_LEN(bt);
        for (mrb_int i = 0; i < len; ++i) {
            mrb_value entry = mrb_ary_ref(mrb, bt, i);
            if (mrb_string_p(entry)) {
                error.backtrace.push_back(
                    std::string(RSTRING_PTR(entry), RSTRING_LEN(entry))
                );
            }
        }
    }

    // Parse file and line from first backtrace entry
    // Format is typically "filename:line:in `method'" or just "filename:line"
    if (!error.backtrace.empty()) {
        const std::string& first = error.backtrace[0];
        size_t colon1 = first.find(':');
        if (colon1 != std::string::npos) {
            error.file = first.substr(0, colon1);
            size_t colon2 = first.find(':', colon1 + 1);
            if (colon2 != std::string::npos) {
                try {
                    error.line = std::stoi(first.substr(colon1 + 1, colon2 - colon1 - 1));
                } catch (...) {
                    error.line = 0;
                }
            }
        }
    }

    // Fallback if no file info found
    if (error.file.empty()) {
        error.file = "(unknown)";
    }

    return error;
}

bool check_error(mrb_state* mrb, const char* context) {
    if (!mrb || !mrb->exc) {
        return false;
    }

    // Delegate to Loader for centralized error handling
    Loader::instance().handle_exception(mrb, context);
    return true;
}

void safe_call(mrb_state* mrb, const std::string& func) {
    safe_call(mrb, func, std::vector<mrb_value>{});
}

void safe_call(mrb_state* mrb, const std::string& func, mrb_value arg) {
    safe_call(mrb, func, std::vector<mrb_value>{arg});
}

void safe_call(mrb_state* mrb, const std::string& func, const std::vector<mrb_value>& args) {
    if (!mrb) return;

    mrb_funcall_argv(mrb, mrb_top_self(mrb), mrb_intern_cstr(mrb, func.c_str()),
                     static_cast<mrb_int>(args.size()), args.empty() ? nullptr : args.data());

    check_error(mrb, func.c_str());
}

} // namespace scripting
} // namespace gmr
