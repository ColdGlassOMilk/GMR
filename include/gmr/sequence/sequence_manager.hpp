#ifndef GMR_SEQUENCE_MANAGER_HPP
#define GMR_SEQUENCE_MANAGER_HPP

#include <mruby.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <cstdint>

namespace gmr {
namespace sequence {

using SequenceHandle = int32_t;
constexpr SequenceHandle INVALID_SEQUENCE_HANDLE = -1;

// Step types for the sequence
enum class StepType {
    Call,       // Execute a block immediately
    Wait,       // Wait for a duration
    WaitUntil   // Wait until a condition is true
};

struct Step {
    StepType type;
    mrb_value block = mrb_nil_value();  // For Call, or condition for WaitUntil
    float duration = 0.0f;              // For Wait
};

// Sequence state - stored in manager
struct SequenceState {
    std::vector<Step> steps;
    size_t current_step = 0;
    float wait_remaining = 0.0f;
    bool scaled = true;           // Respect Time.scale?
    bool active = true;           // Currently running?
    bool cancelled = false;       // Marked for removal?
    bool completed = false;       // Finished all steps?
    std::string name;             // Optional name for cancellation
    mrb_value ruby_seq_obj = mrb_nil_value();  // Ruby Sequence object (for GC)
    mrb_value on_complete = mrb_nil_value();   // Completion callback

    bool should_update() const {
        return active && !cancelled && !completed;
    }
};

// SequenceManager - singleton that owns all sequence state
class SequenceManager {
public:
    static SequenceManager& instance();

    // Lifecycle
    SequenceHandle create();
    void destroy(SequenceHandle handle);
    SequenceState* get(SequenceHandle handle);

    // Control
    void cancel(SequenceHandle handle);
    void cancel_by_name(const std::string& name);

    // Update all sequences - called from main loop
    void update(mrb_state* mrb, float dt_scaled, float dt_unscaled);

    // Cleanup
    void clear(mrb_state* mrb);

    // Debug
    size_t count() const { return sequences_.size(); }
    size_t active_count() const;

    SequenceManager(const SequenceManager&) = delete;
    SequenceManager& operator=(const SequenceManager&) = delete;

private:
    SequenceManager() = default;

    void advance_step(mrb_state* mrb, SequenceState& seq);
    void cleanup_finished(mrb_state* mrb);

    std::unordered_map<SequenceHandle, SequenceState> sequences_;
    std::vector<SequenceHandle> sequences_to_remove_;
    SequenceHandle next_id_ = 0;
};

} // namespace sequence
} // namespace gmr

#endif // GMR_SEQUENCE_MANAGER_HPP
