#ifndef GMR_CONSOLE_MODULE_HPP
#define GMR_CONSOLE_MODULE_HPP

#include <mruby.h>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace gmr {
namespace console {

// Console styling configuration
struct ConsoleStyle {
    // Colors (RGBA)
    uint8_t bg_color[4] = {15, 20, 30, 240};           // Background
    uint8_t fg_color[4] = {255, 255, 255, 255};        // Foreground text
    uint8_t prompt_color[4] = {100, 200, 255, 255};    // Prompt ">>"
    uint8_t input_color[4] = {255, 255, 255, 255};     // User input
    uint8_t result_color[4] = {120, 255, 150, 255};    // Success results
    uint8_t error_color[4] = {255, 100, 100, 255};     // Errors
    uint8_t warning_color[4] = {255, 200, 100, 255};   // Warnings
    uint8_t info_color[4] = {160, 160, 180, 255};      // Info text
    uint8_t border_color[4] = {60, 120, 180, 255};     // Bottom border
    uint8_t cursor_color[4] = {255, 255, 255, 200};    // Cursor
    uint8_t scrollbar_bg[4] = {40, 45, 60, 150};       // Scrollbar track
    uint8_t scrollbar_fg[4] = {80, 120, 180, 200};     // Scrollbar thumb

    // Layout
    int font_size = 14;
    int line_height = 18;
    int padding = 10;
    int height = 300;              // Console height when open
    int width = 0;                 // 0 = full screen width

    // Position: 0=top, 1=bottom
    int position = 0;              // Default: drop from top

    // Animation
    float slide_speed = 2000.0f;   // Pixels per second

    // Input repeat
    float backspace_delay = 0.4f;
    float backspace_rate = 0.03f;

    // Limits
    int max_history = 1000;
    int max_command_history = 100;
};

// Output entry type
enum class OutputType {
    INFO,
    INPUT,
    RESULT,
    ERROR,
    WARNING,
    SYSTEM
};

// Single output entry
struct OutputEntry {
    OutputType type;
    std::string text;
};

// Custom command handler
using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;

// Console state and logic
class ConsoleModule {
public:
    static ConsoleModule& instance();

    // Initialization
    void init(mrb_state* mrb);
    void shutdown();
    bool is_initialized() const { return initialized_; }

    // Configuration
    void configure(const ConsoleStyle& style);
    const ConsoleStyle& style() const { return style_; }

    // Enable/disable (for Release builds)
    void enable(bool enabled = true);
    bool is_enabled() const { return enabled_; }

    // Toggle key configuration (default: grave/backtick = 96)
    void set_toggle_key(int key) { toggle_key_ = key; }
    int toggle_key() const { return toggle_key_; }

    // State
    bool is_open() const { return open_; }
    bool is_visible() const { return current_y_ > -style_.height + 1; }
    void show();
    void hide();
    void toggle();

    // Update & draw (called by engine)
    bool update(float dt);  // Returns true if console consumed input
    void draw();

    // Output
    void println(const std::string& text, OutputType type = OutputType::INFO);
    void clear_output();

    // Command registration (for Debug and Release)
    void register_command(const std::string& name,
                         const std::string& description,
                         CommandHandler handler);
    bool has_command(const std::string& name) const;
    void unregister_command(const std::string& name);

    // Ruby code evaluation control (Debug only)
    void allow_ruby_eval(bool allow) { allow_ruby_eval_ = allow; }
    bool ruby_eval_allowed() const { return allow_ruby_eval_; }

    // Get mruby state
    mrb_state* mrb() const { return mrb_; }

    // Disable copy
    ConsoleModule(const ConsoleModule&) = delete;
    ConsoleModule& operator=(const ConsoleModule&) = delete;

private:
    ConsoleModule();
    ~ConsoleModule();

    // Input handling
    void handle_key_input();
    void handle_char_input();
    void insert_text(const std::string& str);
    void delete_char_before_cursor();
    void delete_char_at_cursor();
    void navigate_history(int direction);
    void execute_input();

    // Evaluation
    void run_command(const std::string& cmd, const std::vector<std::string>& args);
    void run_ruby_code(const std::string& code);
    void display_result(const std::string& result);
    void display_error(const std::string& error);

    // Built-in commands
    void register_builtin_commands();
    std::string cmd_help(const std::vector<std::string>& args);
    std::string cmd_clear(const std::vector<std::string>& args);
    std::string cmd_history(const std::vector<std::string>& args);
    std::string cmd_height(const std::vector<std::string>& args);
    std::string cmd_vars(const std::vector<std::string>& args);

    // Rendering helpers
    void draw_background();
    void draw_output();
    void draw_input_line();
    void draw_scrollbar();
    void get_color_for_type(OutputType type, uint8_t* out) const;

    // Scrolling
    int visible_lines() const;
    int max_scroll() const;

    // Trim history
    void trim_history();

    // Multi-line detection (Debug only)
    bool is_input_complete(const std::string& code) const;

    // State
    bool initialized_ = false;
    bool enabled_ = false;  // Must call Console.enable to activate
    bool open_ = false;
    mrb_state* mrb_ = nullptr;

    // Animation
    float target_y_ = 0;
    float current_y_ = 0;

    // Input
    std::string input_;
    int cursor_pos_ = 0;
    float cursor_blink_ = 0;
    float backspace_timer_ = 0;

    // Multi-line state (Debug only)
    bool in_multiline_ = false;
    std::string multiline_buffer_;

    // Command history
    std::vector<std::string> command_history_;
    int history_index_ = -1;
    std::string saved_input_;

    // Output history
    std::vector<OutputEntry> output_;
    int scroll_offset_ = 0;

    // Registered commands
    struct Command {
        std::string name;
        std::string description;
        CommandHandler handler;
    };
    std::unordered_map<std::string, Command> commands_;

    // Style configuration
    ConsoleStyle style_;

    // Toggle key (default: grave/backtick)
    int toggle_key_ = 96;

    // Ruby eval permission (Debug only by default)
#if defined(GMR_DEBUG_ENABLED)
    bool allow_ruby_eval_ = true;
#else
    bool allow_ruby_eval_ = false;
#endif
};

// Register GMR::Console module with mruby
void register_console_module(mrb_state* mrb);

} // namespace console
} // namespace gmr

#endif // GMR_CONSOLE_MODULE_HPP
