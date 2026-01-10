# frozen_string_literal: true

require "json"
require "fileutils"

module Gmrcli
  module Commands
    # Bump command - increment engine version following semantic versioning
    #
    # This command manages the engine version stored in engine.json and creates
    # git tags for releases. It follows strict semantic versioning (SemVer).
    #
    # ## Usage
    #
    #   gmrcli bump major     # 0.1.0 -> 1.0.0
    #   gmrcli bump minor     # 0.1.0 -> 0.2.0
    #   gmrcli bump revision  # 0.1.0 -> 0.1.1
    #   gmrcli bump patch     # alias for revision
    #
    # ## Behavior
    #
    # 1. Validates the working tree is clean (no uncommitted changes)
    # 2. Reads current version from engine.json
    # 3. Increments the requested version component
    # 4. Updates engine.json and engine/language/version.json
    # 5. Commits the changes
    # 6. Creates an annotated git tag X.Y.Z (strict SemVer, no prefix)
    #
    class Bump
      VALID_PARTS = %w[major minor revision patch].freeze

      attr_reader :options

      def initialize(options = {})
        @options = {
          verbose: false
        }.merge(options)
      end

      def run(part)
        @start_time = Time.now

        # Normalize 'patch' to 'revision' for consistency
        part = "revision" if part == "patch"

        validate_part!(part)
        validate_git_state!

        current = read_current_version
        new_version = increment_version(current, part)

        JsonEmitter.emit("bump.start", {
          current_version: current,
          new_version: new_version,
          part: part
        })

        UI.banner
        UI.step "Bumping version: #{current} -> #{new_version}"

        update_engine_json(new_version)
        update_version_json(new_version)
        create_git_commit_and_tag(new_version)

        emit_result(current, new_version, part)
      end

      private

      def verbose?
        options[:verbose]
      end

      def validate_part!(part)
        return if VALID_PARTS.include?(part)

        raise Error.new(
          "Invalid version part: #{part}",
          code: "VERSION.INVALID_PART",
          suggestions: ["Valid parts: major, minor, revision (or patch)"]
        )
      end

      def validate_git_state!
        # Check if we're in a git repository
        unless Dir.exist?(File.join(Platform.gmr_root, ".git"))
          raise Error.new(
            "Not in a git repository",
            code: "VERSION.NOT_GIT_REPO",
            suggestions: ["Run from within the GMR repository"]
          )
        end

        # Check if git is available
        unless Platform.command_exists?("git")
          raise Error.new(
            "Git is not available",
            code: "VERSION.GIT_NOT_FOUND",
            suggestions: ["Install git and ensure it's in your PATH"]
          )
        end

        # Check for clean working tree
        status = Shell.capture("git status --porcelain", chdir: Platform.gmr_root)
        return if status.nil? || status.strip.empty?

        raise Error.new(
          "Working tree has uncommitted changes",
          code: "VERSION.DIRTY_TREE",
          details: "Please commit or stash changes before bumping version",
          suggestions: [
            "git status  # to see changes",
            "git stash   # to temporarily save changes",
            "git add . && git commit -m 'message'  # to commit changes"
          ]
        )
      end

      def engine_json_path
        File.join(Platform.gmr_root, "engine.json")
      end

      def version_json_path
        File.join(Platform.gmr_root, "engine", "language", "version.json")
      end

      def read_current_version
        unless File.exist?(engine_json_path)
          raise Error.new(
            "engine.json not found",
            code: "VERSION.NO_ENGINE_JSON",
            suggestions: ["Ensure you're in the GMR repository root"]
          )
        end

        data = JSON.parse(File.read(engine_json_path))
        version = data.dig("engine", "version")

        unless version && version.match?(/^\d+\.\d+\.\d+$/)
          raise Error.new(
            "Invalid version format in engine.json: #{version}",
            code: "VERSION.INVALID_FORMAT",
            suggestions: ["Version must be in format X.Y.Z (e.g., 0.1.0)"]
          )
        end

        version
      end

      def increment_version(version, part)
        parts = version.split(".").map(&:to_i)

        case part
        when "major"
          [parts[0] + 1, 0, 0].join(".")
        when "minor"
          [parts[0], parts[1] + 1, 0].join(".")
        when "revision"
          [parts[0], parts[1], parts[2] + 1].join(".")
        end
      end

      def update_engine_json(new_version)
        data = JSON.parse(File.read(engine_json_path))
        data["engine"]["version"] = new_version
        File.write(engine_json_path, JSON.pretty_generate(data) + "\n")
        UI.info "Updated engine.json"
        JsonEmitter.emit("bump.file_updated", { file: "engine.json" })
      end

      def update_version_json(new_version)
        unless File.exist?(version_json_path)
          raise Error.new(
            "engine/language/version.json not found",
            code: "VERSION.NO_VERSION_JSON",
            details: "Both engine.json and engine/language/version.json must exist and stay in sync",
            suggestions: ["Ensure engine/language/version.json exists in the repository"]
          )
        end

        data = JSON.parse(File.read(version_json_path))
        data["engine"]["version"] = new_version
        File.write(version_json_path, JSON.pretty_generate(data) + "\n")
        UI.info "Updated engine/language/version.json"
        JsonEmitter.emit("bump.file_updated", { file: "engine/language/version.json" })
      end

      def create_git_commit_and_tag(new_version)
        tag_name = new_version

        # Check if tag already exists
        existing_tags = Shell.capture("git tag -l #{tag_name}", chdir: Platform.gmr_root)
        if existing_tags && existing_tags.strip == tag_name
          raise Error.new(
            "Git tag #{tag_name} already exists",
            code: "VERSION.TAG_EXISTS",
            suggestions: [
              "git tag -d #{tag_name}  # to delete the existing tag",
              "Choose a different version"
            ]
          )
        end

        # Stage version files
        UI.info "Staging version files..."
        Shell.run!("git add engine.json engine/language/version.json", chdir: Platform.gmr_root, verbose: verbose?)

        # Commit
        UI.info "Creating commit..."
        commit_message = "Bump version to #{new_version}"
        Shell.run!(
          "git commit -m \"#{commit_message}\"",
          chdir: Platform.gmr_root,
          verbose: verbose?
        )
        JsonEmitter.emit("bump.committed", { message: commit_message })

        # Create annotated tag
        UI.info "Creating git tag #{tag_name}..."
        tag_message = "Release #{new_version}"
        Shell.run!(
          "git tag -a #{tag_name} -m \"#{tag_message}\"",
          chdir: Platform.gmr_root,
          verbose: verbose?
        )
        JsonEmitter.emit("bump.tagged", { tag: tag_name, message: tag_message })

        UI.success "Created git tag: #{tag_name}"
        UI.info "To push: git push && git push --tags"
      end

      def emit_result(old_version, new_version, part)
        files_modified = ["engine.json", "engine/language/version.json"]

        result = {
          previous_version: old_version,
          new_version: new_version,
          part_incremented: part,
          tag: new_version,
          files_modified: files_modified,
          duration_seconds: (Time.now - @start_time).round(2)
        }

        JsonEmitter.emit_success_envelope(command: "bump", result: result)
        UI.success "Version bumped to #{new_version}"
      end
    end
  end
end
