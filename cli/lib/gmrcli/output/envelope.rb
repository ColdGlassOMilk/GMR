# frozen_string_literal: true

require "time"

module Gmrcli
  module Output
    # Builds standardized JSON envelopes for CLI responses
    # All machine-readable output follows this structure
    class Envelope
      class << self
        # Build a success envelope
        # @param command [String] the command that was executed
        # @param result [Hash] command-specific result data
        # @param start_time [Time] when the command started
        # @return [Hash] the complete success envelope
        def success(command:, result:, start_time: nil)
          {
            protocol: Protocol::VERSION,
            status: "success",
            command: command.to_s,
            result: result,
            metadata: build_metadata(start_time)
          }
        end

        # Build an error envelope
        # @param command [String] the command that was executed
        # @param error [Hash] error data with code, message, details, suggestions
        # @param start_time [Time] when the command started
        # @return [Hash] the complete error envelope
        def error(command:, error:, start_time: nil)
          {
            protocol: Protocol::VERSION,
            status: "error",
            command: command.to_s,
            error: normalize_error(error),
            metadata: build_metadata(start_time)
          }
        end

        # Build a progress event (for NDJSON streaming)
        # @param type [Symbol, String] event type
        # @param data [Hash] event-specific data
        # @return [Hash] the progress event
        def event(type:, **data)
          {
            protocol: Protocol::VERSION,
            type: type.to_s,
            timestamp: Time.now.iso8601(3),
            **data
          }
        end

        # Build command start event
        def command_start(command:, **data)
          event(
            type: "command_start",
            command: command.to_s,
            **data
          )
        end

        # Build stage start event
        def stage_start(stage_id:, stage_name: nil)
          event(
            type: "stage_start",
            stage_id: stage_id.to_s,
            stage_name: stage_name || stage_id.to_s.tr("_", " ").capitalize
          )
        end

        # Build progress event
        def progress(stage_id:, percent:, message:, substage: nil)
          event(
            type: "progress",
            stage_id: stage_id.to_s,
            percent: percent.to_i.clamp(0, 100),
            message: message,
            substage: substage
          ).compact
        end

        # Build stage complete event
        def stage_complete(stage_id:, duration_ms: nil)
          event(
            type: "stage_complete",
            stage_id: stage_id.to_s,
            status: "success",
            duration_ms: duration_ms
          ).compact
        end

        # Build stage skip event
        def stage_skip(stage_id:, reason: nil)
          event(
            type: "stage_skip",
            stage_id: stage_id.to_s,
            status: "skipped",
            reason: reason
          ).compact
        end

        # Build stage error event
        def stage_error(stage_id:, error:)
          event(
            type: "stage_error",
            stage_id: stage_id.to_s,
            status: "error",
            error: normalize_error(error)
          )
        end

        # Build log event
        def log(message:, level: :info)
          event(
            type: "log",
            level: level.to_s,
            message: message
          )
        end

        private

        def build_metadata(start_time)
          {
            timestamp: Time.now.iso8601(3),
            duration_ms: start_time ? elapsed_ms(start_time) : nil,
            gmrcli_version: VERSION
          }.compact
        end

        def elapsed_ms(start_time)
          ((Time.now - start_time) * 1000).to_i
        end

        def normalize_error(error)
          case error
          when Hash
            {
              code: error[:code] || "INTERNAL.UNEXPECTED",
              message: error[:message],
              details: error[:details],
              suggestions: error[:suggestions],
              metadata: error[:metadata]
            }.compact
          when Gmrcli::Error
            error.to_h
          else
            {
              code: "INTERNAL.UNEXPECTED",
              message: error.to_s
            }
          end
        end
      end
    end
  end
end
