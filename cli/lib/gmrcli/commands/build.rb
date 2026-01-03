# frozen_string_literal: true

require "fileutils"
require "pathname"

module Gmrcli
  module Commands
    # Build command - builds GMR for various targets
    #
    # The build process uses two directories:
    # - Engine directory (GMR_ROOT): Contains CMakeLists.txt, deps, src - the GMR engine
    # - Project directory (current working dir): Where game scripts live, and where
    #   build output (build/, release/) is placed
    #
    # This allows multiple projects to share the same GMR engine installation.
    class Build
      TARGETS = %w[debug release web clean all].freeze

      attr_reader :options, :project_dir

      def initialize(options = {})
        @options = {
          verbose: false,
          rebuild: false
        }.merge(options)

        # Determine project directory - find project root from current dir, or fall back to engine root
        @project_dir = Platform.find_project_root(Dir.pwd) || Platform.gmr_root
      end

      # === Public Build Methods ===

      def debug
        JsonEmitter.build_start(:debug)
        UI.banner
        build_native("Debug")
        JsonEmitter.build_complete(output_path: File.join(release_dir, gmr_exe))
      end

      def release
        JsonEmitter.build_start(:release)
        UI.banner
        build_native("Release")
        JsonEmitter.build_complete(output_path: File.join(release_dir, gmr_exe))
      end

      def web
        JsonEmitter.build_start(:web)
        UI.banner
        build_web
        JsonEmitter.build_complete(output_path: File.join(web_release_dir, "gmr.html"))
      end

      def clean
        UI.step "Cleaning build artifacts..."

        cleaned = []

        # Clean build and release directories in project directory
        build_root = File.join(project_dir, "build")
        release_root = File.join(project_dir, "release")

        [build_root, release_root].each do |dir|
          if Dir.exist?(dir)
            FileUtils.rm_rf(dir)
            cleaned << File.basename(dir)
          end
        end

        if cleaned.empty?
          UI.info "Nothing to clean"
        else
          UI.success "Cleaned: #{cleaned.join(', ')}"
        end
      end

      def all
        UI.banner
        build_native("Debug")
        build_native("Release")
        build_web
      end

      private

      def verbose?
        options[:verbose]
      end

      # Run a stage with JSON event tracking
      def run_stage(stage_id, stage_name)
        JsonEmitter.stage_start(stage_id, stage_name)
        yield
        JsonEmitter.stage_complete(stage_id)
      rescue StandardError => e
        error_details = e.respond_to?(:details) ? e.details : nil
        error_suggestions = e.respond_to?(:suggestions) ? e.suggestions : []
        JsonEmitter.stage_error(stage_id, e.message, details: error_details, suggestions: error_suggestions)
        raise
      end

      # === Native Build ===

      def build_native(build_type)
        UI.step "Building native #{build_type.upcase}..."

        run_stage(:validate, "Validating Environment") do
          validate_native_environment!
        end

        # Clean if rebuild requested
        if options[:rebuild]
          UI.info "Cleaning previous build..."
          FileUtils.rm_rf(build_dir)
        end

        FileUtils.mkdir_p(build_dir)

        # Configure
        # Always use streaming mode to avoid hangs with captured output
        run_stage(:configure, "Configuring Build") do
          JsonEmitter.stage_progress(:configure, 10, "Running CMake", substage: "cmake")
          UI.info "Configuring..."
          cmake_args = build_cmake_args(build_type)

          begin
            Shell.cmake(cmake_args.join(" "), chdir: build_dir, verbose: true)
          rescue CommandError => e
            raise BuildError.new(
              "CMake configuration failed",
              details: e.output,
              suggestions: [
                "Check that all dependencies are installed",
                "Run 'gmrcli setup' if you haven't already",
                "Run with --verbose for more details"
              ]
            )
          end
        end

        # Build
        # Always use streaming mode to avoid hangs with captured output
        run_stage(:compile, "Compiling") do
          JsonEmitter.stage_progress(:compile, 10, "Starting compilation", substage: "ninja")
          UI.info "Building with #{Platform.nproc} threads..."
          begin
            Shell.ninja("", chdir: build_dir, verbose: true)
          rescue CommandError => e
            raise BuildError.new(
              "Compilation failed",
              details: e.output,
              suggestions: [
                "Check the error messages above",
                "Run with --verbose for full output"
              ]
            )
          end
        end

        run_stage(:completion, "Complete") do
          UI.success "#{build_type} build complete!"
          UI.info "Executable: #{File.join(release_dir, gmr_exe)}"
        end
      end

      def build_cmake_args(build_type)
        # Point to the engine's CMakeLists.txt (absolute path)
        # Normalize to forward slashes for CMake compatibility on Windows
        # Quote paths to handle spaces in directory names
        engine_path = engine_dir.gsub("\\", "/")
        project_path = project_dir.gsub("\\", "/")

        args = [
          "\"#{engine_path}\"",
          "-G Ninja",
          "-DCMAKE_BUILD_TYPE=#{build_type}",
          # Tell CMake where to find game scripts (quote for spaces in path)
          "-DGMR_PROJECT_DIR=\"#{project_path}\""
        ]

        # Specify ninja path explicitly to ensure CMake finds it
        # Use dynamic lookup instead of hardcoded MSYS2 path for IDE compatibility
        ninja_path = Platform.command_path("ninja")
        if ninja_path
          args << "-DCMAKE_MAKE_PROGRAM=#{ninja_path.gsub('\\', '/')}"
        end

        args
      end

      def validate_native_environment!
        errors = []

        unless Platform.command_exists?("cmake")
          errors << "cmake not found"
        end

        unless Platform.command_exists?("ninja")
          errors << "ninja not found"
        end

        raylib_lib = File.join(Platform.deps_dir, "raylib", "native", "lib", "libraylib.a")
        unless File.exist?(raylib_lib)
          errors << "raylib not built"
        end

        # Check for mruby in deps directory first (portable/IDE setup), then system paths
        mruby_lib = File.join(Platform.deps_dir, "mruby", "native", "lib", "libmruby.a")
        unless File.exist?(mruby_lib)
          # Fallback to system paths for MSYS2/system installs
          system_mruby = Platform.mingw64? ? "/mingw64/lib/libmruby.a" : "/usr/local/lib/libmruby.a"
          unless File.exist?(system_mruby)
            errors << "mruby not installed"
          end
        end

        unless errors.empty?
          raise MissingDependencyError.new(
            "Missing build dependencies: #{errors.join(', ')}",
            suggestions: ["Run 'gmrcli setup' to install dependencies"]
          )
        end
      end

      # === Web Build ===

      def build_web
        UI.step "Building for WEB..."

        run_stage(:validate, "Validating Environment") do
          validate_web_environment!
        end

        # Clean if rebuild requested
        if options[:rebuild]
          UI.info "Cleaning previous build..."
          FileUtils.rm_rf(web_build_dir)
        end

        FileUtils.mkdir_p(web_build_dir)

        env = emscripten_env

        # Configure with Ninja generator
        # Use quoted absolute paths - relative paths break with spaces in path names
        run_stage(:configure, "Configuring Build") do
          JsonEmitter.stage_progress(:configure, 10, "Running emcmake", substage: "cmake")
          UI.info "Configuring..."

          # Use absolute paths with forward slashes, properly quoted for spaces
          # Normalize all paths to forward slashes for CMake compatibility
          engine_path = engine_dir.gsub("\\", "/")
          project_path = project_dir.gsub("\\", "/")

          UI.info "Build dir: #{web_build_dir}"
          UI.info "Engine path: #{engine_path}"
          UI.info "Project path: #{project_path}"

          cmake_args = [
            "\"#{engine_path}\"",
            "-G Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DPLATFORM=Web",
            "-DGMR_PROJECT_DIR=\"#{project_path}\""
          ]

          # Specify ninja path explicitly to ensure CMake finds it
          ninja_path = Platform.command_path("ninja")
          if ninja_path
            cmake_args << "-DCMAKE_MAKE_PROGRAM=#{ninja_path.gsub('\\', '/')}"
          end

          # Always use verbose (streaming) mode - capture mode can hang on emscripten
          cmake_cmd = "emcmake cmake #{cmake_args.join(' ')}"
          UI.info "Command: #{cmake_cmd}"

          begin
            Shell.run!(
              cmake_cmd,
              chdir: web_build_dir,
              env: env,
              verbose: true
            )
          rescue CommandError => e
            raise BuildError.new(
              "CMake configuration failed for web build",
              details: e.output,
              suggestions: [
                "Make sure Emscripten is properly set up",
                "Run 'gmrutil setup' if you haven't already"
              ]
            )
          end
        end

        # Build with emmake ninja -j1 (single-threaded to avoid ASYNCIFY hangs on Windows)
        # Must use emmake wrapper to ensure Emscripten environment is properly set
        # Always use verbose (streaming) mode - capture mode can hang on emscripten
        run_stage(:compile, "Compiling (WASM)") do
          JsonEmitter.stage_progress(:compile, 10, "Starting WASM compilation", substage: "emcc")
          UI.info "Building (single-threaded for ASYNCIFY compatibility)..."
          begin
            Shell.run!(
              "emmake ninja -j1",
              chdir: web_build_dir,
              env: env,
              verbose: true
            )
          rescue CommandError => e
            raise BuildError.new(
              "Web compilation failed",
              details: e.output,
              suggestions: ["Check the error messages above"]
            )
          end
        end

        run_stage(:completion, "Complete") do
          UI.success "Web build complete!"
          UI.info "Output: #{File.join(web_release_dir, 'gmr.html')}"
          UI.blank
          UI.info "To test locally:"
          UI.command_hint "gmrcli run web", "Start local server"
        end
      end

      def validate_web_environment!
        errors = []

        # Check for emcc (either in PATH or via wrappers)
        emcc_wrapper = File.join(Platform.bin_dir, "emcc")
        unless Platform.command_exists?("emcc") || File.exist?(emcc_wrapper)
          errors << "emcc not found"
        end

        raylib_lib = File.join(Platform.deps_dir, "raylib", "web", "lib", "libraylib.a")
        unless File.exist?(raylib_lib)
          errors << "raylib-web not built"
        end

        mruby_lib = File.join(Platform.deps_dir, "mruby", "web", "lib", "libmruby.a")
        unless File.exist?(mruby_lib)
          errors << "mruby-web not built"
        end

        unless errors.empty?
          raise MissingDependencyError.new(
            "Missing web build dependencies: #{errors.join(', ')}",
            suggestions: [
              "Run 'gmrutil setup' to install dependencies",
              "For web builds, don't use --native-only"
            ]
          )
        end
      end

      def emscripten_env
        # Helper to normalize paths to forward slashes (CMake/Emscripten prefer this)
        normalize = ->(path) { path&.gsub("\\", "/") }

        emsdk_dir = normalize.call(File.join(Platform.deps_dir, "emsdk"))
        emscripten_path = normalize.call(File.join(emsdk_dir, "upstream", "emscripten"))

        # Find node and python directories (versioned subdirectories)
        node_version_dir = Dir.glob(File.join(emsdk_dir, "node", "*")).first
        python_version_dir = Dir.glob(File.join(emsdk_dir, "python", "*")).first

        # Node bin directory and executable
        node_bin_dir = node_version_dir ? normalize.call(File.join(node_version_dir, "bin")) : nil
        node_exe = node_bin_dir ? normalize.call(File.join(node_bin_dir, "node.exe")) : nil

        # Python executable
        python_exe = python_version_dir ? normalize.call(File.join(python_version_dir, "python.exe")) : nil

        # Emscripten config file
        em_config = normalize.call(File.join(emsdk_dir, ".emscripten"))

        path_additions = [
          normalize.call(Platform.bin_dir),
          emscripten_path,
          node_bin_dir,
          python_version_dir ? normalize.call(python_version_dir) : nil,
          normalize.call(File.join(emsdk_dir, "upstream", "bin")),
          normalize.call(File.join(Platform.mingw_root, "bin")),
        ].compact.join(File::PATH_SEPARATOR)

        # Use user's home directory for cache (no spaces in path)
        # USERPROFILE is Windows-style (C:\Users\...), HOME may be Unix-style (/c/Users/...)
        # Convert HOME to Windows-style if needed
        home = ENV["USERPROFILE"] || Platform.to_windows_path(ENV["HOME"]) || "C:/tmp"
        cache_dir = normalize.call(File.join(home, ".emcache"))

        env = {
          "PATH" => "#{path_additions}#{File::PATH_SEPARATOR}#{ENV['PATH']}",
          "EMSDK" => emsdk_dir,
          "EM_CONFIG" => em_config,
          "EM_CACHE" => cache_dir,
          "RAYLIB_WEB_PATH" => normalize.call(File.join(Platform.deps_dir, "raylib", "web")),
          "MRUBY_WEB_PATH" => normalize.call(File.join(Platform.deps_dir, "mruby", "web"))
        }

        # Add python and node paths if found
        env["EMSDK_PYTHON"] = python_exe if python_exe && File.exist?(python_exe)
        env["EMSDK_NODE"] = node_exe if node_exe && File.exist?(node_exe)

        env
      end

      # === Helpers ===

      # Build output goes to project directory
      def build_dir
        File.join(project_dir, "build", "native")
      end

      def web_build_dir
        File.join(project_dir, "build", "web")
      end

      def release_dir
        File.join(project_dir, "release")
      end

      def web_release_dir
        File.join(project_dir, "release", "web")
      end

      # Engine directory (for CMakeLists.txt reference)
      def engine_dir
        Platform.gmr_root
      end

      def gmr_exe
        Platform.gmr_executable
      end
    end
  end
end
