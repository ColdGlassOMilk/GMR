#!/usr/bin/env ruby
# frozen_string_literal: true

# GMR CLI Documentation Generator
# Parses gmrcli Ruby files and generates markdown documentation
#
# Usage: ruby generate_cli_docs.rb [options]
#   -o, --output DIR     Output directory (default: docs/cli)

require 'optparse'
require 'fileutils'

module GMRCliDocs
  # Parsed command definition from Thor
  class CommandDef
    attr_accessor :name, :description, :long_desc, :options, :aliases, :default_task

    def initialize(name)
      @name = name
      @description = nil
      @long_desc = nil
      @options = []
      @aliases = []
      @default_task = false
    end
  end

  # Parsed option definition from Thor
  class OptionDef
    attr_accessor :name, :type, :default, :aliases, :description, :class_option

    def initialize(name)
      @name = name
      @type = :boolean
      @default = nil
      @aliases = []
      @description = nil
      @class_option = false
    end
  end

  # Parses Thor CLI definitions from bin/gmrcli
  class ThorParser
    def parse_file(filepath)
      content = File.read(filepath, encoding: 'utf-8')
      commands = {}
      class_options = []
      current_command = nil
      current_long_desc = nil

      lines = content.lines
      i = 0

      while i < lines.length
        line = lines[i]

        # Parse class_option
        if line =~ /class_option\s+:(\w+)/
          opt = parse_option_line(line, lines, i)
          opt.class_option = true
          class_options << opt

        # Parse desc "command", "description"
        elsif line =~ /desc\s+"([^"]+)"\s*,\s*"([^"]+)"/
          cmd_name = $1.split.first
          cmd_desc = $2
          current_command = CommandDef.new(cmd_name)
          current_command.description = cmd_desc
          commands[cmd_name] = current_command

        # Parse long_desc heredoc
        elsif line =~ /long_desc\s+<<[~-](\w+)/
          delimiter = $1
          long_desc_lines = []
          i += 1
          while i < lines.length && lines[i].strip != delimiter
            long_desc_lines << lines[i]
            i += 1
          end
          if current_command
            current_command.long_desc = long_desc_lines.join.strip
          end

        # Parse option for current command
        elsif line =~ /^\s+option\s+:(\w+)/ && current_command
          opt = parse_option_line(line, lines, i)
          current_command.options << opt

        # Parse method definition (command handler)
        elsif line =~ /def\s+(\w+)\s*\(?/
          method_name = $1
          current_command = nil unless method_name == 'initialize'

        # Parse map alias
        elsif line =~ /map\s+"([^"]+)"\s*=>\s*:(\w+)/
          alias_name = $1
          target = $2
          if commands[target]
            commands[target].aliases << alias_name
          end

        # Parse default_task
        elsif line =~ /default_task\s+:(\w+)/
          task_name = $1
          if commands[task_name]
            commands[task_name].default_task = true
          end
        end

        i += 1
      end

      { commands: commands, class_options: class_options }
    end

    private

    def parse_option_line(line, lines, index)
      # Extract option name
      line =~ /:(\w+)/
      opt = OptionDef.new($1)

      # Combine continuation lines
      full_line = line.strip
      while full_line.end_with?(',') && index + 1 < lines.length
        index += 1
        full_line += ' ' + lines[index].strip
      end

      # Parse type
      if full_line =~ /type:\s*:(\w+)/
        opt.type = $1.to_sym
      end

      # Parse default
      if full_line =~ /default:\s*(\S+?)(?:,|$)/
        default_val = $1
        opt.default = case default_val
                      when 'true' then true
                      when 'false' then false
                      when 'nil' then nil
                      when /^\d+$/ then default_val.to_i
                      when /^"([^"]*)"$/ then $1
                      else default_val
                      end
      end

      # Parse aliases
      if full_line =~ /aliases:\s*\[([^\]]+)\]/
        aliases = $1.scan(/"([^"]+)"/).flatten
        opt.aliases = aliases
      elsif full_line =~ /aliases:\s*"([^"]+)"/
        opt.aliases = [$1]
      end

      # Parse description
      if full_line =~ /desc:\s*"([^"]+)"/
        opt.description = $1
      end

      opt
    end
  end

  # Parses stage definitions from command files
  class StageParser
    def parse_file(filepath)
      content = File.read(filepath, encoding: 'utf-8')
      stages = []

      content.scan(/run_stage\s*\(\s*:(\w+)\s*,\s*"([^"]+)"\s*\)/) do |id, name|
        stages << { id: id, name: name }
      end

      stages
    end
  end

  # Parses error codes from error_codes.rb
  class ErrorCodeParser
    def parse_file(filepath)
      content = File.read(filepath, encoding: 'utf-8')
      codes = []

      content.scan(/"([A-Z_]+(?:\.[A-Z_]+)+)"\s*=>\s*\{\s*exit_code:\s*(\d+)\s*,\s*description:\s*"([^"]+)"/) do |code, exit_code, description|
        codes << { code: code, exit_code: exit_code.to_i, description: description }
      end

      codes
    end
  end

  # Generates markdown documentation
  class MarkdownGenerator
    def initialize(options)
      @options = options
    end

    def generate(parsed_data, output_dir)
      FileUtils.mkdir_p(output_dir)

      # Generate README index
      generate_readme(output_dir, parsed_data)

      # Generate command docs
      parsed_data[:commands].each do |name, cmd|
        next if name == 'version' || name == 'info' # Skip simple commands
        generate_command_doc(output_dir, cmd, parsed_data)
      end

      # Generate error codes doc
      if parsed_data[:error_codes]&.any?
        generate_error_codes(output_dir, parsed_data[:error_codes])
      end
    end

    private

    def generate_readme(output_dir, data)
      path = File.join(output_dir, 'README.md')
      File.open(path, 'w') do |f|
        f.puts "# gmrcli Reference"
        f.puts
        f.puts "Command-line interface for GMR game development."
        f.puts
        f.puts "## Commands"
        f.puts
        f.puts "| Command | Description |"
        f.puts "|---------|-------------|"

        data[:commands].each do |name, cmd|
          default_marker = cmd.default_task ? " (default)" : ""
          aliases = cmd.aliases.any? ? " (alias: #{cmd.aliases.join(', ')})" : ""
          f.puts "| [#{name}](#{name}.md)#{default_marker}#{aliases} | #{cmd.description} |"
        end

        f.puts
        f.puts "## Global Options"
        f.puts
        f.puts "These options are available for all commands:"
        f.puts
        f.puts "| Option | Type | Default | Description |"
        f.puts "|--------|------|---------|-------------|"

        data[:class_options].each do |opt|
          aliases = opt.aliases.any? ? " (`#{opt.aliases.first}`)" : ""
          default = opt.default.nil? ? "-" : "`#{opt.default}`"
          f.puts "| `--#{opt.name}`#{aliases} | #{opt.type} | #{default} | #{opt.description || ''} |"
        end

        f.puts
        f.puts "## See Also"
        f.puts
        f.puts "- [Error Codes](error-codes.md)"
        f.puts "- [API Reference](../api/README.md)"
        f.puts
        f.puts "---"
        f.puts
        f.puts "*Generated by `generate_cli_docs.rb`.*"
      end

      puts "  Generated: #{path}"
    end

    def generate_command_doc(output_dir, cmd, data)
      path = File.join(output_dir, "#{cmd.name}.md")
      File.open(path, 'w') do |f|
        f.puts "# gmrcli #{cmd.name}"
        f.puts
        f.puts cmd.description
        f.puts
        f.puts "## Usage"
        f.puts
        f.puts "```bash"
        f.puts "gmrcli #{cmd.name} [options]"
        f.puts "```"
        f.puts

        if cmd.long_desc
          f.puts "## Description"
          f.puts
          f.puts cmd.long_desc
          f.puts
        end

        # Command-specific options
        all_options = cmd.options + data[:class_options]
        if all_options.any?
          f.puts "## Options"
          f.puts
          f.puts "| Option | Alias | Type | Default | Description |"
          f.puts "|--------|-------|------|---------|-------------|"

          cmd.options.each do |opt|
            alias_str = opt.aliases.first || ""
            default = opt.default.nil? ? "-" : "`#{opt.default}`"
            f.puts "| `--#{opt.name}` | #{alias_str} | #{opt.type} | #{default} | #{opt.description || ''} |"
          end

          f.puts
        end

        # Stages if available
        if data[:stages] && data[:stages][cmd.name]
          f.puts "## Stages"
          f.puts
          data[:stages][cmd.name].each_with_index do |stage, i|
            f.puts "#{i + 1}. #{stage[:name]}"
          end
          f.puts
        end

        # Examples
        f.puts "## Examples"
        f.puts
        f.puts "```bash"
        f.puts "# Basic usage"
        f.puts "gmrcli #{cmd.name}"

        if cmd.options.any?
          opt = cmd.options.first
          if opt.type == :boolean
            f.puts
            f.puts "# With #{opt.name} flag"
            f.puts "gmrcli #{cmd.name} --#{opt.name}"
          end
        end
        f.puts "```"
        f.puts
        f.puts "---"
        f.puts
        f.puts "*See also: [CLI Reference](README.md)*"
      end

      puts "  Generated: #{path}"
    end

    def generate_error_codes(output_dir, codes)
      path = File.join(output_dir, 'error-codes.md')
      File.open(path, 'w') do |f|
        f.puts "# Error Codes"
        f.puts
        f.puts "Machine-readable error codes returned by gmrcli."
        f.puts

        # Group by category
        categories = codes.group_by { |c| c[:code].split('.').first }

        categories.each do |category, cat_codes|
          f.puts "## #{category}"
          f.puts
          f.puts "| Code | Exit | Description |"
          f.puts "|------|------|-------------|"

          cat_codes.sort_by { |c| c[:code] }.each do |code|
            f.puts "| `#{code[:code]}` | #{code[:exit_code]} | #{code[:description]} |"
          end
          f.puts
        end

        f.puts "---"
        f.puts
        f.puts "*See also: [CLI Reference](README.md)*"
      end

      puts "  Generated: #{path}"
    end
  end

  # Main entry point
  def self.run(options)
    puts "GMR CLI Documentation Generator"
    puts "================================"

    cli_root = options[:cli_root]
    bin_file = File.join(cli_root, 'bin', 'gmrcli')
    commands_dir = File.join(cli_root, 'lib', 'gmrcli', 'commands')
    error_codes_file = File.join(cli_root, 'lib', 'gmrcli', 'error_codes.rb')

    unless File.exist?(bin_file)
      warn "Error: bin/gmrcli not found at #{bin_file}"
      exit 1
    end

    thor_parser = ThorParser.new
    stage_parser = StageParser.new
    error_parser = ErrorCodeParser.new

    # Parse Thor definitions
    puts "\nParsing CLI definitions..."
    puts "  Parsing: bin/gmrcli"
    thor_data = thor_parser.parse_file(bin_file)

    puts "Found #{thor_data[:commands].size} commands"
    puts "Found #{thor_data[:class_options].size} global options"

    # Parse stages from command files
    stages = {}
    if Dir.exist?(commands_dir)
      Dir.glob(File.join(commands_dir, '*.rb')).each do |cmd_file|
        cmd_name = File.basename(cmd_file, '.rb')
        puts "  Parsing: commands/#{cmd_name}.rb"
        cmd_stages = stage_parser.parse_file(cmd_file)
        stages[cmd_name] = cmd_stages if cmd_stages.any?
      end
    end

    # Parse error codes
    error_codes = []
    if File.exist?(error_codes_file)
      puts "  Parsing: error_codes.rb"
      error_codes = error_parser.parse_file(error_codes_file)
      puts "Found #{error_codes.size} error codes"
    end

    # Combine all parsed data
    parsed_data = {
      commands: thor_data[:commands],
      class_options: thor_data[:class_options],
      stages: stages,
      error_codes: error_codes
    }

    # Generate markdown
    puts "\nGenerating documentation..."
    generator = MarkdownGenerator.new(options)
    generator.generate(parsed_data, options[:output_dir])

    puts "\nDone!"
  end
end

# Parse command line arguments
options = {
  output_dir: 'docs/cli',
  cli_root: nil
}

OptionParser.new do |opts|
  opts.banner = "Usage: generate_cli_docs.rb [options]"

  opts.on("-o", "--output DIR", "Output directory") do |d|
    options[:output_dir] = d
  end

  opts.on("-c", "--cli-root DIR", "CLI source root directory") do |d|
    options[:cli_root] = d
  end

  opts.on("-h", "--help", "Show this help") do
    puts opts
    exit
  end
end.parse!

# Find CLI root if not specified
if options[:cli_root].nil?
  script_dir = File.dirname(__FILE__)
  project_root = File.expand_path('..', script_dir)
  options[:cli_root] = File.join(project_root, 'cli')
end

# Update output dir to be relative to project root if not absolute
unless options[:output_dir].start_with?('/') || options[:output_dir] =~ /^[A-Za-z]:/
  script_dir = File.dirname(__FILE__)
  project_root = File.expand_path('..', script_dir)
  options[:output_dir] = File.join(project_root, options[:output_dir])
end

GMRCliDocs.run(options)
