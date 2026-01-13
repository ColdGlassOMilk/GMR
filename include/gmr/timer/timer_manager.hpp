#ifndef GMR_TIMER_MANAGER_HPP
#define GMR_TIMER_MANAGER_HPP

#include <mruby.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace gmr {
namespace timer {

using TimerHandle = int32_t;
constexpr TimerHandle INVALID_TIMER_HANDLE = -1;

// Timer state - stored in manager, referenced by handle
struct TimerState {
    float delay = 0.0f;           // Initial delay before first fire
    float interval = 0.0f;        // Repeat interval (0 = one-shot)
    float elapsed = 0.0f;         // Time accumulated
    bool scaled = true;           // Respect Time.scale?
    bool active = true;           // Currently running?
    bool cancelled = false;       // Marked for removal?
    bool fired = false;           // Has fired at least once? (for one-shot tracking)
    mrb_value callback = mrb_nil_value();  // Ruby block to call
    mrb_value ruby_timer_obj = mrb_nil_value();  // The Ruby Timer object (for GC protection)
    std::string name;             // Optional name for cancellation by name

    bool should_update() const {
        return active && !cancelled;
    }
};

// TimerManager - singleton that owns all timer state
class TimerManager {
public:
    static TimerManager& instance();

    // Lifecycle
    TimerHandle create();
    void destroy(TimerHandle handle);
    TimerState* get(TimerHandle handle);
    bool valid(TimerHandle handle) const;

    // Control
    void cancel(TimerHandle handle);
    void pause(TimerHandle handle);
    void resume(TimerHandle handle);

    // Cancel by name (cancels ALL timers with that name)
    void cancel_by_name(const std::string& name);

    // Update all timers - called from main loop
    // dt_scaled: delta time with Time.scale applied
    // dt_unscaled: raw delta time
    void update(mrb_state* mrb, float dt_scaled, float dt_unscaled);

    // Cleanup - called on hot reload
    void clear(mrb_state* mrb);

    // Debug
    size_t count() const { return timers_.size(); }
    size_t active_count() const;

    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;

private:
    TimerManager() = default;

    void invoke_callback(mrb_state* mrb, mrb_value callback);
    void cleanup_finished(mrb_state* mrb);

    std::unordered_map<TimerHandle, TimerState> timers_;
    std::vector<TimerHandle> timers_to_remove_;
    TimerHandle next_id_ = 0;
};

} // namespace timer
} // namespace gmr

#endif // GMR_TIMER_MANAGER_HPP
