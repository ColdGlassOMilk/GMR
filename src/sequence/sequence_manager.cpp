#include "gmr/sequence/sequence_manager.hpp"
#include "gmr/scripting/helpers.hpp"

namespace gmr {
namespace sequence {

SequenceManager& SequenceManager::instance() {
    static SequenceManager instance;
    return instance;
}

// ============================================================================
// Lifecycle
// ============================================================================

SequenceHandle SequenceManager::create() {
    SequenceHandle handle = next_id_++;
    sequences_[handle] = SequenceState();
    return handle;
}

void SequenceManager::destroy(SequenceHandle handle) {
    auto it = sequences_.find(handle);
    if (it != sequences_.end()) {
        sequences_.erase(it);
    }
}

SequenceState* SequenceManager::get(SequenceHandle handle) {
    auto it = sequences_.find(handle);
    if (it != sequences_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// Control
// ============================================================================

void SequenceManager::cancel(SequenceHandle handle) {
    auto* seq = get(handle);
    if (seq) {
        seq->cancelled = true;
        seq->active = false;
    }
}

void SequenceManager::cancel_by_name(const std::string& name) {
    if (name.empty()) return;

    for (auto& [handle, seq] : sequences_) {
        if (seq.name == name) {
            seq.cancelled = true;
            seq.active = false;
        }
    }
}

size_t SequenceManager::active_count() const {
    size_t count = 0;
    for (const auto& [handle, seq] : sequences_) {
        if (seq.should_update()) {
            ++count;
        }
    }
    return count;
}

// ============================================================================
// Update Loop
// ============================================================================

void SequenceManager::update(mrb_state* mrb, float dt_scaled, float dt_unscaled) {
    // Build list of handles to update
    std::vector<SequenceHandle> handles_to_update;
    handles_to_update.reserve(sequences_.size());

    for (auto& [handle, seq] : sequences_) {
        if (seq.should_update()) {
            handles_to_update.push_back(handle);
        }
    }

    // Update each sequence
    for (SequenceHandle handle : handles_to_update) {
        SequenceState* seq = get(handle);
        if (!seq || !seq->should_update()) continue;

        float dt = seq->scaled ? dt_scaled : dt_unscaled;

        // Process current step
        if (seq->current_step >= seq->steps.size()) {
            // No more steps - mark completed
            seq->completed = true;
            seq->active = false;

            // Fire completion callback
            if (!mrb_nil_p(seq->on_complete)) {
                scripting::safe_method_call(mrb, seq->on_complete, "call");
            }
            continue;
        }

        Step& step = seq->steps[seq->current_step];

        switch (step.type) {
            case StepType::Call: {
                // Execute the block immediately
                scripting::safe_method_call(mrb, step.block, "call");
                advance_step(mrb, *seq);
                break;
            }

            case StepType::Wait: {
                // Wait for duration
                seq->wait_remaining -= dt;
                if (seq->wait_remaining <= 0.0f) {
                    advance_step(mrb, *seq);
                }
                break;
            }

            case StepType::WaitUntil: {
                // Check condition
                mrb_value result = scripting::safe_method_call(mrb, step.block, "call");
                if (mrb_test(result)) {
                    advance_step(mrb, *seq);
                }
                break;
            }
        }
    }

    // Cleanup finished sequences
    cleanup_finished(mrb);
}

void SequenceManager::advance_step(mrb_state* mrb, SequenceState& seq) {
    seq.current_step++;

    // Initialize next step if there is one
    if (seq.current_step < seq.steps.size()) {
        Step& next = seq.steps[seq.current_step];
        if (next.type == StepType::Wait) {
            seq.wait_remaining = next.duration;
        }
    }
}

void SequenceManager::cleanup_finished(mrb_state* mrb) {
    sequences_to_remove_.clear();

    for (auto& [handle, seq] : sequences_) {
        if (seq.cancelled || seq.completed) {
            if (!mrb_nil_p(seq.ruby_seq_obj)) {
                mrb_gc_unregister(mrb, seq.ruby_seq_obj);
            }
            sequences_to_remove_.push_back(handle);
        }
    }

    for (SequenceHandle handle : sequences_to_remove_) {
        sequences_.erase(handle);
    }
}

// ============================================================================
// Cleanup
// ============================================================================

void SequenceManager::clear(mrb_state* mrb) {
    for (auto& [handle, seq] : sequences_) {
        if (!mrb_nil_p(seq.ruby_seq_obj)) {
            mrb_gc_unregister(mrb, seq.ruby_seq_obj);
        }
    }

    sequences_.clear();
}

} // namespace sequence
} // namespace gmr
