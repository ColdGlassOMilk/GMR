# frozen_string_literal: true

require "json"
require "time"

module Gmrcli
  # JSON event emitter for machine-readable output (NDJSON format)
  # Each event is a single JSON object on its own line for streaming
  module JsonEmitter
    # Stage definitions for different build targets
    SETUP_STAGES = {
      native: [
        { id: "environment", name: "Environment Detection", weight: 5 },
        { id: "directories", name: "Directory Setup", weight: 5 },
        { id: "packages", name: "Installing Packages", weight: 10, optional: true },
        { id: "mruby_native", name: "Building mruby (native)", weight: 35 },
        { id: "raylib_native", name: "Building raylib (native)", weight: 35 },
        { id: "verification", name: "Verification", weight: 5 },
        { id: "completion", name: "Complete", weight: 5 }
      ],
      web: [
        { id: "environment", name: "Environment Detection", weight: 5 },
        { id: "directories", name: "Directory Setup", weight: 5 },
        { id: "emscripten", name: "Setting up Emscripten", weight: 20 },
        { id: "raylib_web", name: "Building raylib (web)", weight: 30 },
        { id: "mruby_web", name: "Building mruby (web)", weight: 30 },
        { id: "verification", name: "Verification", weight: 5 },
        { id: "completion", name: "Complete", weight: 5 }
      ],
      full: [
        { id: "environment", name: "Environment Detection", weight: 3 },
        { id: "directories", name: "Directory Setup", weight: 2 },
        { id: "packages", name: "Installing Packages", weight: 5, optional: true },
        { id: "mruby_native", name: "Building mruby (native)", weight: 18 },
        { id: "raylib_native", name: "Building raylib (native)", weight: 17 },
        { id: "emscripten", name: "Setting up Emscripten", weight: 10 },
        { id: "raylib_web", name: "Building raylib (web)", weight: 17 },
        { id: "mruby_web", name: "Building mruby (web)", weight: 18 },
        { id: "verification", name: "Verification", weight: 5 },
        { id: "completion", name: "Complete", weight: 5 }
      ]
    }.freeze

    BUILD_STAGES = {
      debug: [
        { id: "validate", name: "Validating Environment", weight: 10 },
        { id: "configure", name: "Configuring Build", weight: 20 },
        { id: "compile", name: "Compiling", weight: 60 },
        { id: "completion", name: "Complete", weight: 10 }
      ],
      release: [
        { id: "validate", name: "Validating Environment", weight: 10 },
        { id: "configure", name: "Configuring Build", weight: 20 },
        { id: "compile", name: "Compiling", weight: 60 },
        { id: "completion", name: "Complete", weight: 10 }
      ],
      web: [
        { id: "validate", name: "Validating Environment", weight: 10 },
        { id: "configure", name: "Configuring Build", weight: 20 },
        { id: "compile", name: "Compiling (WASM)", weight: 60 },
        { id: "completion", name: "Complete", weight: 10 }
      ]
    }.freeze

    ESTIMATED_TIMES = {
      setup: { native: 10, web: 15, full: 25 },
      build: { debug: 2, release: 3, web: 5 }
    }.freeze

    class << self
      def enabled?
        @enabled ||= false
      end

      def enable!
        @enabled = true
        @start_time = Time.now
        @stage_start_times = {}
      end

      def disable!
        @enabled = false
      end

      # Emit a JSON event (one line of NDJSON)
      def emit(event_type, data = {})
        return unless enabled?

        event = {
          type: event_type.to_s,
          timestamp: Time.now.iso8601(3),
          **data
        }
        puts event.to_json
        $stdout.flush
      end

      # === Setup Events ===

      def setup_start(target)
        return unless enabled?

        target_sym = target.to_s.to_sym
        stages = SETUP_STAGES[target_sym] || SETUP_STAGES[:native]

        emit(:setup_start, {
          target: target.to_s,
          stages: stages.reject { |s| s[:optional] }.map { |s| { id: s[:id], name: s[:name] } },
          estimatedMinutes: ESTIMATED_TIMES[:setup][target_sym] || 15
        })
      end

      def setup_complete(duration_ms: nil)
        return unless enabled?

        duration = duration_ms || elapsed_ms(@start_time)
        emit(:setup_complete, {
          status: "success",
          durationMs: duration
        })
      end

      # === Build Events ===

      def build_start(build_type)
        return unless enabled?

        type_sym = build_type.to_s.downcase.to_sym
        stages = BUILD_STAGES[type_sym] || BUILD_STAGES[:debug]

        emit(:build_start, {
          buildType: build_type.to_s,
          stages: stages.map { |s| { id: s[:id], name: s[:name] } },
          estimatedMinutes: ESTIMATED_TIMES[:build][type_sym] || 3
        })
      end

      def build_complete(output_path: nil, duration_ms: nil)
        return unless enabled?

        duration = duration_ms || elapsed_ms(@start_time)
        emit(:build_complete, {
          status: "success",
          outputPath: output_path,
          durationMs: duration
        })
      end

      # === Stage Events ===

      def stage_start(stage_id, stage_name = nil)
        return unless enabled?

        @stage_start_times[stage_id] = Time.now
        emit(:stage_start, {
          stageId: stage_id.to_s,
          stageName: stage_name || stage_id.to_s.tr("_", " ").capitalize
        })
      end

      def stage_progress(stage_id, percent, message, substage: nil)
        return unless enabled?

        emit(:stage_progress, {
          stageId: stage_id.to_s,
          percent: percent.to_i,
          message: message,
          substage: substage
        })
      end

      def stage_complete(stage_id, duration_ms: nil)
        return unless enabled?

        start_time = @stage_start_times[stage_id]
        duration = duration_ms || (start_time ? elapsed_ms(start_time) : nil)

        emit(:stage_complete, {
          stageId: stage_id.to_s,
          status: "success",
          durationMs: duration
        })
      end

      def stage_skip(stage_id, reason: nil)
        return unless enabled?

        emit(:stage_skip, {
          stageId: stage_id.to_s,
          status: "skipped",
          reason: reason
        })
      end

      def stage_error(stage_id, message, details: nil, suggestions: [])
        return unless enabled?

        emit(:stage_error, {
          stageId: stage_id.to_s,
          status: "error",
          error: {
            message: message,
            details: details,
            suggestions: suggestions
          }
        })
      end

      # === Utility ===

      def log(message, level: :info)
        return unless enabled?

        emit(:log, {
          level: level.to_s,
          message: message
        })
      end

      private

      def elapsed_ms(start_time)
        return nil unless start_time

        ((Time.now - start_time) * 1000).to_i
      end
    end
  end
end
