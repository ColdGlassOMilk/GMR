# frozen_string_literal: true

module Gmrcli
  # Platform detection and system information
  module Platform
    class << self
      # Operating system detection
      def windows?
        !!(RUBY_PLATFORM =~ /mingw|mswin|cygwin/)
      end

      def macos?
        !!(RUBY_PLATFORM =~ /darwin/)
      end

      def linux?
        !!(RUBY_PLATFORM =~ /linux/)
      end

      def unix?
        macos? || linux?
      end

      # Windows-specific environment detection
      def msys2?
        windows? && !ENV["MSYSTEM"].nil?
      end

      def mingw64?
        ENV["MSYSTEM"] == "MINGW64"
      end

      def mingw32?
        ENV["MSYSTEM"] == "MINGW32"
      end

      def wsl?
        return false unless linux?
        return @wsl if defined?(@wsl)

        @wsl = File.exist?("/proc/version") &&
               File.read("/proc/version").downcase.include?("microsoft")
      end

      # Human-readable platform name
      def name
        @name ||= detect_name
      end

      # Get short platform identifier for paths/configs
      def id
        if mingw64?
          "mingw64"
        elsif mingw32?
          "mingw32"
        elsif windows?
          "windows"
        elsif macos?
          "macos"
        elsif wsl?
          "wsl"
        elsif linux?
          "linux"
        else
          "unknown"
        end
      end

      # Convert Unix-style path (/c/path) to Windows-style (C:/path) for Ruby file operations
      # This is needed when environment variables contain Unix-style paths from Git Bash
      def to_windows_path(path)
        return path unless path && windows?
        # Convert /c/path to C:/path
        path.sub(%r{^/([a-zA-Z])/}, '\1:/')
      end

      # System paths
      def mingw_root
        # Check for custom MINGW_PREFIX first (set by GMRuby IDE or custom toolchains)
        if ENV["MINGW_PREFIX"]
          mingw_path = to_windows_path(ENV["MINGW_PREFIX"])
          return mingw_path if Dir.exist?(mingw_path)
        end
        # Default MSYS2 location
        "C:/msys64/mingw64"
      end

      def home_dir
        ENV["HOME"] || ENV["USERPROFILE"] || "~"
      end

      # GMR-specific paths
      def gmr_root
        return @gmr_root if defined?(@gmr_root)

        # Check environment variable first (convert from Unix path if needed)
        if ENV["GMR_ROOT"]
          gmr_path = to_windows_path(ENV["GMR_ROOT"])
          return @gmr_root = gmr_path if Dir.exist?(gmr_path)
        end

        # Walk up from current dir looking for GMR engine markers
        @gmr_root = find_gmr_root(Dir.pwd)
      end

      def deps_dir
        File.join(gmr_root, "deps")
      end

      def bin_dir
        File.join(gmr_root, "bin")
      end

      # Find a GMR project root (has game/scripts/main.rb)
      def find_project_root(start_dir = Dir.pwd)
        dir = File.expand_path(start_dir)
        while dir != File.dirname(dir)
          if File.exist?(File.join(dir, "game", "scripts", "main.rb"))
            return dir
          end
          dir = File.dirname(dir)
        end
        nil
      end

      # Check if directory is a GMR project
      def gmr_project?(dir)
        File.exist?(File.join(dir, "game", "scripts", "main.rb"))
      end

      # Check if directory is the GMR engine root
      def gmr_engine?(dir)
        File.exist?(File.join(dir, "CMakeLists.txt")) &&
          File.exist?(File.join(dir, "src", "main.cpp"))
      end

      # Get the toolchain root directory (where cmake, ninja, git, ruby are installed)
      # For GMRuby IDE, this is the parent of gmr_root (e.g., C:/gmruby)
      # For MSYS2, this is the MINGW_PREFIX (e.g., /mingw64)
      def toolchain_root
        return @toolchain_root if defined?(@toolchain_root)

        # Check for MINGW_PREFIX first (set by GMRuby IDE or custom toolchains)
        mingw_prefix = to_windows_path(ENV["MINGW_PREFIX"])
        if mingw_prefix && Dir.exist?(mingw_prefix)
          # For IDE, MINGW_PREFIX is the mingw64 folder, toolchain root is its parent
          @toolchain_root = File.dirname(mingw_prefix)
        else
          # Fall back to parent of gmr_root (also convert from Unix path if needed)
          gmr = to_windows_path(gmr_root)
          @toolchain_root = File.dirname(gmr)
        end
      end

      # Command/tool detection
      def command_exists?(cmd)
        return @command_cache[cmd] if @command_cache&.key?(cmd)

        @command_cache ||= {}

        # First check if the executable exists directly in known locations
        # This is more reliable than which/where for portable/IDE environments
        if windows?
          exe_name = "#{cmd}.exe"
          known_paths = []

          # Check GMRuby IDE toolchain root (e.g., C:/gmruby or /c/gmruby)
          # Tools are installed at: toolchain_root/cmake, toolchain_root/ninja, etc.
          root = toolchain_root
          known_paths += [
            File.join(root, "cmake", "bin", exe_name),
            File.join(root, "ninja", exe_name),
            File.join(root, "mingw64", "bin", exe_name),
            File.join(root, "git", "bin", exe_name),
            File.join(root, "ruby", "bin", exe_name)
          ]

          # Also check gmr/deps for tools built by gmrcli setup (mruby, raylib, etc.)
          deps = deps_dir
          known_paths += [
            File.join(deps, "mruby", "native", "bin", exe_name),
            File.join(deps, "mruby", "source", "build", "native", "bin", exe_name)
          ]

          if known_paths.any? { |p| File.exist?(p) }
            @command_cache[cmd] = true
            return true
          end
        end

        # Fall back to which/where for system-installed tools
        # Use /dev/null for MSYS2/MinGW (bash), NUL for native Windows cmd
        @command_cache[cmd] = if msys2?
                                system("which #{cmd} >/dev/null 2>&1") || system("where #{cmd} >/dev/null 2>&1")
                              elsif windows?
                                system("where #{cmd} >NUL 2>&1")
                              else
                                system("which #{cmd} >/dev/null 2>&1")
                              end
      end

      def command_path(cmd)
        return nil unless command_exists?(cmd)

        # First check known locations for portable/IDE environments
        if windows?
          exe_name = "#{cmd}.exe"
          known_paths = []

          # Check GMRuby IDE toolchain root (e.g., C:/gmruby or /c/gmruby)
          root = toolchain_root
          known_paths += [
            File.join(root, "cmake", "bin", exe_name),
            File.join(root, "ninja", exe_name),
            File.join(root, "mingw64", "bin", exe_name),
            File.join(root, "git", "bin", exe_name),
            File.join(root, "ruby", "bin", exe_name)
          ]

          # Also check gmr/deps for tools built by gmrcli setup
          deps = deps_dir
          known_paths += [
            File.join(deps, "mruby", "native", "bin", exe_name),
            File.join(deps, "mruby", "source", "build", "native", "bin", exe_name)
          ]

          found = known_paths.find { |p| File.exist?(p) }
          return found if found
        end

        # Fall back to which/where for system-installed tools
        if msys2?
          # Try which first (for Unix-style tools), then where (for Windows tools)
          path = `which #{cmd} 2>/dev/null`.strip
          path.empty? ? `where #{cmd} 2>/dev/null`.lines.first&.strip : path
        elsif windows?
          `where #{cmd} 2>NUL`.lines.first&.strip
        else
          `which #{cmd} 2>/dev/null`.strip
        end
      end

      # CPU/threading
      def nproc
        @nproc ||= detect_nproc
      end

      # Executable names
      def exe_extension
        windows? ? ".exe" : ""
      end

      def gmr_executable
        "gmr#{exe_extension}"
      end

      # Shell to use for commands
      def shell
        if mingw64? || mingw32?
          "bash"
        elsif windows?
          "cmd"
        else
          ENV["SHELL"] || "bash"
        end
      end

      # Validate environment for GMR development
      def validate_environment!
        errors = []

        if windows? && !mingw64?
          errors << "GMR requires MSYS2 MinGW64 environment on Windows"
        end

        unless command_exists?("cmake")
          errors << "cmake not found"
        end

        unless command_exists?("ninja") || command_exists?("make")
          errors << "ninja or make not found"
        end

        unless errors.empty?
          raise EnvironmentError.new(
            "Invalid development environment",
            details: errors.join(", "),
            suggestions: [
              "Run 'gmrcli setup' to install dependencies",
              "On Windows, run from MSYS2 MinGW64 terminal"
            ]
          )
        end

        true
      end

      private

      def detect_name
        if mingw64?
          "Windows (MSYS2 MinGW64)"
        elsif mingw32?
          "Windows (MSYS2 MinGW32)"
        elsif msys2?
          "Windows (MSYS2 #{ENV['MSYSTEM']})"
        elsif windows?
          "Windows"
        elsif macos?
          "macOS"
        elsif wsl?
          "WSL (Windows Subsystem for Linux)"
        elsif linux?
          "Linux"
        else
          "Unknown"
        end
      end

      def detect_nproc
        count = if windows?
                  ENV["NUMBER_OF_PROCESSORS"]&.to_i
                elsif macos?
                  `sysctl -n hw.ncpu 2>/dev/null`.strip.to_i
                else
                  `nproc 2>/dev/null`.strip.to_i
                end
        count.positive? ? count : 4
      end

      def find_gmr_root(start_dir)
        dir = File.expand_path(start_dir)
        while dir != File.dirname(dir)
          if gmr_engine?(dir)
            return dir
          end
          dir = File.dirname(dir)
        end
        # Fallback to current directory
        Dir.pwd
      end
    end
  end
end
