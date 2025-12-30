#include "gmr/bindings/util.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "raylib.h"
#include <cstdlib>

namespace gmr {
namespace bindings {

static mrb_value mrb_random_int(mrb_state* mrb, mrb_value) {
    mrb_int min, max;
    mrb_get_args(mrb, "ii", &min, &max);
    return mrb_fixnum_value(GetRandomValue(min, max));
}

static mrb_value mrb_random_float(mrb_state* mrb, mrb_value) {
    return mrb_float_value(mrb, static_cast<double>(GetRandomValue(0, RAND_MAX)) / RAND_MAX);
}

static mrb_value mrb_quit(mrb_state*, mrb_value) {
    CloseWindow();
    exit(0);
    return mrb_nil_value();
}

void register_util(mrb_state* mrb) {
    define_method(mrb, "random_int", mrb_random_int, MRB_ARGS_REQ(2));
    define_method(mrb, "random_float", mrb_random_float, MRB_ARGS_NONE());
    define_method(mrb, "quit", mrb_quit, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
