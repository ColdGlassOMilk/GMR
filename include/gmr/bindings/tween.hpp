#ifndef GMR_BINDINGS_TWEEN_HPP
#define GMR_BINDINGS_TWEEN_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register GMR::Tween class
void register_tween(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
