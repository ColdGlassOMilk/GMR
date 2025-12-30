#include "gmr/scripting/helpers.hpp"

namespace gmr {
namespace scripting {

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
    
    if (mrb->exc) {
        mrb_print_error(mrb);
        mrb->exc = nullptr;
    }
}

} // namespace scripting
} // namespace gmr
