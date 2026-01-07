#ifndef GMR_INPUT_CONTEXT_HPP
#define GMR_INPUT_CONTEXT_HPP

#include "gmr/input/input_event.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace gmr {
namespace input {

/// @brief Represents a named input context with its own set of actions, callbacks, and state machine bindings
/// @details Input contexts allow scoping input mappings to different game states (gameplay, menu, dialogue, etc.)
struct InputContext {
    std::string name;
    std::unordered_map<std::string, ActionDefinition> actions;
    std::unordered_map<int, InputCallback> callbacks;
    std::vector<StateMachineInputBinding> sm_bindings;

    /// @brief If true, global actions are blocked when this context is active
    /// @details Used by contexts like console or pause menu that should prevent game input
    bool blocks_global = false;

    InputContext() = default;
    explicit InputContext(const std::string& ctx_name) : name(ctx_name) {}
};

/// @brief LIFO stack of input contexts
/// @details Only the top context receives input. Supports push/pop for temporary contexts
/// (e.g., pause menu) and set for complete context switches.
class ContextStack {
public:
    static ContextStack& instance();

    // === Stack Operations ===

    /// @brief Push a context onto the stack (makes it active)
    /// @param name Context name (auto-creates if doesn't exist)
    void push(const std::string& name);

    /// @brief Pop the top context from the stack
    /// @details Does nothing if stack is empty
    void pop();

    /// @brief Replace the entire stack with a single context
    /// @param name Context name to set as the only active context
    void set(const std::string& name);

    /// @brief Clear the stack (no active context)
    void clear();

    // === Query ===

    /// @brief Get the name of the currently active context
    /// @returns Empty string if no context is active
    std::string current_name() const;

    /// @brief Get the currently active context
    /// @returns nullptr if no context is active
    InputContext* current();
    const InputContext* current() const;

    /// @brief Check if a context exists in the definitions (not necessarily active)
    bool has(const std::string& name) const;

    /// @brief Check if the stack is empty
    bool empty() const;

    /// @brief Check if a specific context is currently active (anywhere in stack)
    bool is_active(const std::string& name) const;

    // === Context Definition ===

    /// @brief Get or create a context by name
    /// @param name Context name
    /// @returns Reference to the context (creates if doesn't exist)
    InputContext& define(const std::string& name);

    /// @brief Get a context by name (doesn't create)
    /// @returns nullptr if not found
    InputContext* get(const std::string& name);
    const InputContext* get(const std::string& name) const;

    // === Action Resolution ===

    /// @brief Resolve an action by checking current context, then falling back to global
    /// @param action Action name to resolve
    /// @returns nullptr if action not found in any context
    ActionDefinition* resolve_action(const std::string& action);

    /// @brief Check if an action exists in the currently active context
    bool action_active(const std::string& action) const;

    // === Cleanup ===

    /// @brief Clear all contexts and the stack (for hot reload)
    void clear_all();

    // === Debug ===
    size_t stack_size() const { return stack_.size(); }
    size_t context_count() const { return contexts_.size(); }

private:
    ContextStack();
    ContextStack(const ContextStack&) = delete;
    ContextStack& operator=(const ContextStack&) = delete;

    std::vector<std::string> stack_;  // Active context names (top = current)
    std::unordered_map<std::string, InputContext> contexts_;

    static constexpr const char* GLOBAL_CONTEXT = "global";
};

} // namespace input
} // namespace gmr

#endif // GMR_INPUT_CONTEXT_HPP
