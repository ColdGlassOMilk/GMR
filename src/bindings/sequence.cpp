#include "gmr/bindings/sequence.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/sequence/sequence_manager.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/string.h>

namespace gmr {
namespace bindings {

// ============================================================================
// Sequence Class
// ============================================================================

/// @class GMR::Sequence
/// @description A step-by-step execution sequence for multi-step behaviors.
///   Useful for boss patterns, cutscenes, tutorials, and anything that needs
///   "do A, wait, do B, wait for condition, do C" logic.
/// @example Sequence.run do |s|
///     s.call { @boss.telegraph(:attack) }
///     s.wait(0.8)
///     s.call { @boss.attack }
///     s.wait_until { @boss.attack_complete? }
///     s.call { @boss.recover }
///   end
///
/// @example # Named sequence for cancellation
///   Sequence.run(:boss_intro) do |s|
///     s.call { show_title("BOSS BATTLE") }
///     s.wait(2.0)
///     s.call { hide_title }
///   end
///   # Later: Sequence.cancel(:boss_intro)

struct SequenceData {
    sequence::SequenceHandle handle;
};

static void sequence_data_free(mrb_state*, void* ptr) {
    delete static_cast<SequenceData*>(ptr);
}

static const mrb_data_type sequence_data_type = {
    "Sequence", sequence_data_free
};

static RClass* sequence_class_ptr = nullptr;

static SequenceData* get_sequence_data(mrb_state* mrb, mrb_value self) {
    return static_cast<SequenceData*>(mrb_data_get_ptr(mrb, self, &sequence_data_type));
}

// Create Ruby Sequence object wrapping a handle
static mrb_value create_sequence_object(mrb_state* mrb, sequence::SequenceHandle handle) {
    if (!sequence_class_ptr) {
        RClass* gmr = get_gmr_module(mrb);
        sequence_class_ptr = mrb_class_get_under(mrb, gmr, "Sequence");
    }

    auto* data = new SequenceData{handle};
    mrb_value obj = mrb_obj_value(mrb_data_object_alloc(mrb, sequence_class_ptr, data, &sequence_data_type));

    // Register with GC
    mrb_gc_register(mrb, obj);

    // Store reference back
    sequence::SequenceState* state = sequence::SequenceManager::instance().get(handle);
    if (state) {
        state->ruby_seq_obj = obj;
    }

    return obj;
}

// ============================================================================
// Module Functions
// ============================================================================

/// @function run
/// @description Create and start a new sequence. The block receives the sequence
///   as a builder to define steps.
/// @param name [Symbol] (optional) Name for cancellation via Sequence.cancel(:name)
/// @param block [Block] Builder block that receives the sequence
/// @returns [Sequence] The sequence object
/// @example Sequence.run { |s| s.wait(1.0); s.call { puts "done" } }
/// @example Sequence.run(:intro) { |s| ... }
static mrb_value mrb_sequence_run(mrb_state* mrb, mrb_value) {
    mrb_value name_or_block = mrb_nil_value();
    mrb_value block = mrb_nil_value();

    // Parse args: optional name symbol, required block
    mrb_int argc = mrb_get_argc(mrb);
    if (argc == 0) {
        // Get block
        mrb_get_args(mrb, "&!", &block);
    } else {
        mrb_get_args(mrb, "o&!", &name_or_block, &block);
    }

    std::string name;
    if (mrb_symbol_p(name_or_block)) {
        name = mrb_sym_name(mrb, mrb_symbol(name_or_block));
    }

    // Create sequence
    sequence::SequenceHandle handle = sequence::SequenceManager::instance().create();
    sequence::SequenceState* state = sequence::SequenceManager::instance().get(handle);
    if (!state) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Failed to create sequence");
        return mrb_nil_value();
    }

    state->name = name;

    // Create Ruby object
    mrb_value seq_obj = create_sequence_object(mrb, handle);

    // Yield the sequence to the builder block
    // The block calls s.call, s.wait, etc. to add steps
    mrb_yield(mrb, block, seq_obj);

    // Initialize first step if it's a wait
    if (!state->steps.empty() && state->steps[0].type == sequence::StepType::Wait) {
        state->wait_remaining = state->steps[0].duration;
    }

    return seq_obj;
}

/// @function cancel
/// @description Cancel all sequences with the given name.
/// @param name [Symbol] The name of the sequence(s) to cancel
/// @returns [nil]
/// @example Sequence.cancel(:boss_intro)
static mrb_value mrb_sequence_cancel_by_name(mrb_state* mrb, mrb_value) {
    mrb_sym name;
    mrb_get_args(mrb, "n", &name);

    std::string name_str = mrb_sym_name(mrb, name);
    sequence::SequenceManager::instance().cancel_by_name(name_str);

    return mrb_nil_value();
}

// ============================================================================
// Builder Methods (called during run block)
// ============================================================================

/// @method call
/// @description Add a step that executes a block immediately.
/// @param block [Block] The code to execute
/// @returns [self]
/// @example s.call { @boss.telegraph(:attack) }
static mrb_value mrb_sequence_call(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&!", &block);

    SequenceData* data = get_sequence_data(mrb, self);
    if (!data) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Sequence");
        return mrb_nil_value();
    }

    sequence::SequenceState* state = sequence::SequenceManager::instance().get(data->handle);
    if (!state) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Sequence no longer valid");
        return mrb_nil_value();
    }

    sequence::Step step;
    step.type = sequence::StepType::Call;
    step.block = block;
    state->steps.push_back(step);

    return self;
}

/// @method wait
/// @description Add a step that waits for a duration.
/// @param duration [Float] Time to wait in seconds
/// @returns [self]
/// @example s.wait(0.5)
static mrb_value mrb_sequence_wait(mrb_state* mrb, mrb_value self) {
    mrb_float duration;
    mrb_get_args(mrb, "f", &duration);

    if (duration < 0.0) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Wait duration cannot be negative");
        return mrb_nil_value();
    }

    SequenceData* data = get_sequence_data(mrb, self);
    if (!data) return mrb_nil_value();

    sequence::SequenceState* state = sequence::SequenceManager::instance().get(data->handle);
    if (!state) return mrb_nil_value();

    sequence::Step step;
    step.type = sequence::StepType::Wait;
    step.duration = static_cast<float>(duration);
    state->steps.push_back(step);

    return self;
}

/// @method wait_until
/// @description Add a step that waits until a condition becomes true.
/// @param block [Block] Condition block that returns true when ready to proceed
/// @returns [self]
/// @example s.wait_until { @boss.attack_complete? }
static mrb_value mrb_sequence_wait_until(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&!", &block);

    SequenceData* data = get_sequence_data(mrb, self);
    if (!data) return mrb_nil_value();

    sequence::SequenceState* state = sequence::SequenceManager::instance().get(data->handle);
    if (!state) return mrb_nil_value();

    sequence::Step step;
    step.type = sequence::StepType::WaitUntil;
    step.block = block;
    state->steps.push_back(step);

    return self;
}

/// @method then
/// @description Add a completion callback. Alias for on_complete.
/// @param block [Block] Block to call when sequence completes
/// @returns [self]
/// @example Sequence.run { |s| s.wait(1.0) }.then { puts "done!" }
static mrb_value mrb_sequence_then(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&!", &block);

    SequenceData* data = get_sequence_data(mrb, self);
    if (!data) return self;

    sequence::SequenceState* state = sequence::SequenceManager::instance().get(data->handle);
    if (!state) return self;

    state->on_complete = block;
    return self;
}

// ============================================================================
// Instance Methods
// ============================================================================

/// @method cancel
/// @description Cancel this sequence.
/// @returns [self]
/// @example @my_sequence.cancel
static mrb_value mrb_sequence_instance_cancel(mrb_state* mrb, mrb_value self) {
    SequenceData* data = get_sequence_data(mrb, self);
    if (!data) return self;

    sequence::SequenceManager::instance().cancel(data->handle);
    return self;
}

/// @method active?
/// @description Check if this sequence is still running.
/// @returns [Boolean] true if sequence is active
static mrb_value mrb_sequence_active(mrb_state* mrb, mrb_value self) {
    SequenceData* data = get_sequence_data(mrb, self);
    if (!data) return mrb_false_value();

    sequence::SequenceState* state = sequence::SequenceManager::instance().get(data->handle);
    if (!state) return mrb_false_value();

    return to_mrb_bool(mrb, state->should_update());
}

/// @method completed?
/// @description Check if this sequence has completed all steps.
/// @returns [Boolean] true if sequence completed
static mrb_value mrb_sequence_completed(mrb_state* mrb, mrb_value self) {
    SequenceData* data = get_sequence_data(mrb, self);
    if (!data) return mrb_false_value();

    sequence::SequenceState* state = sequence::SequenceManager::instance().get(data->handle);
    if (!state) return mrb_true_value();  // Gone = completed

    return to_mrb_bool(mrb, state->completed);
}

// ============================================================================
// Registration
// ============================================================================

void register_sequence(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // Define Sequence class
    sequence_class_ptr = mrb_define_class_under(mrb, gmr, "Sequence", mrb->object_class);
    MRB_SET_INSTANCE_TT(sequence_class_ptr, MRB_TT_CDATA);

    // Module functions
    mrb_define_class_method(mrb, sequence_class_ptr, "run", mrb_sequence_run, MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
    mrb_define_class_method(mrb, sequence_class_ptr, "cancel", mrb_sequence_cancel_by_name, MRB_ARGS_REQ(1));

    // Builder methods
    mrb_define_method(mrb, sequence_class_ptr, "call", mrb_sequence_call, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, sequence_class_ptr, "wait", mrb_sequence_wait, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sequence_class_ptr, "wait_until", mrb_sequence_wait_until, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, sequence_class_ptr, "then", mrb_sequence_then, MRB_ARGS_BLOCK());

    // Instance methods
    mrb_define_method(mrb, sequence_class_ptr, "cancel", mrb_sequence_instance_cancel, MRB_ARGS_NONE());
    mrb_define_method(mrb, sequence_class_ptr, "active?", mrb_sequence_active, MRB_ARGS_NONE());
    mrb_define_method(mrb, sequence_class_ptr, "completed?", mrb_sequence_completed, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
