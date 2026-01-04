# Console - Drop-down developer console (IRB-style)
# Toggle with ` (backtick/grave key)
#
# Usage: Type Ruby code and press Enter to evaluate
# Built-in commands: clear, exit, help, vars, history

# Output capture buffer for console
$__console_output_buffer = []

# Module aliases for convenience
G = GMR::Graphics
I = GMR::Input
W = GMR::Window

module Console
  @open = false
  @target_y = 0
  @current_y = 0
  @height = 300
  @input = ""
  @cursor_pos = 0
  @history = []
  @command_history = []
  @history_index = -1
  @cursor_blink = 0
  @scroll_offset = 0
  @saved_input = ""
  @backspace_timer = 0
  @backspace_repeat_delay = 0.4
  @backspace_repeat_rate = 0.03
  @capturing = false

  class << self
    attr_accessor :open, :height, :capturing
    attr_reader :input, :history, :command_history

    # Key codes as methods for mruby compatibility
    def key_grave;     96;  end
    def key_enter;     257; end
    def key_backspace; 259; end
    def key_up;        265; end
    def key_down;      264; end
    def key_escape;    256; end
    def key_tab;       258; end
    def key_left;      263; end
    def key_right;     262; end
    def key_home;      268; end
    def key_end;       269; end
    def key_delete;    261; end
    def key_page_up;   266; end
    def key_page_down; 267; end

    # Helper to call the C++ eval_ruby binding via global function
    def call_eval_ruby(code)
      __console_eval(code)
    end

    def init
      @open = false
      @target_y = -@height
      @current_y = -@height
      @input = ""
      @cursor_pos = 0
      @command_history = []
      @history = [
        { type: :info, text: "=== GMR Console ===" },
        { type: :info, text: "Type Ruby code and press Enter to evaluate." },
        { type: :info, text: "Type 'help' for commands. Press ` or ESC to close." },
        { type: :info, text: "" }
      ]
      @history_index = -1
      @cursor_blink = 0
      @scroll_offset = 0
      @saved_input = ""
      @backspace_timer = 0
      @capturing = false
    end

    def toggle
      @open = !@open
      @target_y = @open ? 0 : -@height
    end

    def show
      @open = true
      @target_y = 0
    end

    def hide
      @open = false
      @target_y = -@height
    end

    def opened?
      @open || @current_y > -@height + 5
    end

    def visible?
      @current_y > -@height + 1
    end

    def println(text, type = :info)
      @history << { type: type, text: text.to_s }
      @scroll_offset = 0
      trim_history
    end

    def update(dt)
      @cursor_blink = (@cursor_blink + dt * 3) % 2

      # Toggle console with grave key
      if I.key_pressed?(key_grave)
        toggle
        return true
      end

      return false unless opened?

      # Animate slide
      speed = 2000 * dt
      if @current_y < @target_y
        @current_y = [@current_y + speed, @target_y].min
      elsif @current_y > @target_y
        @current_y = [@current_y - speed, @target_y].max
      end

      return false unless @open

      # ESC to close
      if I.key_pressed?(key_escape)
        hide
        return true
      end

      # Execute on Enter
      if I.key_pressed?(key_enter)
        execute_command
        return true
      end

      # Backspace with repeat
      if I.key_pressed?(key_backspace)
        delete_char_before_cursor
        @backspace_timer = 0
      elsif I.key_down?(key_backspace)
        @backspace_timer += dt
        if @backspace_timer > @backspace_repeat_delay
          if (@backspace_timer - @backspace_repeat_delay) % @backspace_repeat_rate < dt
            delete_char_before_cursor
          end
        end
      else
        @backspace_timer = 0
      end

      # Delete key
      if I.key_pressed?(key_delete)
        delete_char_at_cursor
        return true
      end

      # Cursor movement
      if I.key_pressed?(key_left)
        @cursor_pos = [@cursor_pos - 1, 0].max
        return true
      end
      if I.key_pressed?(key_right)
        @cursor_pos = [@cursor_pos + 1, @input.length].min
        return true
      end
      if I.key_pressed?(key_home)
        @cursor_pos = 0
        return true
      end
      if I.key_pressed?(key_end)
        @cursor_pos = @input.length
        return true
      end

      # History navigation
      if I.key_pressed?(key_up)
        navigate_history(-1)
        return true
      end
      if I.key_pressed?(key_down)
        navigate_history(1)
        return true
      end

      # Page up/down for scrolling
      if I.key_pressed?(key_page_up)
        @scroll_offset = [@scroll_offset + 10, max_scroll].min
        return true
      end
      if I.key_pressed?(key_page_down)
        @scroll_offset = [@scroll_offset - 10, 0].max
        return true
      end

      # Mouse wheel scroll
      wheel = I.mouse_wheel
      if wheel != 0
        @scroll_offset = [[@scroll_offset - wheel.to_i * 3, 0].max, max_scroll].min
        return true
      end

      # Tab completion (simple - insert spaces)
      if I.key_pressed?(key_tab)
        insert_text("  ")
        return true
      end

      # Character input
      loop do
        char = I.char_pressed
        break unless char
        next if char == 96  # Skip grave key
        next if char < 32 || char > 126  # Skip non-printable ASCII

        insert_text(char.chr)
      end

      true  # Console consumes all input when open
    end

    def draw
      return unless visible?

      y = @current_y.to_i
      width = W.width

      # Semi-transparent background
      G.draw_rect(0, y, width, @height, [15, 20, 30, 240])

      # Top border (subtle)
      G.draw_rect(0, y, width, 1, [40, 50, 70, 255])

      # Bottom border (highlighted)
      G.draw_rect(0, y + @height - 2, width, 2, [60, 120, 180, 255])

      # Draw history/output
      draw_output(y, width)

      # Draw input line
      draw_input_line(y, width)

      # Draw scroll indicator
      draw_scrollbar(y, width)
    end

    private

    def draw_output(y, width)
      font_size = 14
      line_height = 18
      padding = 10
      output_height = @height - 40
      visible_count = output_height / line_height

      # Calculate visible range
      total_lines = @history.length
      start_idx = [total_lines - visible_count - @scroll_offset, 0].max
      end_idx = [total_lines - @scroll_offset, total_lines].min

      draw_y = y + padding

      (start_idx...end_idx).each do |i|
        entry = @history[i]
        color = color_for_type(entry[:type])

        text = entry[:text].to_s
        # Truncate very long lines
        max_chars = (width - padding * 2 - 20) / 7
        if text.length > max_chars
          text = text[0...max_chars - 3] + "..."
        end

        G.draw_text(text, padding, draw_y, font_size, color)
        draw_y += line_height
      end
    end

    def draw_input_line(y, width)
      font_size = 14
      padding = 10
      input_y = y + @height - 28

      # Input background
      G.draw_rect(0, input_y - 4, width, 28, [25, 30, 45, 255])

      # Prompt
      prompt = ">> "
      prompt_color = @open ? [100, 200, 255, 255] : [100, 100, 100, 255]
      G.draw_text(prompt, padding, input_y, font_size, prompt_color)

      prompt_width = G.measure_text(prompt, font_size)
      text_x = padding + prompt_width

      # Input text
      G.draw_text(@input, text_x, input_y, font_size, [255, 255, 255, 255])

      # Cursor
      if @open && @cursor_blink < 1
        cursor_x = text_x + G.measure_text(@input[0...@cursor_pos], font_size)
        G.draw_rect(cursor_x, input_y, 2, font_size + 2, [255, 255, 255, 200])
      end

      # Show history index if navigating
      if @history_index >= 0
        indicator = "[#{@command_history.length - @history_index}/#{@command_history.length}]"
        indicator_width = G.measure_text(indicator, 12)
        G.draw_text(indicator, width - indicator_width - padding, input_y + 1, 12, [80, 80, 100, 255])
      end
    end

    def draw_scrollbar(y, width)
      return if @history.length <= visible_lines

      bar_height = @height - 50
      bar_x = width - 6
      bar_y = y + 8

      # Track
      G.draw_rect(bar_x, bar_y, 4, bar_height, [40, 45, 60, 150])

      # Thumb
      visible_ratio = visible_lines.to_f / @history.length
      thumb_height = [(visible_ratio * bar_height).to_i, 20].max

      scroll_ratio = @scroll_offset.to_f / [max_scroll, 1].max
      thumb_y = bar_y + ((1 - scroll_ratio) * (bar_height - thumb_height)).to_i

      G.draw_rect(bar_x, thumb_y, 4, thumb_height, [80, 120, 180, 200])
    end

    def color_for_type(type)
      case type
      when :input  then [120, 180, 255, 255]      # Blue - user input
      when :result then [120, 255, 150, 255]      # Green - successful result
      when :error  then [255, 100, 100, 255]      # Red - error
      when :warn   then [255, 200, 100, 255]      # Yellow - warning
      when :info   then [160, 160, 180, 255]      # Gray - info
      else              [200, 200, 200, 255]      # Default
      end
    end

    def visible_lines
      (@height - 50) / 18
    end

    def max_scroll
      [@history.length - visible_lines, 0].max
    end

    def insert_text(str)
      @input = @input[0...@cursor_pos] + str + @input[@cursor_pos..-1].to_s
      @cursor_pos += str.length
    end

    def delete_char_before_cursor
      return if @cursor_pos <= 0
      @input = @input[0...(@cursor_pos - 1)] + @input[@cursor_pos..-1].to_s
      @cursor_pos -= 1
    end

    def delete_char_at_cursor
      return if @cursor_pos >= @input.length
      @input = @input[0...@cursor_pos] + @input[(@cursor_pos + 1)..-1].to_s
    end

    def navigate_history(direction)
      return if @command_history.empty?

      if direction < 0  # Up - older
        if @history_index < 0
          @saved_input = @input
          @history_index = @command_history.length - 1
        elsif @history_index > 0
          @history_index -= 1
        end
      else  # Down - newer
        if @history_index >= 0
          @history_index += 1
          if @history_index >= @command_history.length
            @history_index = -1
          end
        end
      end

      @input = @history_index >= 0 ? @command_history[@history_index].dup : @saved_input.to_s
      @cursor_pos = @input.length
    end

    def execute_command
      cmd = @input.strip

      # Clear input first to ensure it happens
      @input = ""
      @cursor_pos = 0

      return if cmd.empty?

      # Save to command history
      @command_history << cmd unless @command_history.last == cmd
      @command_history.shift if @command_history.length > 100
      @history_index = -1
      @saved_input = ""

      # Echo input
      @history << { type: :input, text: ">> #{cmd}" }

      # Handle built-in commands
      cmd_lower = cmd.downcase
      if cmd_lower == "clear" || cmd_lower == "cls"
        @history.clear
        @history << { type: :info, text: "Console cleared." }
      elsif cmd_lower == "exit" || cmd_lower == "quit" || cmd_lower == "close"
        hide
      elsif cmd_lower == "help" || cmd_lower == "?"
        show_help
      elsif cmd_lower == "vars"
        show_global_vars
      elsif cmd_lower == "history"
        show_command_history
      elsif cmd_lower.start_with?("height ")
        # Parse "height 123" without regex
        height_str = cmd_lower.sub("height ", "").strip
        new_height = height_str.to_i
        if new_height >= 100 && new_height <= W.height - 50
          @height = new_height
          @target_y = @open ? 0 : -@height
          @history << { type: :info, text: "Console height set to #{new_height}" }
        else
          @history << { type: :error, text: "Height must be between 100 and #{W.height - 50}" }
        end
      else
        # Evaluate as Ruby code
        run_ruby_code(cmd)
      end

      @scroll_offset = 0
      trim_history
    end

    def run_ruby_code(code)
      # Clear output buffer and enable capturing
      $__console_output_buffer.clear
      @capturing = true

      # Call the C++ eval_ruby function (defined on Kernel)
      result_array = call_eval_ruby(code)

      # Stop capturing
      @capturing = false

      # First, display any captured output (from puts/print/p)
      $__console_output_buffer.each do |line|
        @history << { type: :info, text: line }
      end

      # Handle the result
      if result_array && result_array.is_a?(Array) && result_array.length >= 2
        success = result_array[0]
        result = result_array[1]

        if success
          # Split result into lines if it's long
          result_str = result.to_s
          if result_str.length > 500
            result_str = result_str[0...500] + "... (truncated)"
          end

          result_str.split("\n").each_with_index do |line, i|
            prefix = i == 0 ? "=> " : "   "
            @history << { type: :result, text: "#{prefix}#{line}" }
          end
        else
          # Error - might be multi-line
          result.to_s.split("\n").each do |line|
            @history << { type: :error, text: line }
          end
        end
      else
        @history << { type: :error, text: "eval_ruby returned unexpected result: #{result_array.inspect}" }
      end
    end

    def show_help
      @history << { type: :info, text: "=== Console Commands ===" }
      @history << { type: :info, text: "  clear, cls    - Clear console output" }
      @history << { type: :info, text: "  exit, quit    - Close console" }
      @history << { type: :info, text: "  help, ?       - Show this help" }
      @history << { type: :info, text: "  vars          - List global ($) variables" }
      @history << { type: :info, text: "  history       - Show command history" }
      @history << { type: :info, text: "  height <n>    - Set console height" }
      @history << { type: :info, text: "" }
      @history << { type: :info, text: "=== Tips ===" }
      @history << { type: :info, text: "  - Type any Ruby expression to evaluate" }
      @history << { type: :info, text: "  - Use $variables to access game state" }
      @history << { type: :info, text: "  - Up/Down arrows navigate history" }
      @history << { type: :info, text: "  - Page Up/Down or mouse wheel to scroll" }
      @history << { type: :info, text: "" }
      @history << { type: :info, text: "=== Examples ===" }
      @history << { type: :result, text: "  $player_x = 100" }
      @history << { type: :result, text: "  W.width" }
      @history << { type: :result, text: "  W.toggle_fullscreen" }
      @history << { type: :result, text: "  W.set_title('New Title')" }
    end

    def show_global_vars
      @history << { type: :info, text: "=== Global Variables ===" }

      result_arr = call_eval_ruby("global_variables.select { |v| v.to_s.start_with?('$') && !v.to_s.start_with?('$_') && !v.to_s.start_with?('$-') }")
      success = result_arr && result_arr[0]
      result = result_arr && result_arr[1]

      if success
        # Parse the array from the result - use tr instead of gsub with regex
        vars_str = result.tr('[],:$', ' ').split.reject { |v| v.empty? }

        if vars_str.empty?
          @history << { type: :info, text: "  (no user-defined global variables)" }
        else
          vars_str.each do |var_name|
            var_name = "$#{var_name}" unless var_name.start_with?('$')
            result_arr2 = call_eval_ruby(var_name)
            success2 = result_arr2 && result_arr2[0]
            value = result_arr2 && result_arr2[1]
            if success2
              # Truncate long values
              val_str = value.to_s
              val_str = val_str[0...60] + "..." if val_str.length > 60
              @history << { type: :result, text: "  #{var_name} = #{val_str}" }
            end
          end
        end
      else
        @history << { type: :error, text: "  Error listing variables" }
      end
    end

    def show_command_history
      @history << { type: :info, text: "=== Command History (last 20) ===" }

      start = [@command_history.length - 20, 0].max
      @command_history[start..-1].each_with_index do |cmd, i|
        @history << { type: :info, text: "  #{start + i + 1}. #{cmd}" }
      end

      if @command_history.empty?
        @history << { type: :info, text: "  (no commands in history)" }
      end
    end

    def trim_history
      @history.shift while @history.length > 1000
    end
  end
end

# Global helper to call eval_ruby (accessible from within module)
def __console_eval(code)
  eval_ruby(code)
end

# Initialize console on load
Console.init

# Global functions for the engine to call
def console_update(dt)
  Console.update(dt)
end

def console_draw
  Console.draw
end

def console_open?
  Console.opened?
end

def console_show
  Console.show
end

def console_hide
  Console.hide
end

def console_print(text, type = :info)
  Console.println(text, type)
end

# Override print to capture output when console is evaluating
alias __original_print print
def print(*args)
  if Console.capturing
    $__console_output_buffer << args.map { |a| a.to_s }.join
  end
  __original_print(*args)
end

# Override p to capture output when console is evaluating
alias __original_p p
def p(*args)
  if Console.capturing
    args.each do |arg|
      $__console_output_buffer << arg.inspect
    end
  end
  __original_p(*args)
end
