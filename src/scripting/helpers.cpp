#include "gmr/scripting/helpers.hpp"
#include "gmr/scripting/loader.hpp"
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/error.h>
#include <mruby/internal.h>
#include <mruby/proc.h>
#include <cstdio>

#if defined(GMR_DEBUG_ENABLED)
#include "gmr/debug/debug_server.hpp"
#endif

namespace gmr {
namespace scripting {

// =============================================================================
// Protected Call Data Structures
// =============================================================================

// Data structure for protected calls (top-level functions)
struct ProtectedCallData {
    mrb_sym method_sym;
    const std::vector<mrb_value>* args;
};

// Data structure for protected method calls (on objects)
struct ProtectedMethodCallData {
    mrb_value obj;
    mrb_sym method_sym;
    const std::vector<mrb_value>* args;
};

// Data structure for protected block yields
struct ProtectedYieldData {
    mrb_value block;
    const std::vector<mrb_value>* args;
};

// =============================================================================
// Protected Call Body Functions
// =============================================================================

// Protected call wrapper function (top-level)
static mrb_value protected_call_body(mrb_state* mrb, void* userdata) {
    auto* data = static_cast<ProtectedCallData*>(userdata);
    return mrb_funcall_argv(mrb, mrb_top_self(mrb), data->method_sym,
                            static_cast<mrb_int>(data->args->size()),
                            data->args->empty() ? nullptr : data->args->data());
}

// Protected method call wrapper function (on objects)
static mrb_value protected_method_call_body(mrb_state* mrb, void* userdata) {
    auto* data = static_cast<ProtectedMethodCallData*>(userdata);
    return mrb_funcall_argv(mrb, data->obj, data->method_sym,
                            static_cast<mrb_int>(data->args->size()),
                            data->args->empty() ? nullptr : data->args->data());
}

// Protected yield wrapper function (block yields)
static mrb_value protected_yield_body(mrb_state* mrb, void* userdata) {
    auto* data = static_cast<ProtectedYieldData*>(userdata);
    if (data->args->empty()) {
        return mrb_yield(mrb, data->block, mrb_nil_value());
    } else if (data->args->size() == 1) {
        return mrb_yield(mrb, data->block, (*data->args)[0]);
    } else {
        return mrb_yield_argv(mrb, data->block,
                              static_cast<mrb_int>(data->args->size()),
                              data->args->data());
    }
}

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

#if defined(GMR_DEBUG_ENABLED)
    // Notify debugger of exception before handling it
    auto& debug_server = gmr::debug::DebugServer::instance();
    if (debug_server.is_connected()) {
        // Capture exception info before it's cleared
        auto error = capture_exception(mrb);
        if (error) {
            debug_server.pause_on_exception(
                mrb,
                error->exception_class.c_str(),
                error->message.c_str(),
                error->file.c_str(),
                error->line
            );
        }
    }
#endif

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

    // Use mrb_protect_error to safely catch any exceptions without crashing
    ProtectedCallData data;
    data.method_sym = mrb_intern_cstr(mrb, func.c_str());
    data.args = &args;

    mrb_bool error = FALSE;
    mrb_protect_error(mrb, protected_call_body, &data, &error);

    if (error) {
#ifdef PLATFORM_WEB
        // On web, mrb_protect_error is broken - try to get exception details
        if (func == "init") {
            printf("[WEB] safe_call('%s') raised an exception\n", func.c_str());
            if (mrb->exc) {
                auto err = capture_exception(mrb);
                if (err) {
                    printf("[WEB] Exception: %s - %s\n",
                           err->exception_class.c_str(), err->message.c_str());
                }
            }
        }
#endif
        // Exception was raised - handle it properly
        check_error(mrb, func.c_str());
    }
}

mrb_value safe_method_call(mrb_state* mrb, mrb_value obj, const char* method, const std::vector<mrb_value>& args) {
    if (!mrb) return mrb_nil_value();

    // Use mrb_protect_error to safely catch any exceptions without crashing
    ProtectedMethodCallData data;
    data.obj = obj;
    data.method_sym = mrb_intern_cstr(mrb, method);
    data.args = &args;

    mrb_bool error = FALSE;
    mrb_value result = mrb_protect_error(mrb, protected_method_call_body, &data, &error);

    if (error) {
        // Exception was raised - handle it properly
        check_error(mrb, method);
        return mrb_nil_value();
    }

    return result;
}

// =============================================================================
// Safe Block Yield Functions
// =============================================================================

mrb_value safe_yield(mrb_state* mrb, mrb_value block, mrb_value arg) {
    return safe_yield(mrb, block, std::vector<mrb_value>{arg});
}

mrb_value safe_yield(mrb_state* mrb, mrb_value block, const std::vector<mrb_value>& args) {
    if (!mrb) return mrb_nil_value();
    if (mrb_nil_p(block)) return mrb_nil_value();

    // Use mrb_protect_error to safely catch any exceptions without crashing
    ProtectedYieldData data;
    data.block = block;
    data.args = &args;

    mrb_bool error = FALSE;
    mrb_value result = mrb_protect_error(mrb, protected_yield_body, &data, &error);

    if (error) {
        // Exception was raised - handle it properly
        check_error(mrb, "block yield");
        return mrb_nil_value();
    }

    return result;
}

// =============================================================================
// Safe Instance Exec
// =============================================================================

// Data structure for protected instance_exec calls
struct ProtectedInstanceExecData {
    mrb_value receiver;
    mrb_value block;
};

// Protected instance_exec wrapper function
static mrb_value protected_instance_exec_body(mrb_state* mrb, void* userdata) {
    auto* data = static_cast<ProtectedInstanceExecData*>(userdata);
    // Use mrb_funcall_with_block to properly pass the block for instance_exec
    // instance_exec requires the block to be passed as a block, not an argument
    return mrb_funcall_with_block(mrb, data->receiver,
        mrb_intern_lit(mrb, "instance_exec"), 0, nullptr, data->block);
}

mrb_value safe_instance_exec(mrb_state* mrb, mrb_value receiver, mrb_value block) {
    if (!mrb) return mrb_nil_value();
    if (mrb_nil_p(block)) return mrb_nil_value();
    if (mrb_nil_p(receiver)) return mrb_nil_value();  // Receiver must be valid

    // In mruby, blocks passed via &block should be proc objects
    // Skip the proc check for now - let instance_exec handle validation
    // The block comes from mrb_get_args with "&" which should be valid

    // Use mrb_protect_error to safely catch any exceptions without crashing
    ProtectedInstanceExecData data;
    data.receiver = receiver;
    data.block = block;

    mrb_bool error = FALSE;
    mrb_value result = mrb_protect_error(mrb, protected_instance_exec_body, &data, &error);

    if (error) {
        // Exception was raised - handle it properly
        check_error(mrb, "instance_exec");
        return mrb_nil_value();
    }

    return result;
}

// =============================================================================
// Safe Exception Clearing
// =============================================================================

void safe_clear_exception(mrb_state* mrb, const char* context) {
    if (!mrb || !mrb->exc) return;

#if defined(GMR_DEBUG_ENABLED)
    // In debug builds, capture and log the exception before clearing
    auto error = capture_exception(mrb);
    if (error) {
        // Log at debug level - visible only in debug builds
        fprintf(stderr, "[DEBUG] Cleared exception in %s: %s: %s\n",
                context ? context : "(unknown)",
                error->exception_class.c_str(),
                error->message.c_str());
        if (!error->backtrace.empty()) {
            fprintf(stderr, "        at %s\n", error->backtrace[0].c_str());
        }
    }
#else
    // In release builds, just use context to avoid unused parameter warning
    (void)context;
#endif

    // Clear the exception
    mrb->exc = nullptr;
}

} // namespace scripting
} // namespace gmr
