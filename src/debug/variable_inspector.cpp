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
#include <sstream>
#include <cstdio>

namespace gmr {
namespace debug {

std::string serialize_value(mrb_state* mrb, mrb_value val, SerializeContext& ctx) {
    std::ostringstream oss;

    // Check depth limit
    if (ctx.current_depth > ctx.max_depth) {
        oss << "{\"type\":\"...\",\"value\":\"<max depth>\"}";
        return oss.str();
    }

    // Cycle detection for non-immediate values
    if (!mrb_immediate_p(val) && mrb_type(val) != MRB_TT_SYMBOL) {
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
    (void)frame_index;  // TODO: Evaluate in frame context

    if (!mrb || expr.empty()) {
        return "{\"type\":\"Error\",\"value\":\"Invalid expression\"}";
    }

    // Simple evaluation using mrb_load_string
    mrb_value result = mrb_load_string(mrb, expr.c_str());

    // Check for exception
    if (mrb->exc) {
        // GMR_UNSAFE_MRUBY_CALL: Debugger expression evaluation - returns error JSON.
        // This is intentional: debugger should not raise; evaluation errors are
        // serialized and returned to the IDE for display.
        GMR_UNSAFE_MRUBY_CALL("debugger eval - errors returned as JSON, not raised")
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb_value msg = mrb_funcall(mrb, exc, "message", 0);
        std::string error_msg = mrb_string_p(msg) ?
            std::string(RSTRING_PTR(msg), RSTRING_LEN(msg)) : "Error";
        scripting::safe_clear_exception(mrb, "variable_inspector eval");
        return "{\"type\":\"Error\",\"value\":\"" + json_escape(error_msg) + "\"}";
    }

    SerializeContext ctx;
    return serialize_value(mrb, result, ctx);
}

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
