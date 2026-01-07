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
require 'stringio'

# Try to load coderay for better syntax highlighting
begin
  require 'coderay'
  CODERAY_AVAILABLE = true
rescue LoadError
  CODERAY_AVAILABLE = false
end

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

  # ===== CLI Documentation Parsing =====

  # Parsed CLI command definition from Thor
  class CLICommand
    attr_accessor :name, :description, :long_desc, :options, :aliases, :default_task, :stages

    def initialize(name)
      @name = name
      @description = nil
      @long_desc = nil
      @options = []
      @aliases = []
      @default_task = false
      @stages = []
    end
  end

  # Parsed CLI option definition from Thor
  class CLIOption
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
  class CLIParser
    def parse_file(filepath)
      return { commands: {}, class_options: [] } unless File.exist?(filepath)

      content = File.read(filepath, encoding: 'utf-8')
      commands = {}
      class_options = []
      current_command = nil

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
          current_command = CLICommand.new(cmd_name)
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

    # Parse stages from command files
    def parse_stages(commands_dir)
      stages = {}
      return stages unless Dir.exist?(commands_dir)

      Dir.glob(File.join(commands_dir, '*.rb')).each do |cmd_file|
        cmd_name = File.basename(cmd_file, '.rb')
        content = File.read(cmd_file, encoding: 'utf-8')
        cmd_stages = []

        content.scan(/run_stage\s*\(\s*:(\w+)\s*,\s*"([^"]+)"\s*\)/) do |id, name|
          cmd_stages << { id: id, name: name }
        end

        stages[cmd_name] = cmd_stages if cmd_stages.any?
      end

      stages
    end

    # Parse error codes from error_codes.rb
    def parse_error_codes(filepath)
      return [] unless File.exist?(filepath)

      content = File.read(filepath, encoding: 'utf-8')
      codes = []

      content.scan(/"([A-Z_]+(?:\.[A-Z_]+)+)"\s*=>\s*\{\s*exit_code:\s*(\d+)\s*,\s*description:\s*"([^"]+)"/) do |code, exit_code, description|
        codes << { code: code, exit_code: exit_code.to_i, description: description }
      end

      codes
    end

    private

    def parse_option_line(line, lines, index)
      # Extract option name
      line =~ /:(\w+)/
      opt = CLIOption.new($1)

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

  # Documentation hierarchy - defines how modules/classes are organized
  CATEGORY_HIERARCHY = {
    'engine' => {
      name: 'Engine',
      description: 'Core engine modules for game development.',
      children: {
        'graphics' => {
          name: 'Graphics',
          description: 'Drawing primitives, textures, sprites, and rendering.',
          modules: ['GMR::Graphics'],
          classes: ['Sprite', 'Camera2D', 'GMR::Graphics::Texture', 'GMR::Graphics::Tilemap']
        },
        'animation' => {
          name: 'Animation',
          description: 'Tweening, sprite animations, and easing functions.',
          modules: ['GMR::Ease'],
          classes: ['GMR::Tween', 'GMR::SpriteAnimation']
        },
        'input' => {
          name: 'Input',
          description: 'Keyboard, mouse, and gamepad input handling.',
          modules: ['GMR::Input']
        },
        'scene' => {
          name: 'Scene',
          description: 'Scene management and node hierarchy.',
          modules: ['GMR::Scene'],
          classes: ['Scene', 'SceneManager', 'Node', 'Transform2D']
        },
        'audio' => {
          name: 'Audio',
          description: 'Sound effects and music playback.',
          modules: ['GMR::Audio'],
          classes: ['GMR::Audio::Sound']
        },
        'state-machine' => {
          name: 'State Machine',
          description: 'Lightweight state machines for gameplay logic.',
          classes: ['GMR::StateMachine']
        },
        'console' => {
          name: 'Console',
          description: 'In-game developer console and REPL.',
          modules: ['GMR::Console']
        },
        'utilities' => {
          name: 'Utilities',
          description: 'Math, collision, window, time, and system utilities.',
          modules: ['GMR::Math', 'GMR::Collision', 'GMR::Window', 'GMR::Time', 'GMR::System']
        }
      }
    },
    'types' => {
      name: 'Types',
      description: 'Core data types for positions, vectors, and bounds.',
      classes: ['Vec2', 'Vec3', 'Rect']
    },
    'cli' => {
      name: 'CLI',
      description: 'Command-line interface for GMR projects.'
    }
  }.freeze

  # Module/class descriptions for documentation
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
    'GMR::System' => 'System utilities and debugging.',
    'GMR::Math' => 'Math utilities including random, distance, and angle calculations.',
    'GMR::Ease' => 'Easing functions for smooth animations.',
    'GMR::Console' => 'In-game developer console for debugging.',
    'GMR::Scene' => 'Scene lifecycle and management.',
    'GMR::Tween' => 'Value interpolation over time with easing.',
    'GMR::SpriteAnimation' => 'Frame-based sprite animations.',
    'GMR::StateMachine' => 'Finite state machine for gameplay logic.',
    'Vec2' => '2D vector for positions and directions.',
    'Vec3' => '3D vector for positions and colors.',
    'Rect' => 'Rectangle for bounds and collision areas.',
    'Sprite' => 'Drawable 2D sprite with built-in transform properties.',
    'Camera2D' => '2D camera for scrolling, zooming, and screen effects.',
    'Transform2D' => '2D transform with position, rotation, scale, and hierarchy.',
    'Node' => 'Scene graph node with hierarchical transforms.',
    'Scene' => 'Game scene with lifecycle callbacks.',
    'SceneManager' => 'Manages scene stack and transitions.'
  }.freeze

  # Represents a category in the documentation hierarchy
  class Category
    attr_reader :id, :name, :description, :path
    attr_accessor :children, :modules, :classes

    def initialize(id, name, description = nil, path = nil)
      @id = id
      @name = name
      @description = description
      @path = path || id
      @children = {}
      @modules = []
      @classes = []
    end
  end

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
          when 'parent'
            # @parent sets the namespace for the current class
            if current_class && !value.empty?
              current_class = "#{value}::#{current_class}"
            end
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
    attr_reader :modules, :classes, :constants, :globals, :cli_commands, :cli_class_options, :cli_error_codes

    def initialize
      @modules = {}
      @classes = {}
      @constants = {}
      @globals = {
        'lifecycle' => {},
        'helpers' => {}
      }
      @module_vars = {}  # Maps variable names to module paths
      @cli_commands = {}
      @cli_class_options = []
      @cli_error_codes = []
    end

    def set_cli_data(commands, class_options, error_codes)
      @cli_commands = commands
      @cli_class_options = class_options
      @cli_error_codes = error_codes
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

    # Find the category path for a module or class
    # Returns path like "engine/graphics" or "types"
    def category_for(path)
      CATEGORY_HIERARCHY.each do |cat_id, cat_config|
        # Check top-level modules/classes
        return cat_id if cat_config[:modules]&.include?(path)
        return cat_id if cat_config[:classes]&.include?(path)

        # Check children
        next unless cat_config[:children]
        cat_config[:children].each do |sub_id, sub_config|
          return "#{cat_id}/#{sub_id}" if sub_config[:modules]&.include?(path)
          return "#{cat_id}/#{sub_id}" if sub_config[:classes]&.include?(path)
        end
      end
      nil # uncategorized
    end

    # Get category info by path (e.g., "engine/graphics")
    def category_info(cat_path)
      parts = cat_path.split('/')
      config = CATEGORY_HIERARCHY[parts[0]]
      return nil unless config

      if parts.length > 1 && config[:children]
        config = config[:children][parts[1]]
      end
      config
    end

    # Group modules and classes by their category
    def grouped_by_category
      result = {}

      @modules.each do |path, data|
        cat = category_for(path) || 'uncategorized'
        result[cat] ||= { modules: {}, classes: {} }
        result[cat][:modules][path] = data
      end

      @classes.each do |path, data|
        cat = category_for(path) || 'uncategorized'
        result[cat] ||= { modules: {}, classes: {} }
        result[cat][:classes][path] = data
      end

      result
    end

    # Build the full category tree structure
    def build_category_tree
      build_tree_recursive(CATEGORY_HIERARCHY)
    end

    private

    def build_tree_recursive(hierarchy, parent_path = nil)
      categories = {}
      hierarchy.each do |id, config|
        path = parent_path ? "#{parent_path}/#{id}" : id
        cat = Category.new(id, config[:name], config[:description], path)
        cat.modules = config[:modules] || []
        cat.classes = config[:classes] || []

        if config[:children]
          cat.children = build_tree_recursive(config[:children], path)
        end

        categories[id] = cat
      end
      categories
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

  # Builds navigation tree from model for docs output
  class NavigationBuilder
    def initialize(model)
      @model = model
    end

    # Build the complete navigation tree
    def build_tree
      {
        id: 'root',
        label: 'GMR Documentation',
        path: 'README.md',
        type: :root,
        children: build_top_level
      }
    end

    private

    def build_top_level
      nodes = []

      CATEGORY_HIERARCHY.each do |cat_id, cat_config|
        nodes << build_category_node(cat_id, cat_config)
      end

      nodes
    end

    def build_category_node(cat_id, cat_config, parent_path = nil)
      path = parent_path ? "#{parent_path}/#{cat_id}" : cat_id
      node = {
        id: cat_id,
        label: cat_config[:name],
        path: "#{path}/README.md",
        type: :category,
        description: cat_config[:description],
        children: []
      }

      # Add child categories first
      if cat_config[:children]
        cat_config[:children].each do |sub_id, sub_config|
          node[:children] << build_category_node(sub_id, sub_config, path)
        end
      end

      # Add modules in this category
      (cat_config[:modules] || []).each do |mod_path|
        mod_data = @model.modules[mod_path]
        next unless mod_data
        node[:children] << build_module_node(mod_path, mod_data, path)
      end

      # Add classes in this category
      (cat_config[:classes] || []).each do |class_path|
        class_data = @model.classes[class_path]
        next unless class_data
        node[:children] << build_class_node(class_path, class_data, path)
      end

      node
    end

    def build_module_node(mod_path, mod_data, cat_path)
      name = mod_path.split('::').last
      filename = name.downcase.gsub('::', '-')

      node = {
        id: path_to_id(mod_path),
        label: mod_path,
        path: "#{cat_path}/#{filename}.md",
        type: :module,
        description: MODULE_DESCRIPTIONS[mod_path] || mod_data['description'],
        children: []
      }

      # Add functions as leaf nodes
      (mod_data['functions'] || {}).keys.sort.each do |func_name|
        node[:children] << {
          id: "#{path_to_id(mod_path)}-#{func_name}",
          label: func_name,
          anchor: "##{func_name.gsub('?', '').gsub('!', '').gsub('=', '')}",
          type: :function
        }
      end

      node
    end

    def build_class_node(class_path, class_data, cat_path)
      name = class_path.split('::').last
      filename = name.downcase.gsub('::', '-')

      node = {
        id: path_to_id(class_path),
        label: name,
        path: "#{cat_path}/#{filename}.md",
        type: :class,
        description: MODULE_DESCRIPTIONS[class_path] || class_data['description'],
        children: []
      }

      # Add class methods
      (class_data['classMethods'] || {}).keys.sort.each do |method_name|
        node[:children] << {
          id: "#{path_to_id(class_path)}-#{method_name}",
          label: ".#{method_name}",
          anchor: "##{name.downcase}-#{method_name.gsub('?', '').gsub('!', '').gsub('=', '')}",
          type: :class_method
        }
      end

      # Add instance methods
      (class_data['instanceMethods'] || {}).keys.sort.each do |method_name|
        node[:children] << {
          id: "#{path_to_id(class_path)}-#{method_name}",
          label: "##{method_name}",
          anchor: "##{name.downcase}-#{method_name.gsub('?', '').gsub('!', '').gsub('=', '')}",
          type: :instance_method
        }
      end

      node
    end

    def path_to_id(path)
      path.downcase.gsub('::', '-')
    end
  end

  # Generates Markdown documentation files with hierarchical structure
  class MarkdownGenerator
    def initialize(options)
      @options = options
    end

    def generate(model, output_dir)
      @model = model
      @output_dir = output_dir
      @nav_builder = NavigationBuilder.new(model)

      FileUtils.mkdir_p(output_dir)

      # Generate landing page
      generate_landing_page

      # Generate category structure
      generate_categories(CATEGORY_HIERARCHY)

      puts "  Markdown generation complete"
    end

    private

    # Generate the main landing page
    def generate_landing_page
      path = File.join(@output_dir, 'README.md')
      File.open(path, 'w') do |f|
        f.puts "# GMR Documentation"
        f.puts
        f.puts "Welcome to the GMR game engine documentation."
        f.puts
        f.puts "**Version:** #{@options[:engine_version]}"
        f.puts
        f.puts "## Quick Start"
        f.puts
        f.puts "```ruby"
        f.puts "# main.rb - A simple GMR game"
        f.puts "include GMR"
        f.puts ""
        f.puts "def init"
        f.puts "  @sprite = Sprite.new(\"assets/player.png\")"
        f.puts "  @sprite.x = 160"
        f.puts "  @sprite.y = 120"
        f.puts "end"
        f.puts ""
        f.puts "def update"
        f.puts "  if Input.key_down?(:right)"
        f.puts "    @sprite.x += 2"
        f.puts "  end"
        f.puts "end"
        f.puts ""
        f.puts "def draw"
        f.puts "  Graphics.clear([20, 20, 40])"
        f.puts "  @sprite.draw"
        f.puts "end"
        f.puts "```"
        f.puts
        f.puts "## Documentation"
        f.puts
        f.puts "| Section | Description |"
        f.puts "|---------|-------------|"

        CATEGORY_HIERARCHY.each do |cat_id, cat_config|
          f.puts "| [#{cat_config[:name]}](#{cat_id}/README.md) | #{cat_config[:description]} |"
        end

        f.puts
        f.puts "## Built-in Types"
        f.puts
        f.puts "| Type | Description |"
        f.puts "|------|-------------|"
        TYPE_DEFINITIONS.each do |type_name, type_def|
          f.puts "| `#{type_name}` | #{type_def['description']} |"
        end
        f.puts
        f.puts "---"
        f.puts
        f.puts "*Generated by GMRCli Docs from C++ bindings.*"
      end
      puts "  Generated: #{path}"
    end

    # Recursively generate category structure
    def generate_categories(hierarchy, parent_path = nil, depth = 0)
      hierarchy.each do |cat_id, cat_config|
        cat_path = parent_path ? "#{parent_path}/#{cat_id}" : cat_id
        dir_path = File.join(@output_dir, cat_path)
        FileUtils.mkdir_p(dir_path)

        # Generate category README
        generate_category_readme(cat_id, cat_config, cat_path, depth)

        # Generate child categories
        if cat_config[:children]
          generate_categories(cat_config[:children], cat_path, depth + 1)
        end

        # Generate module pages
        (cat_config[:modules] || []).each do |mod_path|
          mod_data = @model.modules[mod_path]
          generate_module_page(mod_path, mod_data, cat_path, cat_config) if mod_data
        end

        # Generate class pages
        (cat_config[:classes] || []).each do |class_path|
          class_data = @model.classes[class_path]
          generate_class_page(class_path, class_data, cat_path, cat_config) if class_data
        end
      end
    end

    # Generate a category README
    def generate_category_readme(cat_id, cat_config, cat_path, depth)
      path = File.join(@output_dir, cat_path, 'README.md')
      File.open(path, 'w') do |f|
        # Breadcrumbs
        f.puts breadcrumbs(cat_path)
        f.puts

        # Title
        f.puts "# #{cat_config[:name]}"
        f.puts
        f.puts cat_config[:description] if cat_config[:description]
        f.puts

        # Child categories
        if cat_config[:children]&.any?
          f.puts "## Categories"
          f.puts
          cat_config[:children].each do |sub_id, sub_config|
            f.puts "- [#{sub_config[:name]}](#{sub_id}/README.md) - #{sub_config[:description]}"
          end
          f.puts
        end

        # Modules in this category
        if cat_config[:modules]&.any?
          f.puts "## Modules"
          f.puts
          cat_config[:modules].each do |mod_path|
            mod_data = @model.modules[mod_path]
            next unless mod_data
            name = mod_path.split('::').last
            filename = name.downcase
            desc = MODULE_DESCRIPTIONS[mod_path] || mod_data['description'] || 'API reference'
            f.puts "- [#{mod_path}](#{filename}.md) - #{desc}"
          end
          f.puts
        end

        # Classes in this category
        if cat_config[:classes]&.any?
          f.puts "## Classes"
          f.puts
          cat_config[:classes].each do |class_path|
            class_data = @model.classes[class_path]
            next unless class_data
            name = class_path.split('::').last
            filename = name.downcase
            desc = MODULE_DESCRIPTIONS[class_path] || class_data['description'] || 'Class reference'
            f.puts "- [#{name}](#{filename}.md) - #{desc}"
          end
          f.puts
        end

        # Navigation footer
        f.puts "---"
        f.puts
        back_path = depth > 0 ? '../README.md' : '../README.md'
        home_path = '../' * (depth + 1) + 'README.md'
        if depth > 0
          f.puts "[Back](../README.md) | [Documentation Home](#{home_path})"
        else
          f.puts "[Documentation Home](../README.md)"
        end
      end
      puts "  Generated: #{path}"
    end

    # Generate a module documentation page
    def generate_module_page(mod_path, mod_data, cat_path, cat_config)
      name = mod_path.split('::').last
      filename = name.downcase + '.md'
      path = File.join(@output_dir, cat_path, filename)

      File.open(path, 'w') do |f|
        # Breadcrumbs
        f.puts breadcrumbs(cat_path, name)
        f.puts

        # Title
        f.puts "# #{mod_path}"
        f.puts
        f.puts MODULE_DESCRIPTIONS[mod_path] || mod_data['description'] || "API reference for #{mod_path}."
        f.puts

        # Table of contents
        toc = generate_toc(mod_data)
        if toc
          f.puts toc
          f.puts
        end

        # Functions
        if mod_data['functions']&.any?
          f.puts "## Functions"
          f.puts
          mod_data['functions'].each do |func_name, func|
            write_function(f, func_name, func)
          end
        end

        # Navigation footer
        f.puts "---"
        f.puts
        f.puts "[Back to #{cat_config[:name]}](README.md) | [Documentation Home](#{'../' * (cat_path.count('/') + 2)}README.md)"
      end
      puts "  Generated: #{path}"
    end

    # Generate a class documentation page
    def generate_class_page(class_path, class_data, cat_path, cat_config)
      name = class_path.split('::').last
      filename = name.downcase + '.md'
      path = File.join(@output_dir, cat_path, filename)

      File.open(path, 'w') do |f|
        # Breadcrumbs
        f.puts breadcrumbs(cat_path, name)
        f.puts

        # Title
        f.puts "# #{name}"
        f.puts
        f.puts MODULE_DESCRIPTIONS[class_path] || class_data['description'] || "Class reference for #{name}."
        f.puts

        # Table of contents
        toc = generate_class_toc(class_data)
        if toc
          f.puts toc
          f.puts
        end

        # Class methods
        if class_data['classMethods']&.any?
          f.puts "## Class Methods"
          f.puts
          class_data['classMethods'].each do |method_name, method|
            write_function(f, ".#{method_name}", method, name)
          end
        end

        # Instance methods
        if class_data['instanceMethods']&.any?
          f.puts "## Instance Methods"
          f.puts
          class_data['instanceMethods'].each do |method_name, method|
            write_function(f, "##{method_name}", method, name)
          end
        end

        # Navigation footer
        f.puts "---"
        f.puts
        f.puts "[Back to #{cat_config[:name]}](README.md) | [Documentation Home](#{'../' * (cat_path.count('/') + 2)}README.md)"
      end
      puts "  Generated: #{path}"
    end

    # Generate breadcrumbs
    def breadcrumbs(cat_path, current = nil)
      parts = cat_path.split('/')
      crumbs = ["[GMR Docs](#{'../' * (parts.length + 1)}README.md)"]

      # Build up path for each part
      parts.each_with_index do |part, i|
        config = get_category_config(parts[0..i].join('/'))
        label = config ? config[:name] : part.capitalize
        rel_path = '../' * (parts.length - i) + parts[i] + '/README.md'
        if i == parts.length - 1 && current.nil?
          crumbs << "**#{label}**"
        else
          crumbs << "[#{label}](#{rel_path})"
        end
      end

      crumbs << "**#{current}**" if current

      crumbs.join(' > ')
    end

    # Get category config by path
    def get_category_config(path)
      parts = path.split('/')
      config = CATEGORY_HIERARCHY[parts[0]]
      return nil unless config

      parts[1..].each do |part|
        return nil unless config[:children]
        config = config[:children][part]
        return nil unless config
      end
      config
    end

    # Generate table of contents for module
    def generate_toc(mod_data)
      items = []

      if mod_data['functions']&.any?
        items << "- [Functions](#functions)"
        mod_data['functions'].keys.sort.each do |name|
          anchor = name.gsub('?', '').gsub('!', '').gsub('=', '').downcase
          items << "  - [#{name}](##{anchor})"
        end
      end

      return nil if items.length < 4

      "## Table of Contents\n\n#{items.join("\n")}"
    end

    # Generate table of contents for class
    def generate_class_toc(class_data)
      items = []

      if class_data['classMethods']&.any?
        items << "- [Class Methods](#class-methods)"
        class_data['classMethods'].keys.sort.each do |name|
          anchor = name.gsub('?', '').gsub('!', '').gsub('=', '').downcase
          items << "  - [.#{name}](##{anchor})"
        end
      end

      if class_data['instanceMethods']&.any?
        items << "- [Instance Methods](#instance-methods)"
        class_data['instanceMethods'].keys.sort.each do |name|
          anchor = name.gsub('?', '').gsub('!', '').gsub('=', '').downcase
          items << "  - [##{name}](##{anchor})"
        end
      end

      return nil if items.length < 4

      "## Table of Contents\n\n#{items.join("\n")}"
    end

    # Write a function/method to the file
    def write_function(f, name, func, class_name = nil)
      signature = func['signature'] || name
      anchor_name = name.gsub(/^[.#]/, '').gsub('?', '').gsub('!', '').gsub('=', '').downcase

      f.puts "<a id=\"#{anchor_name}\"></a>"
      f.puts
      f.puts "### #{signature}"
      f.puts

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
          type_str += ", default: `#{param['default']}`" if param['default']
          f.puts "| `#{param['name']}` | `#{type_str}` | #{param['description'] || ''} |"
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
        f.puts
        f.puts "```ruby"
        f.puts func['example']
        f.puts "```"
        f.puts
      end

      f.puts "---"
      f.puts
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

    # Ruby syntax highlighter - uses CodeRay if available, falls back to custom highlighter
    def self.highlight_ruby(code)
      return '' unless code

      if CODERAY_AVAILABLE
        # Use CodeRay for high-quality syntax highlighting
        # Output as HTML spans with CodeRay's token classes
        CodeRay.scan(code.to_s, :ruby).html(css: :class, line_numbers: false)
      else
        # Fallback to our simple highlighter
        highlight_ruby_fallback(code)
      end
    end

    # Fallback highlighter when CodeRay is not available
    # Uses placeholder tokens to avoid regex conflicts with generated HTML
    def self.highlight_ruby_fallback(code)
      return '' unless code

      # Escape HTML first
      escaped = code.to_s
        .gsub('&', '&amp;')
        .gsub('<', '&lt;')
        .gsub('>', '&gt;')

      # Use placeholder tokens to avoid regex matching inside generated spans
      tokens = []
      token_id = 0

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
        background-color: var(--bg-primary);
        color: var(--text-primary);
        line-height: 1.6;
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

      .label-default {
        background: var(--accent-green);
        color: var(--bg-primary);
      }

      .label-alias {
        background: var(--accent-magenta);
        color: var(--bg-primary);
      }

      /* Stage list for CLI commands */
      .stage-list {
        list-style: none;
        counter-reset: stage;
        margin: 1rem 0;
      }

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

      /* Sidebar */
      .layout {
        display: flex;
        gap: 2rem;
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
        .nav-content { flex-direction: column; gap: 1rem; }
      }

      /* Syntax Highlighting - Fallback custom highlighter */
      pre code .hl-keyword { color: #bb9af7; font-weight: 500; }
      pre code .hl-string { color: #9ece6a; }
      pre code .hl-number { color: #ff9e64; }
      pre code .hl-comment { color: #565f89; font-style: italic; }
      pre code .hl-symbol { color: #7dcfff; }
      pre code .hl-class { color: #7aa2f7; }
      pre code .hl-method { color: #7aa2f7; }
      pre code .hl-ivar { color: #f7768e; }
      pre code .hl-gvar { color: #f7768e; }
      pre code .hl-const { color: #ff9e64; }

      /* CodeRay Syntax Highlighting - Tokyo Night theme */
      /* More specific selectors to override pre code color */
      pre code .CodeRay { background: transparent; color: var(--text-primary); }
      pre code .CodeRay .keyword { color: #bb9af7; font-weight: 500; }
      pre code .CodeRay .reserved { color: #bb9af7; font-weight: 500; }
      pre code .CodeRay .string { color: #9ece6a; }
      pre code .CodeRay .char { color: #9ece6a; }
      pre code .CodeRay .content { color: #9ece6a; }
      pre code .CodeRay .delimiter { color: #89ddff; }
      pre code .CodeRay .integer { color: #ff9e64; }
      pre code .CodeRay .float { color: #ff9e64; }
      pre code .CodeRay .comment { color: #565f89; font-style: italic; }
      pre code .CodeRay .symbol { color: #7dcfff; }
      pre code .CodeRay .class { color: #7aa2f7; font-weight: 500; }
      pre code .CodeRay .constant { color: #ff9e64; }
      pre code .CodeRay .function { color: #7aa2f7; }
      pre code .CodeRay .method { color: #7aa2f7; }
      pre code .CodeRay .instance-variable { color: #f7768e; }
      pre code .CodeRay .global-variable { color: #f7768e; }
      pre code .CodeRay .class-variable { color: #f7768e; }
      pre code .CodeRay .predefined { color: #7dcfff; }
      pre code .CodeRay .predefined-constant { color: #ff9e64; }
      pre code .CodeRay .predefined-type { color: #7aa2f7; }
      pre code .CodeRay .operator { color: #89ddff; }
      pre code .CodeRay .shell { color: #9ece6a; }
      pre code .CodeRay .regexp { color: #f7768e; }
      pre code .CodeRay .key { color: #7dcfff; }
      pre code .CodeRay .value { color: var(--text-primary); }
      pre code .CodeRay .escape { color: #ff9e64; }
      pre code .CodeRay .inline { color: #e0af68; }
      pre code .CodeRay .ident { color: var(--text-primary); }
    CSS

    # Additional CSS for single-file layout
    SINGLE_FILE_CSS = <<~CSS
      /* ===== Single-file Layout ===== */
      html, body {
        height: 100%;
        margin: 0;
        padding: 0;
        overflow: hidden;
      }

      .doc-container {
        display: flex;
        height: 100vh;
        width: 100%;
      }

      /* ===== Sidebar ===== */
      .sidebar {
        width: 280px;
        height: 100%;
        background: var(--bg-secondary);
        border-right: 1px solid var(--border-color);
        display: flex;
        flex-direction: column;
        flex-shrink: 0;
      }

      .sidebar-header {
        padding: 1.5rem;
        border-bottom: 1px solid var(--border-color);
        flex-shrink: 0;
      }

      .logo {
        font-size: 1.5rem;
        font-weight: 700;
        color: var(--accent-magenta);
        margin: 0;
      }

      .version {
        color: var(--text-muted);
        font-size: 0.85rem;
      }

      .search-container {
        padding: 1rem 1.5rem;
        border-bottom: 1px solid var(--border-color);
        flex-shrink: 0;
        position: relative;
      }

      #search-input {
        width: 100%;
        padding: 0.5rem 0.75rem;
        background: var(--bg-primary);
        border: 1px solid var(--border-color);
        border-radius: 4px;
        color: var(--text-primary);
        font-size: 0.9rem;
      }

      #search-input:focus {
        outline: none;
        border-color: var(--accent-blue);
      }

      #search-input::placeholder {
        color: var(--text-muted);
      }

      .search-results {
        display: none;
        position: absolute;
        top: 100%;
        left: 1.5rem;
        right: 1.5rem;
        background: var(--bg-primary);
        border: 1px solid var(--border-color);
        border-top: none;
        border-radius: 0 0 4px 4px;
        max-height: 300px;
        overflow-y: auto;
        z-index: 200;
      }

      .search-results.active {
        display: block;
      }

      .search-result-item {
        padding: 0.5rem 0.75rem;
        cursor: pointer;
        border-bottom: 1px solid var(--border-color);
      }

      .search-result-item:last-child {
        border-bottom: none;
      }

      .search-result-item:hover {
        background: var(--bg-tertiary);
      }

      .search-result-item .result-name {
        color: var(--accent-blue);
        font-weight: 500;
      }

      .search-result-item .result-path {
        color: var(--text-muted);
        font-size: 0.75rem;
      }

      /* ===== Navigation Tree ===== */
      .nav-tree {
        flex: 1;
        overflow-y: auto;
        padding: 0.5rem 0;
      }

      .nav-tree::-webkit-scrollbar {
        width: 8px;
      }

      .nav-tree::-webkit-scrollbar-track {
        background: var(--bg-secondary);
      }

      .nav-tree::-webkit-scrollbar-thumb {
        background: var(--bg-tertiary);
        border-radius: 4px;
      }

      .nav-tree::-webkit-scrollbar-thumb:hover {
        background: var(--text-muted);
      }

      .nav-category {
        margin-bottom: 0.25rem;
      }

      .nav-category-header {
        display: flex;
        align-items: center;
        padding: 0.5rem 1rem;
        cursor: pointer;
        color: var(--text-primary);
        font-weight: 600;
        font-size: 0.9rem;
        transition: background 0.15s;
        user-select: none;
      }

      .nav-category-header:hover {
        background: var(--bg-tertiary);
      }

      .nav-toggle {
        width: 16px;
        height: 16px;
        margin-right: 0.5rem;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 0.7rem;
        color: var(--text-muted);
        transition: transform 0.2s;
      }

      .nav-toggle.collapsed {
        transform: rotate(-90deg);
      }

      .nav-children {
        overflow: hidden;
        max-height: 2000px;
        transition: max-height 0.3s ease-out;
      }

      .nav-children.collapsed {
        max-height: 0;
      }

      .nav-item {
        display: block;
        padding: 0.35rem 1rem 0.35rem 2.5rem;
        color: var(--text-secondary);
        font-size: 0.85rem;
        text-decoration: none;
        transition: background 0.15s, color 0.15s;
        cursor: pointer;
      }

      .nav-item:hover {
        background: var(--bg-tertiary);
        color: var(--text-primary);
      }

      .nav-item.active {
        background: var(--bg-tertiary);
        color: var(--accent-blue);
        border-left: 3px solid var(--accent-blue);
        padding-left: calc(2.5rem - 3px);
      }

      .nav-item.hidden {
        display: none;
      }

      .nav-category.hidden {
        display: none;
      }

      .nav-item.method {
        padding-left: 3rem;
        font-size: 0.8rem;
        color: var(--text-muted);
      }

      .nav-item.method:hover {
        color: var(--text-secondary);
      }

      /* Depth-based indentation - consistent hierarchy */
      .nav-category.depth-0 > .nav-category-header { padding-left: 1rem; }
      .nav-category.depth-1 > .nav-category-header { padding-left: 2rem; }
      .nav-category.depth-2 > .nav-category-header { padding-left: 3rem; }
      .nav-category.depth-3 > .nav-category-header { padding-left: 4rem; }

      .nav-item.depth-0 { padding-left: 1rem; }
      .nav-item.depth-1 { padding-left: 2rem; }
      .nav-item.depth-2 { padding-left: 3rem; }
      .nav-item.depth-3 { padding-left: 4rem; }
      .nav-item.depth-4 { padding-left: 5rem; }

      /* Active states need to account for border */
      .nav-item.depth-0.active { padding-left: calc(1rem - 3px); }
      .nav-item.depth-1.active { padding-left: calc(2rem - 3px); }
      .nav-item.depth-2.active { padding-left: calc(3rem - 3px); }
      .nav-item.depth-3.active { padding-left: calc(4rem - 3px); }
      .nav-item.depth-4.active { padding-left: calc(5rem - 3px); }

      /* ===== Main Content ===== */
      .main-content {
        flex: 1;
        height: 100%;
        overflow-y: auto;
        padding: 2rem 3rem;
      }

      .doc-section {
        display: none;
        max-width: 900px;
      }

      .doc-section.active {
        display: block;
      }

      /* Breadcrumbs */
      .breadcrumbs {
        display: flex;
        align-items: center;
        gap: 0.5rem;
        margin-bottom: 1.5rem;
        font-size: 0.9rem;
        color: var(--text-muted);
      }

      .breadcrumbs a {
        color: var(--accent-blue);
        text-decoration: none;
        cursor: pointer;
      }

      .breadcrumbs a:hover {
        text-decoration: underline;
      }

      .breadcrumbs .separator {
        color: var(--text-muted);
      }

      .breadcrumbs .current {
        color: var(--text-primary);
        font-weight: 500;
      }

      /* ===== Mobile Menu Button ===== */
      .mobile-menu-btn {
        display: none;
      }

      /* ===== Mobile Responsiveness ===== */
      @media (max-width: 900px) {
        .sidebar {
          position: fixed;
          left: 0;
          top: 0;
          z-index: 100;
          transform: translateX(-100%);
          transition: transform 0.3s;
        }

        .sidebar.open {
          transform: translateX(0);
        }

        .main-content {
          padding: 1.5rem;
          width: 100%;
        }

        .mobile-menu-btn {
          display: block;
          position: fixed;
          top: 1rem;
          left: 1rem;
          z-index: 101;
          background: var(--bg-secondary);
          border: 1px solid var(--border-color);
          border-radius: 4px;
          padding: 0.5rem;
          color: var(--text-primary);
          cursor: pointer;
        }
      }
    CSS

    def initialize(options)
      @options = options
    end

    def generate(model, output_dir)
      @model = model
      FileUtils.mkdir_p(output_dir)

      # Generate single-file HTML
      path = File.join(output_dir, 'index.html')
      File.open(path, 'w') do |f|
        f.puts generate_single_file_html
      end

      puts "  Generated: #{path}"
      puts "  HTML generation complete (single-file)"
    end

    private

    def html_escape(text)
      return '' unless text
      text.to_s
        .gsub('&', '&amp;')
        .gsub('<', '&lt;')
        .gsub('>', '&gt;')
        .gsub('"', '&quot;')
    end

    # Generate breadcrumb HTML for a module or class
    def breadcrumbs_html(path, display_name)
      # Find the category for this path
      cat_path = @model.category_for(path)
      return '' unless cat_path

      crumbs = ['<a data-section="index">Home</a>']

      if cat_path.include?('/')
        # e.g., "engine/graphics"
        parts = cat_path.split('/')
        parent_cat = CATEGORY_HIERARCHY[parts[0]]
        if parent_cat
          crumbs << "<a data-section=\"#{parts[0]}\">#{html_escape(parent_cat[:name])}</a>"
          if parent_cat[:children] && parent_cat[:children][parts[1]]
            sub_cat = parent_cat[:children][parts[1]]
            crumbs << "<span class=\"current\">#{html_escape(sub_cat[:name])}</span>"
          end
        end
      else
        # Top-level category like "types"
        parent_cat = CATEGORY_HIERARCHY[cat_path]
        if parent_cat
          crumbs << "<span class=\"current\">#{html_escape(parent_cat[:name])}</span>"
        end
      end

      '<nav class="breadcrumbs">' + crumbs.join('<span class="separator">&rsaquo;</span>') + '</nav>'
    end

    def generate_single_file_html
      nav_tree = build_nav_tree
      sections = build_all_sections

      html = []
      html << '<!DOCTYPE html>'
      html << '<html lang="en">'
      html << '<head>'
      html << '<meta charset="UTF-8">'
      html << '<meta name="viewport" content="width=device-width, initial-scale=1.0">'
      html << '<title>GMR API Documentation</title>'
      html << '<style>'
      html << DARK_THEME_CSS
      html << SINGLE_FILE_CSS
      html << '</style>'
      html << '</head>'
      html << '<body>'
      html << '<div class="doc-container">'
      html << '<aside class="sidebar">'
      html << '<div class="sidebar-header">'
      html << '<h1 class="logo">GMR</h1>'
      html << "<div class=\"version\">v#{@options[:engine_version]}</div>"
      html << '</div>'
      html << '<div class="search-container">'
      html << '<input type="text" id="search-input" placeholder="Search..." />'
      html << '</div>'
      html << "<nav class=\"nav-tree\" id=\"nav-tree\">#{nav_tree}</nav>"
      html << '</aside>'
      html << '<main class="main-content">'
      html << sections
      html << '</main>'
      html << '</div>'
      html << '<script>'
      html << inline_javascript
      html << '</script>'
      html << '</body>'
      html << '</html>'

      html.join("\n")
    end

    def build_nav_tree
      html = ''

      # Home link
      html += '<div class="nav-item depth-0" data-section="index">Home</div>'

      # Build navigation from category hierarchy
      CATEGORY_HIERARCHY.each do |cat_id, cat_config|
        html += build_nav_category(cat_id, cat_config)
      end

      html
    end

    def build_nav_category(cat_id, cat_config, parent_path = nil, depth = 0)
      path = parent_path ? "#{parent_path}-#{cat_id}" : cat_id
      html = "<div class=\"nav-category depth-#{depth}\">"

      # Category header - clickable to show category overview
      # Collapse sub-categories (depth > 0) by default
      toggle_collapsed = depth > 0 ? ' collapsed' : ''
      html += "<div class=\"nav-category-header\" data-section=\"#{path}\" data-toggle=\"#{path}\">"
      html += "<span class=\"nav-toggle#{toggle_collapsed}\">&#9662;</span>"
      html += "#{html_escape(cat_config[:name])}"
      html += '</div>'

      # Children container - collapsed by default for sub-categories
      children_collapsed = depth > 0 ? ' collapsed' : ''
      html += "<div class=\"nav-children#{children_collapsed}\" id=\"nav-#{path}\">"

      # Child categories
      if cat_config[:children]
        cat_config[:children].each do |sub_id, sub_config|
          html += build_nav_category(sub_id, sub_config, path, depth + 1)
        end
      end

      # Modules
      (cat_config[:modules] || []).each do |mod_path|
        mod_data = @model.modules[mod_path]
        next unless mod_data

        section_id = mod_path.downcase.gsub('::', '-')
        html += "<div class=\"nav-item depth-#{depth + 1}\" data-section=\"#{section_id}\">#{html_escape(mod_path)}</div>"
      end

      # Classes
      (cat_config[:classes] || []).each do |class_path|
        class_data = @model.classes[class_path]
        next unless class_data

        name = class_path.split('::').last
        section_id = class_path.downcase.gsub('::', '-')
        html += "<div class=\"nav-item depth-#{depth + 1}\" data-section=\"#{section_id}\">#{html_escape(name)}</div>"
      end

      # CLI commands (only for 'cli' category)
      if cat_id == 'cli' && @model.cli_commands.any?
        @model.cli_commands.each do |name, cmd|
          section_id = "cli-#{name}"
          label = name
          label += " (default)" if cmd.default_task
          html += "<div class=\"nav-item depth-#{depth + 1}\" data-section=\"#{section_id}\">#{html_escape(label)}</div>"
        end
        # Error codes
        if @model.cli_error_codes.any?
          html += "<div class=\"nav-item depth-#{depth + 1}\" data-section=\"cli-error-codes\">Error Codes</div>"
        end
      end

      html += '</div></div>'
      html
    end

    def build_all_sections
      sections = []

      # Index/home section
      sections << build_index_section

      # Build category overview sections
      CATEGORY_HIERARCHY.each do |cat_id, cat_config|
        sections << build_category_sections(cat_id, cat_config)
      end

      # Build sections for each module
      @model.modules.each do |mod_path, mod_data|
        sections << build_module_section(mod_path, mod_data)
      end

      # Build sections for each class
      @model.classes.each do |class_path, class_data|
        sections << build_class_section(class_path, class_data)
      end

      # Build sections for CLI commands
      @model.cli_commands.each do |name, cmd|
        sections << build_cli_command_section(name, cmd)
      end

      # Build error codes section
      if @model.cli_error_codes.any?
        sections << build_cli_error_codes_section
      end

      sections.join("\n")
    end

    def build_category_sections(cat_id, cat_config, parent_path = nil)
      path = parent_path ? "#{parent_path}-#{cat_id}" : cat_id
      sections = []

      # Build this category's overview section
      sections << build_category_overview(path, cat_config)

      # Recursively build child category sections
      if cat_config[:children]
        cat_config[:children].each do |sub_id, sub_config|
          sections << build_category_sections(sub_id, sub_config, path)
        end
      end

      sections.join("\n")
    end

    def build_category_overview(section_id, cat_config)
      html = []
      html << "<section class=\"doc-section\" id=\"#{section_id}\">"
      html << "<h1>#{html_escape(cat_config[:name])}</h1>"
      html << "<p>#{html_escape(cat_config[:description] || "#{cat_config[:name]} documentation.")}</p>"

      # List child categories
      if cat_config[:children] && !cat_config[:children].empty?
        html << "<h2>Categories</h2>"
        html << "<div class=\"card-grid\">"
        cat_config[:children].each do |sub_id, sub_config|
          sub_path = "#{section_id}-#{sub_id}"
          html << "<div class=\"card\">"
          html << "<h3><a href=\"#\" data-section=\"#{sub_path}\">#{html_escape(sub_config[:name])}</a></h3>"
          html << "<p>#{html_escape(sub_config[:description] || '')}</p>"
          html << "</div>"
        end
        html << "</div>"
      end

      # List modules
      modules = cat_config[:modules] || []
      if !modules.empty?
        html << "<h2>Modules</h2>"
        html << "<ul>"
        modules.each do |mod_path|
          mod_data = @model.modules[mod_path]
          next unless mod_data
          mod_section_id = mod_path.downcase.gsub('::', '-')
          html << "<li><a href=\"#\" data-section=\"#{mod_section_id}\">#{html_escape(mod_path)}</a>"
          html << " - #{html_escape(mod_data[:description] || '')}</li>"
        end
        html << "</ul>"
      end

      # List classes
      classes = cat_config[:classes] || []
      if !classes.empty?
        html << "<h2>Classes</h2>"
        html << "<ul>"
        classes.each do |class_path|
          class_data = @model.classes[class_path]
          next unless class_data
          class_section_id = class_path.downcase.gsub('::', '-')
          name = class_path.split('::').last
          html << "<li><a href=\"#\" data-section=\"#{class_section_id}\">#{html_escape(name)}</a>"
          html << " - #{html_escape(class_data[:description] || '')}</li>"
        end
        html << "</ul>"
      end

      # List CLI commands (only for 'cli' section)
      if section_id == 'cli' && @model.cli_commands.any?
        html << "<h2>Commands</h2>"
        html << "<div class=\"card-grid\">"
        @model.cli_commands.each do |name, cmd|
          cmd_section_id = "cli-#{name}"
          badges = []
          badges << '<span class="label label-default">default</span>' if cmd.default_task
          cmd.aliases.each { |a| badges << "<span class=\"label label-alias\">#{html_escape(a)}</span>" }
          badge_str = badges.any? ? " #{badges.join(' ')}" : ''
          html << "<div class=\"card\">"
          html << "<h3><a href=\"#\" data-section=\"#{cmd_section_id}\">#{html_escape(name)}</a>#{badge_str}</h3>"
          html << "<p>#{html_escape(cmd.description || '')}</p>"
          html << "</div>"
        end
        html << "</div>"

        # Global options
        if @model.cli_class_options.any?
          html << "<h2>Global Options</h2>"
          html << "<p>These options are available for all commands:</p>"
          html << "<table>"
          html << "<thead><tr><th>Option</th><th>Type</th><th>Default</th><th>Description</th></tr></thead>"
          html << "<tbody>"
          @model.cli_class_options.each do |opt|
            alias_str = opt.aliases.first ? " (<code>#{html_escape(opt.aliases.first)}</code>)" : ''
            default = opt.default.nil? ? '-' : "<code>#{html_escape(opt.default.to_s)}</code>"
            html << "<tr><td><code>--#{html_escape(opt.name)}</code>#{alias_str}</td>"
            html << "<td><span class=\"label label-type\">#{opt.type}</span></td>"
            html << "<td>#{default}</td>"
            html << "<td>#{html_escape(opt.description)}</td></tr>"
          end
          html << "</tbody></table>"
        end

        # Link to error codes
        if @model.cli_error_codes.any?
          html << "<h2>See Also</h2>"
          html << "<ul>"
          html << "<li><a href=\"#\" data-section=\"cli-error-codes\">Error Codes</a> - Machine-readable error codes</li>"
          html << "</ul>"
        end
      end

      html << "</section>"
      html.join("\n")
    end

    def build_index_section
      <<~HTML
        <section class="doc-section active" id="index">
          <h1>GMR API Documentation</h1>
          <p>Welcome to the GMR game engine API reference. Use the sidebar to navigate.</p>

          <h2>Quick Start</h2>
          <pre><code class="language-ruby">#{HTMLGenerator.highlight_ruby(quick_start_example)}</code></pre>

          <h2>Engine Modules</h2>
          <div class="card-grid">
            #{build_category_cards}
          </div>

          <h2>Built-in Types</h2>
          <table>
            <thead><tr><th>Type</th><th>Description</th></tr></thead>
            <tbody>
              #{TYPE_DEFINITIONS.map { |name, def_| "<tr><td><code>#{html_escape(name)}</code></td><td>#{html_escape(def_['description'])}</td></tr>" }.join("\n              ")}
            </tbody>
          </table>

          <footer class="footer">
            <p>Generated by GMRCli Docs &bull; GMR #{@options[:engine_version]}</p>
          </footer>
        </section>
      HTML
    end

    def quick_start_example
      <<~RUBY
        # main.rb - A simple GMR game
        include GMR

        def init
          @sprite = Sprite.new("assets/player.png")
          @sprite.x = 160
          @sprite.y = 120
        end

        def update
          if Input.key_down?(:right)
            @sprite.x += 2
          end
        end

        def draw
          Graphics.clear([20, 20, 40])
          @sprite.draw
        end
      RUBY
    end

    def build_category_cards
      cards = []

      CATEGORY_HIERARCHY.each do |cat_id, cat_config|
        next unless cat_config[:children] # Skip top-level categories without children

        cat_config[:children].each do |sub_id, sub_config|
          # Link to the category overview section, not the first item
          section_id = "#{cat_id}-#{sub_id}"
          cards << <<~HTML
            <div class="card">
              <h3><a href="#" data-section="#{section_id}">#{html_escape(sub_config[:name])}</a></h3>
              <p>#{html_escape(sub_config[:description])}</p>
            </div>
          HTML
        end
      end

      cards.join("\n")
    end

    def build_module_section(mod_path, mod_data)
      section_id = mod_path.downcase.gsub('::', '-')
      desc = MODULE_DESCRIPTIONS[mod_path] || mod_data['description'] || "API reference for #{mod_path}."

      content = StringIO.new
      content.puts "<section class=\"doc-section\" id=\"#{section_id}\">"
      content.puts breadcrumbs_html(mod_path, mod_path)
      content.puts "<h1>#{html_escape(mod_path)}</h1>"
      content.puts "<p>#{html_escape(desc)}</p>"

      if mod_data['functions']&.any?
        content.puts '<h2>Functions</h2>'
        mod_data['functions'].each do |name, func|
          content.puts render_function_block(name, func)
        end
      end

      content.puts '</section>'
      content.string
    end

    def build_class_section(class_path, class_data)
      name = class_path.split('::').last
      section_id = class_path.downcase.gsub('::', '-')
      desc = MODULE_DESCRIPTIONS[class_path] || class_data['description'] || "Class reference for #{name}."

      content = StringIO.new
      content.puts "<section class=\"doc-section\" id=\"#{section_id}\">"
      content.puts breadcrumbs_html(class_path, name)
      content.puts "<h1>#{html_escape(name)}</h1>"
      content.puts "<p>#{html_escape(desc)}</p>"

      if class_data['classMethods']&.any?
        content.puts '<h2>Class Methods</h2>'
        class_data['classMethods'].each do |method_name, method|
          content.puts render_function_block(method_name, method, name, :class_method)
        end
      end

      if class_data['instanceMethods']&.any?
        content.puts '<h2>Instance Methods</h2>'
        class_data['instanceMethods'].each do |method_name, method|
          content.puts render_function_block(method_name, method, name, :instance_method)
        end
      end

      content.puts '</section>'
      content.string
    end

    def render_function_block(name, func, class_name = nil, method_type = nil)
      css_class = case method_type
                  when :class_method then 'function-block class-method'
                  when :instance_method then 'function-block instance-method'
                  else 'function-block'
                  end

      anchor = class_name ? "#{class_name.downcase}-#{name}" : name
      signature = func['signature'] || name

      html = "<div class=\"#{css_class}\" id=\"#{html_escape(anchor)}\">"
      html += "<div class=\"function-signature\">#{html_escape(signature)}</div>"

      if func['description']
        html += "<div class=\"function-description\">#{html_escape(func['description'])}</div>"
      end

      # Parameters
      if func['params']&.any?
        html += '<h4>Parameters</h4>'
        html += '<table>'
        html += '<thead><tr><th>Name</th><th>Type</th><th>Description</th></tr></thead>'
        html += '<tbody>'
        func['params'].each do |param|
          optional = param['optional'] ? ' <span class="label label-optional">optional</span>' : ''
          default = param['default'] ? " (default: <code>#{html_escape(param['default'])}</code>)" : ''
          html += "<tr><td><code>#{html_escape(param['name'])}</code></td>"
          html += "<td><code>#{html_escape(param['type'])}</code>#{optional}#{default}</td>"
          html += "<td>#{html_escape(param['description'])}</td></tr>"
        end
        html += '</tbody></table>'
      end

      # Returns
      if func['returns']
        ret = func['returns']
        desc = ret['description'] ? " &mdash; #{html_escape(ret['description'])}" : ''
        html += '<h4>Returns</h4>'
        html += "<p><code>#{html_escape(ret['type'])}</code>#{desc}</p>"
      end

      # Example
      if func['example']
        html += '<h4>Example</h4>'
        html += '<pre><code class="language-ruby">'
        html += HTMLGenerator.highlight_ruby(func['example'])
        html += '</code></pre>'
      end

      html += '</div>'
      html
    end

    def build_cli_command_section(name, cmd)
      section_id = "cli-#{name}"
      html = []
      html << "<section class=\"doc-section\" id=\"#{section_id}\">"
      html << breadcrumbs_html("cli/#{name}", "gmrcli #{name}")

      # Title with badges
      badges = []
      badges << '<span class="label label-default">default</span>' if cmd.default_task
      cmd.aliases.each do |a|
        badges << "<span class=\"label label-alias\">#{html_escape(a)}</span>"
      end
      badge_str = badges.any? ? " #{badges.join(' ')}" : ''
      html << "<h1>gmrcli #{html_escape(name)}#{badge_str}</h1>"
      html << "<p>#{html_escape(cmd.description)}</p>"

      # Usage
      html << '<h2>Usage</h2>'
      html << '<pre><code class="language-bash">'
      html << "gmrcli #{html_escape(name)} [options]"
      html << '</code></pre>'

      # Long description
      if cmd.long_desc
        html << '<h2>Description</h2>'
        html << "<p>#{html_escape(cmd.long_desc).gsub("\n", '<br>')}</p>"
      end

      # Options
      if cmd.options.any?
        html << '<h2>Options</h2>'
        html << '<table>'
        html << '<thead><tr><th>Option</th><th>Alias</th><th>Type</th><th>Default</th><th>Description</th></tr></thead>'
        html << '<tbody>'
        cmd.options.each do |opt|
          alias_str = opt.aliases.first ? "<code>#{html_escape(opt.aliases.first)}</code>" : '-'
          default = opt.default.nil? ? '-' : "<code>#{html_escape(opt.default.to_s)}</code>"
          html << "<tr><td><code>--#{html_escape(opt.name)}</code></td>"
          html << "<td>#{alias_str}</td>"
          html << "<td><span class=\"label label-type\">#{opt.type}</span></td>"
          html << "<td>#{default}</td>"
          html << "<td>#{html_escape(opt.description)}</td></tr>"
        end
        html << '</tbody></table>'
      end

      # Stages
      if cmd.stages.any?
        html << '<h2>Stages</h2>'
        html << '<ol class="stage-list">'
        cmd.stages.each do |stage|
          html << "<li>#{html_escape(stage[:name])}</li>"
        end
        html << '</ol>'
      end

      # Examples
      html << '<h2>Examples</h2>'
      html << '<pre><code class="language-bash">'
      example = "# Basic usage\ngmrcli #{name}"
      if cmd.options.any? && cmd.options.first.type == :boolean
        example += "\n\n# With #{cmd.options.first.name} flag\ngmrcli #{name} --#{cmd.options.first.name}"
      end
      html << html_escape(example)
      html << '</code></pre>'

      html << '</section>'
      html.join("\n")
    end

    def build_cli_error_codes_section
      html = []
      html << '<section class="doc-section" id="cli-error-codes">'
      html << breadcrumbs_html('cli/error-codes', 'Error Codes')
      html << '<h1>Error Codes</h1>'
      html << '<p>Machine-readable error codes returned by gmrcli.</p>'

      # Group by category
      categories = @model.cli_error_codes.group_by { |c| c[:code].split('.').first }

      categories.each do |category, cat_codes|
        html << "<h2>#{html_escape(category)}</h2>"
        html << '<table>'
        html << '<thead><tr><th>Code</th><th>Exit</th><th>Description</th></tr></thead>'
        html << '<tbody>'
        cat_codes.sort_by { |c| c[:code] }.each do |code|
          html << "<tr><td><code>#{html_escape(code[:code])}</code></td>"
          html << "<td>#{code[:exit_code]}</td>"
          html << "<td>#{html_escape(code[:description])}</td></tr>"
        end
        html << '</tbody></table>'
      end

      html << '</section>'
      html.join("\n")
    end

    def inline_javascript
      <<~JS
        // Navigate to a section by ID
        function navigateTo(sectionId) {
          // Hide all sections
          document.querySelectorAll('.doc-section').forEach(s => s.classList.remove('active'));

          // Show target section
          const section = document.getElementById(sectionId);
          if (section) {
            section.classList.add('active');
            // Scroll the main content area to top
            const mainContent = document.querySelector('.main-content');
            if (mainContent) {
              mainContent.scrollTop = 0;
            }
          }

          // Update active state in nav
          document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
          const navItem = document.querySelector('.nav-item[data-section="' + sectionId + '"]');
          if (navItem) {
            navItem.classList.add('active');
            // Expand parent categories to show the active item
            let parent = navItem.parentElement;
            while (parent) {
              if (parent.classList.contains('nav-children')) {
                parent.classList.remove('collapsed');
                const header = parent.previousElementSibling;
                if (header && header.classList.contains('nav-category-header')) {
                  const toggle = header.querySelector('.nav-toggle');
                  if (toggle) toggle.classList.remove('collapsed');
                }
              }
              parent = parent.parentElement;
            }
          }

          // Update URL hash
          history.pushState(null, '', '#' + sectionId);

          // Close mobile menu
          document.querySelector('.sidebar').classList.remove('open');

          // Clear search
          const searchInput = document.getElementById('search-input');
          if (searchInput.value) {
            searchInput.value = '';
            resetSearch();
          }
        }

        // Reset search and restore default collapsed state
        function resetSearch() {
          document.querySelectorAll('.nav-item').forEach(item => {
            item.classList.remove('hidden');
          });
          document.querySelectorAll('.nav-category').forEach(cat => {
            cat.classList.remove('hidden');
          });
          // Re-collapse sub-categories (depth > 0)
          document.querySelectorAll('.nav-category.depth-1 > .nav-children, .nav-category.depth-2 > .nav-children').forEach(children => {
            children.classList.add('collapsed');
            const header = children.previousElementSibling;
            if (header) {
              const toggle = header.querySelector('.nav-toggle');
              if (toggle) toggle.classList.add('collapsed');
            }
          });
        }

        // Navigation toggle (collapse/expand categories) AND navigate to category page
        document.querySelectorAll('.nav-category-header').forEach(header => {
          header.addEventListener('click', (e) => {
            e.stopPropagation(); // Prevent duplicate handling

            const targetId = header.dataset.toggle;
            const children = document.getElementById('nav-' + targetId);
            const toggle = header.querySelector('.nav-toggle');

            // Toggle expand/collapse
            if (children) {
              children.classList.toggle('collapsed');
              toggle.classList.toggle('collapsed');
            }

            // Navigate to category overview section
            const sectionId = header.dataset.section;
            if (sectionId) {
              navigateTo(sectionId);
            }
          });
        });

        // Section navigation (for nav items and breadcrumbs)
        document.addEventListener('click', (e) => {
          const target = e.target.closest('[data-section]');
          if (target) {
            e.preventDefault();
            navigateTo(target.dataset.section);
          }
        });

        // Search functionality
        const searchInput = document.getElementById('search-input');
        searchInput.addEventListener('input', (e) => {
          const query = e.target.value.toLowerCase().trim();

          if (query === '') {
            resetSearch();
            return;
          }

          // Hide all items and categories first
          document.querySelectorAll('.nav-item').forEach(item => {
            item.classList.add('hidden');
          });
          document.querySelectorAll('.nav-category').forEach(cat => {
            cat.classList.add('hidden');
          });

          // Helper to show and expand parent categories
          function showParents(element) {
            let parent = element.parentElement;
            while (parent) {
              if (parent.classList.contains('nav-children')) {
                parent.classList.remove('collapsed');
                const header = parent.previousElementSibling;
                if (header && header.classList.contains('nav-category-header')) {
                  const toggle = header.querySelector('.nav-toggle');
                  if (toggle) toggle.classList.remove('collapsed');
                }
              }
              if (parent.classList.contains('nav-category')) {
                parent.classList.remove('hidden');
              }
              parent = parent.parentElement;
            }
          }

          // Show matching nav items
          document.querySelectorAll('.nav-item').forEach(item => {
            const text = item.textContent.toLowerCase();
            if (text.includes(query)) {
              item.classList.remove('hidden');
              showParents(item);
            }
          });

          // Show matching category headers
          document.querySelectorAll('.nav-category-header').forEach(header => {
            const text = header.textContent.toLowerCase();
            if (text.includes(query)) {
              const cat = header.parentElement;
              if (cat) {
                cat.classList.remove('hidden');
                // Expand this category to show its children
                const children = header.nextElementSibling;
                if (children && children.classList.contains('nav-children')) {
                  children.classList.remove('collapsed');
                  const toggle = header.querySelector('.nav-toggle');
                  if (toggle) toggle.classList.remove('collapsed');
                  // Show all children of this matching category
                  children.querySelectorAll('.nav-item').forEach(item => item.classList.remove('hidden'));
                  children.querySelectorAll('.nav-category').forEach(subcat => subcat.classList.remove('hidden'));
                }
                showParents(cat);
              }
            }
          });
        });

        // Clear search on Escape
        searchInput.addEventListener('keydown', (e) => {
          if (e.key === 'Escape') {
            searchInput.value = '';
            resetSearch();
            searchInput.blur();
          }
        });

        // Handle initial hash
        window.addEventListener('load', () => {
          const hash = window.location.hash.slice(1);
          if (hash) {
            navigateTo(hash);
          }
        });

        // Handle browser back/forward
        window.addEventListener('popstate', () => {
          const hash = window.location.hash.slice(1) || 'index';
          navigateTo(hash);
        });
      JS
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
          'fullName' => 'Game Middleware for Ruby',
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

    # Parse CLI documentation if cli_root is specified
    if options[:cli_root]
      puts "\nParsing CLI documentation..."
      cli_parser = CLIParser.new
      bin_file = File.join(options[:cli_root], 'bin', 'gmrcli')
      commands_dir = File.join(options[:cli_root], 'lib', 'gmrcli', 'commands')
      error_codes_file = File.join(options[:cli_root], 'lib', 'gmrcli', 'error_codes.rb')

      cli_data = cli_parser.parse_file(bin_file)
      stages = cli_parser.parse_stages(commands_dir)
      error_codes = cli_parser.parse_error_codes(error_codes_file)

      # Attach stages to commands
      cli_data[:commands].each do |name, cmd|
        cmd.stages = stages[name] || []
      end

      model.set_cli_data(cli_data[:commands], cli_data[:class_options], error_codes)
      puts "  Found #{cli_data[:commands].size} CLI commands"
      puts "  Found #{cli_data[:class_options].size} global options"
      puts "  Found #{error_codes.size} error codes"
    end

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
  cli_root: nil,
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

  opts.on("-c", "--cli-root DIR", "CLI source root directory for CLI docs") do |d|
    options[:cli_root] = d
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

  # Default CLI root to project_root/cli
  options[:cli_root] ||= File.join(project_root, 'cli')
end

GMRDocs.run(options)
