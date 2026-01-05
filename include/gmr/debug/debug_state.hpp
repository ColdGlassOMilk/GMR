#ifndef GMR_DEBUG_STATE_HPP
#define GMR_DEBUG_STATE_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <string>
#include <cstdint>

namespace gmr {
namespace debug {

// Debugger execution state
enum class DebugState {
    RUNNING,        // Normal execution
    PAUSED,         // Waiting for debug command
    STEPPING_OVER,  // Execute until same call depth, different line
    STEPPING_INTO,  // Execute single line, enter function calls
    STEPPING_OUT    // Execute until call depth decreases
};

// Reason for pausing
enum class PauseReason {
    NONE,
    BREAKPOINT,
    STEP,
    PAUSE_COMMAND,
    EXCEPTION
};

// Tracks debugger state and stepping context
class DebugStateManager {
public:
    static DebugStateManager& instance();

    // State queries
    DebugState current_state() const { return state_; }
    bool is_paused() const { return state_ == DebugState::PAUSED; }
    bool is_stepping() const;
    bool pause_requested() const { return pause_requested_; }

    // State transitions
    void set_running();
    void set_paused(PauseReason reason);
    void set_stepping_over(int call_depth, const char* file, int32_t line);
    void set_stepping_into(const char* file, int32_t line);
    void set_stepping_out(int call_depth);

    // Pause request (can be set from any thread, checked in hook)
    void request_pause();
    void clear_pause_request();

    // Stepping context
    int step_start_depth() const { return step_start_depth_; }
    const std::string& step_start_file() const { return step_start_file_; }
    int32_t step_start_line() const { return step_start_line_; }

    // Pause reason
    PauseReason last_pause_reason() const { return pause_reason_; }

    // Reset all state
    void reset();

    DebugStateManager(const DebugStateManager&) = delete;
    DebugStateManager& operator=(const DebugStateManager&) = delete;

private:
    DebugStateManager() = default;

    DebugState state_ = DebugState::RUNNING;
    PauseReason pause_reason_ = PauseReason::NONE;
    bool pause_requested_ = false;

    // Stepping context
    int step_start_depth_ = 0;
    std::string step_start_file_;
    int32_t step_start_line_ = 0;
};

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_DEBUG_STATE_HPP
