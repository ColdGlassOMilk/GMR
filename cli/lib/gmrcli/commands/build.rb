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
    #
    # ## Build Size Reporting
    #
    # After a successful build, the command reports the size of all build artifacts:
    #
    # For native builds:
    # - Executable size
    # - Assets folder size (if present)
    # - Total size
    #
    # For web builds:
    # - HTML file size
    # - JavaScript file size
    # - WebAssembly (.wasm) file size
    # - Data file size
    # - Total size
    #
    # Size information is included in both terminal output and JSON result.
    #
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
        @start_time = Time.now
        @stages_completed = []
        JsonEmitter.build_start(:debug)
        UI.banner
        build_native("Debug")
        output_path = File.join(release_dir, gmr_exe)
        JsonEmitter.build_complete(
          output_path: output_path,
          result: build_result("debug", output_path)
        )
      end

      def release
        @start_time = Time.now
        @stages_completed = []
        JsonEmitter.build_start(:release)
        UI.banner
        build_native("Release")
        output_path = File.join(release_dir, gmr_exe)
        JsonEmitter.build_complete(
          output_path: output_path,
          result: build_result("release", output_path)
        )
      end

      def web
        @start_time = Time.now
        @stages_completed = []
        JsonEmitter.build_start(:web)
        UI.banner
        build_web
        output_path = File.join(web_release_dir, "gmr.html")
        JsonEmitter.build_complete(
          output_path: output_path,
          result: build_result("web", output_path)
        )
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

        # Emit JSON result for clean command
        JsonEmitter.emit_success_envelope(
          command: "build",
          result: {
            target: "clean",
            directories_cleaned: cleaned
          }
        )
      end

      def all
        @start_time = Time.now
        @stages_completed = []
        UI.banner
        build_native("Debug")
        build_native("Release")
        build_web

        # Emit combined result
        JsonEmitter.emit_success_envelope(
          command: "build",
          result: {
            target: "all",
            stages_completed: @stages_completed,
            artifacts: [
              { type: "executable", target: "debug", path: File.join(release_dir, gmr_exe) },
              { type: "executable", target: "release", path: File.join(release_dir, gmr_exe) },
              { type: "wasm", target: "web", path: File.join(web_release_dir, "gmr.html") }
            ]
          }
        )
      end

      # Build the final result object for JSON output.
      #
      # Returns a hash containing:
      # - target: The build target (debug, release, web)
      # - output_path: Path to the main output file
      # - stages_completed: Array of completed build stage IDs
      # - artifacts: Array of build artifacts with type, path, and size
      # - total_size: Total size in bytes of all artifacts
      # - total_size_human: Human-readable total size (e.g., "2.45 MB")
      #
      # @param target [String] Build target (debug, release, web)
      # @param output_path [String] Path to the main output file
      # @return [Hash] Build result data
      def build_result(target, output_path)
        artifacts = case target
                    when "web"
                      [
                        { type: "html", path: output_path },
                        { type: "js", path: output_path.sub(".html", ".js") },
                        { type: "wasm", path: output_path.sub(".html", ".wasm") },
                        { type: "data", path: output_path.sub(".html", ".data") }
                      ]
                    else
                      [{ type: "executable", path: output_path }]
                    end

        # Add file sizes to artifacts
        artifacts.each do |artifact|
          artifact[:size] = File.size(artifact[:path]) if File.exist?(artifact[:path])
        end

        # Calculate total size and add assets for native builds
        total_size = artifacts.sum { |a| a[:size] || 0 }

        if target != "web"
          # For native builds, include assets folder size
          assets_dir = File.join(release_dir, "assets")
          if Dir.exist?(assets_dir)
            assets_size = calculate_directory_size(assets_dir)
            artifacts << { type: "assets", path: assets_dir, size: assets_size }
            total_size += assets_size
          end
        end

        {
          target: target,
          output_path: output_path,
          stages_completed: @stages_completed || [],
          artifacts: artifacts,
          total_size: total_size,
          total_size_human: format_size(total_size)
        }
      end

      # Calculate total size of a directory recursively.
      #
      # Traverses all subdirectories and sums the size of all files.
      #
      # @param dir [String] Directory path to calculate size for
      # @return [Integer] Total size in bytes
      def calculate_directory_size(dir)
        return 0 unless Dir.exist?(dir)

        Dir.glob(File.join(dir, "**", "*"))
           .select { |f| File.file?(f) }
           .sum { |f| File.size(f) }
      end

      # Format bytes into human-readable size.
      #
      # Converts byte count to appropriate unit (B, KB, MB, GB).
      #
      # @example
      #   format_size(1024)      # => "1.00 KB"
      #   format_size(1536000)   # => "1.46 MB"
      #
      # @param bytes [Integer, nil] Size in bytes
      # @return [String] Human-readable size string
      def format_size(bytes)
        return "0 B" if bytes.nil? || bytes == 0

        units = ["B", "KB", "MB", "GB"]
        unit_index = 0
        size = bytes.to_f

        while size >= 1024 && unit_index < units.length - 1
          size /= 1024
          unit_index += 1
        end

        if unit_index == 0
          "#{size.to_i} #{units[unit_index]}"
        else
          "%.2f #{units[unit_index]}" % size
        end
      end

      # Display build output sizes in terminal.
      #
      # Prints a formatted table showing the size of each build artifact
      # and the total size. Output format differs for native vs web builds.
      #
      # Native build output:
      #   Build Size:
      #     Exe      2.45 MB
      #     Assets   156.32 KB
      #     ──────────────────────
      #     Total    2.60 MB
      #
      # Web build output:
      #   Build Size:
      #     HTML   4.21 KB
      #     JS     52.18 KB
      #     WASM   1.82 MB
      #     Data   312.45 KB
      #     ────────────────────
      #     Total  2.18 MB
      #
      # @param target [String] Build target ("native" or "web")
      # @param output_path [String] Path to the main output file
      def display_build_sizes(target, output_path)
        UI.blank
        UI.info "Build Size:"

        if target == "web"
          # Web build: HTML + JS + WASM + data
          files = [
            ["HTML", output_path],
            ["JS", output_path.sub(".html", ".js")],
            ["WASM", output_path.sub(".html", ".wasm")],
            ["Data", output_path.sub(".html", ".data")]
          ]

          total = 0
          files.each do |label, path|
            if File.exist?(path)
              size = File.size(path)
              total += size
              UI.info "  #{label.ljust(6)} #{format_size(size)}"
            end
          end
          UI.info "  #{'─' * 20}"
          UI.info "  #{'Total'.ljust(6)} #{format_size(total)}"
        else
          # Native build: executable + assets
          total = 0

          if File.exist?(output_path)
            exe_size = File.size(output_path)
            total += exe_size
            UI.info "  #{'Exe'.ljust(8)} #{format_size(exe_size)}"
          end

          assets_dir = File.join(release_dir, "assets")
          if Dir.exist?(assets_dir)
            assets_size = calculate_directory_size(assets_dir)
            total += assets_size
            UI.info "  #{'Assets'.ljust(8)} #{format_size(assets_size)}"
          end

          UI.info "  #{'─' * 22}"
          UI.info "  #{'Total'.ljust(8)} #{format_size(total)}"
        end
      end

      private

      def verbose?
        options[:verbose]
      end

      # Run a stage with JSON event tracking
      def run_stage(stage_id, stage_name)
        JsonEmitter.stage_start(stage_id, stage_name)
        yield
        @stages_completed << stage_id.to_s if @stages_completed
        JsonEmitter.stage_complete(stage_id)
      rescue StandardError => e
        error_details = e.respond_to?(:details) ? e.details : nil
        error_suggestions = e.respond_to?(:suggestions) ? e.suggestions : []
        error_code = e.respond_to?(:code) ? e.code : nil
        JsonEmitter.stage_error(stage_id, e.message, details: error_details, suggestions: error_suggestions, code: error_code)
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
              stage: :cmake,
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
              stage: :compile,
              details: e.output,
              suggestions: [
                "Check the error messages above",
                "Run with --verbose for full output"
              ]
            )
          end
        end

        run_stage(:completion, "Complete") do
          exe_path = File.join(release_dir, gmr_exe)
          UI.success "#{build_type} build complete!"
          UI.info "Executable: #{exe_path}"
          display_build_sizes("native", exe_path)
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
              stage: :cmake,
              details: e.output,
              suggestions: [
                "Make sure Emscripten is properly set up",
                "Run 'gmrcli setup' if you haven't already"
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
              stage: :compile,
              details: e.output,
              suggestions: ["Check the error messages above"]
            )
          end
        end

        run_stage(:completion, "Complete") do
          html_path = File.join(web_release_dir, "gmr.html")
          UI.success "Web build complete!"
          UI.info "Output: #{html_path}"
          display_build_sizes("web", html_path)
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
