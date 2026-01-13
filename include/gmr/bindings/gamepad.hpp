#ifndef GMR_BINDINGS_GAMEPAD_HPP
#define GMR_BINDINGS_GAMEPAD_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

void register_gamepad(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif // GMR_BINDINGS_GAMEPAD_HPP
