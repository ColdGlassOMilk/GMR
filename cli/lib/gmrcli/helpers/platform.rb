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

      # System paths
      def mingw_root
        "C:/msys64/mingw64"
      end

      def home_dir
        ENV["HOME"] || ENV["USERPROFILE"] || "~"
      end

      # GMR-specific paths
      def gmr_root
        return @gmr_root if defined?(@gmr_root)

        # Check environment variable first
        return @gmr_root = ENV["GMR_ROOT"] if ENV["GMR_ROOT"] && Dir.exist?(ENV["GMR_ROOT"])

        # Walk up from current dir looking for GMR engine markers
        @gmr_root = find_gmr_root(Dir.pwd)
      end

      def deps_dir
        File.join(gmr_root, "deps")
      end

      def bin_dir
        File.join(gmr_root, "bin")
      end

      # Find a GMR project root (has scripts/main.rb)
      def find_project_root(start_dir = Dir.pwd)
        dir = File.expand_path(start_dir)
        while dir != File.dirname(dir)
          if File.exist?(File.join(dir, "scripts", "main.rb"))
            return dir
          end
          dir = File.dirname(dir)
        end
        nil
      end

      # Check if directory is a GMR project
      def gmr_project?(dir)
        File.exist?(File.join(dir, "scripts", "main.rb"))
      end

      # Check if directory is the GMR engine root
      def gmr_engine?(dir)
        File.exist?(File.join(dir, "CMakeLists.txt")) &&
          File.exist?(File.join(dir, "src", "main.cpp"))
      end

      # Command/tool detection
      def command_exists?(cmd)
        return @command_cache[cmd] if @command_cache&.key?(cmd)

        @command_cache ||= {}
        @command_cache[cmd] = if windows?
                                system("where #{cmd} >nul 2>&1")
                              else
                                system("which #{cmd} >/dev/null 2>&1")
                              end
      end

      def command_path(cmd)
        return nil unless command_exists?(cmd)

        if windows?
          `where #{cmd} 2>nul`.lines.first&.strip
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
