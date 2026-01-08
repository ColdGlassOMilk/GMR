#ifndef GMR_EVENT_QUEUE_HPP
#define GMR_EVENT_QUEUE_HPP

#include "gmr/event/event.hpp"
#include <mruby.h>
#include <functional>
#include <vector>
#include <unordered_map>

namespace gmr {
namespace event {

using SubscriptionHandle = int32_t;
constexpr SubscriptionHandle INVALID_SUBSCRIPTION = -1;

class EventQueue {
public:
    static EventQueue& instance();

    // === Publishing ===
    template<typename EventT>
    void enqueue(EventT event);

    // === Subscribing (C++) ===
    // Returns handle for unsubscription
    template<typename EventT>
    SubscriptionHandle subscribe(std::function<void(const EventT&)> callback);

    // === Subscribing (Ruby) ===
    // For future Ruby integration - subscribes with mruby callback
    SubscriptionHandle subscribe_ruby(mrb_state* mrb, EventType type,
                                       mrb_value callback,
                                       mrb_value context = mrb_nil_value());

    // === Unsubscribing ===
    void unsubscribe(mrb_state* mrb, SubscriptionHandle handle);

    // === Per-frame dispatch (called from main.cpp) ===
    void dispatch(mrb_state* mrb);

    // === Cleanup ===
    void clear(mrb_state* mrb);

    // === Debug ===
    size_t pending_count() const { return event_queue_.size(); }
    size_t subscription_count() const { return subscriptions_.size() + ruby_subscriptions_.size(); }

private:
    EventQueue() = default;
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    // Double-buffered queue for safe iteration
    std::vector<Event> event_queue_;
    std::vector<Event> processing_queue_;

    // C++ subscriptions
    struct Subscription {
        SubscriptionHandle handle;
        EventType type;
        std::function<void(const Event&)> callback;
    };
    std::unordered_map<SubscriptionHandle, Subscription> subscriptions_;

    // Ruby subscriptions (need GC protection)
    struct RubySubscription {
        SubscriptionHandle handle;
        EventType type;
        mrb_value callback;
        mrb_value context;
    };
    std::unordered_map<SubscriptionHandle, RubySubscription> ruby_subscriptions_;

    SubscriptionHandle next_handle_{0};
};

// Template implementations (must be in header)

template<typename EventT>
void EventQueue::enqueue(EventT event) {
    event_queue_.emplace_back(std::move(event));
}

template<typename EventT>
SubscriptionHandle EventQueue::subscribe(std::function<void(const EventT&)> callback) {
    SubscriptionHandle handle = next_handle_++;

    Subscription sub;
    sub.handle = handle;
    sub.type = EventT::type;
    sub.callback = [cb = std::move(callback)](const Event& e) {
        if (auto* typed = std::get_if<EventT>(&e)) {
            cb(*typed);
        }
    };

    subscriptions_[handle] = std::move(sub);
    return handle;
}

} // namespace event
} // namespace gmr

#endif
