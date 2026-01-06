#ifndef GMR_BINDINGS_STATE_MACHINE_HPP
#define GMR_BINDINGS_STATE_MACHINE_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register GMR::StateMachine class and Object#state_machine mixin
void register_state_machine(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
