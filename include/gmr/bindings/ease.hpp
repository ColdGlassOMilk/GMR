#ifndef GMR_BINDINGS_EASE_HPP
#define GMR_BINDINGS_EASE_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register GMR::Ease module
void register_ease(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
