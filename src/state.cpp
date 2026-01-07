#include "gmr/state.hpp"

namespace gmr {

State& State::instance() {
    static State instance;
    return instance;
}

} // namespace gmr
