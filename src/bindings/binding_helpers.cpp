#include "gmr/bindings/binding_helpers.hpp"

namespace gmr {
namespace bindings {

Color parse_color(mrb_state* mrb, mrb_value* argv, mrb_int argc, const Color& default_color) {
    if (argc == 0) {
        return default_color;
    }
    
    if (argc == 1 && mrb_array_p(argv[0])) {
        mrb_value arr = argv[0];
        mrb_int len = RARRAY_LEN(arr);
        
        if (len < 3) {
            mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected array of 3-4 elements [r,g,b] or [r,g,b,a]");
            return default_color;
        }
        
        uint8_t r = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, arr, 0)));
        uint8_t g = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, arr, 1)));
        uint8_t b = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, arr, 2)));
        uint8_t a = len > 3 ? static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, arr, 3))) : 255;
        
        return Color{r, g, b, a};
    }
    
    if (argc >= 3) {
        uint8_t r = static_cast<uint8_t>(mrb_fixnum(argv[0]));
        uint8_t g = static_cast<uint8_t>(mrb_fixnum(argv[1]));
        uint8_t b = static_cast<uint8_t>(mrb_fixnum(argv[2]));
        uint8_t a = argc > 3 ? static_cast<uint8_t>(mrb_fixnum(argv[3])) : 255;
        
        return Color{r, g, b, a};
    }
    
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected [r,g,b,a] array or 3-4 integers");
    return default_color;
}

} // namespace bindings
} // namespace gmr
