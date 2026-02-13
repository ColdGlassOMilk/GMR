# frozen_string_literal: true

require "fileutils"

module Gmrcli
  module Commands
    # Test command - builds and runs GMR unit tests
    #
    # The test command handles:
    # - Building the test executable with CMake + Ninja
    # - Running tests via CTest
    # - Parsing and reporting test results
    #
    class Test
      include Stageable

      attr_reader :options, :project_dir

      def initialize(options = {})
        @options = {
          verbose: false,
          rebuild: false,
          filter: nil
        }.merge(options)

        @project_dir = Platform.gmr_root
      end

      def run(target = "all")
        @start_time = Time.now
        @stages_completed = []

        JsonEmitter.command_start("test", target: target)

        UI.banner
        UI.step "Running GMR Unit Tests"

        run_stage(:validate, "Validating Environment") do
          validate_environment!
        end

        # Clean if rebuild requested
        if options[:rebuild]
          UI.info "Cleaning previous test build..."
          FileUtils.rm_rf(build_dir)
        end

        run_stage(:configure, "Configuring Test Build") do
          configure_build
        end

        run_stage(:compile, "Compiling Tests") do
          compile_tests
        end

        run_stage(:test, "Running Tests") do
          run_tests(target)
        end

        # Emit success result
        JsonEmitter.emit_success_envelope(
          command: "test",
          result: {
            target: target,
            stages_completed: @stages_completed,
            elapsed_seconds: (Time.now - @start_time).round(2)
          }
        )
      end

      private

      def verbose?
        options[:verbose]
      end

      def build_dir
        File.join(project_dir, "build", "test")
      end

      def validate_environment!
        errors = []

        unless Platform.command_exists?("cmake")
          errors << "cmake not found"
        end

        unless Platform.command_exists?("ninja")
          errors << "ninja not found"
        end

        # Check for raylib (tests link against gmr_core which needs raylib)
        raylib_root = File.join(Platform.deps_dir, "raylib", "native")
        raylib_lib = File.join(Platform.lib_path(raylib_root), "libraylib.a")
        unless File.exist?(raylib_lib)
          errors << "raylib not built"
        end

        # Check for mruby
        mruby_root = File.join(Platform.deps_dir, "mruby", "native")
        mruby_lib = File.join(Platform.lib_path(mruby_root), "libmruby.a")
        unless File.exist?(mruby_lib)
          # Check system paths with lib64 support
          system_paths = if Platform.mingw64?
            ["/mingw64/lib/libmruby.a"]
          else
            ["/usr/local/lib/libmruby.a", "/usr/local/lib64/libmruby.a"]
          end

          unless system_paths.any? { |p| File.exist?(p) }
            errors << "mruby not installed"
          end
        end

        unless errors.empty?
          raise MissingDependencyError.new(
            "Missing test dependencies: #{errors.join(', ')}",
            suggestions: ["Run 'gmrcli setup' to install dependencies"]
          )
        end
      end

      def configure_build
        FileUtils.mkdir_p(build_dir)

        UI.info "Configuring with tests enabled..."

        engine_path = project_dir.gsub("\\", "/")

        cmake_args = [
          "\"#{engine_path}\"",
          "-G Ninja",
          "-DCMAKE_BUILD_TYPE=Debug",
          "-DGMR_BUILD_TESTS=ON",
          "-DGMR_PROJECT_DIR=\"#{engine_path}\""
        ]

        # Specify ninja path explicitly
        ninja_path = Platform.command_path("ninja")
        if ninja_path
          cmake_args << "-DCMAKE_MAKE_PROGRAM=#{ninja_path.gsub('\\', '/')}"
        end

        begin
          Shell.cmake(cmake_args.join(" "), chdir: build_dir, verbose: verbose?)
        rescue CommandError => e
          raise BuildError.new(
            "CMake configuration failed for tests",
            stage: :cmake,
            details: e.output,
            suggestions: [
              "Check that all dependencies are installed",
              "Run 'gmrcli setup' if you haven't already"
            ]
          )
        end
      end

      def compile_tests
        UI.info "Building test executable..."

        begin
          # Build just the test target, not the main executable
          Shell.ninja("gmr_tests", chdir: build_dir, verbose: verbose?)
        rescue CommandError => e
          raise BuildError.new(
            "Test compilation failed",
            stage: :compile,
            details: e.output,
            suggestions: ["Check the error messages above"]
          )
        end

        UI.success "Test executable built successfully"
      end

      def run_tests(target)
        UI.info "Running tests..."
        UI.blank

        # Build the test command - try multiple possible locations
        # CMake outputs to tests/bin/ per our CMakeLists.txt configuration
        possible_paths = [
          File.join(build_dir, "tests", "bin", "gmr_tests.exe"),
          File.join(build_dir, "tests", "bin", "gmr_tests"),
          File.join(build_dir, "tests", "gmr_tests.exe"),
          File.join(build_dir, "tests", "gmr_tests"),
          File.join(build_dir, "gmr_tests.exe"),
          File.join(build_dir, "gmr_tests")
        ]

        test_exe = possible_paths.find { |p| File.exist?(p) }

        unless test_exe
          raise Error.new(
            "Test executable not found. Searched:\n  #{possible_paths.join("\n  ")}",
            code: "TEST.EXE_NOT_FOUND",
            suggestions: ["Try running with --rebuild to clean and rebuild"]
          )
        end

        UI.info "Found test executable: #{test_exe}"

        # Build command with optional filter
        cmd = "\"#{test_exe}\""

        # Add filter if specified
        if options[:filter]
          # Catch2 uses [tag] syntax for filtering
          cmd += " \"[#{options[:filter]}]\""
        elsif target != "all"
          # If target is specified (not "all"), use it as a tag filter
          cmd += " \"[#{target}]\""
        end

        # Add verbose flag if requested
        cmd += " --success" if verbose?

        begin
          # Run tests and capture output
          Shell.run!(cmd, chdir: build_dir, verbose: true)
          UI.blank
          UI.success "All tests passed!"
        rescue CommandError => e
          UI.blank
          # Tests failed - still report the failure
          raise Error.new(
            "Some tests failed",
            code: "TEST.TESTS_FAILED",
            details: e.output,
            suggestions: [
              "Review the test output above",
              "Run with --verbose for more details",
              "Run with --filter=TAG to run specific test groups"
            ]
          )
        end
      end
    end
  end
end
