# frozen_string_literal: true

require "pastel"
require "tty-spinner"

module Gmrcli
  # UI helpers for consistent, beautiful output
  module UI
    class << self
      def pastel
        @pastel ||= Pastel.new(enabled: $stdout.tty?)
      end

      # === Banners and Headers ===

      def banner
        # puts pastel.cyan <<~BANNER

        #   +===================================================================+
        #   |                         GMR Utility                               |
        #   |                    Games Made with Ruby                           |
        #   +===================================================================+
        # BANNER
      end

      def header(title)
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
        puts pastel.green("\n> #{message}")
      end

      def info(message)
        puts pastel.blue("  i #{message}")
      end

      def success(message)
        puts pastel.green("  + #{message}")
      end

      def warn(message)
        puts pastel.yellow("  ! #{message}")
      end

      def error(message)
        puts pastel.red("  x #{message}")
      end

      def debug(message)
        puts pastel.dim("  # #{message}") if ENV["GMR_DEBUG"]
      end

      # === Formatted Output ===

      def list(items, indent: 4)
        items.each do |item|
          puts "#{' ' * indent}- #{item}"
        end
      end

      def table(rows, headers: nil)
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
        status = ok ? pastel.green(value) : pastel.red(value)
        puts "  #{label.ljust(20)} #{status}"
      end

      # === Interactive Elements ===

      def spinner(message, success_message: nil, error_message: nil)
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
        print pastel.blue("  ... #{message}")
        result = yield
        puts pastel.green(" done")
        result
      rescue StandardError => e
        puts pastel.red(" failed")
        raise
      end

      # === Confirmation ===

      def confirm?(message, default: true)
        default_hint = default ? "[Y/n]" : "[y/N]"
        print "#{message} #{default_hint} "

        response = $stdin.gets&.strip&.downcase
        return default if response.nil? || response.empty?

        %w[y yes].include?(response)
      end

      # === Spacing ===

      def blank
        puts ""
      end

      def divider
        puts pastel.dim("  " + "-" * 60)
      end

      # === Next Steps ===

      def next_steps(steps)
        puts ""
        puts pastel.bold("Next steps:")
        puts ""
        steps.each_with_index do |step_text, i|
          puts "  #{pastel.cyan("#{i + 1}.")} #{step_text}"
        end
        puts ""
      end

      def command_hint(command, description = nil)
        hint = "  #{pastel.cyan(command)}"
        hint += "  #{pastel.dim("# #{description}")}" if description
        puts hint
      end
    end
  end
end
