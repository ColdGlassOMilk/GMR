#ifndef GMR_BINDINGS_NODE_HPP
#define GMR_BINDINGS_NODE_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register Node class
void register_node(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
