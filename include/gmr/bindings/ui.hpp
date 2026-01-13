#ifndef GMR_BINDINGS_UI_HPP
#define GMR_BINDINGS_UI_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register the UI module and classes (GMR::UI, GMR::UI::Node, etc.)
void register_ui(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
