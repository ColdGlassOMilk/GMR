#ifndef GMR_BINDINGS_WINDOW_HPP
#define GMR_BINDINGS_WINDOW_HPP

#include <mruby.h>
#include "raylib.h"

namespace gmr {
namespace bindings {

void register_window(mrb_state* mrb);
void cleanup_window();
RenderTexture2D& get_render_target();

} // namespace bindings
} // namespace gmr

#endif
