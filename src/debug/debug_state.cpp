#if defined(GMR_DEBUG_ENABLED)

#include "gmr/debug/debug_state.hpp"

namespace gmr {
namespace debug {

DebugStateManager& DebugStateManager::instance() {
    static DebugStateManager instance;
    return instance;
}

bool DebugStateManager::is_stepping() const {
    return state_ == DebugState::STEPPING_OVER ||
           state_ == DebugState::STEPPING_INTO ||
           state_ == DebugState::STEPPING_OUT;
}

void DebugStateManager::set_running() {
    state_ = DebugState::RUNNING;
    pause_reason_ = PauseReason::NONE;
}

void DebugStateManager::set_paused(PauseReason reason) {
    state_ = DebugState::PAUSED;
    pause_reason_ = reason;
}

void DebugStateManager::set_stepping_over(int call_depth, const char* file, int32_t line) {
    state_ = DebugState::STEPPING_OVER;
    step_start_depth_ = call_depth;
    step_start_file_ = file ? file : "";
    step_start_line_ = line;
}

void DebugStateManager::set_stepping_into(const char* file, int32_t line) {
    state_ = DebugState::STEPPING_INTO;
    step_start_file_ = file ? file : "";
    step_start_line_ = line;
}

void DebugStateManager::set_stepping_out(int call_depth) {
    state_ = DebugState::STEPPING_OUT;
    step_start_depth_ = call_depth;
}

void DebugStateManager::request_pause() {
    pause_requested_ = true;
}

void DebugStateManager::clear_pause_request() {
    pause_requested_ = false;
}

void DebugStateManager::reset() {
    state_ = DebugState::RUNNING;
    pause_reason_ = PauseReason::NONE;
    pause_requested_ = false;
    step_start_depth_ = 0;
    step_start_file_.clear();
    step_start_line_ = 0;
}

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
