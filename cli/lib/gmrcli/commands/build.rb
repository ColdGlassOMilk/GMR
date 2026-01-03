# frozen_string_literal: true

require "fileutils"

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

        # Determine project directory - use current directory or find project root
        @project_dir = Platform.find_project_root(Dir.pwd) || Dir.pwd
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
        run_stage(:configure, "Configuring Build") do
          JsonEmitter.stage_progress(:configure, 10, "Running CMake", substage: "cmake")
          UI.info "Configuring..."
          cmake_args = build_cmake_args(build_type)

          begin
            Shell.cmake(cmake_args.join(" "), chdir: build_dir, verbose: verbose?)
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
        run_stage(:compile, "Compiling") do
          JsonEmitter.stage_progress(:compile, 10, "Starting compilation", substage: "ninja")
          UI.info "Building with #{Platform.nproc} threads..."
          begin
            Shell.ninja("", chdir: build_dir, verbose: verbose?)
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

        # Configure with Ninja generator (same as native builds)
        run_stage(:configure, "Configuring Build") do
          JsonEmitter.stage_progress(:configure, 10, "Running emcmake", substage: "cmake")
          UI.info "Configuring..."
          # Point to the engine's CMakeLists.txt (absolute path)
          # Quote paths to handle spaces in directory names
          engine_path = engine_dir.gsub("\\", "/")
          project_path = project_dir.gsub("\\", "/")
          cmake_args = [
            "\"#{engine_path}\"",
            "-G Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DPLATFORM=Web",
            "-DGMR_PROJECT_DIR=\"#{project_path}\""
          ]
          # Specify ninja path explicitly to ensure CMake finds it
          # Use dynamic lookup instead of hardcoded MSYS2 path for IDE compatibility
          ninja_path = Platform.command_path("ninja")
          if ninja_path
            cmake_args << "-DCMAKE_MAKE_PROGRAM=#{ninja_path.gsub('\\', '/')}"
          end

          begin
            Shell.run!(
              "emcmake cmake #{cmake_args.join(' ')}",
              chdir: web_build_dir,
              env: env,
              verbose: verbose?
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
        run_stage(:compile, "Compiling (WASM)") do
          JsonEmitter.stage_progress(:compile, 10, "Starting WASM compilation", substage: "emcc")
          UI.info "Building (single-threaded for ASYNCIFY compatibility)..."
          begin
            Shell.run!(
              "emmake ninja -j1",
              chdir: web_build_dir,
              env: env,
              verbose: verbose?
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
        emsdk_dir = File.join(Platform.deps_dir, "emsdk")
        emscripten_path = File.join(emsdk_dir, "upstream", "emscripten")

        # Find node
        node_dir = Dir.glob(File.join(emsdk_dir, "node", "*", "bin")).first

        path_additions = [
          Platform.bin_dir,
          emscripten_path,
          node_dir,
          "/usr/bin" # MSYS2 make is here
        ].compact.join(File::PATH_SEPARATOR)

        # Use a cache path with NO SPACES to avoid Windows short path issues
        # The project path has spaces (OneDrive), so we must use an external location
        cache_dir = if Platform.windows?
                      "C:/tmp/emcache"
                    else
                      File.join(ENV['HOME'], ".emcache")
                    end

        {
          "PATH" => "#{path_additions}#{File::PATH_SEPARATOR}#{ENV['PATH']}",
          "EMSDK" => emsdk_dir,
          "EM_CACHE" => cache_dir,
          "RAYLIB_WEB_PATH" => File.join(Platform.deps_dir, "raylib", "web"),
          "MRUBY_WEB_PATH" => File.join(Platform.deps_dir, "mruby", "web")
        }
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
