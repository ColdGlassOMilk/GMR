#ifndef GMR_BINDINGS_CAMERA_HPP
#define GMR_BINDINGS_CAMERA_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register Camera2D class
void register_camera(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
