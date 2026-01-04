#ifndef GMR_BINDINGS_MATH_HPP
#define GMR_BINDINGS_MATH_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register Vec2, Vec3, Rect classes as top-level Ruby classes
void register_math(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
