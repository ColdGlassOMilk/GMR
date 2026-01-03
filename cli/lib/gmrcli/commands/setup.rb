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

        # Determine target type for JSON output
        target = determine_target
        JsonEmitter.setup_start(target)

        UI.banner

        run_stage(:environment, "Environment Detection") { detect_environment }
        run_stage(:directories, "Directory Setup") { clean_if_requested; create_directories }

        unless skip_native?
          unless options[:skip_pacman]
            run_stage(:packages, "Installing Packages") { install_system_packages }
          else
            JsonEmitter.stage_skip(:packages, reason: "Skipped via --skip-pacman")
          end
          run_stage(:mruby_native, "Building mruby (native)") { build_mruby_native }
          run_stage(:raylib_native, "Building raylib (native)") { build_raylib_native }
        else
          JsonEmitter.stage_skip(:packages, reason: "Native build skipped")
          JsonEmitter.stage_skip(:mruby_native, reason: "Native build skipped")
          JsonEmitter.stage_skip(:raylib_native, reason: "Native build skipped")
        end

        unless skip_web?
          run_stage(:emscripten, "Setting up Emscripten") { setup_emscripten }
          run_stage(:raylib_web, "Building raylib (web)") { build_raylib_web }
          run_stage(:mruby_web, "Building mruby (web)") { build_mruby_web; create_env_script }
        else
          JsonEmitter.stage_skip(:emscripten, reason: "Web build skipped")
          JsonEmitter.stage_skip(:raylib_web, reason: "Web build skipped")
          JsonEmitter.stage_skip(:mruby_web, reason: "Web build skipped")
        end

        run_stage(:verification, "Verification") { verify_installation }
        run_stage(:completion, "Complete") { show_completion }

        JsonEmitter.setup_complete
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

      # Determine the target type based on options
      def determine_target
        if skip_web?
          :native
        elsif skip_native?
          :web
        else
          :full
        end
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
          git
          bison
          unzip
          tar
        ]
      end

      # === Build mruby Native ===

      def build_mruby_native
        src_dir = File.join(Platform.deps_dir, "mruby", "source")
        install_dir = File.join(Platform.deps_dir, "mruby", "native")

        lib_exists = File.exist?(File.join(install_dir, "lib", "libmruby.a"))
        headers_exist = File.exist?(File.join(install_dir, "include", "mruby.h"))

        if lib_exists && headers_exist
          UI.step "mruby native already built (skipping)"
          UI.info "To rebuild: delete #{install_dir}"
          JsonEmitter.stage_progress(:mruby_native, 100, "Already built", substage: "cached")
          return
        end

        # If lib exists but headers are missing, just copy headers
        if lib_exists && !headers_exist && Dir.exist?(File.join(src_dir, "include"))
          UI.step "Copying missing mruby headers..."
          FileUtils.mkdir_p(File.join(install_dir, "include"))
          FileUtils.cp_r(
            Dir[File.join(src_dir, "include", "*")],
            File.join(install_dir, "include")
          )
          UI.success "mruby headers installed"
          JsonEmitter.stage_progress(:mruby_native, 100, "Headers installed", substage: "fixed")
          return
        end

        UI.step "Building mruby (native)..."

        # Clone if needed (might already exist from web build)
        unless Dir.exist?(src_dir)
          JsonEmitter.stage_progress(:mruby_native, 10, "Cloning repository", substage: "clone")
          UI.spinner("Cloning mruby") do
            Shell.git_clone("https://github.com/mruby/mruby.git", src_dir, verbose: verbose?)
          end
        end

        # Clean previous build
        FileUtils.rm_rf(File.join(src_dir, "build", "native"))

        # Create build config for native
        JsonEmitter.stage_progress(:mruby_native, 20, "Creating build config", substage: "configure")
        create_mruby_native_config(src_dir)

        config_path = File.join(src_dir, "build_config", "native.rb")
        build_env = { "MRUBY_CONFIG" => config_path }

        begin
          JsonEmitter.stage_progress(:mruby_native, 30, "Compiling", substage: "compile")
          UI.spinner("Compiling mruby for native") do
            Shell.run!(
              "rake",
              chdir: src_dir,
              env: build_env,
              verbose: verbose?
            )
          end
        rescue CommandError
          UI.warn "Build failed, retrying with clean..."
          JsonEmitter.stage_progress(:mruby_native, 50, "Retrying build", substage: "retry")
          Shell.run("rake clean", chdir: src_dir, env: build_env)
          UI.spinner("Retrying mruby build") do
            Shell.run!(
              "rake",
              chdir: src_dir,
              env: build_env,
              verbose: true
            )
          end
        end

        # Install
        JsonEmitter.stage_progress(:mruby_native, 90, "Installing", substage: "install")
        UI.spinner("Installing") do
          FileUtils.mkdir_p(File.join(install_dir, "lib"))
          FileUtils.mkdir_p(File.join(install_dir, "include"))

          # Copy library
          lib_src = File.join(src_dir, "build", "native", "lib", "libmruby.a")
          lib_dst = File.join(install_dir, "lib")
          $stderr.puts "[DEBUG] Copying lib: #{lib_src} -> #{lib_dst}"
          FileUtils.cp(lib_src, lib_dst)

          # Copy headers
          include_src = File.join(src_dir, "include")
          include_dst = File.join(install_dir, "include")
          header_files = Dir[File.join(include_src, "*")]
          $stderr.puts "[DEBUG] Copying headers: #{header_files.inspect} -> #{include_dst}"
          FileUtils.cp_r(header_files, include_dst)

          # Verify
          copied_headers = Dir[File.join(include_dst, "*")]
          $stderr.puts "[DEBUG] Copied headers result: #{copied_headers.inspect}"
        end

        UI.success "mruby native built"
      end

      def create_mruby_native_config(src_dir)
        config_content = <<~RUBY
          MRuby::Build.new('native') do |conf|
            toolchain :gcc

            conf.cc do |cc|
              cc.flags = %w(-O2)
            end

            conf.cxx do |cxx|
              cxx.flags = %w(-O2)
            end

            # mrbc compiler - required for compiling Ruby source to bytecode
            conf.gem core: 'mruby-bin-mrbc'

            # Core gems for native build
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
          end
        RUBY

        FileUtils.mkdir_p(File.join(src_dir, "build_config"))
        File.write(File.join(src_dir, "build_config", "native.rb"), config_content)
      end

      # === Build raylib Native ===

      def build_raylib_native
        src_dir = File.join(Platform.deps_dir, "raylib", "source")
        install_dir = File.join(Platform.deps_dir, "raylib", "native")

        if File.exist?(File.join(install_dir, "lib", "libraylib.a"))
          UI.step "raylib native already built (skipping)"
          UI.info "To rebuild: delete #{install_dir}"
          JsonEmitter.stage_progress(:raylib_native, 100, "Already built", substage: "cached")
          return
        end

        UI.step "Building raylib (native)..."

        # Clone if needed
        unless Dir.exist?(src_dir)
          JsonEmitter.stage_progress(:raylib_native, 10, "Cloning repository", substage: "clone")
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

        # Use ninja from PATH (works with both MSYS2 and custom toolchains)
        if Platform.mingw64? && Platform.command_exists?("ninja")
          ninja_path = Platform.command_path("ninja")
          cmake_args << "-DCMAKE_MAKE_PROGRAM=\"#{ninja_path}\"" if ninja_path
        end

        JsonEmitter.stage_progress(:raylib_native, 20, "Configuring", substage: "configure")
        UI.spinner("Configuring") do
          Shell.cmake(cmake_args.join(" "), chdir: build_dir, verbose: verbose?)
        end

        # Build
        JsonEmitter.stage_progress(:raylib_native, 40, "Compiling", substage: "compile")
        UI.spinner("Compiling") do
          Shell.ninja("", chdir: build_dir, verbose: verbose?)
        end

        # Install
        JsonEmitter.stage_progress(:raylib_native, 90, "Installing", substage: "install")
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
          JsonEmitter.stage_progress(:emscripten, 10, "Cloning emsdk repository", substage: "clone")
          UI.spinner("Cloning emsdk") do
            Shell.git_clone("https://github.com/emscripten-core/emsdk.git", emsdk_dir, verbose: verbose?)
          end
        else
          JsonEmitter.stage_progress(:emscripten, 10, "emsdk already cloned", substage: "clone")
        end

        # Install emscripten if needed
        unless Dir.exist?(emscripten_path)
          JsonEmitter.stage_progress(:emscripten, 30, "Installing Emscripten SDK", substage: "install")
          UI.spinner("Installing Emscripten (this takes a while)") do
            Shell.run!("./emsdk install latest", chdir: emsdk_dir, verbose: verbose?)
          end
          JsonEmitter.stage_progress(:emscripten, 70, "Activating Emscripten", substage: "activate")
          UI.spinner("Activating Emscripten") do
            Shell.run!("./emsdk activate latest", chdir: emsdk_dir, verbose: verbose?)
          end
        else
          JsonEmitter.stage_progress(:emscripten, 70, "Emscripten already installed", substage: "cached")
          UI.info "Emscripten already installed"
        end

        # Create wrapper scripts
        JsonEmitter.stage_progress(:emscripten, 90, "Creating wrapper scripts", substage: "wrappers")
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
          JsonEmitter.stage_progress(:raylib_web, 100, "Already built", substage: "cached")
          return
        end

        UI.step "Building raylib (web)..."

        # Clone if needed (might already exist from native build)
        unless Dir.exist?(src_dir)
          JsonEmitter.stage_progress(:raylib_web, 10, "Cloning repository", substage: "clone")
          UI.spinner("Cloning raylib") do
            Shell.git_clone("https://github.com/raysan5/raylib.git", src_dir, verbose: verbose?)
          end
        else
          JsonEmitter.stage_progress(:raylib_web, 10, "Source already cloned", substage: "clone")
        end

        build_dir = File.join(src_dir, "build-web")
        FileUtils.rm_rf(build_dir)
        FileUtils.mkdir_p(build_dir)

        # Need to source emscripten environment
        env_vars = emscripten_env

        cmake_args = "-DCMAKE_BUILD_TYPE=Release -DPLATFORM=Web " \
                     "-DBUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX=\"#{install_dir}\" -G Ninja"
        # Use ninja from PATH (works with both MSYS2 and custom toolchains)
        if Platform.mingw64? && Platform.command_exists?("ninja")
          ninja_path = Platform.command_path("ninja")
          cmake_args += " -DCMAKE_MAKE_PROGRAM=\"#{ninja_path}\"" if ninja_path
        end

        JsonEmitter.stage_progress(:raylib_web, 20, "Configuring", substage: "configure")
        UI.spinner("Configuring") do
          Shell.run!(
            "emcmake cmake .. #{cmake_args}",
            chdir: build_dir,
            env: env_vars,
            verbose: verbose?
          )
        end

        JsonEmitter.stage_progress(:raylib_web, 40, "Compiling", substage: "compile")
        UI.spinner("Compiling") do
          Shell.run!("ninja", chdir: build_dir, env: env_vars, verbose: verbose?)
        end

        JsonEmitter.stage_progress(:raylib_web, 90, "Installing", substage: "install")
        UI.spinner("Installing") do
          Shell.run!("ninja install", chdir: build_dir, env: env_vars, verbose: verbose?)
        end

        UI.success "raylib web built"
      end

      # === Build mruby Web ===

      def build_mruby_web
        src_dir = File.join(Platform.deps_dir, "mruby", "source")
        install_dir = File.join(Platform.deps_dir, "mruby", "web")

        lib_exists = File.exist?(File.join(install_dir, "lib", "libmruby.a"))
        headers_exist = File.exist?(File.join(install_dir, "include", "mruby.h"))

        if lib_exists && headers_exist
          UI.step "mruby web already built (skipping)"
          UI.info "To rebuild: delete #{install_dir}"
          JsonEmitter.stage_progress(:mruby_web, 100, "Already built", substage: "cached")
          return
        end

        # If lib exists but headers are missing, just copy headers
        if lib_exists && !headers_exist && Dir.exist?(File.join(src_dir, "include"))
          UI.step "Copying missing mruby headers..."
          FileUtils.mkdir_p(File.join(install_dir, "include"))
          FileUtils.cp_r(
            Dir[File.join(src_dir, "include", "*")],
            File.join(install_dir, "include")
          )
          UI.success "mruby headers installed"
          JsonEmitter.stage_progress(:mruby_web, 100, "Headers installed", substage: "fixed")
          return
        end

        UI.step "Building mruby (web)..."

        # Clone if needed
        unless Dir.exist?(src_dir)
          JsonEmitter.stage_progress(:mruby_web, 10, "Cloning repository", substage: "clone")
          UI.spinner("Cloning mruby") do
            Shell.git_clone("https://github.com/mruby/mruby.git", src_dir, verbose: verbose?)
          end
        else
          JsonEmitter.stage_progress(:mruby_web, 10, "Source already cloned", substage: "clone")
        end

        # Clean previous build
        FileUtils.rm_rf(File.join(src_dir, "build", "emscripten"))

        # Create build config
        JsonEmitter.stage_progress(:mruby_web, 20, "Creating build config", substage: "configure")
        create_mruby_web_config(src_dir)

        config_path = File.join(src_dir, "build_config", "emscripten.rb")
        env_vars = emscripten_env.merge({ "MRUBY_CONFIG" => config_path })

        begin
          JsonEmitter.stage_progress(:mruby_web, 30, "Compiling", substage: "compile")
          UI.spinner("Compiling mruby for web") do
            Shell.run!(
              "rake",
              chdir: src_dir,
              env: env_vars,
              verbose: verbose?
            )
          end
        rescue CommandError
          UI.warn "Build failed, retrying with clean..."
          JsonEmitter.stage_progress(:mruby_web, 50, "Retrying build", substage: "retry")
          Shell.run("rake clean", chdir: src_dir, env: env_vars)
          UI.spinner("Retrying mruby build") do
            Shell.run!(
              "rake",
              chdir: src_dir,
              env: env_vars,
              verbose: true
            )
          end
        end

        # Install
        JsonEmitter.stage_progress(:mruby_web, 90, "Installing", substage: "install")
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
        # Use locally built mruby from deps directory
        File.join(Platform.deps_dir, "mruby", "native", "lib", "libmruby.a")
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
