#include "gmr/input/input_context.hpp"
#include <algorithm>

namespace gmr {
namespace input {

ContextStack::ContextStack() {
    // Initialize with global context
    contexts_[GLOBAL_CONTEXT] = InputContext(GLOBAL_CONTEXT);
}

ContextStack& ContextStack::instance() {
    static ContextStack instance;
    return instance;
}

// === Stack Operations ===

void ContextStack::push(const std::string& name) {
    // Auto-create context if it doesn't exist
    if (contexts_.find(name) == contexts_.end()) {
        contexts_[name] = InputContext(name);
    }
    stack_.push_back(name);
}

void ContextStack::pop() {
    if (!stack_.empty()) {
        stack_.pop_back();
    }
}

void ContextStack::set(const std::string& name) {
    stack_.clear();
    push(name);
}

void ContextStack::clear() {
    stack_.clear();
}

// === Query ===

std::string ContextStack::current_name() const {
    if (stack_.empty()) {
        return "";
    }
    return stack_.back();
}

InputContext* ContextStack::current() {
    if (stack_.empty()) {
        return nullptr;
    }
    auto it = contexts_.find(stack_.back());
    return (it != contexts_.end()) ? &it->second : nullptr;
}

const InputContext* ContextStack::current() const {
    if (stack_.empty()) {
        return nullptr;
    }
    auto it = contexts_.find(stack_.back());
    return (it != contexts_.end()) ? &it->second : nullptr;
}

bool ContextStack::has(const std::string& name) const {
    return contexts_.find(name) != contexts_.end();
}

bool ContextStack::empty() const {
    return stack_.empty();
}

bool ContextStack::is_active(const std::string& name) const {
    return std::find(stack_.begin(), stack_.end(), name) != stack_.end();
}

// === Context Definition ===

InputContext& ContextStack::define(const std::string& name) {
    auto it = contexts_.find(name);
    if (it == contexts_.end()) {
        contexts_[name] = InputContext(name);
        return contexts_[name];
    }
    return it->second;
}

InputContext* ContextStack::get(const std::string& name) {
    auto it = contexts_.find(name);
    return (it != contexts_.end()) ? &it->second : nullptr;
}

const InputContext* ContextStack::get(const std::string& name) const {
    auto it = contexts_.find(name);
    return (it != contexts_.end()) ? &it->second : nullptr;
}

// === Action Resolution ===

ActionDefinition* ContextStack::resolve_action(const std::string& action) {
    // Check current context first
    if (InputContext* ctx = current()) {
        auto it = ctx->actions.find(action);
        if (it != ctx->actions.end()) {
            return &it->second;
        }
    }

    // Fall back to global context
    InputContext* global = get(GLOBAL_CONTEXT);
    if (global) {
        auto it = global->actions.find(action);
        if (it != global->actions.end()) {
            return &it->second;
        }
    }

    return nullptr;
}

bool ContextStack::action_active(const std::string& action) const {
    // Check current context
    if (const InputContext* ctx = current()) {
        if (ctx->actions.find(action) != ctx->actions.end()) {
            return true;
        }
    }

    // Check global context
    const InputContext* global = get(GLOBAL_CONTEXT);
    if (global) {
        return global->actions.find(action) != global->actions.end();
    }

    return false;
}

// === Cleanup ===

void ContextStack::clear_all() {
    stack_.clear();
    contexts_.clear();
    // Re-initialize global context
    contexts_[GLOBAL_CONTEXT] = InputContext(GLOBAL_CONTEXT);
}

} // namespace input
} // namespace gmr
