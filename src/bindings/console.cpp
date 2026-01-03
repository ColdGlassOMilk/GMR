#include "gmr/bindings/console.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>

namespace gmr {
namespace bindings {

// Evaluate Ruby code and return [success, result_string]
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

void register_console(mrb_state* mrb) {
    // Define eval_ruby as a global Kernel method
    mrb_define_method(mrb, mrb->kernel_module, "eval_ruby", mrb_eval_ruby, MRB_ARGS_REQ(1));
}

} // namespace bindings
} // namespace gmr