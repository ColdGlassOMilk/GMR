#ifndef GMR_BINDINGS_GRAPHICS_HPP
#define GMR_BINDINGS_GRAPHICS_HPP

#include <mruby.h>
#include <mruby/data.h>
#include "gmr/types.hpp"

namespace gmr {
namespace bindings {

void register_graphics(mrb_state* mrb);

// Font data structure for Graphics::Font Ruby objects
struct FontData {
    FontHandle handle;
};

// Font data type for mruby type checking
extern const mrb_data_type font_data_type;

// Helper to safely extract FontData from a Ruby Font object
FontData* get_font_data(mrb_state* mrb, mrb_value self);

} // namespace bindings
} // namespace gmr

#endif
