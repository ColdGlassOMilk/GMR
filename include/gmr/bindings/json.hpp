#ifndef GMR_BINDINGS_JSON_HPP
#define GMR_BINDINGS_JSON_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

/// Register the GMR::JSON module for JSON parsing and stringifying.
void register_json(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif // GMR_BINDINGS_JSON_HPP
