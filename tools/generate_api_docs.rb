#!/usr/bin/env ruby
# frozen_string_literal: true

# GMR API Documentation Generator
# Parses C++ binding files and generates api.json, syntax.json, and version.json
# Optionally generates markdown and HTML documentation files
#
# Usage: ruby generate_api_docs.rb [options]
#   -s, --source FILE    Add source file to parse (can be repeated)
#   -o, --output DIR     Output directory (default: engine/language)
#   -m, --markdown DIR   Generate markdown docs to directory
#   --html DIR           Generate HTML docs to directory (dark theme)
#   -v, --version VER    Engine version (default: 0.1.0)
#   --raylib VER         Raylib version (default: 5.6-dev)
#   --mruby VER          mRuby version (default: 3.4.0)

require 'json'
require 'optparse'
require 'fileutils'

module GMRDocs
  # Maps mrb_get_args format characters to Ruby types
  ARG_TYPE_MAP = {
    'i' => 'Integer',
    'f' => 'Float',
    'z' => 'String',
    'n' => 'Symbol',
    'o' => 'Object',
    'A' => 'Array',
    'H' => 'Hash',
    'b' => 'Boolean',
    '&' => 'Block',
    '*' => 'Array'
  }.freeze

  # Static type definitions that don't change
  TYPE_DEFINITIONS = {
    'Color' => {
      'kind' => 'alias',
      'type' => 'Array<Integer>',
      'description' => 'RGBA color as [r, g, b] or [r, g, b, a], values 0-255. Alpha defaults to 255 if omitted.',
      'examples' => ['[255, 0, 0]', '[255, 0, 0, 128]']
    },
    'KeyCode' => {
      'kind' => 'union',
      'types' => ['Integer', 'Symbol', 'Array<Integer|Symbol>'],
      'description' => 'Key identifier - integer constant (KEY_*), symbol (:space, :a, :left, etc.), or array of keys to check any match',
      'symbols' => %w[
        space escape enter return tab backspace delete insert
        up down left right home end page_up page_down
        left_shift right_shift left_control right_control left_alt right_alt
        f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12
        a b c d e f g h i j k l m n o p q r s t u v w x y z
        0 1 2 3 4 5 6 7 8 9
      ]
    },
    'MouseButton' => {
      'kind' => 'union',
      'types' => ['Integer', 'Symbol'],
      'description' => 'Mouse button identifier - integer constant (MOUSE_*) or symbol',
      'symbols' => %w[left right middle side extra forward back]
    }
  }.freeze

  # Represents a parsed documentation entry
  class DocEntry
    attr_reader :kind, :name, :line_number
    attr_accessor :description, :signature, :params, :returns, :raises, :example, :aliases, :parent

    def initialize(kind, name, line_number)
      @kind = kind
      @name = name
      @line_number = line_number
      @description = nil
      @signature = nil
      @params = []
      @returns = nil
      @raises = []
      @example = nil
      @aliases = []
      @parent = nil
    end

    def to_h
      h = {}
      h['signature'] = @signature if @signature
      h['description'] = @description if @description
      h['params'] = @params unless @params.empty?
      h['returns'] = @returns if @returns
      h['raises'] = @raises unless @raises.empty?
      h['example'] = @example if @example
      h
    end
  end

  # Represents a parsed binding registration
  class BindingDef
    attr_reader :type, :module_var, :name, :handler, :args_spec, :line_number
    attr_accessor :arg_types

    def initialize(type, module_var, name, handler, args_spec, line_number)
      @type = type
      @module_var = module_var
      @name = name
      @handler = handler
      @args_spec = args_spec
      @line_number = line_number
      @arg_types = []
    end

    # Parse MRB_ARGS_* to get required/optional counts
    def parse_args_spec
      case @args_spec
      when /MRB_ARGS_NONE/
        { required: 0, optional: 0 }
      when /MRB_ARGS_REQ\((\d+)\)/
        { required: $1.to_i, optional: 0 }
      when /MRB_ARGS_OPT\((\d+)\)/
        { required: 0, optional: $1.to_i }
      when /MRB_ARGS_ARG\((\d+),\s*(\d+)\)/
        { required: $1.to_i, optional: $2.to_i }
      when /MRB_ARGS_ANY/
        { required: 0, optional: -1 }
      else
        { required: 0, optional: 0 }
      end
    end
  end

  # Parses /// doc comments from C++ files
  class DocParser
    def parse_file(filepath)
      entries = []
      current_entry = nil
      current_module = nil
      current_class = nil
      last_tag = nil

      File.readlines(filepath, encoding: 'utf-8').each_with_index do |line, index|
        line_num = index + 1

        # Parse doc comment tags
        if line =~ %r{^///\s*@(\w+)\s*(.*)$}
          tag = $1
          value = $2.strip

          case tag
          when 'module'
            current_module = value
            current_class = nil
          when 'class'
            current_class = value
          when 'function', 'classmethod', 'method', 'constant'
            current_entry = DocEntry.new(tag.to_sym, value, line_num)
            current_entry.parent = current_class || current_module
            entries << current_entry
          when 'description'
            current_entry&.description = value
          when 'signature'
            current_entry&.signature = value
          when 'param'
            if current_entry && value =~ /^(\w+)\s+\[([^\]]+)\]\s*(.*)$/
              current_entry.params << {
                'name' => $1,
                'type' => $2,
                'description' => $3.strip
              }
            elsif current_entry && value =~ /^(\w+)\s+\[([^\]]+)\]\s*\(optional(?:,\s*default:\s*([^)]+))?\)\s*(.*)$/
              current_entry.params << {
                'name' => $1,
                'type' => $2,
                'optional' => true,
                'default' => $3,
                'description' => $4.strip
              }.compact
            end
          when 'returns'
            if current_entry && value =~ /^\[([^\]]+)\]\s*(.*)$/
              current_entry.returns = { 'type' => $1 }
              current_entry.returns['description'] = $2.strip unless $2.strip.empty?
            end
          when 'raises'
            if current_entry && value =~ /^\[([^\]]+)\]\s*(.*)$/
              current_entry.raises << "#{$1} #{$2}".strip
            end
          when 'example'
            current_entry&.example = value
          when 'alias'
            current_entry&.aliases << value
          end
          last_tag = tag

        # Continue previous tag (multi-line)
        elsif line =~ %r{^///\s?(.*)$}
          continuation = $1
          if current_entry && last_tag == 'example' && current_entry.example
            current_entry.example += "\n" + continuation
          elsif current_entry && last_tag == 'description' && current_entry.description
            current_entry.description += ' ' + continuation.strip
          end

        # Parse existing signature comments: // GMR::Module.function(args)
        elsif line =~ %r{^//\s*(GMR(?:::\w+)*)[.#](\w+[?=]?)\(([^)]*)\)\s*$}
          module_path = $1
          func_name = $2
          args = $3

          # If we have a pending entry without signature, use this
          if current_entry && current_entry.signature.nil?
            current_entry.signature = "#{func_name}(#{args})"
            current_entry.parent ||= module_path
          end

        # Parse existing signature comments: // module.function(args) style
        elsif line =~ %r{^//\s*(\w+)\.([\w?=]+)\(([^)]*)\)\s*$}
          func_name = $2
          args = $3
          if current_entry && current_entry.signature.nil?
            current_entry.signature = "#{func_name}(#{args})"
          end
        end
      end

      entries
    end
  end

  # Parses mrb_define_* binding registrations
  class BindingParser
    PATTERNS = {
      module_function: /mrb_define_module_function\s*\(\s*mrb\s*,\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*(\w+)\s*,\s*(MRB_ARGS_\w+\([^)]*\))\s*\)/,
      class_method: /mrb_define_class_method\s*\(\s*mrb\s*,\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*(\w+)\s*,\s*(MRB_ARGS_\w+\([^)]*\))\s*\)/,
      instance_method: /mrb_define_method\s*\(\s*mrb\s*,\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*(\w+)\s*,\s*(MRB_ARGS_\w+\([^)]*\))\s*\)/,
      constant: /mrb_define_const\s*\(\s*mrb\s*,\s*(\w+)\s*,\s*"([^"]+)"\s*,\s*mrb_fixnum_value\s*\(\s*(\w+)\s*\)\s*\)/,
      class_def: /(\w+)\s*=\s*mrb_define_class_under\s*\(\s*mrb\s*,\s*(\w+)\s*,\s*"([^"]+)"/,
      module_def: /mrb_define_module_under\s*\(\s*mrb\s*,\s*(\w+)\s*,\s*"([^"]+)"\s*\)/
    }.freeze

    def parse_file(filepath)
      content = File.read(filepath, encoding: 'utf-8')
      bindings = []

      # Extract handler argument types from mrb_get_args calls
      arg_specs = extract_arg_specs(content)

      content.each_line.with_index do |line, index|
        line_num = index + 1

        PATTERNS.each do |type, pattern|
          if line =~ pattern
            binding = create_binding(type, Regexp.last_match, line_num)
            binding.arg_types = arg_specs[binding.handler] if binding.handler && arg_specs[binding.handler]
            bindings << binding
          end
        end
      end

      bindings
    end

    private

    def extract_arg_specs(content)
      specs = {}

      # Find mrb_get_args calls within handler functions
      content.scan(/static\s+mrb_value\s+(\w+)\s*\([^)]*\)\s*\{(.*?)(?=^static\s+mrb_value|\z)/m) do |handler, body|
        if body =~ /mrb_get_args\s*\(\s*mrb\s*,\s*"([^"]+)"/
          specs[handler] = parse_format_string($1)
        end
      end

      specs
    end

    def parse_format_string(format)
      types = []
      optional = false

      format.each_char do |c|
        if c == '|'
          optional = true
        elsif ARG_TYPE_MAP[c]
          types << { type: ARG_TYPE_MAP[c], optional: optional }
        end
      end

      types
    end

    def create_binding(type, match, line_num)
      case type
      when :module_function
        BindingDef.new(type, match[1], match[2], match[3], match[4], line_num)
      when :class_method
        BindingDef.new(type, match[1], match[2], match[3], match[4], line_num)
      when :instance_method
        BindingDef.new(type, match[1], match[2], match[3], match[4], line_num)
      when :constant
        BindingDef.new(type, match[1], match[2], nil, nil, line_num)
      when :class_def
        BindingDef.new(type, match[2], match[3], match[1], nil, line_num)
      when :module_def
        BindingDef.new(type, match[1], match[2], nil, nil, line_num)
      end
    end
  end

  # Builds the API model from parsed docs and bindings
  class APIModel
    attr_reader :modules, :classes, :constants, :globals

    def initialize
      @modules = {}
      @classes = {}
      @constants = {}
      @globals = {
        'lifecycle' => {},
        'helpers' => {}
      }
      @module_vars = {}  # Maps variable names to module paths
    end

    def add_module_var(var_name, module_path)
      @module_vars[var_name] = module_path
    end

    def get_module_path(var_name)
      @module_vars[var_name]
    end

    def add_module(path, description = nil)
      @modules[path] ||= {
        'description' => description || "TODO: Add description",
        'functions' => {}
      }
    end

    def add_class(path, description = nil)
      @classes[path] ||= {
        'kind' => 'class',
        'description' => description || "TODO: Add description",
        'classMethods' => {},
        'instanceMethods' => {}
      }
    end

    def add_function(module_path, name, doc)
      add_module(module_path)
      @modules[module_path]['functions'][name] = doc
    end

    def add_class_method(class_path, name, doc)
      add_class(class_path)
      @classes[class_path]['classMethods'][name] = doc
    end

    def add_instance_method(class_path, name, doc)
      add_class(class_path)
      @classes[class_path]['instanceMethods'][name] = doc
    end

    def add_constant(module_path, name, value = nil)
      @constants[module_path] ||= {}
      @constants[module_path][name] = value
    end

    def add_lifecycle(name, doc)
      @globals['lifecycle'][name] = doc
    end

    def add_helper(name, doc)
      @globals['helpers'][name] = doc
    end

    # Merge modules and classes for output
    def to_modules_hash
      result = {}

      @modules.each do |path, data|
        result[path] = data.dup
        result[path].delete('functions') if result[path]['functions'].empty?
      end

      @classes.each do |path, data|
        result[path] = data.dup
        result[path].delete('classMethods') if result[path]['classMethods'].empty?
        result[path].delete('instanceMethods') if result[path]['instanceMethods'].empty?
      end

      result
    end
  end

  # Merges documentation with binding definitions
  class APIMerger
    def initialize
      @model = APIModel.new
    end

    def merge(doc_entries, binding_entries)
      # Build module variable mappings from bindings
      build_module_mappings(binding_entries)

      # Process doc entries
      doc_entries.each do |entry|
        process_doc_entry(entry)
      end

      # Process bindings without docs (generate stubs)
      process_undocumented_bindings(binding_entries, doc_entries)

      @model
    end

    private

    def build_module_mappings(bindings)
      # Map variable names to full module paths
      @model.add_module_var('gmr', 'GMR')
      @model.add_module_var('graphics', 'GMR::Graphics')
      @model.add_module_var('audio', 'GMR::Audio')
      @model.add_module_var('input', 'GMR::Input')
      @model.add_module_var('window', 'GMR::Window')
      @model.add_module_var('time', 'GMR::Time')
      @model.add_module_var('system', 'GMR::System')
      @model.add_module_var('collision', 'GMR::Collision')

      # Class variable mappings
      @model.add_module_var('texture_class', 'GMR::Graphics::Texture')
      @model.add_module_var('sound_class', 'GMR::Audio::Sound')
      @model.add_module_var('tilemap_class', 'GMR::Graphics::Tilemap')
    end

    def process_doc_entry(entry)
      parent = resolve_parent(entry.parent)

      case entry.kind
      when :function
        @model.add_function(parent, entry.name, entry.to_h)
      when :classmethod
        @model.add_class_method(parent, entry.name, entry.to_h)
      when :method
        @model.add_instance_method(parent, entry.name, entry.to_h)
      end
    end

    def resolve_parent(parent_ref)
      return 'GMR' unless parent_ref
      return parent_ref if parent_ref.include?('::')
      @model.get_module_path(parent_ref.downcase) || parent_ref
    end

    def process_undocumented_bindings(bindings, docs)
      documented_handlers = docs.map { |d| d.name }.to_set

      bindings.each do |binding|
        next if binding.type == :class_def || binding.type == :module_def
        next if documented_handlers.include?(binding.name)

        parent = resolve_binding_parent(binding)
        stub = generate_stub(binding)

        case binding.type
        when :module_function
          @model.add_function(parent, binding.name, stub)
        when :class_method
          @model.add_class_method(parent, binding.name, stub)
        when :instance_method
          @model.add_instance_method(parent, binding.name, stub)
        when :constant
          @model.add_constant(parent, binding.name)
        end
      end
    end

    def resolve_binding_parent(binding)
      @model.get_module_path(binding.module_var) || 'GMR'
    end

    def generate_stub(binding)
      args = binding.parse_args_spec
      params = []

      if binding.arg_types.any?
        binding.arg_types.each_with_index do |arg, i|
          param = {
            'name' => "arg#{i + 1}",
            'type' => arg[:type]
          }
          param['optional'] = true if arg[:optional]
          params << param
        end
      elsif args[:required] > 0 || args[:optional] > 0
        args[:required].times { |i| params << { 'name' => "arg#{i + 1}", 'type' => 'any' } }
        args[:optional].times { |i| params << { 'name' => "arg#{args[:required] + i + 1}", 'type' => 'any', 'optional' => true } }
      end

      signature = binding.name
      if params.any?
        param_names = params.map { |p| p['optional'] ? "[#{p['name']}]" : p['name'] }
        signature = "#{binding.name}(#{param_names.join(', ')})"
      end

      {
        'signature' => signature,
        'description' => 'TODO: Add documentation',
        'params' => params,
        'returns' => { 'type' => 'unknown' }
      }
    end
  end

  # Generates Markdown documentation files
  class MarkdownGenerator
    # Module name to filename mapping
    MODULE_FILES = {
      'GMR' => 'core',
      'GMR::Graphics' => 'graphics',
      'GMR::Graphics::Texture' => 'graphics',
      'GMR::Graphics::Tilemap' => 'graphics',
      'GMR::Input' => 'input',
      'GMR::Audio' => 'audio',
      'GMR::Audio::Sound' => 'audio',
      'GMR::Window' => 'window',
      'GMR::Collision' => 'collision',
      'GMR::Time' => 'time',
      'GMR::System' => 'system'
    }.freeze

    # Module descriptions for headers
    MODULE_DESCRIPTIONS = {
      'GMR' => 'Core engine functions and lifecycle hooks.',
      'GMR::Graphics' => 'Drawing primitives, textures, and rendering.',
      'GMR::Graphics::Texture' => 'Loaded image textures for drawing sprites.',
      'GMR::Graphics::Tilemap' => 'Tilemap rendering from Tiled JSON exports.',
      'GMR::Input' => 'Keyboard, mouse, and gamepad input handling.',
      'GMR::Audio' => 'Sound effects and music playback.',
      'GMR::Audio::Sound' => 'Loaded audio file for playback.',
      'GMR::Window' => 'Window management and display settings.',
      'GMR::Collision' => 'Collision detection between shapes.',
      'GMR::Time' => 'Frame timing and delta time access.',
      'GMR::System' => 'System utilities and debugging.'
    }.freeze

    def initialize(options)
      @options = options
    end

    def generate(model, output_dir)
      FileUtils.mkdir_p(output_dir)

      # Group modules and classes by output file
      files = {}

      model.modules.each do |path, data|
        filename = MODULE_FILES[path] || path.downcase.gsub('::', '_')
        files[filename] ||= { modules: [], classes: [] }
        files[filename][:modules] << [path, data]
      end

      model.classes.each do |path, data|
        filename = MODULE_FILES[path] || path.downcase.gsub('::', '_')
        files[filename] ||= { modules: [], classes: [] }
        files[filename][:classes] << [path, data]
      end

      # Generate each file
      files.each do |filename, content|
        generate_file(output_dir, filename, content)
      end

      # Generate index
      generate_index(output_dir, files, model)
    end

    private

    def generate_file(output_dir, filename, content)
      path = File.join(output_dir, "#{filename}.md")
      File.open(path, 'w') do |f|
        # Find primary module for header
        primary = content[:modules].first || content[:classes].first
        primary_path = primary&.first || filename.capitalize

        f.puts "# #{primary_path}"
        f.puts
        f.puts MODULE_DESCRIPTIONS[primary_path] || "API reference for #{primary_path}."
        f.puts

        # Document modules
        content[:modules].each do |mod_path, mod_data|
          write_module(f, mod_path, mod_data)
        end

        # Document classes
        content[:classes].each do |class_path, class_data|
          write_class(f, class_path, class_data)
        end
      end

      puts "  Generated: #{path}"
    end

    def write_module(f, path, data)
      return unless data['functions']&.any?

      f.puts "## Functions"
      f.puts

      data['functions'].each do |name, func|
        write_function(f, name, func, path)
      end
    end

    def write_class(f, path, data)
      class_name = path.split('::').last
      f.puts "## #{class_name}"
      f.puts
      f.puts data['description'] || MODULE_DESCRIPTIONS[path] || "TODO: Add description"
      f.puts

      # Class methods
      if data['classMethods']&.any?
        f.puts "### Class Methods"
        f.puts
        data['classMethods'].each do |name, method|
          write_function(f, name, method, "#{class_name}.", is_class_method: true)
        end
      end

      # Instance methods
      if data['instanceMethods']&.any?
        f.puts "### Instance Methods"
        f.puts
        data['instanceMethods'].each do |name, method|
          write_function(f, name, method, "#{class_name}#", is_instance_method: true)
        end
      end
    end

    def write_function(f, name, func, prefix = '', is_class_method: false, is_instance_method: false)
      # Header with signature
      signature = func['signature'] || name
      f.puts "### #{signature}"
      f.puts

      # Description
      if func['description']
        f.puts func['description']
        f.puts
      end

      # Parameters table
      if func['params']&.any?
        f.puts "**Parameters:**"
        f.puts
        f.puts "| Name | Type | Description |"
        f.puts "|------|------|-------------|"
        func['params'].each do |param|
          type_str = param['type']
          type_str += " (optional)" if param['optional']
          type_str += ", default: #{param['default']}" if param['default']
          f.puts "| #{param['name']} | #{type_str} | #{param['description'] || ''} |"
        end
        f.puts
      end

      # Returns
      if func['returns']
        ret = func['returns']
        ret_desc = ret['description'] ? " - #{ret['description']}" : ""
        f.puts "**Returns:** `#{ret['type']}`#{ret_desc}"
        f.puts
      end

      # Raises
      if func['raises']&.any?
        f.puts "**Raises:**"
        func['raises'].each do |err|
          f.puts "- #{err}"
        end
        f.puts
      end

      # Example
      if func['example']
        f.puts "**Example:**"
        f.puts "```ruby"
        f.puts func['example']
        f.puts "```"
        f.puts
      end

      f.puts "---"
      f.puts
    end

    def generate_index(output_dir, files, model)
      path = File.join(output_dir, "README.md")
      File.open(path, 'w') do |f|
        f.puts "# GMR Ruby API Reference"
        f.puts
        f.puts "Auto-generated API documentation for GMR #{@options[:engine_version]}."
        f.puts
        f.puts "## Modules"
        f.puts
        f.puts "| Module | Description |"
        f.puts "|--------|-------------|"

        model.modules.keys.sort.each do |mod_path|
          filename = MODULE_FILES[mod_path] || mod_path.downcase.gsub('::', '_')
          desc = MODULE_DESCRIPTIONS[mod_path] || "API reference"
          f.puts "| [#{mod_path}](#{filename}.md) | #{desc} |"
        end
        f.puts

        if model.classes.any?
          f.puts "## Classes"
          f.puts
          f.puts "| Class | Description |"
          f.puts "|-------|-------------|"

          model.classes.keys.sort.each do |class_path|
            filename = MODULE_FILES[class_path] || class_path.downcase.gsub('::', '_')
            desc = MODULE_DESCRIPTIONS[class_path] || "Class reference"
            f.puts "| [#{class_path}](#{filename}.md) | #{desc} |"
          end
          f.puts
        end

        f.puts "## Types"
        f.puts
        f.puts "| Type | Description |"
        f.puts "|------|-------------|"
        TYPE_DEFINITIONS.each do |type_name, type_def|
          f.puts "| #{type_name} | #{type_def['description']} |"
        end
        f.puts
        f.puts "---"
        f.puts
        f.puts "*Generated by `generate_api_docs.rb` from C++ bindings.*"
      end

      puts "  Generated: #{path}"
    end
  end

  # Generates HTML documentation files with dark theme
  class HTMLGenerator
    # Ruby keywords for syntax highlighting
    RUBY_KEYWORDS = %w[
      def end class module if else elsif unless case when while until for do
      begin rescue ensure raise return yield break next redo retry
      true false nil self super
      and or not in
      attr_reader attr_writer attr_accessor
      require require_relative include extend prepend
      public private protected
      lambda proc
    ].freeze

    # Simple Ruby syntax highlighter
    # Uses placeholder tokens to avoid regex conflicts with generated HTML
    def self.highlight_ruby(code)
      return '' unless code

      # Escape HTML first
      escaped = code.to_s
        .gsub('&', '&amp;')
        .gsub('<', '&lt;')
        .gsub('>', '&gt;')

      # Use placeholder tokens to avoid regex matching inside generated spans
      # Format: \x00TYPE\x01content\x02
      tokens = []
      token_id = 0

      # Helper to create a token placeholder
      make_token = lambda do |type, content|
        id = token_id
        token_id += 1
        tokens << [id, type, content]
        "\x00#{id}\x02"
      end

      # Comments (must be first to avoid highlighting inside comments)
      escaped = escaped.gsub(/(#.*)$/) { make_token.call('comment', $1) }

      # Strings (double and single quoted)
      escaped = escaped.gsub(/("(?:[^"\\]|\\.)*")/) { make_token.call('string', $1) }
      escaped = escaped.gsub(/('(?:[^'\\]|\\.)*')/) { make_token.call('string', $1) }

      # Symbols (but not :: which is namespace separator)
      escaped = escaped.gsub(/(?<!:)(:[\w?!]+)/) { make_token.call('symbol', $1) }

      # Numbers
      escaped = escaped.gsub(/\b(\d+\.?\d*)\b/) { make_token.call('number', $1) }

      # Module/Class names (capitalized words)
      escaped = escaped.gsub(/\b([A-Z][A-Za-z0-9_]*)\b/) { make_token.call('class', $1) }

      # Keywords (only match whole words not already tokenized)
      keyword_pattern = /\b(#{RUBY_KEYWORDS.join('|')})\b/
      escaped = escaped.gsub(keyword_pattern) { make_token.call('keyword', $1) }

      # Method calls (dot-prefixed)
      escaped = escaped.gsub(/\.(\w+[?!]?)/) { '.' + make_token.call('method', $1) }

      # Instance variables
      escaped = escaped.gsub(/(@\w+)/) { make_token.call('ivar', $1) }

      # Global variables
      escaped = escaped.gsub(/(\$\w+)/) { make_token.call('gvar', $1) }

      # Now replace all tokens with actual spans
      tokens.each do |id, type, content|
        escaped = escaped.gsub("\x00#{id}\x02", "<span class=\"hl-#{type}\">#{content}</span>")
      end

      escaped
    end

    # Shared CSS for all HTML pages
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

      .container {
        max-width: 1200px;
        margin: 0 auto;
        padding: 2rem;
      }

      /* Navigation */
      .nav {
        background: var(--bg-secondary);
        border-bottom: 1px solid var(--border-color);
        padding: 1rem 2rem;
        position: sticky;
        top: 0;
        z-index: 100;
      }

      .nav-content {
        max-width: 1200px;
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

      .nav-links {
        display: flex;
        gap: 1.5rem;
        list-style: none;
      }

      .nav-links a {
        color: var(--text-secondary);
        text-decoration: none;
        font-size: 0.9rem;
        transition: color 0.2s;
      }

      .nav-links a:hover, .nav-links a.active {
        color: var(--accent-blue);
      }

      /* Headings */
      h1 {
        font-size: 2.5rem;
        color: var(--accent-magenta);
        margin-bottom: 0.5rem;
        border-bottom: 2px solid var(--border-color);
        padding-bottom: 0.5rem;
      }

      h2 {
        font-size: 1.75rem;
        color: var(--accent-cyan);
        margin: 2rem 0 1rem;
        padding-bottom: 0.25rem;
        border-bottom: 1px solid var(--border-color);
      }

      h3 {
        font-size: 1.25rem;
        color: var(--accent-green);
        margin: 1.5rem 0 0.75rem;
      }

      h4 {
        font-size: 1.1rem;
        color: var(--accent-orange);
        margin: 1rem 0 0.5rem;
      }

      p { margin-bottom: 1rem; }

      a {
        color: var(--link-color);
        text-decoration: none;
        transition: color 0.2s;
      }

      a:hover { color: var(--link-hover); }

      /* Code */
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

      pre code {
        background: none;
        padding: 0;
        color: var(--text-primary);
      }

      /* Tables */
      table {
        width: 100%;
        border-collapse: collapse;
        margin: 1rem 0;
        font-size: 0.9rem;
      }

      th, td {
        padding: 0.75rem 1rem;
        text-align: left;
        border-bottom: 1px solid var(--border-color);
      }

      th {
        background: var(--bg-secondary);
        color: var(--accent-cyan);
        font-weight: 600;
      }

      tr:hover { background: var(--bg-secondary); }

      /* Cards */
      .card {
        background: var(--bg-secondary);
        border: 1px solid var(--border-color);
        border-radius: 8px;
        padding: 1.5rem;
        margin: 1rem 0;
      }

      .card-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
        gap: 1rem;
        margin: 1.5rem 0;
      }

      .card h3 { margin-top: 0; }

      /* Function/Method blocks */
      .function-block {
        background: var(--bg-secondary);
        border: 1px solid var(--border-color);
        border-left: 4px solid var(--accent-green);
        border-radius: 0 8px 8px 0;
        padding: 1.25rem;
        margin: 1.5rem 0;
      }

      .function-block.class-method {
        border-left-color: var(--accent-magenta);
      }

      .function-block.instance-method {
        border-left-color: var(--accent-cyan);
      }

      .function-signature {
        font-family: 'JetBrains Mono', 'Fira Code', monospace;
        font-size: 1.1rem;
        color: var(--accent-green);
        margin-bottom: 0.75rem;
      }

      .function-block.class-method .function-signature {
        color: var(--accent-magenta);
      }

      .function-block.instance-method .function-signature {
        color: var(--accent-cyan);
      }

      .function-description {
        color: var(--text-secondary);
        margin-bottom: 1rem;
      }

      /* Labels */
      .label {
        display: inline-block;
        padding: 0.2rem 0.5rem;
        border-radius: 4px;
        font-size: 0.75rem;
        font-weight: 600;
        text-transform: uppercase;
      }

      .label-optional {
        background: var(--accent-yellow);
        color: var(--bg-primary);
      }

      .label-type {
        background: var(--accent-blue);
        color: var(--bg-primary);
      }

      /* Sidebar */
      .layout {
        display: flex;
        gap: 2rem;
      }

      .sidebar {
        width: 250px;
        flex-shrink: 0;
        position: sticky;
        top: 80px;
        height: fit-content;
        max-height: calc(100vh - 100px);
        overflow-y: auto;
      }

      .sidebar-section {
        margin-bottom: 1.5rem;
      }

      .sidebar-title {
        font-size: 0.75rem;
        text-transform: uppercase;
        color: var(--text-muted);
        margin-bottom: 0.5rem;
        letter-spacing: 0.05em;
      }

      .sidebar-links {
        list-style: none;
      }

      .sidebar-links a {
        display: block;
        padding: 0.35rem 0;
        color: var(--text-secondary);
        font-size: 0.9rem;
      }

      .sidebar-links a:hover,
      .sidebar-links a.active {
        color: var(--accent-blue);
      }

      .content {
        flex: 1;
        min-width: 0;
      }

      /* Breadcrumb */
      .breadcrumb {
        font-size: 0.85rem;
        color: var(--text-muted);
        margin-bottom: 1rem;
      }

      .breadcrumb a { color: var(--text-secondary); }

      /* Footer */
      .footer {
        margin-top: 3rem;
        padding-top: 1.5rem;
        border-top: 1px solid var(--border-color);
        color: var(--text-muted);
        font-size: 0.85rem;
        text-align: center;
      }

      /* Responsive */
      @media (max-width: 768px) {
        .layout { flex-direction: column; }
        .sidebar {
          width: 100%;
          position: static;
          max-height: none;
        }
        .nav-content { flex-direction: column; gap: 1rem; }
      }

      /* Syntax Highlighting */
      .hl-keyword { color: #bb9af7; font-weight: 500; }
      .hl-string { color: #9ece6a; }
      .hl-number { color: #ff9e64; }
      .hl-comment { color: #565f89; font-style: italic; }
      .hl-symbol { color: #7dcfff; }
      .hl-class { color: #7aa2f7; }
      .hl-method { color: #7aa2f7; }
      .hl-ivar { color: #f7768e; }
      .hl-gvar { color: #f7768e; }
      .hl-const { color: #ff9e64; }
    CSS

    def initialize(options)
      @options = options
    end

    def generate(model, output_dir)
      FileUtils.mkdir_p(output_dir)

      # Write CSS file
      File.write(File.join(output_dir, 'styles.css'), DARK_THEME_CSS)
      puts "  Generated: #{File.join(output_dir, 'styles.css')}"

      # Group modules and classes by output file
      files = {}
      MarkdownGenerator::MODULE_FILES.each do |path, filename|
        files[filename] ||= { modules: [], classes: [] }
      end

      model.modules.each do |path, data|
        filename = MarkdownGenerator::MODULE_FILES[path] || path.downcase.gsub('::', '_')
        files[filename] ||= { modules: [], classes: [] }
        files[filename][:modules] << [path, data]
      end

      model.classes.each do |path, data|
        filename = MarkdownGenerator::MODULE_FILES[path] || path.downcase.gsub('::', '_')
        files[filename] ||= { modules: [], classes: [] }
        files[filename][:classes] << [path, data]
      end

      # Generate each page
      nav_items = build_nav_items(model)
      files.each do |filename, content|
        next if content[:modules].empty? && content[:classes].empty?
        generate_page(output_dir, filename, content, nav_items)
      end

      # Generate index
      generate_index(output_dir, model, nav_items)
    end

    private

    def build_nav_items(model)
      items = []
      model.modules.keys.sort.each do |path|
        filename = MarkdownGenerator::MODULE_FILES[path] || path.downcase.gsub('::', '_')
        items << { name: path, file: "#{filename}.html", type: :module }
      end
      model.classes.keys.sort.each do |path|
        filename = MarkdownGenerator::MODULE_FILES[path] || path.downcase.gsub('::', '_')
        items << { name: path, file: "#{filename}.html", type: :class }
      end
      items.uniq { |i| i[:file] }
    end

    def html_escape(text)
      return '' unless text
      text.to_s
        .gsub('&', '&amp;')
        .gsub('<', '&lt;')
        .gsub('>', '&gt;')
        .gsub('"', '&quot;')
    end

    def generate_page(output_dir, filename, content, nav_items)
      path = File.join(output_dir, "#{filename}.html")

      primary = content[:modules].first || content[:classes].first
      primary_path = primary&.first || filename.capitalize
      title = primary_path
      description = MarkdownGenerator::MODULE_DESCRIPTIONS[primary_path] || "API reference for #{primary_path}."

      File.open(path, 'w') do |f|
        f.puts html_header(title, nav_items, filename)

        f.puts '<div class="container">'
        f.puts '<div class="layout">'

        # Sidebar
        f.puts render_sidebar(content, filename)

        # Main content
        f.puts '<div class="content">'
        f.puts "<h1>#{html_escape(title)}</h1>"
        f.puts "<p>#{html_escape(description)}</p>"

        # Modules
        content[:modules].each do |mod_path, mod_data|
          render_module(f, mod_path, mod_data)
        end

        # Classes
        content[:classes].each do |class_path, class_data|
          render_class(f, class_path, class_data)
        end

        f.puts '</div>' # content
        f.puts '</div>' # layout
        f.puts '</div>' # container

        f.puts html_footer
      end

      puts "  Generated: #{path}"
    end

    def render_sidebar(content, current_file)
      html = '<aside class="sidebar">'

      # Functions section
      functions = []
      content[:modules].each do |_, data|
        functions.concat(data['functions']&.keys || [])
      end

      unless functions.empty?
        html += '<div class="sidebar-section">'
        html += '<div class="sidebar-title">Functions</div>'
        html += '<ul class="sidebar-links">'
        functions.sort.each do |name|
          html += "<li><a href=\"##{name}\">#{html_escape(name)}</a></li>"
        end
        html += '</ul></div>'
      end

      # Classes section
      content[:classes].each do |class_path, class_data|
        class_name = class_path.split('::').last
        html += '<div class="sidebar-section">'
        html += "<div class=\"sidebar-title\">#{html_escape(class_name)}</div>"
        html += '<ul class="sidebar-links">'

        (class_data['classMethods']&.keys || []).sort.each do |name|
          html += "<li><a href=\"##{class_name}-#{name}\">.#{html_escape(name)}</a></li>"
        end
        (class_data['instanceMethods']&.keys || []).sort.each do |name|
          html += "<li><a href=\"##{class_name}-#{name}\">##{html_escape(name)}</a></li>"
        end

        html += '</ul></div>'
      end

      html += '</aside>'
      html
    end

    def render_module(f, path, data)
      return unless data['functions']&.any?

      f.puts '<h2>Functions</h2>'

      data['functions'].each do |name, func|
        render_function(f, name, func)
      end
    end

    def render_class(f, path, data)
      class_name = path.split('::').last
      f.puts "<h2>#{html_escape(class_name)}</h2>"
      desc = data['description'] || MarkdownGenerator::MODULE_DESCRIPTIONS[path]
      f.puts "<p>#{html_escape(desc)}</p>" if desc

      if data['classMethods']&.any?
        f.puts '<h3>Class Methods</h3>'
        data['classMethods'].each do |name, method|
          render_function(f, name, method, class_name, :class_method)
        end
      end

      if data['instanceMethods']&.any?
        f.puts '<h3>Instance Methods</h3>'
        data['instanceMethods'].each do |name, method|
          render_function(f, name, method, class_name, :instance_method)
        end
      end
    end

    def render_function(f, name, func, class_name = nil, method_type = nil)
      css_class = case method_type
                  when :class_method then 'function-block class-method'
                  when :instance_method then 'function-block instance-method'
                  else 'function-block'
                  end

      anchor = class_name ? "#{class_name}-#{name}" : name
      signature = func['signature'] || name

      f.puts "<div class=\"#{css_class}\" id=\"#{html_escape(anchor)}\">"
      f.puts "<div class=\"function-signature\">#{html_escape(signature)}</div>"

      if func['description']
        f.puts "<div class=\"function-description\">#{html_escape(func['description'])}</div>"
      end

      # Parameters
      if func['params']&.any?
        f.puts '<h4>Parameters</h4>'
        f.puts '<table>'
        f.puts '<thead><tr><th>Name</th><th>Type</th><th>Description</th></tr></thead>'
        f.puts '<tbody>'
        func['params'].each do |param|
          optional = param['optional'] ? ' <span class="label label-optional">optional</span>' : ''
          default = param['default'] ? " (default: <code>#{html_escape(param['default'])}</code>)" : ''
          f.puts "<tr><td><code>#{html_escape(param['name'])}</code></td>"
          f.puts "<td><code>#{html_escape(param['type'])}</code>#{optional}#{default}</td>"
          f.puts "<td>#{html_escape(param['description'])}</td></tr>"
        end
        f.puts '</tbody></table>'
      end

      # Returns
      if func['returns']
        ret = func['returns']
        f.puts '<h4>Returns</h4>'
        desc = ret['description'] ? " &mdash; #{html_escape(ret['description'])}" : ''
        f.puts "<p><code>#{html_escape(ret['type'])}</code>#{desc}</p>"
      end

      # Example
      if func['example']
        f.puts '<h4>Example</h4>'
        f.puts '<pre><code class="language-ruby">'
        f.puts HTMLGenerator.highlight_ruby(func['example'])
        f.puts '</code></pre>'
      end

      f.puts '</div>'
    end

    def generate_index(output_dir, model, nav_items)
      path = File.join(output_dir, 'index.html')

      File.open(path, 'w') do |f|
        f.puts html_header('GMR Ruby API', nav_items, 'index')

        f.puts '<div class="container">'
        f.puts '<h1>GMR Ruby API Reference</h1>'
        f.puts "<p>Auto-generated API documentation for GMR #{@options[:engine_version]}.</p>"

        # Modules grid
        f.puts '<h2>Modules</h2>'
        f.puts '<div class="card-grid">'
        model.modules.keys.sort.each do |mod_path|
          filename = MarkdownGenerator::MODULE_FILES[mod_path] || mod_path.downcase.gsub('::', '_')
          desc = MarkdownGenerator::MODULE_DESCRIPTIONS[mod_path] || 'API reference'
          f.puts '<div class="card">'
          f.puts "<h3><a href=\"#{filename}.html\">#{html_escape(mod_path)}</a></h3>"
          f.puts "<p>#{html_escape(desc)}</p>"
          f.puts '</div>'
        end
        f.puts '</div>'

        # Classes grid
        if model.classes.any?
          f.puts '<h2>Classes</h2>'
          f.puts '<div class="card-grid">'
          model.classes.keys.sort.each do |class_path|
            filename = MarkdownGenerator::MODULE_FILES[class_path] || class_path.downcase.gsub('::', '_')
            desc = MarkdownGenerator::MODULE_DESCRIPTIONS[class_path] || 'Class reference'
            f.puts '<div class="card">'
            f.puts "<h3><a href=\"#{filename}.html\">#{html_escape(class_path)}</a></h3>"
            f.puts "<p>#{html_escape(desc)}</p>"
            f.puts '</div>'
          end
          f.puts '</div>'
        end

        # Types table
        f.puts '<h2>Types</h2>'
        f.puts '<table>'
        f.puts '<thead><tr><th>Type</th><th>Description</th></tr></thead>'
        f.puts '<tbody>'
        TYPE_DEFINITIONS.each do |type_name, type_def|
          f.puts "<tr><td><code>#{html_escape(type_name)}</code></td>"
          f.puts "<td>#{html_escape(type_def['description'])}</td></tr>"
        end
        f.puts '</tbody></table>'

        f.puts '</div>'
        f.puts html_footer
      end

      puts "  Generated: #{path}"
    end

    def html_header(title, nav_items, current_file)
      nav_html = nav_items.map do |item|
        active = item[:file] == "#{current_file}.html" ? ' class="active"' : ''
        "<a href=\"#{item[:file]}\"#{active}>#{html_escape(item[:name].split('::').last)}</a>"
      end.join("\n          ")

      <<~HTML
        <!DOCTYPE html>
        <html lang="en">
        <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>#{html_escape(title)} - GMR API</title>
          <link rel="stylesheet" href="styles.css">
        </head>
        <body>
          <nav class="nav">
            <div class="nav-content">
              <a href="index.html" class="nav-brand">GMR API</a>
              <ul class="nav-links">
                #{nav_html}
              </ul>
            </div>
          </nav>
      HTML
    end

    def html_footer
      <<~HTML
          <footer class="footer">
            <p>Generated by <code>generate_api_docs.rb</code> from C++ bindings.</p>
            <p>GMR #{@options[:engine_version]} &bull; Ruby API Reference</p>
          </footer>
        </body>
        </html>
      HTML
    end
  end

  # Generates JSON output files
  class JSONGenerator
    def initialize(options)
      @options = options
    end

    def generate_api_json(model, output_path)
      # Build types including class references
      types = TYPE_DEFINITIONS.dup

      model.classes.each do |path, data|
        class_name = path.split('::').last
        types[class_name] = {
          'kind' => 'class',
          'module' => path,
          'description' => data['description']
        }
      end

      api = {
        'meta' => {
          'version' => '1.0.0',
          'engine' => 'GMR',
          'mrubyVersion' => @options[:mruby_version],
          'raylibVersion' => @options[:raylib_version]
        },
        'types' => types,
        'modules' => model.to_modules_hash,
        'globals' => model.globals
      }

      write_json(output_path, api)
    end

    def generate_syntax_json(model, output_path)
      # Extract function names by module
      functions = {}
      model.modules.each do |path, data|
        next unless data['functions']&.any?
        functions[path] = data['functions'].keys
      end

      model.classes.each do |path, data|
        methods = []
        methods.concat(data['classMethods'].keys) if data['classMethods']&.any?
        methods.concat(data['instanceMethods'].keys) if data['instanceMethods']&.any?
        functions[path] = methods if methods.any?
      end

      syntax = {
        'language' => {
          'id' => 'gmr-ruby',
          'name' => 'GMR Ruby',
          'baseLanguage' => 'ruby',
          'mrubyVersion' => @options[:mruby_version]
        },
        'comments' => {
          'lineComment' => '#',
          'blockComment' => { 'start' => '=begin', 'end' => '=end' }
        },
        'keywords' => {
          'control' => %w[if elsif else unless then case when while until for loop do begin rescue ensure raise retry return break next redo yield],
          'definition' => %w[def end class module include extend prepend attr_reader attr_writer attr_accessor alias undef],
          'modifier' => %w[public private protected],
          'logical' => %w[and or not in],
          'special' => %w[self super nil true false __FILE__ __LINE__ __ENCODING__ defined?]
        },
        'operators' => {
          'arithmetic' => %w[+ - * / % **],
          'comparison' => %w[== != < > <= >= <=> === =~ !~],
          'assignment' => %w[= += -= *= /= %= **= &&= ||= &= |= ^= <<= >>=],
          'logical' => %w[&& || !],
          'bitwise' => %w[& | ^ ~ << >>],
          'range' => %w[.. ...],
          'ternary' => %w[? :],
          'access' => %w[:: . -> => &. ::]
        },
        'brackets' => {
          'pairs' => [['(', ')'], ['[', ']'], ['{', '}'], ['do', 'end']]
        },
        'literals' => {
          'string' => {
            'singleQuote' => "'",
            'doubleQuote' => '"',
            'heredoc' => ['<<-', '<<~', '<<'],
            'percent' => %w[%q %Q %w %W %i %I %x]
          },
          'symbol' => { 'prefix' => ':', 'percent' => '%s' },
          'regex' => { 'delimiters' => ['/'], 'percent' => '%r', 'flags' => %w[i m x o] },
          'number' => {
            'integer' => { 'decimal' => true, 'hexadecimal' => '0x', 'octal' => '0o', 'binary' => '0b', 'separator' => '_' },
            'float' => { 'decimal' => true, 'scientific' => 'e', 'separator' => '_' }
          }
        },
        'builtins' => {
          'classes' => %w[Object Class Module BasicObject String Integer Float Numeric Symbol Array Hash Range Regexp Proc Method TrueClass FalseClass NilClass Exception StandardError RuntimeError ArgumentError TypeError NameError NoMethodError Kernel Comparable Enumerable Math Fiber],
          'methods' => %w[puts print p gets require require_relative load lambda proc raise fail catch throw loop block_given? caller rand srand sleep sprintf format eval instance_eval class_eval module_eval method send public_send respond_to? is_a? kind_of? instance_of? nil? empty? frozen? to_s to_i to_f to_a to_h to_sym inspect clone dup freeze tap each each_with_index map collect select find_all reject reduce inject find detect sort sort_by min max minmax sum length size count first last push pop shift unshift insert delete delete_at flatten compact uniq reverse shuffle sample join split strip chomp chop gsub sub match upcase downcase capitalize swapcase include? index rindex start_with? end_with? any? all? none? one? empty?]
        },
        'engine' => {
          'modules' => model.modules.keys.sort,
          'classes' => model.classes.keys.sort,
          'functions' => functions,
          'constants' => build_constants_section(model),
          'lifecycleHooks' => %w[init update draw],
          'aliases' => {
            'G' => 'GMR::Graphics',
            'I' => 'GMR::Input',
            'W' => 'GMR::Window',
            'T' => 'GMR::Time',
            'S' => 'GMR::System'
          }
        }
      }

      write_json(output_path, syntax)
    end

    def generate_version_json(model, output_path)
      version = {
        'engine' => {
          'name' => 'GMR',
          'fullName' => 'Games Made with Ruby',
          'version' => @options[:engine_version],
          'description' => 'A modern, cross-platform game framework combining the elegance of Ruby with the performance of C++'
        },
        'language' => {
          'id' => 'gmr-ruby',
          'name' => 'GMR Ruby',
          'baseLanguage' => 'ruby',
          'mrubyVersion' => @options[:mruby_version],
          'rubyCompatibility' => '~3.0'
        },
        'runtime' => {
          'raylib' => @options[:raylib_version],
          'platforms' => %w[windows linux macos web]
        },
        'schema' => {
          'syntax' => '1.0.0',
          'api' => '1.0.0'
        }
      }

      write_json(output_path, version)
    end

    private

    def build_constants_section(model)
      # Static constants definition for Input
      {
        'GMR::Input' => {
          'mouse' => %w[MOUSE_LEFT MOUSE_RIGHT MOUSE_MIDDLE MOUSE_SIDE MOUSE_EXTRA MOUSE_FORWARD MOUSE_BACK],
          'keys' => {
            'special' => %w[KEY_SPACE KEY_ESCAPE KEY_ENTER KEY_TAB KEY_BACKSPACE KEY_DELETE KEY_INSERT],
            'arrows' => %w[KEY_UP KEY_DOWN KEY_LEFT KEY_RIGHT],
            'navigation' => %w[KEY_HOME KEY_END KEY_PAGE_UP KEY_PAGE_DOWN],
            'modifiers' => %w[KEY_LEFT_SHIFT KEY_RIGHT_SHIFT KEY_LEFT_CONTROL KEY_RIGHT_CONTROL KEY_LEFT_ALT KEY_RIGHT_ALT],
            'function' => %w[KEY_F1 KEY_F2 KEY_F3 KEY_F4 KEY_F5 KEY_F6 KEY_F7 KEY_F8 KEY_F9 KEY_F10 KEY_F11 KEY_F12],
            'letters' => %w[KEY_A KEY_B KEY_C KEY_D KEY_E KEY_F KEY_G KEY_H KEY_I KEY_J KEY_K KEY_L KEY_M KEY_N KEY_O KEY_P KEY_Q KEY_R KEY_S KEY_T KEY_U KEY_V KEY_W KEY_X KEY_Y KEY_Z],
            'numbers' => %w[KEY_0 KEY_1 KEY_2 KEY_3 KEY_4 KEY_5 KEY_6 KEY_7 KEY_8 KEY_9]
          }
        }
      }
    end

    def write_json(path, data)
      File.write(path, JSON.pretty_generate(data) + "\n")
      puts "  Generated: #{path}"
    end
  end

  # Main entry point
  def self.run(options)
    puts "GMR API Documentation Generator"
    puts "================================"

    doc_parser = DocParser.new
    binding_parser = BindingParser.new
    merger = APIMerger.new
    json_generator = JSONGenerator.new(options)

    all_docs = []
    all_bindings = []

    # Parse all source files
    puts "\nParsing source files..."
    options[:sources].each do |src|
      unless File.exist?(src)
        warn "  Warning: File not found: #{src}"
        next
      end

      puts "  Parsing: #{File.basename(src)}"
      all_docs.concat(doc_parser.parse_file(src))
      all_bindings.concat(binding_parser.parse_file(src))
    end

    puts "\nFound #{all_docs.size} documented entries"
    puts "Found #{all_bindings.size} binding registrations"

    # Merge documentation with bindings
    model = merger.merge(all_docs, all_bindings)

    # Ensure output directory exists
    FileUtils.mkdir_p(options[:output_dir])

    # Generate JSON output files
    puts "\nGenerating JSON files..."
    json_generator.generate_api_json(model, File.join(options[:output_dir], 'api.json'))
    json_generator.generate_syntax_json(model, File.join(options[:output_dir], 'syntax.json'))
    json_generator.generate_version_json(model, File.join(options[:output_dir], 'version.json'))

    # Generate markdown documentation if requested
    if options[:markdown_dir]
      puts "\nGenerating Markdown documentation..."
      md_generator = MarkdownGenerator.new(options)
      md_generator.generate(model, options[:markdown_dir])
    end

    # Generate HTML documentation if requested
    if options[:html_dir]
      puts "\nGenerating HTML documentation..."
      html_generator = HTMLGenerator.new(options)
      html_generator.generate(model, options[:html_dir])
    end

    puts "\nDone!"
  end
end

# Parse command line arguments
options = {
  sources: [],
  output_dir: 'engine/language',
  markdown_dir: nil,
  html_dir: nil,
  engine_version: '0.1.0',
  raylib_version: '5.6-dev',
  mruby_version: '3.4.0'
}

OptionParser.new do |opts|
  opts.banner = "Usage: generate_api_docs.rb [options]"

  opts.on("-s", "--source FILE", "Add source file to parse") do |f|
    options[:sources] << f
  end

  opts.on("-o", "--output DIR", "Output directory for JSON") do |d|
    options[:output_dir] = d
  end

  opts.on("-m", "--markdown DIR", "Generate markdown docs to directory") do |d|
    options[:markdown_dir] = d
  end

  opts.on("--html DIR", "Generate HTML docs to directory") do |d|
    options[:html_dir] = d
  end

  opts.on("-v", "--version VERSION", "Engine version") do |v|
    options[:engine_version] = v
  end

  opts.on("--raylib VERSION", "Raylib version") do |v|
    options[:raylib_version] = v
  end

  opts.on("--mruby VERSION", "mRuby version") do |v|
    options[:mruby_version] = v
  end

  opts.on("-h", "--help", "Show this help") do
    puts opts
    exit
  end
end.parse!

# If no sources specified, use default binding files
if options[:sources].empty?
  script_dir = File.dirname(__FILE__)
  project_root = File.expand_path('..', script_dir)
  binding_dir = File.join(project_root, 'src', 'bindings')

  Dir.glob(File.join(binding_dir, '*.cpp')).each do |f|
    options[:sources] << f
  end

  # Update output dir to be relative to project root
  options[:output_dir] = File.join(project_root, 'engine', 'language')
end

GMRDocs.run(options)
