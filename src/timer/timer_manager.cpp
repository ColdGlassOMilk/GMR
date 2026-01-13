#include "gmr/timer/timer_manager.hpp"
#include "gmr/scripting/helpers.hpp"
#include <algorithm>

namespace gmr {
namespace timer {

TimerManager& TimerManager::instance() {
    static TimerManager instance;
    return instance;
}

// ============================================================================
// Lifecycle
// ============================================================================

TimerHandle TimerManager::create() {
    TimerHandle handle = next_id_++;
    timers_[handle] = TimerState();
    return handle;
}

void TimerManager::destroy(TimerHandle handle) {
    auto it = timers_.find(handle);
    if (it != timers_.end()) {
        timers_.erase(it);
    }
}

TimerState* TimerManager::get(TimerHandle handle) {
    auto it = timers_.find(handle);
    if (it != timers_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool TimerManager::valid(TimerHandle handle) const {
    return timers_.find(handle) != timers_.end();
}

// ============================================================================
// Control
// ============================================================================

void TimerManager::cancel(TimerHandle handle) {
    auto* timer = get(handle);
    if (timer) {
        timer->cancelled = true;
        timer->active = false;
    }
}

void TimerManager::pause(TimerHandle handle) {
    auto* timer = get(handle);
    if (timer) {
        timer->active = false;
    }
}

void TimerManager::resume(TimerHandle handle) {
    auto* timer = get(handle);
    if (timer && !timer->cancelled) {
        timer->active = true;
    }
}

void TimerManager::cancel_by_name(const std::string& name) {
    if (name.empty()) return;

    for (auto& [handle, timer] : timers_) {
        if (timer.name == name) {
            timer.cancelled = true;
            timer.active = false;
        }
    }
}

size_t TimerManager::active_count() const {
    size_t count = 0;
    for (const auto& [handle, timer] : timers_) {
        if (timer.should_update()) {
            ++count;
        }
    }
    return count;
}

// ============================================================================
// Update Loop
// ============================================================================

void TimerManager::update(mrb_state* mrb, float dt_scaled, float dt_unscaled) {
    // Build list of handles to update (avoid iterator invalidation)
    std::vector<TimerHandle> handles_to_update;
    handles_to_update.reserve(timers_.size());

    for (auto& [handle, timer] : timers_) {
        if (timer.should_update()) {
            handles_to_update.push_back(handle);
        }
    }

    // Update each timer
    for (TimerHandle handle : handles_to_update) {
        TimerState* timer = get(handle);
        if (!timer || !timer->should_update()) continue;

        // Use scaled or unscaled dt based on timer setting
        float dt = timer->scaled ? dt_scaled : dt_unscaled;

        timer->elapsed += dt;

        // Check if timer should fire
        float threshold = timer->fired ? timer->interval : timer->delay;

        if (timer->elapsed >= threshold) {
            // Fire the callback
            invoke_callback(mrb, timer->callback);

            if (timer->interval > 0.0f) {
                // Repeating timer - subtract threshold, keep excess
                timer->elapsed -= threshold;
                timer->fired = true;
            } else {
                // One-shot timer - mark for removal
                timer->cancelled = true;
                timer->active = false;
            }
        }
    }

    // Cleanup finished timers
    cleanup_finished(mrb);
}

void TimerManager::invoke_callback(mrb_state* mrb, mrb_value callback) {
    if (mrb_nil_p(callback)) return;
    scripting::safe_method_call(mrb, callback, "call");
}

void TimerManager::cleanup_finished(mrb_state* mrb) {
    timers_to_remove_.clear();

    for (auto& [handle, timer] : timers_) {
        if (timer.cancelled) {
            // Unregister from GC before removal
            if (!mrb_nil_p(timer.ruby_timer_obj)) {
                mrb_gc_unregister(mrb, timer.ruby_timer_obj);
            }
            timers_to_remove_.push_back(handle);
        }
    }

    for (TimerHandle handle : timers_to_remove_) {
        timers_.erase(handle);
    }
}

// ============================================================================
// Cleanup
// ============================================================================

void TimerManager::clear(mrb_state* mrb) {
    // Unregister all Ruby objects from GC
    for (auto& [handle, timer] : timers_) {
        if (!mrb_nil_p(timer.ruby_timer_obj)) {
            mrb_gc_unregister(mrb, timer.ruby_timer_obj);
        }
    }

    timers_.clear();
    // Don't reset next_id_ - keeps handles unique across reloads
}

} // namespace timer
} // namespace gmr
