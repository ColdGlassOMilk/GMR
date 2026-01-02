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
