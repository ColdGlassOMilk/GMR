# frozen_string_literal: true

module Gmrcli
  # Helper module for Emscripten environment configuration.
  #
  # Provides methods to build the environment variables needed for
  # Emscripten builds (WebAssembly compilation).
  #
  module Emscripten
    class << self
      # Build environment variables for Emscripten compilation.
      #
      # @param include_lib_paths [Boolean] Include RAYLIB_WEB_PATH and MRUBY_WEB_PATH
      # @return [Hash] Environment variables hash
      def env(include_lib_paths: false)
        emsdk_dir = normalize(File.join(Platform.deps_dir, "emsdk"))
        emscripten_path = normalize(File.join(emsdk_dir, "upstream", "emscripten"))

        # Find node and python directories (versioned subdirectories)
        node_version_dir = Dir.glob(File.join(emsdk_dir, "node", "*")).first
        python_version_dir = Dir.glob(File.join(emsdk_dir, "python", "*")).first

        # Node bin directory and executable
        node_bin_dir = node_version_dir ? normalize(File.join(node_version_dir, "bin")) : nil
        node_exe = node_bin_dir ? normalize(File.join(node_bin_dir, "node.exe")) : nil

        # Python executable
        python_exe = python_version_dir ? normalize(File.join(python_version_dir, "python.exe")) : nil

        # Emscripten config file
        em_config = normalize(File.join(emsdk_dir, ".emscripten"))

        path_additions = [
          normalize(Platform.bin_dir),
          emscripten_path,
          node_bin_dir,
          python_version_dir ? normalize(python_version_dir) : nil,
          normalize(File.join(emsdk_dir, "upstream", "bin")),
          Platform.windows? ? normalize(File.join(Platform.mingw_root, "bin")) : nil
        ].compact.join(File::PATH_SEPARATOR)

        # Use user's home directory for cache (no spaces in path)
        cache_dir = emscripten_cache_dir

        env = {
          "PATH" => "#{path_additions}#{File::PATH_SEPARATOR}#{ENV['PATH']}",
          "EMSDK" => emsdk_dir,
          "EM_CONFIG" => em_config,
          "EM_CACHE" => cache_dir
        }

        # Add python and node paths if found
        env["EMSDK_PYTHON"] = python_exe if python_exe && File.exist?(python_exe)
        env["EMSDK_NODE"] = node_exe if node_exe && File.exist?(node_exe)

        # Optionally include library paths for builds that need them
        if include_lib_paths
          env["RAYLIB_WEB_PATH"] = normalize(File.join(Platform.deps_dir, "raylib", "web"))
          env["MRUBY_WEB_PATH"] = normalize(File.join(Platform.deps_dir, "mruby", "web"))
        end

        env
      end

      private

      # Normalize path to forward slashes (CMake/Emscripten prefer this)
      def normalize(path)
        path&.gsub("\\", "/")
      end

      # Get the Emscripten cache directory (must have no spaces in path)
      def emscripten_cache_dir
        if Platform.windows?
          # USERPROFILE is Windows-style (C:\Users\...), HOME may be Unix-style (/c/Users/...)
          home = ENV["USERPROFILE"] || Platform.to_windows_path(ENV["HOME"]) || "C:/tmp"
          normalize(File.join(home, ".emcache"))
        else
          normalize(File.join(ENV["HOME"], ".emcache"))
        end
      end
    end
  end
end
