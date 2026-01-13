#ifndef GMR_BINDINGS_DESTROYABLE_HPP
#define GMR_BINDINGS_DESTROYABLE_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

void register_destroyable(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif // GMR_BINDINGS_DESTROYABLE_HPP
