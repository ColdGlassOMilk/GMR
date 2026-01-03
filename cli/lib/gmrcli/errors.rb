# frozen_string_literal: true

module Gmrcli
  # Base error class for all GMR errors
  class Error < StandardError
    attr_reader :details, :suggestions

    def initialize(message, details: nil, suggestions: [])
      super(message)
      @details = details
      @suggestions = Array(suggestions)
    end
  end

  # Environment/platform errors
  class PlatformError < Error; end
  class MissingDependencyError < Error; end
  class EnvironmentError < Error; end

  # Build errors
  class BuildError < Error; end
  class ConfigurationError < Error; end
  class CompilationError < Error; end

  # Project errors
  class ProjectError < Error; end
  class NotAProjectError < ProjectError; end
  class MissingFileError < ProjectError; end

  # Command execution errors
  class CommandError < Error
    attr_reader :command, :exit_code, :output

    def initialize(message, command: nil, exit_code: nil, output: nil, **kwargs)
      super(message, **kwargs)
      @command = command
      @exit_code = exit_code
      @output = output
    end
  end

  # Error handler module for consistent error display
  module ErrorHandler
    def self.handle(error, ui: UI)
      # In JSON mode, emit error as JSON event
      if defined?(JsonEmitter) && JsonEmitter.enabled?
        emit_json_error(error)
      else
        emit_human_error(error, ui: ui)
      end
    end

    def self.emit_human_error(error, ui: UI)
      case error
      when Gmrcli::Error
        ui.error(error.message)
        ui.info("Details: #{error.details}") if error.details
        error.suggestions.each { |s| ui.info("  -> #{s}") }
      when Interrupt
        ui.warn("\nInterrupted by user")
      else
        ui.error("Unexpected error: #{error.message}")
        ui.info(error.backtrace.first(5).join("\n")) if ENV["GMR_DEBUG"]
      end
    end

    def self.emit_json_error(error)
      error_data = {
        message: error.message,
        type: error.class.name
      }

      if error.respond_to?(:details) && error.details
        error_data[:details] = error.details
      end

      if error.respond_to?(:suggestions) && error.suggestions&.any?
        error_data[:suggestions] = error.suggestions
      end

      if ENV["GMR_DEBUG"] && error.backtrace
        error_data[:backtrace] = error.backtrace.first(5)
      end

      JsonEmitter.emit(:error, { error: error_data })
    end

    def self.wrap(ui: UI)
      yield
    rescue Gmrcli::Error => e
      handle(e, ui: ui)
      exit 1
    rescue Interrupt
      handle(Interrupt.new, ui: ui)
      exit 130
    rescue StandardError => e
      handle(e, ui: ui)
      exit 1
    end
  end
end
