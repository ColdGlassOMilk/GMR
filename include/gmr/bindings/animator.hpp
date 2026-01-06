#ifndef GMR_BINDINGS_ANIMATOR_HPP
#define GMR_BINDINGS_ANIMATOR_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register GMR::Animator class
void register_animator(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
