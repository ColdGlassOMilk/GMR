# frozen_string_literal: true

require "fileutils"

module Gmrcli
  module Commands
    # Build command - builds GMR for various targets
    class Build
      TARGETS = %w[debug release web clean all].freeze

      attr_reader :options

      def initialize(options = {})
        @options = {
          verbose: false,
          rebuild: false
        }.merge(options)
      end

      # === Public Build Methods ===

      def debug
        UI.banner
        build_native("Debug")
      end

      def release
        UI.banner
        build_native("Release")
      end

      def web
        UI.banner
        build_web
      end

      def clean
        UI.step "Cleaning build artifacts..."

        cleaned = []

        # Clean build and release directories
        build_root = File.join(Platform.gmr_root, "build")
        release_root = File.join(Platform.gmr_root, "release")

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

      # === Native Build ===

      def build_native(build_type)
        UI.step "Building native #{build_type.upcase}..."

        validate_native_environment!

        # Clean if rebuild requested
        if options[:rebuild]
          UI.info "Cleaning previous build..."
          FileUtils.rm_rf(build_dir)
        end

        FileUtils.mkdir_p(build_dir)

        # Configure
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

        # Build
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

        UI.success "#{build_type} build complete!"
        UI.info "Executable: #{File.join(release_dir, gmr_exe)}"
      end

      def build_cmake_args(build_type)
        args = [
          "../..",
          "-G Ninja",
          "-DCMAKE_BUILD_TYPE=#{build_type}"
        ]

        if Platform.mingw64?
          args << "-DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/ninja.exe"
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

        validate_web_environment!

        # Clean if rebuild requested
        if options[:rebuild]
          UI.info "Cleaning previous build..."
          FileUtils.rm_rf(web_build_dir)
        end

        FileUtils.mkdir_p(web_build_dir)

        env = emscripten_env

        # Configure with Ninja generator (same as native builds)
        UI.info "Configuring..."
        cmake_args = [
          "../..",
          "-G Ninja",
          "-DCMAKE_BUILD_TYPE=Release",
          "-DPLATFORM=Web"
        ]
        if Platform.mingw64?
          cmake_args << "-DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/ninja.exe"
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

        # Build with emmake ninja -j1 (single-threaded to avoid ASYNCIFY hangs on Windows)
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

        UI.success "Web build complete!"
        UI.info "Output: #{File.join(web_release_dir, 'gmr.html')}"
        UI.blank
        UI.info "To test locally:"
        UI.command_hint "gmrcli run web", "Start local server"
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

      def build_dir
        File.join(Platform.gmr_root, "build", "native")
      end

      def web_build_dir
        File.join(Platform.gmr_root, "build", "web")
      end

      def release_dir
        File.join(Platform.gmr_root, "release")
      end

      def web_release_dir
        File.join(Platform.gmr_root, "release", "web")
      end

      def gmr_exe
        Platform.gmr_executable
      end
    end
  end
end
