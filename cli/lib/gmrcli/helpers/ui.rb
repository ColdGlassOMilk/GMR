# frozen_string_literal: true

require "pastel"
require "tty-spinner"

module Gmrcli
  # UI helpers for consistent output
  # Delegates to Output::Human when in human mode
  # Suppresses output in machine mode (JSON output)
  module UI
    class << self
      def pastel
        @pastel ||= Pastel.new(enabled: $stdout.tty?)
      end

      # Check if we're in human output mode
      def human_mode?
        !json_mode?
      end

      # Check if we're in JSON output mode (machine mode is default)
      def json_mode?
        defined?(JsonEmitter) && JsonEmitter.enabled?
      end

      # === Banners and Headers ===

      def banner
        return if json_mode?

        Output::Human.banner
      end

      def header(title)
        return if json_mode?

        Output::Human.header(title)
      end

      # === Log Levels ===

      def step(message)
        return if json_mode?

        Output::Human.step(message)
      end

      def info(message)
        return if json_mode?

        Output::Human.info(message)
      end

      def success(message)
        return if json_mode?

        Output::Human.success(message)
      end

      def warn(message)
        return if json_mode?

        Output::Human.warn(message)
      end

      def error(message)
        return if json_mode?

        Output::Human.error(message)
      end

      def debug(message)
        return if json_mode?

        Output::Human.debug(message)
      end

      # === Formatted Output ===

      def list(items, indent: 4)
        return if json_mode?

        Output::Human.list(items, indent: indent)
      end

      def table(rows, headers: nil)
        return if json_mode?

        Output::Human.table(rows, headers: headers)
      end

      def status_line(label, value, ok: true)
        return if json_mode?

        Output::Human.status_line(label, value, ok: ok)
      end

      # === Interactive Elements ===

      def spinner(message, success_message: nil, error_message: nil, &block)
        # In JSON mode, just execute the block without spinner UI
        if json_mode?
          return yield
        end

        Output::Human.spinner(message, success_message: success_message, error_message: error_message, &block)
      end

      def progress(message, &block)
        # In JSON mode, just execute the block
        return yield if json_mode?

        Output::Human.progress(message, &block)
      end

      # === Confirmation ===

      def confirm?(message, default: true)
        # In JSON mode, always use default (non-interactive)
        return default if json_mode?

        Output::Human.confirm?(message, default: default)
      end

      # === Spacing ===

      def blank
        return if json_mode?

        Output::Human.blank
      end

      def divider
        return if json_mode?

        Output::Human.divider
      end

      # === Next Steps ===

      def next_steps(steps)
        return if json_mode?

        Output::Human.next_steps(steps)
      end

      def command_hint(command, description = nil)
        return if json_mode?

        Output::Human.command_hint(command, description)
      end
    end
  end
end
