#ifndef GMR_BINDINGS_SERIALIZABLE_HPP
#define GMR_BINDINGS_SERIALIZABLE_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

/// Register the GMR::Serializable module for declarative serialization.
void register_serializable(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif // GMR_BINDINGS_SERIALIZABLE_HPP
