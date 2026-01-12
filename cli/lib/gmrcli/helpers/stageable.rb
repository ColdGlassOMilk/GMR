# frozen_string_literal: true

module Gmrcli
  # Mixin for commands that use staged execution with JSON event tracking.
  #
  # Include this module in command classes that need to track stage progress
  # and emit JSON events for each stage. The including class must define
  # @stages_completed as an array to track completed stages.
  #
  # @example
  #   class MyCommand
  #     include Stageable
  #
  #     def execute
  #       @stages_completed = []
  #       run_stage(:validate, "Validating") { validate! }
  #       run_stage(:build, "Building") { build! }
  #     end
  #   end
  #
  module Stageable
    # Run a stage with JSON event tracking.
    #
    # Emits stage_start event before yielding, stage_complete on success,
    # or stage_error on failure. Tracks completed stages in @stages_completed.
    #
    # @param stage_id [Symbol] Unique identifier for the stage
    # @param stage_name [String] Human-readable stage name for display
    # @yield The block to execute for this stage
    # @raise [StandardError] Re-raises any exception after emitting error event
    def run_stage(stage_id, stage_name)
      JsonEmitter.stage_start(stage_id, stage_name)
      yield
      @stages_completed << stage_id.to_s if @stages_completed
      JsonEmitter.stage_complete(stage_id)
    rescue StandardError => e
      error_details = e.respond_to?(:details) ? e.details : nil
      error_suggestions = e.respond_to?(:suggestions) ? e.suggestions : []
      error_code = e.respond_to?(:code) ? e.code : nil
      JsonEmitter.stage_error(stage_id, e.message, details: error_details, suggestions: error_suggestions, code: error_code)
      raise
    end
  end
end
