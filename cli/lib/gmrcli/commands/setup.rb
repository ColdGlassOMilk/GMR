# frozen_string_literal: true

require "fileutils"

module Gmrcli
  module Commands
    # Setup command - installs all dependencies for GMR development
    class Setup
      attr_reader :options

      def initialize(options = {})
        @options = normalize_options(options)
      end

      def run
        # Handle --fix-ssl separately
        if options[:fix_ssl]
          fix_ssl_certificates
          return
        end

        UI.banner
        detect_environment
        clean_if_requested
        create_directories

        unless skip_native?
          install_system_packages unless options[:skip_pacman]
          build_raylib_native
        end

        unless skip_web?
          setup_emscripten
          build_raylib_web
          build_mruby_web
          create_env_script
        end

        verify_installation
        show_completion
      end

      private

      # Normalize option aliases
      def normalize_options(opts)
        defaults = {
          native_only: false,
          skip_web: false,
          web_only: false,
          skip_native: false,
          skip_pacman: false,
          verbose: false,
          clean: false,
          fix_ssl: false
        }
        defaults.merge(opts)
      end

      # Check if we should skip web builds
      def skip_web?
        options[:native_only] || options[:skip_web]
      end

      # Check if we should skip native builds
      def skip_native?
        options[:web_only] || options[:skip_native]
      end

      def verbose?
        options[:verbose]
      end

      # === SSL Fix ===

      def fix_ssl_certificates
        UI.step "Fixing SSL certificates..."

        UI.spinner("Reinitializing pacman keyring") do
          Shell.run("rm -rf /etc/pacman.d/gnupg", verbose: verbose?)
          Shell.run("pacman-key --init", verbose: verbose?)
          Shell.run("pacman-key --populate msys2", verbose: verbose?)
        end

        UI.spinner("Updating CA certificates") do
          Shell.run("pacman -Sy --noconfirm ca-certificates msys2-keyring", verbose: verbose?)
        end

        UI.spinner("Updating core packages") do
          Shell.run("pacman -S --noconfirm --needed pacman pacman-mirrors msys2-runtime", verbose: verbose?)
        end

        UI.success "SSL fix complete"
        UI.info "Please restart your MSYS2 terminal and run setup again"
      end

      # === Environment Detection ===

      def detect_environment
        UI.step "Detecting environment..."

        if Platform.windows? && !Platform.mingw64?
          raise PlatformError.new(
            "Please run from MSYS2 MinGW64 terminal",
            suggestions: [
              "Open 'MSYS2 MinGW64' from the Start menu",
              "Current environment: #{Platform.name}"
            ]
          )
        end

        unless Platform.windows? || Platform.linux? || Platform.macos?
          raise PlatformError.new("Unsupported platform: #{Platform.name}")
        end

        UI.success "Running on #{Platform.name}"
      end

      # === Cleanup ===

      def clean_if_requested
        return unless options[:clean]

        UI.step "Cleaning previous setup..."

        dirs = [Platform.deps_dir, build_dir, web_build_dir]
        dirs.each do |dir|
          if Dir.exist?(dir)
            FileUtils.rm_rf(dir)
            UI.info "Removed #{dir}"
          end
        end

        UI.success "Cleaned"
      end

      def create_directories
        FileUtils.mkdir_p(Platform.deps_dir)
        FileUtils.mkdir_p(Platform.bin_dir)
      end

      # === System Packages ===

      def install_system_packages
        return unless Platform.mingw64?

        UI.step "Installing system packages via pacman..."

        # Update package database
        UI.spinner("Updating package database") do
          Shell.run("pacman -Sy --noconfirm --disable-download-timeout", verbose: verbose?)
        end

        packages = base_packages
        packages << "ruby" unless options[:native_only] # Ruby needed for mruby web build

        UI.info "Packages: #{packages.join(', ')}"

        begin
          UI.spinner("Installing packages") do
            Shell.run!(
              "pacman -S --noconfirm --needed --disable-download-timeout #{packages.join(' ')}",
              verbose: verbose?,
              error_message: "Package installation failed"
            )
          end
        rescue CommandError => e
          UI.warn "Some packages may have failed, retrying..."
          # Try with overwrite for corrupted packages
          Shell.run(
            "pacman -S --noconfirm --needed --overwrite '*' #{packages.join(' ')}",
            verbose: true
          )
        end
      end

      def base_packages
        %w[
          mingw-w64-x86_64-gcc
          mingw-w64-x86_64-gdb
          mingw-w64-x86_64-cmake
          mingw-w64-x86_64-ninja
          mingw-w64-x86_64-mruby
          git
          bison
          unzip
          tar
        ]
      end

      # === Build raylib Native ===

      def build_raylib_native
        src_dir = File.join(Platform.deps_dir, "raylib", "source")
        install_dir = File.join(Platform.deps_dir, "raylib", "native")

        if File.exist?(File.join(install_dir, "lib", "libraylib.a"))
          UI.step "raylib native already built (skipping)"
          UI.info "To rebuild: delete #{install_dir}"
          return
        end

        UI.step "Building raylib (native)..."

        # Clone if needed
        unless Dir.exist?(src_dir)
          UI.spinner("Cloning raylib") do
            Shell.git_clone("https://github.com/raysan5/raylib.git", src_dir, verbose: verbose?)
          end
        end

        build_dir = File.join(src_dir, "build-native")
        FileUtils.rm_rf(build_dir)
        FileUtils.mkdir_p(build_dir)

        # Configure
        cmake_args = [
          "..",
          "-G Ninja",
          "-DCMAKE_BUILD_TYPE=Release",
          "-DBUILD_SHARED_LIBS=OFF",
          "-DUSE_EXTERNAL_GLFW=OFF",
          "-DBUILD_EXAMPLES=OFF",
          "-DCMAKE_INSTALL_PREFIX=\"#{install_dir}\""
        ]

        if Platform.mingw64?
          cmake_args << "-DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/ninja.exe"
        end

        UI.spinner("Configuring") do
          Shell.cmake(cmake_args.join(" "), chdir: build_dir, verbose: verbose?)
        end

        # Build
        UI.spinner("Compiling") do
          Shell.ninja("", chdir: build_dir, verbose: verbose?)
        end

        # Install
        UI.spinner("Installing") do
          Shell.ninja("install", chdir: build_dir, verbose: verbose?)
        end

        UI.success "raylib native built"
      end

      # === Emscripten Setup ===

      def setup_emscripten
        UI.step "Setting up Emscripten SDK..."

        emsdk_dir = File.join(Platform.deps_dir, "emsdk")
        emscripten_path = File.join(emsdk_dir, "upstream", "emscripten")

        # Clone emsdk if needed
        unless Dir.exist?(emsdk_dir)
          UI.spinner("Cloning emsdk") do
            Shell.git_clone("https://github.com/emscripten-core/emsdk.git", emsdk_dir, verbose: verbose?)
          end
        end

        # Install emscripten if needed
        unless Dir.exist?(emscripten_path)
          UI.spinner("Installing Emscripten (this takes a while)") do
            Shell.run!("./emsdk install latest", chdir: emsdk_dir, verbose: verbose?)
            Shell.run!("./emsdk activate latest", chdir: emsdk_dir, verbose: verbose?)
          end
        else
          UI.info "Emscripten already installed"
        end

        # Create wrapper scripts
        UI.spinner("Creating wrapper scripts") do
          create_emscripten_wrappers(emsdk_dir, emscripten_path)
        end

        UI.success "Emscripten SDK ready"
      end

      def create_emscripten_wrappers(emsdk_dir, emscripten_path)

        %w[emcc em++ emcmake emmake emar emranlib emconfig emrun emsize].each do |tool|
          tool_path = if File.exist?(File.join(emscripten_path, tool))
                        File.join(emscripten_path, tool)
                      elsif File.exist?(File.join(emscripten_path, "#{tool}.py"))
                        File.join(emscripten_path, "#{tool}.py")
                      end

          next unless tool_path

          wrapper_path = File.join(Platform.bin_dir, tool)
          File.write(wrapper_path, <<~WRAPPER)
            #!/bin/bash
            "#{tool_path}" "$@"
          WRAPPER
          FileUtils.chmod("+x", wrapper_path)
        end
      end

      # === Build raylib Web ===

      def build_raylib_web
        src_dir = File.join(Platform.deps_dir, "raylib", "source")
        install_dir = File.join(Platform.deps_dir, "raylib", "web")

        if File.exist?(File.join(install_dir, "lib", "libraylib.a"))
          UI.step "raylib web already built (skipping)"
          UI.info "To rebuild: delete #{install_dir}"
          return
        end

        UI.step "Building raylib (web)..."

        # Clone if needed (might already exist from native build)
        unless Dir.exist?(src_dir)
          UI.spinner("Cloning raylib") do
            Shell.git_clone("https://github.com/raysan5/raylib.git", src_dir, verbose: verbose?)
          end
        end

        build_dir = File.join(src_dir, "build-web")
        FileUtils.rm_rf(build_dir)
        FileUtils.mkdir_p(build_dir)

        # Need to source emscripten environment
        env_vars = emscripten_env

        cmake_args = "-DCMAKE_BUILD_TYPE=Release -DPLATFORM=Web " \
                     "-DBUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX=\"#{install_dir}\" -G Ninja"
        if Platform.mingw64?
          cmake_args += " -DCMAKE_MAKE_PROGRAM=C:/msys64/mingw64/bin/ninja.exe"
        end

        UI.spinner("Configuring") do
          Shell.run!(
            "emcmake cmake .. #{cmake_args}",
            chdir: build_dir,
            env: env_vars,
            verbose: verbose?
          )
        end

        UI.spinner("Compiling") do
          Shell.run!("ninja", chdir: build_dir, env: env_vars, verbose: verbose?)
        end

        UI.spinner("Installing") do
          Shell.run!("ninja install", chdir: build_dir, env: env_vars, verbose: verbose?)
        end

        UI.success "raylib web built"
      end

      # === Build mruby Web ===

      def build_mruby_web
        src_dir = File.join(Platform.deps_dir, "mruby", "source")
        install_dir = File.join(Platform.deps_dir, "mruby", "web")

        if File.exist?(File.join(install_dir, "lib", "libmruby.a"))
          UI.step "mruby web already built (skipping)"
          UI.info "To rebuild: delete #{install_dir}"
          return
        end

        UI.step "Building mruby (web)..."

        # Clone if needed
        unless Dir.exist?(src_dir)
          UI.spinner("Cloning mruby") do
            Shell.git_clone("https://github.com/mruby/mruby.git", src_dir, verbose: verbose?)
          end
        end

        # Clean previous build
        FileUtils.rm_rf(File.join(src_dir, "build", "emscripten"))

        # Create build config
        create_mruby_web_config(src_dir)

        env_vars = emscripten_env
        config_path = File.join(src_dir, "build_config", "emscripten.rb")

        begin
          UI.spinner("Compiling mruby for web") do
            Shell.run!(
              "MRUBY_CONFIG=\"#{config_path}\" rake",
              chdir: src_dir,
              env: env_vars,
              verbose: verbose?
            )
          end
        rescue CommandError
          UI.warn "Build failed, retrying with clean..."
          Shell.run("MRUBY_CONFIG=\"#{config_path}\" rake clean", chdir: src_dir, env: env_vars)
          UI.spinner("Retrying mruby build") do
            Shell.run!(
              "MRUBY_CONFIG=\"#{config_path}\" rake",
              chdir: src_dir,
              env: env_vars,
              verbose: true
            )
          end
        end

        # Install
        UI.spinner("Installing") do
          FileUtils.mkdir_p(File.join(install_dir, "lib"))
          FileUtils.mkdir_p(File.join(install_dir, "include"))
          FileUtils.cp(
            File.join(src_dir, "build", "emscripten", "lib", "libmruby.a"),
            File.join(install_dir, "lib")
          )
          FileUtils.cp_r(
            Dir[File.join(src_dir, "include", "*")],
            File.join(install_dir, "include")
          )
        end

        UI.success "mruby web built"
      end

      def create_mruby_web_config(src_dir)
        config_content = <<~RUBY
          MRuby::CrossBuild.new('emscripten') do |conf|
            toolchain :clang

            conf.cc do |cc|
              cc.command = 'emcc'
              cc.flags = %w(-Os)
            end

            conf.cxx do |cxx|
              cxx.command = 'em++'
              cxx.flags = %w(-Os)
            end

            conf.linker do |linker|
              linker.command = 'emcc'
            end

            conf.archiver do |archiver|
              archiver.command = 'emar'
            end

            # Minimal gems for web build (matches master branch)
            conf.gem core: 'mruby-compiler'
            conf.gem core: 'mruby-error'
            conf.gem core: 'mruby-eval'
            conf.gem core: 'mruby-metaprog'
            conf.gem core: 'mruby-sprintf'
            conf.gem core: 'mruby-math'
            conf.gem core: 'mruby-time'
            conf.gem core: 'mruby-string-ext'
            conf.gem core: 'mruby-array-ext'
            conf.gem core: 'mruby-hash-ext'
            conf.gem core: 'mruby-numeric-ext'
            conf.gem core: 'mruby-proc-ext'
            conf.gem core: 'mruby-symbol-ext'
            conf.gem core: 'mruby-random'
            conf.gem core: 'mruby-object-ext'
            conf.gem core: 'mruby-kernel-ext'
            conf.gem core: 'mruby-class-ext'
            conf.gem core: 'mruby-enum-ext'
            conf.gem core: 'mruby-struct'
            conf.gem core: 'mruby-range-ext'
            conf.gem core: 'mruby-fiber'
            conf.gem core: 'mruby-enumerator'
            conf.gem core: 'mruby-compar-ext'
            conf.gem core: 'mruby-toplevel-ext'

            # Extended gems (commented out to reduce build time/ASYNCIFY overhead)
            # conf.gem core: 'mruby-pack'
            # conf.gem github: 'mattn/mruby-json'
          end
        RUBY

        FileUtils.mkdir_p(File.join(src_dir, "build_config"))
        File.write(File.join(src_dir, "build_config", "emscripten.rb"), config_content)
      end

      # === Environment Script ===

      def create_env_script
        UI.info "Creating env.sh..."

        emsdk_dir = File.join(Platform.deps_dir, "emsdk")
        raylib_web = File.join(Platform.deps_dir, "raylib", "web")
        mruby_web = File.join(Platform.deps_dir, "mruby", "web")

        content = <<~BASH
          #!/bin/bash
          # GMR Environment Setup
          # Source this file before building: source env.sh

          GMR_DIR="#{Platform.gmr_root}"
          export EMSDK="#{emsdk_dir}"
          export RAYLIB_WEB_PATH="#{raylib_web}"
          export MRUBY_WEB_PATH="#{mruby_web}"

          # Add wrapper scripts to PATH
          if [[ -d "$GMR_DIR/bin" ]]; then
              export PATH="$GMR_DIR/bin:$PATH"
          fi

          # Add emscripten to PATH
          if [[ -d "$EMSDK/upstream/emscripten" ]]; then
              export PATH="$EMSDK/upstream/emscripten:$PATH"
          fi

          # Add node to PATH
          NODE_BIN=$(find "$EMSDK/node" -type d -name "bin" 2>/dev/null | head -1)
          if [[ -n "$NODE_BIN" ]]; then
              export PATH="$NODE_BIN:$PATH"
          fi

          # Source emsdk environment
          source "$EMSDK/emsdk_env.sh" 2>/dev/null || true

          echo "GMR environment loaded"
          echo "  RAYLIB_WEB_PATH: $RAYLIB_WEB_PATH"
          echo "  MRUBY_WEB_PATH: $MRUBY_WEB_PATH"
          if command -v emcc &> /dev/null; then
              echo "  emcc: $(emcc --version | head -1)"
          fi
        BASH

        File.write(File.join(Platform.gmr_root, "env.sh"), content)
        FileUtils.chmod("+x", File.join(Platform.gmr_root, "env.sh"))
      end

      # === Verification ===

      def verify_installation
        UI.step "Verifying installation..."

        checks = []

        unless skip_native?
          checks << verify_tool("g++")
          checks << verify_tool("cmake")
          checks << verify_tool("ninja")
          checks << verify_file("raylib-native", File.join(Platform.deps_dir, "raylib", "native", "lib", "libraylib.a"))
          checks << verify_file("mruby", mruby_native_path)
        end

        unless skip_web?
          checks << verify_file("raylib-web", File.join(Platform.deps_dir, "raylib", "web", "lib", "libraylib.a"))
          checks << verify_file("mruby-web", File.join(Platform.deps_dir, "mruby", "web", "lib", "libmruby.a"))
        end

        failed = checks.count { |c| !c }
        if failed > 0
          UI.warn "#{failed} component(s) not found"
        end
      end

      def verify_tool(name)
        if Platform.command_exists?(name)
          version = Shell.capture("#{name} --version")&.lines&.first&.strip || "installed"
          UI.status_line(name, version, ok: true)
          true
        else
          UI.status_line(name, "NOT FOUND", ok: false)
          false
        end
      end

      def verify_file(name, path)
        if File.exist?(path)
          UI.status_line(name, "installed", ok: true)
          true
        else
          UI.status_line(name, "NOT FOUND", ok: false)
          false
        end
      end

      def mruby_native_path
        if Platform.mingw64?
          "/mingw64/lib/libmruby.a"
        else
          "/usr/local/lib/libmruby.a"
        end
      end

      # === Completion ===

      def show_completion
        UI.header "Setup Complete!"

        steps = []

        unless skip_native?
          steps << "gmrcli build debug    # Build debug version"
          steps << "gmrcli build release  # Build release version"
        end

        unless skip_web?
          steps << "gmrcli build web      # Build for web"
        end

        steps << "gmrcli run            # Run the game"

        UI.next_steps(steps)
      end

      # === Helpers ===

      def build_dir
        File.join(Platform.gmr_root, "build", "native")
      end

      def web_build_dir
        File.join(Platform.gmr_root, "build", "web")
      end

      def emscripten_env
        emsdk_dir = File.join(Platform.deps_dir, "emsdk")
        emscripten_path = File.join(emsdk_dir, "upstream", "emscripten")

        # Find node
        node_dir = Dir.glob(File.join(emsdk_dir, "node", "*", "bin")).first

        path_additions = [
          Platform.bin_dir,
          emscripten_path,
          node_dir
        ].compact.join(File::PATH_SEPARATOR)

        # Use a cache path with NO SPACES to avoid Windows short path issues
        # The project path has spaces, so we must use an external location
        cache_dir = if Platform.windows?
                      "C:/tmp/emcache"
                    else
                      File.join(ENV['HOME'], ".emcache")
                    end

        {
          "PATH" => "#{path_additions}#{File::PATH_SEPARATOR}#{ENV['PATH']}",
          "EMSDK" => emsdk_dir,
          "EM_CACHE" => cache_dir
        }
      end
    end
  end
end
