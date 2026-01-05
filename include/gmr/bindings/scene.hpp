#ifndef GMR_BINDINGS_SCENE_HPP
#define GMR_BINDINGS_SCENE_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register GMR::Scene class and GMR::SceneManager module
void register_scene(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
