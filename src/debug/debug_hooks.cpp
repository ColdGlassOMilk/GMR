#if defined(GMR_DEBUG_ENABLED)

#include "gmr/debug/debug_hooks.hpp"
#include "gmr/debug/debug_state.hpp"
#include "gmr/debug/debug_server.hpp"
#include "gmr/debug/breakpoint_manager.hpp"
#include <mruby/debug.h>

namespace gmr {
namespace debug {

void code_fetch_hook(mrb_state* mrb, const mrb_irep* irep,
                     const mrb_code* pc, mrb_value* regs) {
    (void)regs;  // Unused for now

    auto& server = DebugServer::instance();

    // Fast path: if not active, return immediately
    if (!server.is_active()) {
        return;
    }

    // Get current position from irep debug info
    int32_t line = 0;
    const char* file = nullptr;
    uint32_t pc_offset = static_cast<uint32_t>(pc - irep->iseq);

    if (!mrb_debug_get_position(mrb, irep, pc_offset, &line, &file)) {
        return;  // No debug info available
    }

    auto& state = DebugStateManager::instance();
    auto& breakpoints = BreakpointManager::instance();

    bool should_pause = false;
    PauseReason reason = PauseReason::NONE;

    // 1. Check for pause request from IDE
    if (state.pause_requested()) {
        should_pause = true;
        reason = PauseReason::PAUSE_COMMAND;
        state.clear_pause_request();
    }

    // 2. Check for breakpoint hit
    if (!should_pause && breakpoints.has(file, line)) {
        should_pause = true;
        reason = PauseReason::BREAKPOINT;
    }

    // 3. Check stepping conditions
    if (!should_pause && state.is_stepping()) {
        // Calculate current call depth
        int call_depth = static_cast<int>(mrb->c->ci - mrb->c->cibase);

        switch (state.current_state()) {
            case DebugState::STEPPING_OVER:
                // Stop at different line with same or lower call depth
                if (call_depth <= state.step_start_depth()) {
                    // Check if we're on a different line
                    std::string current_file = file ? file : "";
                    if (current_file != state.step_start_file() ||
                        line != state.step_start_line()) {
                        should_pause = true;
                        reason = PauseReason::STEP;
                    }
                }
                break;

            case DebugState::STEPPING_INTO:
                // Stop at any different line
                {
                    std::string current_file = file ? file : "";
                    if (current_file != state.step_start_file() ||
                        line != state.step_start_line()) {
                        should_pause = true;
                        reason = PauseReason::STEP;
                    }
                }
                break;

            case DebugState::STEPPING_OUT:
                // Stop when call depth decreases
                if (call_depth < state.step_start_depth()) {
                    should_pause = true;
                    reason = PauseReason::STEP;
                }
                break;

            default:
                break;
        }
    }

    if (should_pause) {
        server.enter_pause_loop(mrb, file, line, reason);
    }
}

void install_hooks(mrb_state* mrb) {
    if (mrb) {
#ifdef MRB_USE_DEBUG_HOOK
        mrb->code_fetch_hook = code_fetch_hook;
#endif
    }
}

void remove_hooks(mrb_state* mrb) {
    if (mrb) {
#ifdef MRB_USE_DEBUG_HOOK
        mrb->code_fetch_hook = nullptr;
#endif
    }
}

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
