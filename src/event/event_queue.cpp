#include "gmr/event/event_queue.hpp"
#include "gmr/scripting/helpers.hpp"
#include <algorithm>

namespace gmr {
namespace event {

EventQueue& EventQueue::instance() {
    static EventQueue instance;
    return instance;
}

SubscriptionHandle EventQueue::subscribe_ruby(mrb_state* mrb, EventType type,
                                               mrb_value callback,
                                               mrb_value context) {
    SubscriptionHandle handle = next_handle_++;

    RubySubscription sub;
    sub.handle = handle;
    sub.type = type;
    sub.callback = callback;
    sub.context = context;

    // GC protect the Ruby values
    if (!mrb_nil_p(callback)) {
        mrb_gc_register(mrb, callback);
    }
    if (!mrb_nil_p(context)) {
        mrb_gc_register(mrb, context);
    }

    ruby_subscriptions_[handle] = sub;
    return handle;
}

void EventQueue::unsubscribe(mrb_state* mrb, SubscriptionHandle handle) {
    // Try C++ subscriptions first
    subscriptions_.erase(handle);

    // Try Ruby subscriptions
    auto it = ruby_subscriptions_.find(handle);
    if (it != ruby_subscriptions_.end()) {
        // Unprotect from GC
        if (!mrb_nil_p(it->second.callback)) {
            mrb_gc_unregister(mrb, it->second.callback);
        }
        if (!mrb_nil_p(it->second.context)) {
            mrb_gc_unregister(mrb, it->second.context);
        }
        ruby_subscriptions_.erase(it);
    }
}

void EventQueue::dispatch(mrb_state* mrb) {
    // Swap queues for safe iteration (prevents modification during dispatch)
    std::swap(event_queue_, processing_queue_);
    event_queue_.clear();

    // Process all queued events
    for (const Event& event : processing_queue_) {
        EventType type = get_event_type(event);

        // Dispatch to C++ subscribers
        for (const auto& [handle, sub] : subscriptions_) {
            if (sub.type == type) {
                sub.callback(event);
            }
        }

        // Dispatch to Ruby subscribers
        for (const auto& [handle, sub] : ruby_subscriptions_) {
            if (sub.type == type) {
                // For InputActionEvent, we could pass action/phase as arguments
                // For now, just invoke the callback
                if (!mrb_nil_p(sub.context)) {
                    scripting::safe_instance_exec(mrb, sub.context, sub.callback);
                } else {
                    scripting::safe_method_call(mrb, sub.callback, "call");
                }
            }
        }
    }

    processing_queue_.clear();
}

void EventQueue::clear(mrb_state* mrb) {
    event_queue_.clear();
    processing_queue_.clear();
    subscriptions_.clear();

    // Unprotect all Ruby values from GC
    for (auto& [handle, sub] : ruby_subscriptions_) {
        if (!mrb_nil_p(sub.callback)) {
            mrb_gc_unregister(mrb, sub.callback);
        }
        if (!mrb_nil_p(sub.context)) {
            mrb_gc_unregister(mrb, sub.context);
        }
    }
    ruby_subscriptions_.clear();

    // Reset handle counter for clean state
    next_handle_ = 0;
}

} // namespace event
} // namespace gmr
