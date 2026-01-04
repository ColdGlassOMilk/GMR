# frozen_string_literal: true

module Gmrcli
  # Base error class for all GMR errors
  # All errors have a hierarchical error code for machine identification
  class Error < StandardError
    attr_reader :code, :details, :suggestions, :error_metadata

    def initialize(message, code: "INTERNAL.UNEXPECTED", details: nil, suggestions: [], metadata: {})
      super(message)
      @code = code
      @details = details
      @suggestions = Array(suggestions)
      @error_metadata = metadata
    end

    # Get exit code from ErrorCodes registry
    def exit_code
      ErrorCodes.exit_code_for(code)
    end

    # Convert to hash for JSON serialization
    def to_h
      {
        code: code,
        message: message,
        details: details,
        suggestions: suggestions,
        metadata: error_metadata
      }.compact
    end
  end

  # Environment/platform errors
  class PlatformError < Error
    def initialize(message, code: "PLATFORM.UNSUPPORTED", **kwargs)
      super(message, code: code, **kwargs)
    end
  end

  class MissingDependencyError < Error
    def initialize(message, dependency: nil, **kwargs)
      code = dependency ? "SETUP.MISSING_DEP.#{dependency.to_s.upcase}" : "SETUP.MISSING_DEP"
      super(message, code: code, **kwargs)
    end
  end

  class EnvironmentError < Error
    def initialize(message, code: "PLATFORM.WRONG_ENV", **kwargs)
      super(message, code: code, **kwargs)
    end
  end

  # Build errors
  class BuildError < Error
    def initialize(message, stage: nil, **kwargs)
      code = case stage&.to_s&.downcase
             when "cmake", "configure" then "BUILD.CMAKE_FAILED"
             when "compile", "compilation" then "BUILD.COMPILE_FAILED"
             when "link", "linking" then "BUILD.LINK_FAILED"
             else "BUILD.COMPILE_FAILED"
             end
      super(message, code: code, **kwargs)
    end
  end

  class ConfigurationError < Error
    def initialize(message, code: "BUILD.CMAKE_FAILED", **kwargs)
      super(message, code: code, **kwargs)
    end
  end

  class CompilationError < Error
    def initialize(message, code: "BUILD.COMPILE_FAILED", **kwargs)
      super(message, code: code, **kwargs)
    end
  end

  # Project errors
  class ProjectError < Error
    def initialize(message, code: "PROJECT.INVALID", **kwargs)
      super(message, code: code, **kwargs)
    end
  end

  class NotAProjectError < ProjectError
    def initialize(message, **kwargs)
      super(message, code: "PROJECT.NOT_FOUND", **kwargs)
    end
  end

  class MissingFileError < ProjectError
    def initialize(message, **kwargs)
      super(message, code: "PROJECT.MISSING_FILE", **kwargs)
    end
  end

  # Command execution errors
  class CommandError < Error
    attr_reader :command, :command_exit_code, :output

    def initialize(message, command: nil, exit_code: nil, output: nil, code: "INTERNAL.COMMAND_FAILED", **kwargs)
      super(message, code: code, **kwargs)
      @command = command
      @command_exit_code = exit_code
      @output = output
    end

    def to_h
      super.merge(
        command: command,
        command_exit_code: command_exit_code
      ).compact
    end
  end

  # Error handler module for consistent error display
  # Handles both machine (JSON) and human output modes
  module ErrorHandler
    class << self
      attr_accessor :current_command, :command_start_time
    end

    def self.handle(error, ui: UI, command: nil)
      cmd = command || current_command
      # In JSON mode (default), emit structured error
      if defined?(JsonEmitter) && JsonEmitter.enabled?
        emit_json_error(error, command: cmd)
      else
        emit_human_error(error, ui: ui)
      end
    end

    def self.emit_human_error(error, ui: UI)
      case error
      when Gmrcli::Error
        ui.error("[#{error.code}] #{error.message}")
        ui.info("Details: #{error.details}") if error.details
        error.suggestions.each { |s| ui.info("  -> #{s}") }
      when Interrupt
        ui.warn("\nInterrupted by user")
      else
        ui.error("[INTERNAL.UNEXPECTED] #{error.message}")
        ui.info(error.backtrace.first(5).join("\n")) if ENV["GMR_DEBUG"]
      end
    end

    def self.emit_json_error(error, command: nil)
      error_data = if error.respond_to?(:to_h)
                     error.to_h
                   else
                     {
                       code: "INTERNAL.UNEXPECTED",
                       message: error.message
                     }
                   end

      # Add backtrace in debug mode
      if ENV["GMR_DEBUG"] && error.backtrace
        error_data[:backtrace] = error.backtrace.first(5)
      end

      # Emit the final error envelope
      JsonEmitter.emit_error_envelope(
        command: command,
        error: error_data,
        start_time: command_start_time
      )
    end

    # Wrap command execution with error handling
    # Uses error codes to determine exit codes
    def self.wrap(ui: UI, command: nil, &block)
      self.current_command = command
      self.command_start_time = Time.now
      yield
    rescue Gmrcli::Error => e
      handle(e, ui: ui, command: command)
      exit e.exit_code
    rescue Interrupt
      if defined?(JsonEmitter) && JsonEmitter.enabled?
        JsonEmitter.emit(:interrupted, { signal: "SIGINT" })
      else
        ui.warn("\nInterrupted by user")
      end
      exit 130
    rescue StandardError => e
      wrapped = Error.new(e.message, code: "INTERNAL.UNEXPECTED")
      handle(wrapped, ui: ui, command: command)
      exit 1
    ensure
      self.current_command = nil
      self.command_start_time = nil
    end
  end
end
