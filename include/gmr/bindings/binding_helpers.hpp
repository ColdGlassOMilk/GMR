#ifndef GMR_BINDING_HELPERS_HPP
#define GMR_BINDING_HELPERS_HPP

#include "gmr/types.hpp"
#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/hash.h>

namespace gmr {
namespace bindings {

// Parse color from mruby args - supports [r,g,b], [r,g,b,a], or r,g,b / r,g,b,a
Color parse_color(mrb_state* mrb, mrb_value* argv, mrb_int argc, const Color& default_color);

// Helper to return boolean
inline mrb_value to_mrb_bool(mrb_state*, bool value) {
    return value ? mrb_true_value() : mrb_false_value();
}

// Helper to define methods more cleanly
inline void define_method(mrb_state* mrb, const char* name, mrb_func_t func, mrb_aspec aspec) {
    mrb_define_method(mrb, mrb->kernel_module, name, func, aspec);
}

} // namespace bindings
} // namespace gmr

#endif
