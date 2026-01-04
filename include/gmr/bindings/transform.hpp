#ifndef GMR_BINDINGS_TRANSFORM_HPP
#define GMR_BINDINGS_TRANSFORM_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register Transform2D class
void register_transform(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
