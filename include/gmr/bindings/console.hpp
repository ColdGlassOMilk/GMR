#ifndef GMR_BINDINGS_CONSOLE_HPP
#define GMR_BINDINGS_CONSOLE_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

void register_console(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif