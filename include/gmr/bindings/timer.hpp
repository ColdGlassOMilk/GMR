#ifndef GMR_BINDINGS_TIMER_HPP
#define GMR_BINDINGS_TIMER_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

void register_timer(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif // GMR_BINDINGS_TIMER_HPP
