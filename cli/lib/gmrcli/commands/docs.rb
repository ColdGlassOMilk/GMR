# frozen_string_literal: true

require "fileutils"

module Gmrcli
  module Commands
    # Docs command - generates all documentation (API + CLI)
    class Docs
      attr_reader :options

      def initialize(options = {})
        @options = {
          verbose: false
        }.merge(options)
      end

      def run
        @start_time = Time.now
        JsonEmitter.command_start("docs")
        UI.banner

        validate_environment!

        UI.step "Generating documentation..."

        generate_api_docs
        generate_cli_docs

        elapsed = Time.now - @start_time
        UI.success "Documentation generated in #{elapsed.round(1)}s"
        UI.blank

        # Show output locations
        UI.info "Output locations:"
        UI.info "  JSON:     #{json_dir}"
        UI.info "  Markdown: #{markdown_dir}"
        UI.info "  HTML:     #{html_dir}"

        JsonEmitter.emit_success_envelope(
          command: "docs",
          result: {
            elapsed_seconds: elapsed.round(2),
            outputs: {
              json: json_dir,
              markdown: markdown_dir,
              html: html_dir
            }
          }
        )
      end

      private

      def verbose?
        options[:verbose]
      end

      def validate_environment!
        # Check that we're in a GMR engine directory
        unless Platform.gmr_engine?(Platform.gmr_root)
          raise Error.new(
            "Not in a GMR engine directory",
            code: "PROJECT.NOT_FOUND",
            suggestions: ["Run from the GMR engine root directory"]
          )
        end

        # Check that the generator scripts exist
        unless File.exist?(api_generator_script)
          raise Error.new(
            "API generator script not found: #{api_generator_script}",
            code: "PROJECT.MISSING_FILE"
          )
        end

        unless File.exist?(cli_generator_script)
          raise Error.new(
            "CLI generator script not found: #{cli_generator_script}",
            code: "PROJECT.MISSING_FILE"
          )
        end
      end

      def generate_api_docs
        UI.spinner("Generating API documentation") do
          # Build source file arguments
          source_files = Dir.glob(File.join(Platform.gmr_root, "src", "bindings", "*.cpp"))
          source_args = source_files.map { |f| "\"#{f}\"" }.join(" ")

          # Generate JSON, markdown, and HTML
          cmd = [
            "ruby",
            "\"#{api_generator_script}\"",
            source_args,
            "-o \"#{json_dir}\"",
            "-m \"#{markdown_dir}\"",
            "--html \"#{html_dir}\""
          ].join(" ")

          Shell.run!(cmd, verbose: verbose?, error_message: "API documentation generation failed")
        end
        UI.success "API docs generated"
      end

      def generate_cli_docs
        UI.spinner("Generating CLI documentation") do
          cmd = [
            "ruby",
            "\"#{cli_generator_script}\"",
            "-o \"#{cli_markdown_dir}\"",
            "--html \"#{cli_html_dir}\""
          ].join(" ")

          Shell.run!(cmd, verbose: verbose?, error_message: "CLI documentation generation failed")
        end
        UI.success "CLI docs generated"
      end

      # Paths
      def api_generator_script
        File.join(Platform.gmr_root, "tools", "generate_api_docs.rb")
      end

      def cli_generator_script
        File.join(Platform.gmr_root, "tools", "generate_cli_docs.rb")
      end

      def docs_dir
        File.join(Platform.gmr_root, "docs")
      end

      def json_dir
        File.join(Platform.gmr_root, "engine", "language")
      end

      def markdown_dir
        File.join(docs_dir, "api")
      end

      def html_dir
        File.join(docs_dir, "html", "api")
      end

      def cli_markdown_dir
        File.join(docs_dir, "cli")
      end

      def cli_html_dir
        File.join(docs_dir, "html", "cli")
      end
    end
  end
end
