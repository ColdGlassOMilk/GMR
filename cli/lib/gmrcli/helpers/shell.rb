# frozen_string_literal: true

require "open3"
require "fileutils"

module Gmrcli
  # Shell command execution with proper error handling
  module Shell
    class << self
      # Run a command and return success/failure
      # @param command [String] The command to run
      # @param verbose [Boolean] Show command output
      # @param chdir [String] Directory to run in
      # @param env [Hash] Environment variables to set
      # @return [Boolean] true if command succeeded
      def run(command, verbose: false, chdir: nil, env: {})
        result = execute(command, verbose: verbose, chdir: chdir, env: env)
        result[:success]
      end

      # Run a command and raise on failure
      # @param command [String] The command to run
      # @param verbose [Boolean] Show command output
      # @param chdir [String] Directory to run in
      # @param env [Hash] Environment variables to set
      # @param error_message [String] Custom error message
      # @return [String] Command output
      def run!(command, verbose: false, chdir: nil, env: {}, error_message: nil)
        result = execute(command, verbose: verbose, chdir: chdir, env: env)

        unless result[:success]
          msg = error_message || "Command failed: #{command}"
          raise CommandError.new(
            msg,
            command: command,
            exit_code: result[:exit_code],
            output: result[:output],
            suggestions: ["Run with --verbose for more details"]
          )
        end

        result[:output]
      end

      # Run a command silently, return output
      # @param command [String] The command to run
      # @return [String, nil] Output if successful, nil if failed
      def capture(command, chdir: nil, env: {})
        result = execute(command, verbose: false, chdir: chdir, env: env)
        result[:success] ? result[:output] : nil
      end

      # Run a command silently, return output or raise
      def capture!(command, chdir: nil, env: {}, error_message: nil)
        run!(command, verbose: false, chdir: chdir, env: env, error_message: error_message)
      end

      # Run pacman command (Windows/MSYS2 specific)
      def pacman(args, verbose: false)
        run!("pacman #{args}", verbose: verbose, error_message: "Pacman command failed")
      end

      # Run cmake
      def cmake(args, chdir: nil, verbose: false)
        run!("cmake #{args}", chdir: chdir, verbose: verbose, error_message: "CMake configuration failed")
      end

      # Run ninja
      def ninja(args = "", chdir: nil, env: {}, verbose: false)
        jobs = "-j#{Platform.nproc}"
        run!("ninja #{jobs} #{args}".strip, chdir: chdir, env: env, verbose: verbose, error_message: "Build failed")
      end

      # Run make
      def make(args = "", chdir: nil, verbose: false)
        jobs = "-j#{Platform.nproc}"
        run!("make #{jobs} #{args}".strip, chdir: chdir, verbose: verbose, error_message: "Build failed")
      end

      # Run git command
      def git(args, chdir: nil, verbose: false)
        run!("git #{args}", chdir: chdir, verbose: verbose, error_message: "Git command failed")
      end

      # Clone a git repository
      def git_clone(url, destination, depth: 1, verbose: false)
        return if Dir.exist?(destination)

        FileUtils.mkdir_p(File.dirname(destination))
        depth_arg = depth ? "--depth #{depth}" : ""
        git("clone #{depth_arg} #{url} \"#{destination}\"", verbose: verbose)
      end

      # Check if a command exists
      def command_exists?(cmd)
        Platform.command_exists?(cmd)
      end

      # Source a shell script and run a command with that environment
      def with_env_script(script_path, command, verbose: false, chdir: nil)
        unless File.exist?(script_path)
          raise MissingFileError.new(
            "Environment script not found: #{script_path}",
            suggestions: ["Run 'gmrcli setup' first"]
          )
        end

        full_command = "source \"#{script_path}\" && #{command}"
        run!("bash -c '#{full_command}'", verbose: verbose, chdir: chdir)
      end

      private

      def execute(command, verbose:, chdir:, env:)
        UI.debug("Running: #{command}") if ENV["GMR_DEBUG"]
        UI.debug("  in: #{chdir}") if ENV["GMR_DEBUG"] && chdir

        options = {}
        options[:chdir] = chdir if chdir

        full_env = ENV.to_h.merge(env.transform_keys(&:to_s))

        stdout, stderr, status = if verbose
                                   # Stream output in real-time
                                   run_streaming(command, full_env, options)
                                 else
                                   # Capture output silently
                                   Open3.capture3(full_env, command, **options)
                                 end

        {
          success: status.success?,
          exit_code: status.exitstatus,
          output: "#{stdout}\n#{stderr}".strip,
          stdout: stdout,
          stderr: stderr
        }
      rescue Interrupt
        # User pressed Ctrl+C
        raise
      rescue Errno::ENOENT => e
        # Command not found
        {
          success: false,
          exit_code: 127,
          output: e.message,
          stdout: "",
          stderr: e.message
        }
      rescue IOError
        # Stream closed (usually from interrupt)
        raise Interrupt
      rescue StandardError => e
        {
          success: false,
          exit_code: 1,
          output: e.message,
          stdout: "",
          stderr: e.message
        }
      end

      def run_streaming(command, env, options)
        # Use system() for streaming - simpler and more reliable on Windows
        original_dir = Dir.pwd
        Dir.chdir(options[:chdir]) if options[:chdir]

        # Merge environment
        old_env = {}
        env.each do |k, v|
          old_env[k] = ENV[k]
          ENV[k] = v
        end

        begin
          success = system(command)
          status = $?
          [success ? "" : "Command failed", "", status]
        ensure
          # Restore environment
          old_env.each { |k, v| v.nil? ? ENV.delete(k) : ENV[k] = v }
          Dir.chdir(original_dir) if options[:chdir]
        end
      end
    end
  end
end
