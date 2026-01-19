#if defined(GMR_DEBUG_ENABLED)

#include "gmr/debug/variable_inspector.hpp"
#include "gmr/debug/json_protocol.hpp"
#include "gmr/scripting/helpers.hpp"
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/proc.h>
#include <mruby/debug.h>
#include <mruby/compile.h>
#include <sstream>
#include <vector>
#include <cstdio>

namespace gmr {
namespace debug {

namespace {

// Unique global symbol for debugger context - stores local variables during evaluation
static const char* DEBUG_CONTEXT_KEY = "$__gmr_debug_ctx__";

// Frame context extracted from a call frame
struct FrameContext {
    mrb_value self_value;
    std::vector<std::string> local_names;
    std::vector<mrb_value> local_values;
    bool is_cfunc = false;
    bool valid = false;
};

// RAII guard to ensure debug context cleanup on any exit path
struct DebugContextGuard {
    mrb_state* mrb;
    explicit DebugContextGuard(mrb_state* m) : mrb(m) {}
    ~DebugContextGuard() {
        if (mrb) {
            mrb_gv_set(mrb, mrb_intern_cstr(mrb, DEBUG_CONTEXT_KEY), mrb_nil_value());
        }
    }
    // Non-copyable
    DebugContextGuard(const DebugContextGuard&) = delete;
    DebugContextGuard& operator=(const DebugContextGuard&) = delete;
};

// Navigate to target frame by index (0 = current, 1 = caller, etc.)
mrb_callinfo* navigate_to_frame(mrb_state* mrb, int frame_index) {
    if (!mrb || !mrb->c || !mrb->c->ci) return nullptr;
    if (frame_index < 0) return nullptr;

    mrb_callinfo* ci = mrb->c->ci;
    for (int i = 0; i < frame_index && ci > mrb->c->cibase; ++i) {
        ci--;
    }

    // Check we didn't go past the base
    if (ci <= mrb->c->cibase) return nullptr;
    return ci;
}

// Extract context (self, locals) from a call frame
FrameContext extract_frame_context(mrb_state* mrb, mrb_callinfo* ci) {
    FrameContext ctx;
    if (!ci) return ctx;

    ctx.valid = true;
    ctx.self_value = ci->stack[0];

    // C function frames have no Ruby locals
    if (!ci->proc || MRB_PROC_CFUNC_P(ci->proc)) {
        ctx.is_cfunc = true;
        return ctx;
    }

    const mrb_irep* irep = ci->proc->body.irep;
    if (irep && irep->lv) {
        for (uint16_t i = 0; i < irep->nlocals - 1; ++i) {
            mrb_sym sym = irep->lv[i];
            if (sym == 0) continue;
            const char* name = mrb_sym_name(mrb, sym);
            if (name) {
                ctx.local_names.push_back(name);
                ctx.local_values.push_back(ci->stack[i + 1]);
            }
        }
    }
    return ctx;
}

// Store local variable values in a temporary global hash for expression evaluation
void setup_debug_context(mrb_state* mrb, const FrameContext& ctx) {
    mrb_value hash = mrb_hash_new(mrb);
    for (size_t i = 0; i < ctx.local_names.size(); ++i) {
        mrb_value key = mrb_str_new_cstr(mrb, ctx.local_names[i].c_str());
        mrb_hash_set(mrb, hash, key, ctx.local_values[i]);
    }
    mrb_gv_set(mrb, mrb_intern_cstr(mrb, DEBUG_CONTEXT_KEY), hash);
}

// Generate wrapper Ruby code that extracts locals from the debug context hash
std::string generate_wrapper(const FrameContext& ctx, const std::string& expr) {
    std::ostringstream code;
    for (const auto& name : ctx.local_names) {
        code << name << " = " << DEBUG_CONTEXT_KEY << "[\"" << name << "\"]\n";
    }
    code << expr;
    return code.str();
}

} // anonymous namespace

std::string serialize_value(mrb_state* mrb, mrb_value val, SerializeContext& ctx) {
    std::ostringstream oss;

    // Check depth limit
    if (ctx.current_depth > ctx.max_depth) {
        oss << "{\"type\":\"...\",\"value\":\"<max depth>\"}";
        return oss.str();
    }

    // Cycle detection for non-immediate values
    if (!mrb_immediate_p(val) && mrb_type(val) != MRB_TT_SYMBOL) {
        // Cast pointer to integer for identity-based cycle detection in visited set
        uintptr_t addr = reinterpret_cast<uintptr_t>(mrb_ptr(val));
        if (ctx.visited.count(addr)) {
            oss << "{\"type\":\"...\",\"value\":\"<circular>\"}";
            return oss.str();
        }
        ctx.visited.insert(addr);
    }

    ctx.current_depth++;

    switch (mrb_type(val)) {
        case MRB_TT_FALSE:
            if (mrb_nil_p(val)) {
                oss << "{\"type\":\"nil\",\"value\":\"nil\"}";
            } else {
                oss << "{\"type\":\"Boolean\",\"value\":\"false\"}";
            }
            break;

        case MRB_TT_TRUE:
            oss << "{\"type\":\"Boolean\",\"value\":\"true\"}";
            break;

        case MRB_TT_INTEGER:
            oss << "{\"type\":\"Integer\",\"value\":\"" << mrb_integer(val) << "\"}";
            break;

        case MRB_TT_FLOAT:
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "%.15g", mrb_float(val));
                oss << "{\"type\":\"Float\",\"value\":\"" << buf << "\"}";
            }
            break;

        case MRB_TT_SYMBOL:
            {
                const char* sym_name = mrb_sym_name(mrb, mrb_symbol(val));
                oss << "{\"type\":\"Symbol\",\"value\":\":" << json_escape(sym_name ? sym_name : "") << "\"}";
            }
            break;

        case MRB_TT_STRING:
            {
                const char* str = RSTRING_PTR(val);
                mrb_int len = RSTRING_LEN(val);
                std::string s(str, len);
                oss << "{\"type\":\"String\",\"value\":\"" << json_escape(s) << "\"}";
            }
            break;

        case MRB_TT_ARRAY:
            {
                mrb_int len = RARRAY_LEN(val);
                oss << "{\"type\":\"Array\",\"value\":\"[" << len << " elements]\",\"elements\":[";
                int max_elements = 10;  // Limit array display
                for (mrb_int i = 0; i < len && i < max_elements; ++i) {
                    if (i > 0) oss << ",";
                    oss << serialize_value(mrb, mrb_ary_ref(mrb, val, i), ctx);
                }
                if (len > max_elements) {
                    oss << ",{\"type\":\"...\",\"value\":\"<" << (len - max_elements) << " more>\"}";
                }
                oss << "]}";
            }
            break;

        case MRB_TT_HASH:
            {
                mrb_int len = mrb_hash_size(mrb, val);
                oss << "{\"type\":\"Hash\",\"value\":\"{" << len << " pairs}\"}";
            }
            break;

        case MRB_TT_OBJECT:
        case MRB_TT_CLASS:
        case MRB_TT_MODULE:
            {
                const char* class_name = mrb_obj_classname(mrb, val);
                oss << "{\"type\":\"" << json_escape(class_name ? class_name : "Object") << "\"";
                oss << ",\"value\":\"#<" << json_escape(class_name ? class_name : "Object");
                // Cast pointer to integer for Ruby-style object identity display (#<Class:0xADDR>)
                oss << ":0x" << std::hex << reinterpret_cast<uintptr_t>(mrb_ptr(val)) << std::dec;
                oss << ">\"}";
            }
            break;

        case MRB_TT_PROC:
            oss << "{\"type\":\"Proc\",\"value\":\"<proc>\"}";
            break;

        default:
            {
                const char* class_name = mrb_obj_classname(mrb, val);
                oss << "{\"type\":\"" << json_escape(class_name ? class_name : "unknown") << "\"";
                oss << ",\"value\":\"<native>\"}";
            }
            break;
    }

    ctx.current_depth--;
    return oss.str();
}

std::string get_locals_json(mrb_state* mrb, int frame_index) {
    std::ostringstream oss;
    oss << "{";

    if (!mrb || !mrb->c || !mrb->c->ci) {
        oss << "}";
        return oss.str();
    }

    // Navigate to the requested frame
    mrb_callinfo* target_ci = mrb->c->ci;
    for (int i = 0; i < frame_index && target_ci > mrb->c->cibase; ++i) {
        target_ci--;
    }

    if (target_ci <= mrb->c->cibase) {
        oss << "}";
        return oss.str();
    }

    // Get the proc and irep for this frame
    const struct RProc* proc = target_ci->proc;
    if (!proc || MRB_PROC_CFUNC_P(proc)) {
        oss << "}";
        return oss.str();
    }

    const mrb_irep* irep = proc->body.irep;
    if (!irep || !irep->lv) {
        oss << "}";
        return oss.str();
    }

    SerializeContext ctx;
    bool first = true;

    // Local variables are stored on the stack
    // irep->lv contains symbol names, stack contains values
    for (uint16_t i = 0; i < irep->nlocals - 1; ++i) {
        mrb_sym sym = irep->lv[i];
        if (sym == 0) continue;

        const char* name = mrb_sym_name(mrb, sym);
        if (!name) continue;

        // Get value from stack (locals start at stack[1])
        mrb_value val = target_ci->stack[i + 1];

        if (!first) oss << ",";
        first = false;

        oss << "\"" << json_escape(name) << "\":" << serialize_value(mrb, val, ctx);
    }

    oss << "}";
    return oss.str();
}

std::string get_instance_vars_json(mrb_state* mrb, mrb_value obj) {
    std::ostringstream oss;
    oss << "{";

    if (!mrb || mrb_nil_p(obj) || mrb_immediate_p(obj)) {
        oss << "}";
        return oss.str();
    }

    // This would require mrb_iv_foreach which needs a callback
    // For now, return empty - can be expanded later

    oss << "}";
    return oss.str();
}

std::string get_stack_trace_json(mrb_state* mrb) {
    std::ostringstream oss;
    oss << "[";

    if (!mrb || !mrb->c || !mrb->c->ci) {
        oss << "]";
        return oss.str();
    }

    bool first = true;
    int frame_id = 0;

    // Walk from current ci down to base
    for (mrb_callinfo* ci = mrb->c->ci; ci > mrb->c->cibase; ci--, frame_id++) {
        const char* method_name = "???";
        const char* file = "(unknown)";
        int32_t line = 0;

        // Get method name
        if (ci->mid != 0) {
            method_name = mrb_sym_name(mrb, ci->mid);
        }

        // Get file/line from irep debug info
        if (ci->proc && !MRB_PROC_CFUNC_P(ci->proc)) {
            const mrb_irep* irep = ci->proc->body.irep;
            if (irep && ci->pc) {
                uint32_t pc_offset = static_cast<uint32_t>(ci->pc - irep->iseq);
                const char* f = nullptr;
                int32_t l = 0;
                if (mrb_debug_get_position(mrb, irep, pc_offset, &l, &f)) {
                    if (f) file = f;
                    line = l;
                }
            }
        }

        if (!first) oss << ",";
        first = false;

        oss << "{";
        oss << "\"id\":" << frame_id << ",";
        oss << "\"name\":\"" << json_escape(method_name ? method_name : "???") << "\",";
        oss << "\"file\":\"" << json_escape(file) << "\",";
        oss << "\"line\":" << line;
        oss << "}";
    }

    oss << "]";
    return oss.str();
}

std::string evaluate_expression(mrb_state* mrb, const std::string& expr, int frame_index) {
    if (!mrb || expr.empty()) {
        return "{\"type\":\"Error\",\"value\":\"Invalid expression\"}";
    }

    // Navigate to target frame
    mrb_callinfo* target_ci = navigate_to_frame(mrb, frame_index);
    if (!target_ci) {
        return "{\"type\":\"Error\",\"value\":\"Invalid frame index: " +
               std::to_string(frame_index) + "\"}";
    }

    // Extract frame context (self, locals)
    FrameContext ctx = extract_frame_context(mrb, target_ci);

    mrb_value result = mrb_nil_value();

    if (ctx.is_cfunc || ctx.local_names.empty()) {
        // C frame or no locals - evaluate with correct self via instance_exec
        mrbc_context* cxt = mrbc_context_new(mrb);
        mrbc_filename(mrb, cxt, "(debug-eval)");
        std::string proc_code = "proc { " + expr + " }";
        mrb_value proc = mrb_load_string_cxt(mrb, proc_code.c_str(), cxt);
        mrbc_context_free(mrb, cxt);

        if (!mrb->exc) {
            result = mrb_funcall_with_block(mrb, ctx.self_value,
                mrb_intern_lit(mrb, "instance_exec"), 0, nullptr, proc);
        }
    } else {
        // Full frame context with locals - inject via temp global hash
        setup_debug_context(mrb, ctx);
        DebugContextGuard guard(mrb);

        std::string wrapper = generate_wrapper(ctx, expr);
        std::string proc_code = "proc { " + wrapper + " }";

        mrbc_context* cxt = mrbc_context_new(mrb);
        mrbc_filename(mrb, cxt, "(debug-eval)");
        mrb_value proc = mrb_load_string_cxt(mrb, proc_code.c_str(), cxt);
        mrbc_context_free(mrb, cxt);

        if (!mrb->exc) {
            result = mrb_funcall_with_block(mrb, ctx.self_value,
                mrb_intern_lit(mrb, "instance_exec"), 0, nullptr, proc);
        }
    }

    // Handle exceptions - return error JSON, don't raise
    if (mrb->exc) {
        // GMR_UNSAFE_MRUBY_CALL: Debugger expression evaluation - returns error JSON.
        // This is intentional: debugger should not raise; evaluation errors are
        // serialized and returned to the IDE for display.
        GMR_UNSAFE_MRUBY_CALL("debugger eval - errors returned as JSON, not raised")
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb_value msg = mrb_funcall(mrb, exc, "message", 0);
        std::string error_msg = mrb_string_p(msg) ?
            std::string(RSTRING_PTR(msg), RSTRING_LEN(msg)) : "Error";
        mrb->exc = nullptr;
        return "{\"type\":\"Error\",\"value\":\"" + json_escape(error_msg) + "\"}";
    }

    SerializeContext ser_ctx;
    return serialize_value(mrb, result, ser_ctx);
}

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
