# frozen_string_literal: true

module Gmrcli
  # Error code registry for machine-readable error identification
  # Error codes are hierarchical: CATEGORY.SUBCATEGORY.SPECIFIC
  module ErrorCodes
    # Exit code ranges by category:
    # 0       - Success
    # 1       - Generic/internal error
    # 2       - Protocol error
    # 10-19   - Setup errors
    # 20-29   - Build errors
    # 30-39   - Run errors
    # 40-49   - Project errors
    # 50-59   - Platform errors
    # 130     - User interrupt (SIGINT)

    REGISTRY = {
      # Setup errors (10-19)
      "SETUP.MISSING_DEP" => { exit_code: 10, description: "Missing dependency" },
      "SETUP.MISSING_DEP.CMAKE" => { exit_code: 10, description: "CMake not found" },
      "SETUP.MISSING_DEP.NINJA" => { exit_code: 10, description: "Ninja not found" },
      "SETUP.MISSING_DEP.GCC" => { exit_code: 10, description: "GCC not found" },
      "SETUP.MISSING_DEP.GIT" => { exit_code: 10, description: "Git not found" },
      "SETUP.EMSCRIPTEN_FAILED" => { exit_code: 11, description: "Emscripten setup failed" },
      "SETUP.MRUBY_BUILD_FAILED" => { exit_code: 12, description: "mruby build failed" },
      "SETUP.RAYLIB_BUILD_FAILED" => { exit_code: 13, description: "raylib build failed" },
      "SETUP.PACKAGES_FAILED" => { exit_code: 14, description: "Package installation failed" },
      "SETUP.VERIFICATION_FAILED" => { exit_code: 15, description: "Setup verification failed" },

      # Build errors (20-29)
      "BUILD.CMAKE_FAILED" => { exit_code: 20, description: "CMake configuration failed" },
      "BUILD.COMPILE_FAILED" => { exit_code: 21, description: "Compilation failed" },
      "BUILD.MISSING_DEP" => { exit_code: 22, description: "Missing build dependency" },
      "BUILD.INVALID_TARGET" => { exit_code: 23, description: "Invalid build target" },
      "BUILD.LINK_FAILED" => { exit_code: 24, description: "Linking failed" },

      # Run errors (30-39)
      "RUN.EXECUTABLE_NOT_FOUND" => { exit_code: 30, description: "Executable not found" },
      "RUN.PROJECT_NOT_FOUND" => { exit_code: 31, description: "Project not found" },
      "RUN.INVALID_TARGET" => { exit_code: 32, description: "Invalid run target" },
      "RUN.LAUNCH_FAILED" => { exit_code: 33, description: "Failed to launch game" },

      # Project errors (40-49)
      "PROJECT.NOT_FOUND" => { exit_code: 40, description: "Not a GMR project" },
      "PROJECT.INVALID" => { exit_code: 41, description: "Invalid project structure" },
      "PROJECT.ALREADY_EXISTS" => { exit_code: 42, description: "Project already exists" },
      "PROJECT.MISSING_FILE" => { exit_code: 43, description: "Required file missing" },

      # Platform errors (50-59)
      "PLATFORM.UNSUPPORTED" => { exit_code: 50, description: "Unsupported platform" },
      "PLATFORM.WRONG_ENV" => { exit_code: 51, description: "Wrong environment" },
      "PLATFORM.MISSING_TOOL" => { exit_code: 52, description: "Required tool missing" },

      # Version/bump errors (60-69)
      "VERSION.INVALID_PART" => { exit_code: 60, description: "Invalid version part" },
      "VERSION.NOT_GIT_REPO" => { exit_code: 61, description: "Not a git repository" },
      "VERSION.GIT_NOT_FOUND" => { exit_code: 62, description: "Git not available" },
      "VERSION.DIRTY_TREE" => { exit_code: 63, description: "Uncommitted changes" },
      "VERSION.NO_ENGINE_JSON" => { exit_code: 64, description: "engine.json not found" },
      "VERSION.INVALID_FORMAT" => { exit_code: 65, description: "Invalid version format" },
      "VERSION.TAG_EXISTS" => { exit_code: 66, description: "Git tag already exists" },

      # Protocol errors (2)
      "PROTOCOL.UNSUPPORTED" => { exit_code: 2, description: "Unsupported protocol version" },

      # Internal/generic errors (1)
      "INTERNAL.UNEXPECTED" => { exit_code: 1, description: "Unexpected internal error" },
      "INTERNAL.COMMAND_FAILED" => { exit_code: 1, description: "Command execution failed" }
    }.freeze

    class << self
      # Get exit code for an error code
      def exit_code_for(code)
        # Try exact match first
        return REGISTRY.dig(code, :exit_code) if REGISTRY.key?(code)

        # Try parent code (e.g., SETUP.MISSING_DEP.CMAKE -> SETUP.MISSING_DEP)
        parent = parent_code(code)
        return REGISTRY.dig(parent, :exit_code) if parent && REGISTRY.key?(parent)

        # Default to 1 for unknown codes
        1
      end

      # Check if error code is valid (registered)
      def valid?(code)
        REGISTRY.key?(code) || (parent_code(code) && REGISTRY.key?(parent_code(code)))
      end

      # Get description for an error code
      def description_for(code)
        REGISTRY.dig(code, :description) ||
          REGISTRY.dig(parent_code(code), :description) ||
          "Unknown error"
      end

      # List all registered codes
      def all_codes
        REGISTRY.keys
      end

      # List codes by category
      def codes_for_category(category)
        prefix = "#{category}."
        REGISTRY.keys.select { |code| code.start_with?(prefix) }
      end

      private

      # Get parent code by removing last segment
      def parent_code(code)
        parts = code.to_s.split(".")
        return nil if parts.length <= 1

        parts[0..-2].join(".")
      end
    end
  end
end
