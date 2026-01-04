#!/usr/bin/env ruby
# frozen_string_literal: true

# GMR CLI Documentation Generator
# Parses gmrcli Ruby files and generates markdown and HTML documentation
#
# Usage: ruby generate_cli_docs.rb [options]
#   -o, --output DIR     Output directory for markdown (default: docs/cli)
#   --html DIR           Generate HTML docs to directory (dark theme)

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

  # Generates HTML documentation with dark theme
  class HTMLGenerator
    # Shared CSS (same theme as API docs)
    DARK_THEME_CSS = <<~CSS
      :root {
        --bg-primary: #1a1b26;
        --bg-secondary: #24283b;
        --bg-tertiary: #414868;
        --text-primary: #c0caf5;
        --text-secondary: #a9b1d6;
        --text-muted: #565f89;
        --accent-blue: #7aa2f7;
        --accent-cyan: #7dcfff;
        --accent-green: #9ece6a;
        --accent-magenta: #bb9af7;
        --accent-orange: #ff9e64;
        --accent-red: #f7768e;
        --accent-yellow: #e0af68;
        --border-color: #3b4261;
        --code-bg: #1f2335;
        --link-color: #7aa2f7;
        --link-hover: #7dcfff;
      }

      * { box-sizing: border-box; margin: 0; padding: 0; }

      body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
        background: var(--bg-primary);
        color: var(--text-primary);
        line-height: 1.6;
        min-height: 100vh;
      }

      .container { max-width: 1000px; margin: 0 auto; padding: 2rem; }

      .nav {
        background: var(--bg-secondary);
        border-bottom: 1px solid var(--border-color);
        padding: 1rem 2rem;
        position: sticky;
        top: 0;
        z-index: 100;
      }

      .nav-content {
        max-width: 1000px;
        margin: 0 auto;
        display: flex;
        align-items: center;
        gap: 2rem;
      }

      .nav-brand {
        font-size: 1.25rem;
        font-weight: 700;
        color: var(--accent-magenta);
        text-decoration: none;
      }

      .nav-links { display: flex; gap: 1.5rem; list-style: none; }
      .nav-links a { color: var(--text-secondary); text-decoration: none; font-size: 0.9rem; }
      .nav-links a:hover, .nav-links a.active { color: var(--accent-blue); }

      h1 {
        font-size: 2.5rem;
        color: var(--accent-magenta);
        margin-bottom: 0.5rem;
        border-bottom: 2px solid var(--border-color);
        padding-bottom: 0.5rem;
      }

      h2 {
        font-size: 1.5rem;
        color: var(--accent-cyan);
        margin: 2rem 0 1rem;
        padding-bottom: 0.25rem;
        border-bottom: 1px solid var(--border-color);
      }

      h3 { font-size: 1.25rem; color: var(--accent-green); margin: 1.5rem 0 0.75rem; }
      p { margin-bottom: 1rem; }
      a { color: var(--link-color); text-decoration: none; }
      a:hover { color: var(--link-hover); }

      code {
        font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
        background: var(--code-bg);
        padding: 0.15rem 0.4rem;
        border-radius: 4px;
        font-size: 0.9em;
        color: var(--accent-orange);
      }

      pre {
        background: var(--code-bg);
        border: 1px solid var(--border-color);
        border-radius: 8px;
        padding: 1rem;
        overflow-x: auto;
        margin: 1rem 0;
      }

      pre code { background: none; padding: 0; color: var(--text-primary); }

      table { width: 100%; border-collapse: collapse; margin: 1rem 0; font-size: 0.9rem; }
      th, td { padding: 0.75rem 1rem; text-align: left; border-bottom: 1px solid var(--border-color); }
      th { background: var(--bg-secondary); color: var(--accent-cyan); font-weight: 600; }
      tr:hover { background: var(--bg-secondary); }

      .card {
        background: var(--bg-secondary);
        border: 1px solid var(--border-color);
        border-radius: 8px;
        padding: 1.5rem;
        margin: 1rem 0;
      }

      .card-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
        gap: 1rem;
        margin: 1.5rem 0;
      }

      .card h3 { margin-top: 0; }

      .command-block {
        background: var(--bg-secondary);
        border: 1px solid var(--border-color);
        border-left: 4px solid var(--accent-green);
        border-radius: 0 8px 8px 0;
        padding: 1.25rem;
        margin: 1.5rem 0;
      }

      .option-block {
        background: var(--bg-secondary);
        border: 1px solid var(--border-color);
        border-left: 4px solid var(--accent-blue);
        border-radius: 0 8px 8px 0;
        padding: 1rem;
        margin: 1rem 0;
      }

      .label {
        display: inline-block;
        padding: 0.2rem 0.5rem;
        border-radius: 4px;
        font-size: 0.75rem;
        font-weight: 600;
        text-transform: uppercase;
      }

      .label-default { background: var(--accent-green); color: var(--bg-primary); }
      .label-alias { background: var(--accent-blue); color: var(--bg-primary); }
      .label-type { background: var(--accent-magenta); color: var(--bg-primary); }

      .stage-list { list-style: none; counter-reset: stage; }
      .stage-list li {
        counter-increment: stage;
        padding: 0.5rem 0 0.5rem 2.5rem;
        position: relative;
        border-bottom: 1px solid var(--border-color);
      }
      .stage-list li::before {
        content: counter(stage);
        position: absolute;
        left: 0;
        width: 1.75rem;
        height: 1.75rem;
        background: var(--accent-cyan);
        color: var(--bg-primary);
        border-radius: 50%;
        text-align: center;
        line-height: 1.75rem;
        font-weight: 600;
        font-size: 0.85rem;
      }

      .footer {
        margin-top: 3rem;
        padding-top: 1.5rem;
        border-top: 1px solid var(--border-color);
        color: var(--text-muted);
        font-size: 0.85rem;
        text-align: center;
      }

      /* Syntax Highlighting */
      .hl-keyword { color: #bb9af7; }
      .hl-string { color: #9ece6a; }
      .hl-comment { color: #565f89; font-style: italic; }
      .hl-command { color: #7dcfff; }
      .hl-flag { color: #ff9e64; }
    CSS

    def initialize(options)
      @options = options
    end

    def generate(parsed_data, output_dir)
      FileUtils.mkdir_p(output_dir)

      # Write CSS
      File.write(File.join(output_dir, 'styles.css'), DARK_THEME_CSS)
      puts "  Generated: #{File.join(output_dir, 'styles.css')}"

      # Generate index
      generate_index(output_dir, parsed_data)

      # Generate command pages
      parsed_data[:commands].each do |name, cmd|
        next if name == 'version' || name == 'info'
        generate_command_page(output_dir, cmd, parsed_data)
      end

      # Generate error codes page
      if parsed_data[:error_codes]&.any?
        generate_error_codes_page(output_dir, parsed_data[:error_codes])
      end
    end

    private

    def html_escape(text)
      return '' unless text
      text.to_s.gsub('&', '&amp;').gsub('<', '&lt;').gsub('>', '&gt;').gsub('"', '&quot;')
    end

    def highlight_bash(code)
      escaped = html_escape(code)
      # Comments
      escaped = escaped.gsub(/(#.*)$/) { "<span class=\"hl-comment\">#{$1}</span>" }
      # Strings
      escaped = escaped.gsub(/("(?:[^"\\]|\\.)*")/) { "<span class=\"hl-string\">#{$1}</span>" }
      # Flags
      escaped = escaped.gsub(/(--?\w+[-\w]*)/) { "<span class=\"hl-flag\">#{$1}</span>" }
      # gmrcli command
      escaped = escaped.gsub(/\b(gmrcli)\b/) { "<span class=\"hl-command\">#{$1}</span>" }
      escaped
    end

    def generate_index(output_dir, data)
      path = File.join(output_dir, 'index.html')
      File.open(path, 'w') do |f|
        f.puts html_header('gmrcli Reference', data[:commands].keys)

        f.puts '<div class="container">'
        f.puts '<h1>gmrcli Reference</h1>'
        f.puts '<p>Command-line interface for GMR game development.</p>'

        # Commands grid
        f.puts '<h2>Commands</h2>'
        f.puts '<div class="card-grid">'
        data[:commands].each do |name, cmd|
          default = cmd.default_task ? ' <span class="label label-default">default</span>' : ''
          aliases = cmd.aliases.map { |a| "<span class=\"label label-alias\">#{html_escape(a)}</span>" }.join(' ')
          f.puts '<div class="card">'
          f.puts "<h3><a href=\"#{name}.html\">#{html_escape(name)}</a>#{default} #{aliases}</h3>"
          f.puts "<p>#{html_escape(cmd.description)}</p>"
          f.puts '</div>'
        end
        f.puts '</div>'

        # Global options
        if data[:class_options].any?
          f.puts '<h2>Global Options</h2>'
          f.puts '<p>These options are available for all commands:</p>'
          f.puts '<table>'
          f.puts '<thead><tr><th>Option</th><th>Type</th><th>Default</th><th>Description</th></tr></thead>'
          f.puts '<tbody>'
          data[:class_options].each do |opt|
            alias_str = opt.aliases.first ? " (<code>#{html_escape(opt.aliases.first)}</code>)" : ''
            default = opt.default.nil? ? '-' : "<code>#{html_escape(opt.default.to_s)}</code>"
            f.puts "<tr><td><code>--#{html_escape(opt.name)}</code>#{alias_str}</td>"
            f.puts "<td><span class=\"label label-type\">#{opt.type}</span></td>"
            f.puts "<td>#{default}</td>"
            f.puts "<td>#{html_escape(opt.description)}</td></tr>"
          end
          f.puts '</tbody></table>'
        end

        # Links
        f.puts '<h2>See Also</h2>'
        f.puts '<div class="card-grid">'
        f.puts '<div class="card"><h3><a href="error-codes.html">Error Codes</a></h3><p>Machine-readable error codes returned by gmrcli.</p></div>'
        f.puts '<div class="card"><h3><a href="../html/index.html">API Reference</a></h3><p>GMR Ruby API documentation.</p></div>'
        f.puts '</div>'

        f.puts '</div>'
        f.puts html_footer
      end
      puts "  Generated: #{path}"
    end

    def generate_command_page(output_dir, cmd, data)
      path = File.join(output_dir, "#{cmd.name}.html")
      File.open(path, 'w') do |f|
        f.puts html_header("gmrcli #{cmd.name}", data[:commands].keys, cmd.name)

        f.puts '<div class="container">'
        f.puts "<h1>gmrcli #{html_escape(cmd.name)}</h1>"
        f.puts "<p>#{html_escape(cmd.description)}</p>"

        # Usage
        f.puts '<h2>Usage</h2>'
        f.puts '<pre><code>'
        f.puts highlight_bash("gmrcli #{cmd.name} [options]")
        f.puts '</code></pre>'

        # Description
        if cmd.long_desc
          f.puts '<h2>Description</h2>'
          f.puts "<p>#{html_escape(cmd.long_desc).gsub("\n", '<br>')}</p>"
        end

        # Options
        if cmd.options.any?
          f.puts '<h2>Options</h2>'
          f.puts '<table>'
          f.puts '<thead><tr><th>Option</th><th>Alias</th><th>Type</th><th>Default</th><th>Description</th></tr></thead>'
          f.puts '<tbody>'
          cmd.options.each do |opt|
            alias_str = opt.aliases.first ? "<code>#{html_escape(opt.aliases.first)}</code>" : '-'
            default = opt.default.nil? ? '-' : "<code>#{html_escape(opt.default.to_s)}</code>"
            f.puts "<tr><td><code>--#{html_escape(opt.name)}</code></td>"
            f.puts "<td>#{alias_str}</td>"
            f.puts "<td><span class=\"label label-type\">#{opt.type}</span></td>"
            f.puts "<td>#{default}</td>"
            f.puts "<td>#{html_escape(opt.description)}</td></tr>"
          end
          f.puts '</tbody></table>'
        end

        # Stages
        if data[:stages] && data[:stages][cmd.name]&.any?
          f.puts '<h2>Stages</h2>'
          f.puts '<ul class="stage-list">'
          data[:stages][cmd.name].each do |stage|
            f.puts "<li>#{html_escape(stage[:name])}</li>"
          end
          f.puts '</ul>'
        end

        # Examples
        f.puts '<h2>Examples</h2>'
        f.puts '<pre><code>'
        example = "# Basic usage\ngmrcli #{cmd.name}"
        if cmd.options.any? && cmd.options.first.type == :boolean
          example += "\n\n# With #{cmd.options.first.name} flag\ngmrcli #{cmd.name} --#{cmd.options.first.name}"
        end
        f.puts highlight_bash(example)
        f.puts '</code></pre>'

        f.puts '</div>'
        f.puts html_footer
      end
      puts "  Generated: #{path}"
    end

    def generate_error_codes_page(output_dir, codes)
      path = File.join(output_dir, 'error-codes.html')
      File.open(path, 'w') do |f|
        f.puts html_header('Error Codes', [], 'error-codes')

        f.puts '<div class="container">'
        f.puts '<h1>Error Codes</h1>'
        f.puts '<p>Machine-readable error codes returned by gmrcli.</p>'

        # Group by category
        categories = codes.group_by { |c| c[:code].split('.').first }

        categories.each do |category, cat_codes|
          f.puts "<h2>#{html_escape(category)}</h2>"
          f.puts '<table>'
          f.puts '<thead><tr><th>Code</th><th>Exit</th><th>Description</th></tr></thead>'
          f.puts '<tbody>'
          cat_codes.sort_by { |c| c[:code] }.each do |code|
            f.puts "<tr><td><code>#{html_escape(code[:code])}</code></td>"
            f.puts "<td>#{code[:exit_code]}</td>"
            f.puts "<td>#{html_escape(code[:description])}</td></tr>"
          end
          f.puts '</tbody></table>'
        end

        f.puts '</div>'
        f.puts html_footer
      end
      puts "  Generated: #{path}"
    end

    def html_header(title, commands, current = nil)
      nav_links = commands.map do |name|
        active = name == current ? ' class="active"' : ''
        "<a href=\"#{name}.html\"#{active}>#{html_escape(name)}</a>"
      end.join("\n          ")

      <<~HTML
        <!DOCTYPE html>
        <html lang="en">
        <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>#{html_escape(title)} - gmrcli</title>
          <link rel="stylesheet" href="styles.css">
        </head>
        <body>
          <nav class="nav">
            <div class="nav-content">
              <a href="index.html" class="nav-brand">gmrcli</a>
              <ul class="nav-links">
                #{nav_links}
                <a href="error-codes.html"#{current == 'error-codes' ? ' class="active"' : ''}>Errors</a>
              </ul>
            </div>
          </nav>
      HTML
    end

    def html_footer
      <<~HTML
          <footer class="footer">
            <p>Generated by <code>generate_cli_docs.rb</code></p>
            <p>gmrcli &bull; CLI Reference</p>
          </footer>
        </body>
        </html>
      HTML
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
    puts "\nGenerating markdown documentation..."
    md_generator = MarkdownGenerator.new(options)
    md_generator.generate(parsed_data, options[:output_dir])

    # Generate HTML if requested
    if options[:html_dir]
      puts "\nGenerating HTML documentation..."
      html_generator = HTMLGenerator.new(options)
      html_generator.generate(parsed_data, options[:html_dir])
    end

    puts "\nDone!"
  end
end

# Parse command line arguments
options = {
  output_dir: 'docs/cli',
  html_dir: nil,
  cli_root: nil
}

OptionParser.new do |opts|
  opts.banner = "Usage: generate_cli_docs.rb [options]"

  opts.on("-o", "--output DIR", "Output directory for markdown") do |d|
    options[:output_dir] = d
  end

  opts.on("--html DIR", "Generate HTML docs to directory") do |d|
    options[:html_dir] = d
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
