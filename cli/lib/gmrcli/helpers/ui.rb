# frozen_string_literal: true

require "pastel"
require "tty-spinner"

module Gmrcli
  # UI helpers for consistent, beautiful output
  # All output is suppressed when JsonEmitter is enabled (--json mode)
  module UI
    class << self
      def pastel
        @pastel ||= Pastel.new(enabled: $stdout.tty?)
      end

      # Check if we're in JSON output mode (suppress human output)
      def json_mode?
        defined?(JsonEmitter) && JsonEmitter.enabled?
      end

      # === Banners and Headers ===

      def banner
        return if json_mode?
        # Banner output disabled
      end

      def header(title)
        return if json_mode?

        width = 67
        padding = (width - title.length - 2) / 2
        line = "=" * width
        puts ""
        puts pastel.cyan("+#{line}+")
        puts pastel.cyan("|#{' ' * padding}#{title}#{' ' * (width - padding - title.length)}|")
        puts pastel.cyan("+#{line}+")
        puts ""
      end

      # === Log Levels ===

      def step(message)
        return if json_mode?

        puts pastel.green("\n> #{message}")
      end

      def info(message)
        return if json_mode?

        puts pastel.blue("  i #{message}")
      end

      def success(message)
        return if json_mode?

        puts pastel.green("  + #{message}")
      end

      def warn(message)
        return if json_mode?

        puts pastel.yellow("  ! #{message}")
      end

      def error(message)
        return if json_mode?

        puts pastel.red("  x #{message}")
      end

      def debug(message)
        return if json_mode?

        puts pastel.dim("  # #{message}") if ENV["GMR_DEBUG"]
      end

      # === Formatted Output ===

      def list(items, indent: 4)
        return if json_mode?

        items.each do |item|
          puts "#{' ' * indent}- #{item}"
        end
      end

      def table(rows, headers: nil)
        return if json_mode?
        return if rows.empty?

        # Calculate column widths
        all_rows = headers ? [headers] + rows : rows
        widths = all_rows.first.map.with_index do |_, i|
          all_rows.map { |row| row[i].to_s.length }.max
        end

        # Print header
        if headers
          header_line = headers.map.with_index { |h, i| h.to_s.ljust(widths[i]) }.join("  ")
          puts pastel.bold("  #{header_line}")
          puts "  #{widths.map { |w| '-' * w }.join('  ')}"
        end

        # Print rows
        rows.each do |row|
          line = row.map.with_index { |cell, i| cell.to_s.ljust(widths[i]) }.join("  ")
          puts "  #{line}"
        end
      end

      def status_line(label, value, ok: true)
        return if json_mode?

        status = ok ? pastel.green(value) : pastel.red(value)
        puts "  #{label.ljust(20)} #{status}"
      end

      # === Interactive Elements ===

      def spinner(message, success_message: nil, error_message: nil)
        # In JSON mode, just execute the block without spinner UI
        if json_mode?
          begin
            return yield
          rescue StandardError
            raise
          end
        end

        spin = TTY::Spinner.new(
          "[:spinner] #{message}",
          format: :dots,
          success_mark: pastel.green("+"),
          error_mark: pastel.red("x")
        )
        spin.auto_spin

        begin
          result = yield
          spin.success(success_message || "")
          result
        rescue StandardError => e
          spin.error(error_message || e.message)
          raise
        end
      end

      def progress(message)
        # In JSON mode, just execute the block
        return yield if json_mode?

        print pastel.blue("  ... #{message}")
        result = yield
        puts pastel.green(" done")
        result
      rescue StandardError => e
        puts pastel.red(" failed") unless json_mode?
        raise
      end

      # === Confirmation ===

      def confirm?(message, default: true)
        # In JSON mode, always use default (non-interactive)
        return default if json_mode?

        default_hint = default ? "[Y/n]" : "[y/N]"
        print "#{message} #{default_hint} "

        response = $stdin.gets&.strip&.downcase
        return default if response.nil? || response.empty?

        %w[y yes].include?(response)
      end

      # === Spacing ===

      def blank
        return if json_mode?

        puts ""
      end

      def divider
        return if json_mode?

        puts pastel.dim("  " + "-" * 60)
      end

      # === Next Steps ===

      def next_steps(steps)
        return if json_mode?

        puts ""
        puts pastel.bold("Next steps:")
        puts ""
        steps.each_with_index do |step_text, i|
          puts "  #{pastel.cyan("#{i + 1}.")} #{step_text}"
        end
        puts ""
      end

      def command_hint(command, description = nil)
        return if json_mode?

        hint = "  #{pastel.cyan(command)}"
        hint += "  #{pastel.dim("# #{description}")}" if description
        puts hint
      end
    end
  end
end
