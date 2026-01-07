#ifndef GMR_BINDINGS_SPRITE_ANIMATION_HPP
#define GMR_BINDINGS_SPRITE_ANIMATION_HPP

#include <mruby.h>

namespace gmr {
namespace bindings {

// Register GMR::SpriteAnimation class
void register_sprite_animation(mrb_state* mrb);

} // namespace bindings
} // namespace gmr

#endif
