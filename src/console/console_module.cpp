#include "gmr/console/console_module.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/input/input_context.hpp"
#include "gmr/resources/font_manager.hpp"
#include "gmr/state.hpp"
#include "raylib.h"
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/variable.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

#if defined(GMR_DEBUG_ENABLED)
#include "gmr/repl/repl_session.hpp"
#include "gmr/repl/multiline_detector.hpp"
#endif

namespace gmr {
namespace console {

// Key codes
static constexpr int KEY_GRAVE = 96;
static constexpr int KEY_ENTER_CODE = 257;
static constexpr int KEY_BACKSPACE_CODE = 259;
static constexpr int KEY_UP_CODE = 265;
static constexpr int KEY_DOWN_CODE = 264;
static constexpr int KEY_ESCAPE_CODE = 256;
static constexpr int KEY_TAB_CODE = 258;
static constexpr int KEY_LEFT_CODE = 263;
static constexpr int KEY_RIGHT_CODE = 262;
static constexpr int KEY_HOME_CODE = 268;
static constexpr int KEY_END_CODE = 269;
static constexpr int KEY_DELETE_CODE = 261;
static constexpr int KEY_PAGE_UP_CODE = 266;
static constexpr int KEY_PAGE_DOWN_CODE = 267;

// Convert our color array to raylib Color
static ::Color to_raylib_color(const uint8_t* c) {
    return ::Color{c[0], c[1], c[2], c[3]};
}

// Get font for console rendering (custom or default)
static Font get_console_font(FontHandle handle) {
    if (handle != INVALID_HANDLE) {
        auto* font = FontManager::instance().get(handle);
        if (font) return *font;
    }
    return GetFontDefault();
}

// Draw text with font (handles both custom and default fonts)
static void draw_console_text(const char* text, int x, int y, int font_size, ::Color color, FontHandle font_handle) {
    Font font = get_console_font(font_handle);
    float spacing = static_cast<float>(font_size) / 10.0f;
    Vector2 pos = {static_cast<float>(x), static_cast<float>(y)};
    DrawTextEx(font, text, pos, static_cast<float>(font_size), spacing, color);
}

// Measure text width with font
static int measure_console_text(const char* text, int font_size, FontHandle font_handle) {
    Font font = get_console_font(font_handle);
    float spacing = static_cast<float>(font_size) / 10.0f;
    Vector2 size = MeasureTextEx(font, text, static_cast<float>(font_size), spacing);
    return static_cast<int>(size.x);
}

ConsoleModule& ConsoleModule::instance() {
    static ConsoleModule module;
    return module;
}

ConsoleModule::ConsoleModule() {
    target_y_ = -style_.height;
    current_y_ = -style_.height;
}

ConsoleModule::~ConsoleModule() {
    shutdown();
}

void ConsoleModule::init(mrb_state* mrb) {
    if (initialized_) return;

    mrb_ = mrb;
    initialized_ = true;

    // Reset state
    open_ = false;
    target_y_ = -style_.height;
    current_y_ = -style_.height;
    input_.clear();
    cursor_pos_ = 0;
    command_history_.clear();
    history_index_ = -1;
    output_.clear();
    scroll_offset_ = 0;
    in_multiline_ = false;
    multiline_buffer_.clear();

    // Register built-in commands
    register_builtin_commands();

    // Welcome message
    println("=== GMR Console ===", OutputType::INFO);
    if (allow_ruby_eval_) {
        println("Type Ruby code or commands. Press ` or ESC to close.", OutputType::INFO);
    } else {
        println("Type 'help' for commands. Press ` or ESC to close.", OutputType::INFO);
    }
    println("", OutputType::INFO);
}

void ConsoleModule::shutdown() {
    mrb_ = nullptr;
    initialized_ = false;
    output_.clear();
    command_history_.clear();
    commands_.clear();
}

void ConsoleModule::configure(const ConsoleStyle& style) {
    style_ = style;
    // Update target position based on new height
    if (!open_) {
        target_y_ = -style_.height;
        current_y_ = -style_.height;
    }
}

void ConsoleModule::enable(bool enabled) {
    enabled_ = enabled;
    if (!enabled && open_) {
        hide();
    }
}

void ConsoleModule::show() {
    if (!enabled_) return;
    if (open_) return;  // Already open
    open_ = true;
    target_y_ = 0;

    // Push console input context to block game input
    auto& ctx_stack = gmr::input::ContextStack::instance();
    ctx_stack.push("console");
    // Mark console context as blocking so global game actions don't fire
    if (auto* ctx = ctx_stack.current()) {
        ctx->blocks_global = true;
    }
}

void ConsoleModule::hide() {
    if (!open_) return;  // Already closed
    open_ = false;
    target_y_ = -style_.height;

    // Pop console input context to restore game input
    gmr::input::ContextStack::instance().pop();
}

void ConsoleModule::toggle() {
    if (open_) {
        hide();
    } else {
        show();
    }
}

bool ConsoleModule::update(float dt) {
    if (!enabled_ || !initialized_) {
        return false;
    }

    cursor_blink_ = fmodf(cursor_blink_ + dt * 3.0f, 2.0f);

    // Toggle console with toggle key
    if (IsKeyPressed(toggle_key_)) {
        toggle();
        return true;
    }

    // Animate slide (always run animation, even when closing)
    float speed = style_.slide_speed * dt;
    if (current_y_ < target_y_) {
        current_y_ = std::min(current_y_ + speed, target_y_);
    } else if (current_y_ > target_y_) {
        current_y_ = std::max(current_y_ - speed, target_y_);
    }

    // Return early if not visible and not opening
    if (!is_visible() && !open_) return false;

    if (!open_) return false;

    // ESC to close
    if (IsKeyPressed(KEY_ESCAPE_CODE)) {
        hide();
        return true;
    }

    // Execute on Enter
    if (IsKeyPressed(KEY_ENTER_CODE)) {
        execute_input();
        return true;
    }

    // Handle key input
    handle_key_input();

    // Handle character input
    handle_char_input();

    return true;  // Console consumes all input when open
}

void ConsoleModule::handle_key_input() {
    // Backspace with repeat
    if (IsKeyPressed(KEY_BACKSPACE_CODE)) {
        delete_char_before_cursor();
        backspace_timer_ = 0;
    } else if (IsKeyDown(KEY_BACKSPACE_CODE)) {
        backspace_timer_ += GetFrameTime();
        if (backspace_timer_ > style_.backspace_delay) {
            float elapsed = backspace_timer_ - style_.backspace_delay;
            if (fmodf(elapsed, style_.backspace_rate) < GetFrameTime()) {
                delete_char_before_cursor();
            }
        }
    } else {
        backspace_timer_ = 0;
    }

    // Delete key
    if (IsKeyPressed(KEY_DELETE_CODE)) {
        delete_char_at_cursor();
    }

    // Cursor movement
    if (IsKeyPressed(KEY_LEFT_CODE)) {
        cursor_pos_ = std::max(cursor_pos_ - 1, 0);
    }
    if (IsKeyPressed(KEY_RIGHT_CODE)) {
        cursor_pos_ = std::min(cursor_pos_ + 1, static_cast<int>(input_.length()));
    }
    if (IsKeyPressed(KEY_HOME_CODE)) {
        cursor_pos_ = 0;
    }
    if (IsKeyPressed(KEY_END_CODE)) {
        cursor_pos_ = static_cast<int>(input_.length());
    }

    // History navigation
    if (IsKeyPressed(KEY_UP_CODE)) {
        navigate_history(-1);
    }
    if (IsKeyPressed(KEY_DOWN_CODE)) {
        navigate_history(1);
    }

    // Page up/down for scrolling
    if (IsKeyPressed(KEY_PAGE_UP_CODE)) {
        scroll_offset_ = std::min(scroll_offset_ + 10, max_scroll());
    }
    if (IsKeyPressed(KEY_PAGE_DOWN_CODE)) {
        scroll_offset_ = std::max(scroll_offset_ - 10, 0);
    }

    // Mouse wheel scroll (inverted for natural scrolling)
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        scroll_offset_ = std::clamp(scroll_offset_ + static_cast<int>(wheel) * 3, 0, max_scroll());
    }

    // Tab (insert spaces)
    if (IsKeyPressed(KEY_TAB_CODE)) {
        insert_text("  ");
    }
}

void ConsoleModule::handle_char_input() {
    int ch;
    while ((ch = GetCharPressed()) != 0) {
        if (ch == toggle_key_) continue;  // Skip toggle key
        if (ch < 32 || ch > 126) continue;  // Skip non-printable ASCII

        char c = static_cast<char>(ch);
        insert_text(std::string(1, c));
    }
}

void ConsoleModule::insert_text(const std::string& str) {
    input_.insert(cursor_pos_, str);
    cursor_pos_ += static_cast<int>(str.length());
}

void ConsoleModule::delete_char_before_cursor() {
    if (cursor_pos_ <= 0) return;
    input_.erase(cursor_pos_ - 1, 1);
    cursor_pos_--;
}

void ConsoleModule::delete_char_at_cursor() {
    if (cursor_pos_ >= static_cast<int>(input_.length())) return;
    input_.erase(cursor_pos_, 1);
}

void ConsoleModule::navigate_history(int direction) {
    if (command_history_.empty()) return;

    if (direction < 0) {  // Up - older
        if (history_index_ < 0) {
            saved_input_ = input_;
            history_index_ = static_cast<int>(command_history_.size()) - 1;
        } else if (history_index_ > 0) {
            history_index_--;
        }
    } else {  // Down - newer
        if (history_index_ >= 0) {
            history_index_++;
            if (history_index_ >= static_cast<int>(command_history_.size())) {
                history_index_ = -1;
            }
        }
    }

    input_ = (history_index_ >= 0) ? command_history_[history_index_] : saved_input_;
    cursor_pos_ = static_cast<int>(input_.length());
}

void ConsoleModule::execute_input() {
    std::string cmd = input_;

    // Trim whitespace
    size_t start = cmd.find_first_not_of(" \t\n\r");
    size_t end = cmd.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) {
        input_.clear();
        cursor_pos_ = 0;
        return;
    }
    cmd = cmd.substr(start, end - start + 1);

    // Clear input
    input_.clear();
    cursor_pos_ = 0;

    if (cmd.empty()) return;

    // Save to command history
    if (command_history_.empty() || command_history_.back() != cmd) {
        command_history_.push_back(cmd);
        if (command_history_.size() > static_cast<size_t>(style_.max_command_history)) {
            command_history_.erase(command_history_.begin());
        }
    }
    history_index_ = -1;
    saved_input_.clear();

    // Echo input
    std::string prompt = in_multiline_ ? ".. " : ">> ";
    println(prompt + cmd, OutputType::INPUT);

    // Parse command name and arguments
    std::string cmd_name;
    std::vector<std::string> args;

    size_t pos = 0;
    while (pos < cmd.size() && !std::isspace(static_cast<unsigned char>(cmd[pos]))) {
        pos++;
    }
    cmd_name = cmd.substr(0, pos);

    // Parse arguments
    while (pos < cmd.size()) {
        while (pos < cmd.size() && std::isspace(static_cast<unsigned char>(cmd[pos]))) pos++;
        if (pos >= cmd.size()) break;

        if (cmd[pos] == '"' || cmd[pos] == '\'') {
            char quote = cmd[pos++];
            size_t arg_start = pos;
            while (pos < cmd.size() && cmd[pos] != quote) {
                if (cmd[pos] == '\\' && pos + 1 < cmd.size()) pos++;
                pos++;
            }
            args.push_back(cmd.substr(arg_start, pos - arg_start));
            if (pos < cmd.size()) pos++;  // Skip closing quote
        } else {
            size_t arg_start = pos;
            while (pos < cmd.size() && !std::isspace(static_cast<unsigned char>(cmd[pos]))) pos++;
            args.push_back(cmd.substr(arg_start, pos - arg_start));
        }
    }

    // Convert command name to lowercase for built-in commands
    std::string cmd_lower = cmd_name;
    std::transform(cmd_lower.begin(), cmd_lower.end(), cmd_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Check for registered command first
    if (has_command(cmd_lower)) {
        run_command(cmd_lower, args);
    } else if (allow_ruby_eval_) {
        // Fall through to Ruby evaluation (Debug mode)
        run_ruby_code(cmd);
    } else {
        // Release mode: command not found
        println("Unknown command: " + cmd_name + ". Type 'help' for available commands.", OutputType::ERROR);
    }

    scroll_offset_ = 0;
    trim_history();
}

void ConsoleModule::run_command(const std::string& name, const std::vector<std::string>& args) {
    auto it = commands_.find(name);
    if (it == commands_.end()) {
        println("Unknown command: " + name, OutputType::ERROR);
        return;
    }

    try {
        std::string result = it->second.handler(args);

        // Handle special commands
        if (result == "__clear__") {
            clear_output();
            println("Console cleared.", OutputType::INFO);
            return;
        }

        if (!result.empty()) {
            println("=> " + result, OutputType::RESULT);
        }
    } catch (const std::exception& e) {
        println(std::string("Command error: ") + e.what(), OutputType::ERROR);
    }
}

void ConsoleModule::run_ruby_code(const std::string& code) {
    if (!mrb_) {
        println("Error: Ruby VM not initialized", OutputType::ERROR);
        return;
    }

#if defined(GMR_DEBUG_ENABLED)
    // Debug builds: Use full REPL session with multi-line support and output capture

    // Handle multi-line input
    std::string eval_code = code;
    if (in_multiline_) {
        multiline_buffer_ += "\n" + code;
        if (is_input_complete(multiline_buffer_)) {
            eval_code = multiline_buffer_;
            multiline_buffer_.clear();
            in_multiline_ = false;
        } else {
            return;  // Still incomplete
        }
    } else {
        if (!is_input_complete(code)) {
            multiline_buffer_ = code;
            in_multiline_ = true;
            return;
        }
    }

    // Use REPL session if available
    auto& session = repl::ReplSession::instance();
    if (!session.is_ready()) {
        session.init(mrb_);
    }

    auto result = session.evaluate(eval_code);

    // Display captured stdout
    if (!result.stdout_capture.empty()) {
        std::istringstream iss(result.stdout_capture);
        std::string line;
        while (std::getline(iss, line)) {
            println(line, OutputType::INFO);
        }
    }

    // Display captured stderr
    if (!result.stderr_capture.empty()) {
        std::istringstream iss(result.stderr_capture);
        std::string line;
        while (std::getline(iss, line)) {
            println(line, OutputType::WARNING);
        }
    }

    // Handle result based on status
    switch (result.status) {
        case repl::EvalStatus::SUCCESS:
            if (!result.result.empty()) {
                display_result(result.result);
            }
            break;

        case repl::EvalStatus::EVAL_ERROR:
            if (!result.exception_class.empty()) {
                println(result.exception_class + ": " + result.exception_message, OutputType::ERROR);
                for (size_t i = 0; i < result.backtrace.size() && i < 5; i++) {
                    println("  " + result.backtrace[i], OutputType::ERROR);
                }
            } else {
                println("Error: " + result.exception_message, OutputType::ERROR);
            }
            break;

        case repl::EvalStatus::INCOMPLETE:
            in_multiline_ = true;
            multiline_buffer_ = eval_code;
            break;

        case repl::EvalStatus::COMMAND_NOT_FOUND:
            println(result.result, OutputType::ERROR);
            break;

        case repl::EvalStatus::REENTRANCY_BLOCKED:
            println("Cannot evaluate while another evaluation is in progress", OutputType::ERROR);
            break;
    }
#else
    // Release builds: Simple eval using mrb_load_string (no multi-line, no stdout capture)
    mrbc_context* ctx = mrbc_context_new(mrb_);
    mrbc_filename(mrb_, ctx, "(console)");

    mrb_value result = mrb_load_string_cxt(mrb_, code.c_str(), ctx);

    if (mrb_->exc) {
        // Handle exception
        mrb_value exc = mrb_obj_value(mrb_->exc);
        mrb_value msg = mrb_funcall(mrb_, exc, "message", 0);
        mrb_value cls = mrb_funcall(mrb_, exc, "class", 0);
        mrb_value cls_name = mrb_funcall(mrb_, cls, "to_s", 0);

        std::string error_msg = std::string(RSTRING_PTR(cls_name), RSTRING_LEN(cls_name));
        error_msg += ": ";
        error_msg += std::string(RSTRING_PTR(msg), RSTRING_LEN(msg));
        println(error_msg, OutputType::ERROR);

        mrb_->exc = nullptr;  // Clear exception
    } else if (!mrb_nil_p(result)) {
        // Display result
        mrb_value str = mrb_funcall(mrb_, result, "inspect", 0);
        if (!mrb_->exc) {
            display_result(std::string(RSTRING_PTR(str), RSTRING_LEN(str)));
        } else {
            mrb_->exc = nullptr;
        }
    }

    mrbc_context_free(mrb_, ctx);
#endif
}

void ConsoleModule::display_result(const std::string& result) {
    // Truncate long results
    std::string display = result;
    if (display.length() > 500) {
        display = display.substr(0, 500) + "... (truncated)";
    }

    // Split by newlines
    std::istringstream iss(display);
    std::string line;
    bool first = true;
    while (std::getline(iss, line)) {
        std::string prefix = first ? "=> " : "   ";
        println(prefix + line, OutputType::RESULT);
        first = false;
    }
}

void ConsoleModule::display_error(const std::string& error) {
    println(error, OutputType::ERROR);
}

bool ConsoleModule::is_input_complete(const std::string& code) const {
#if defined(GMR_DEBUG_ENABLED)
    return repl::MultilineDetector::is_complete(code);
#else
    (void)code;
    return true;
#endif
}

void ConsoleModule::register_builtin_commands() {
    register_command("help", "List available commands",
        [this](const std::vector<std::string>& args) { return cmd_help(args); });

    register_command("clear", "Clear console output",
        [this](const std::vector<std::string>& args) { return cmd_clear(args); });

    register_command("cls", "Clear console output",
        [this](const std::vector<std::string>& args) { return cmd_clear(args); });

    register_command("history", "Show command history",
        [this](const std::vector<std::string>& args) { return cmd_history(args); });

    register_command("height", "Set console height (usage: height <pixels>)",
        [this](const std::vector<std::string>& args) { return cmd_height(args); });

    register_command("exit", "Close console",
        [this](const std::vector<std::string>&) { hide(); return ""; });

    register_command("quit", "Close console",
        [this](const std::vector<std::string>&) { hide(); return ""; });

    register_command("close", "Close console",
        [this](const std::vector<std::string>&) { hide(); return ""; });

    register_command("version", "Show engine version",
        [](const std::vector<std::string>&) { return "GMR Engine v1.0"; });

#if defined(GMR_DEBUG_ENABLED)
    register_command("vars", "List global ($) variables",
        [this](const std::vector<std::string>& args) { return cmd_vars(args); });
#endif
}

std::string ConsoleModule::cmd_help(const std::vector<std::string>&) {
    println("=== Console Commands ===", OutputType::INFO);
    for (const auto& [name, cmd] : commands_) {
        println("  " + name + " - " + cmd.description, OutputType::INFO);
    }

#if defined(GMR_DEBUG_ENABLED)
    println("", OutputType::INFO);
    println("(Debug mode: arbitrary Ruby code is also allowed)", OutputType::INFO);
#endif

    return "";
}

std::string ConsoleModule::cmd_clear(const std::vector<std::string>&) {
    return "__clear__";
}

std::string ConsoleModule::cmd_history(const std::vector<std::string>&) {
    println("=== Command History (last 20) ===", OutputType::INFO);

    size_t start = command_history_.size() > 20 ? command_history_.size() - 20 : 0;
    for (size_t i = start; i < command_history_.size(); i++) {
        println("  " + std::to_string(i + 1) + ". " + command_history_[i], OutputType::INFO);
    }

    if (command_history_.empty()) {
        println("  (no commands in history)", OutputType::INFO);
    }

    return "";
}

std::string ConsoleModule::cmd_height(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "Current height: " + std::to_string(style_.height);
    }

    int new_height = std::atoi(args[0].c_str());
    int max_height = GetScreenHeight() - 50;

    if (new_height < 100 || new_height > max_height) {
        println("Height must be between 100 and " + std::to_string(max_height), OutputType::ERROR);
        return "";
    }

    style_.height = new_height;
    target_y_ = open_ ? 0 : -style_.height;

    return "Console height set to " + std::to_string(new_height);
}

std::string ConsoleModule::cmd_vars(const std::vector<std::string>&) {
#if defined(GMR_DEBUG_ENABLED)
    if (!mrb_) return "Ruby VM not initialized";

    println("=== Global Variables ===", OutputType::INFO);

    // Evaluate to get global variables
    mrbc_context* ctx = mrbc_context_new(mrb_);
    mrbc_filename(mrb_, ctx, "(console)");

    mrb_value result = mrb_load_string_cxt(mrb_,
        "global_variables.select { |v| s = v.to_s; s.start_with?('$') && !s.start_with?('$_') && !s.start_with?('$-') }",
        ctx);

    if (mrb_->exc) {
        scripting::safe_clear_exception(mrb_, "console /vars command");
        mrbc_context_free(mrb_, ctx);
        return "Error listing variables";
    }

    if (mrb_array_p(result)) {
        mrb_int len = RARRAY_LEN(result);
        if (len == 0) {
            println("  (no user-defined global variables)", OutputType::INFO);
        } else {
            for (mrb_int i = 0; i < len; i++) {
                mrb_value var = mrb_ary_ref(mrb_, result, i);
                if (mrb_symbol_p(var)) {
                    const char* var_name = mrb_sym_name(mrb_, mrb_symbol(var));

                    // Get value
                    mrb_value val = mrb_gv_get(mrb_, mrb_symbol(var));
                    mrb_value inspected = mrb_inspect(mrb_, val);

                    if (!mrb_->exc && mrb_string_p(inspected)) {
                        std::string val_str(RSTRING_PTR(inspected), RSTRING_LEN(inspected));
                        if (val_str.length() > 60) {
                            val_str = val_str.substr(0, 60) + "...";
                        }
                        println("  " + std::string(var_name) + " = " + val_str, OutputType::RESULT);
                    }
                    // Clear any exception from mrb_inspect failure
                    scripting::safe_clear_exception(mrb_, "console /vars inspect");
                }
            }
        }
    }

    mrbc_context_free(mrb_, ctx);
#endif
    return "";
}

void ConsoleModule::register_command(const std::string& name,
                                     const std::string& description,
                                     CommandHandler handler) {
    commands_[name] = Command{name, description, std::move(handler)};
}

bool ConsoleModule::has_command(const std::string& name) const {
    return commands_.find(name) != commands_.end();
}

void ConsoleModule::unregister_command(const std::string& name) {
    commands_.erase(name);
}

void ConsoleModule::println(const std::string& text, OutputType type) {
    output_.push_back(OutputEntry{type, text});
    scroll_offset_ = 0;
    trim_history();
}

void ConsoleModule::clear_output() {
    output_.clear();
    scroll_offset_ = 0;
}

void ConsoleModule::trim_history() {
    while (output_.size() > static_cast<size_t>(style_.max_history)) {
        output_.erase(output_.begin());
    }
}

int ConsoleModule::visible_lines() const {
    return (style_.height - 50) / style_.line_height;
}

int ConsoleModule::max_scroll() const {
    return std::max(static_cast<int>(output_.size()) - visible_lines(), 0);
}

void ConsoleModule::get_color_for_type(OutputType type, uint8_t* out) const {
    const uint8_t* src = nullptr;
    switch (type) {
        case OutputType::INPUT:   src = style_.prompt_color; break;
        case OutputType::RESULT:  src = style_.result_color; break;
        case OutputType::ERROR:   src = style_.error_color; break;
        case OutputType::WARNING: src = style_.warning_color; break;
        case OutputType::SYSTEM:  src = style_.info_color; break;
        case OutputType::INFO:
        default:                  src = style_.info_color; break;
    }
    out[0] = src[0]; out[1] = src[1]; out[2] = src[2]; out[3] = src[3];
}

void ConsoleModule::draw() {
    if (!enabled_ || !is_visible()) return;

    draw_background();
    draw_output();
    draw_input_line();
    draw_scrollbar();
}

void ConsoleModule::draw_background() {
    int y = static_cast<int>(current_y_);
    // Always use full screen width to ensure console spans the entire screen
    int width = GetScreenWidth();

    // Main background
    DrawRectangle(0, y, width, style_.height, to_raylib_color(style_.bg_color));

    // Top border (subtle)
    uint8_t top_border[4] = {40, 50, 70, 255};
    DrawRectangle(0, y, width, 1, to_raylib_color(top_border));

    // Bottom border (highlighted)
    DrawRectangle(0, y + style_.height - 2, width, 2, to_raylib_color(style_.border_color));
}

void ConsoleModule::draw_output() {
    int y = static_cast<int>(current_y_);
    // Always use full screen width
    int width = GetScreenWidth();

    int output_height = style_.height - 40;
    int visible_count = output_height / style_.line_height;

    // Calculate visible range
    int total_lines = static_cast<int>(output_.size());
    int start_idx = std::max(total_lines - visible_count - scroll_offset_, 0);
    int end_idx = std::min(total_lines - scroll_offset_, total_lines);

    int draw_y = y + style_.padding;

    // Calculate average char width for truncation (approximate)
    int avg_char_width = measure_console_text("M", style_.font_size, style_.font) ;
    if (avg_char_width < 1) avg_char_width = 7;

    for (int i = start_idx; i < end_idx; i++) {
        const auto& entry = output_[i];
        uint8_t color[4];
        get_color_for_type(entry.type, color);

        std::string text = entry.text;
        // Truncate very long lines
        int max_chars = (width - style_.padding * 2 - 20) / avg_char_width;
        if (static_cast<int>(text.length()) > max_chars && max_chars > 3) {
            text = text.substr(0, max_chars - 3) + "...";
        }

        draw_console_text(text.c_str(), style_.padding, draw_y, style_.font_size, to_raylib_color(color), style_.font);
        draw_y += style_.line_height;
    }
}

void ConsoleModule::draw_input_line() {
    int y = static_cast<int>(current_y_);
    // Always use full screen width
    int width = GetScreenWidth();
    int input_y = y + style_.height - 28;

    // Input background
    uint8_t input_bg[4] = {25, 30, 45, 255};
    DrawRectangle(0, input_y - 4, width, 28, to_raylib_color(input_bg));

    // Prompt
    const char* prompt = in_multiline_ ? ".. " : ">> ";
    const uint8_t* prompt_color = open_ ? style_.prompt_color : style_.info_color;
    draw_console_text(prompt, style_.padding, input_y, style_.font_size, to_raylib_color(prompt_color), style_.font);

    int prompt_width = measure_console_text(prompt, style_.font_size, style_.font);
    int text_x = style_.padding + prompt_width;

    // Input text
    draw_console_text(input_.c_str(), text_x, input_y, style_.font_size, to_raylib_color(style_.input_color), style_.font);

    // Cursor
    if (open_ && cursor_blink_ < 1.0f) {
        std::string before_cursor = input_.substr(0, cursor_pos_);
        int cursor_x = text_x + measure_console_text(before_cursor.c_str(), style_.font_size, style_.font);
        DrawRectangle(cursor_x, input_y, 2, style_.font_size + 2, to_raylib_color(style_.cursor_color));
    }

    // History index indicator
    if (history_index_ >= 0) {
        std::string indicator = "[" + std::to_string(command_history_.size() - history_index_) +
                               "/" + std::to_string(command_history_.size()) + "]";
        int indicator_width = measure_console_text(indicator.c_str(), 12, style_.font);
        uint8_t indicator_color[4] = {80, 80, 100, 255};
        draw_console_text(indicator.c_str(), width - indicator_width - style_.padding, input_y + 1, 12,
                 to_raylib_color(indicator_color), style_.font);
    }
}

void ConsoleModule::draw_scrollbar() {
    if (output_.size() <= static_cast<size_t>(visible_lines())) return;

    int y = static_cast<int>(current_y_);
    int width = (style_.width > 0) ? style_.width : GetScreenWidth();

    int bar_height = style_.height - 50;
    int bar_x = width - 6;
    int bar_y = y + 8;

    // Track
    DrawRectangle(bar_x, bar_y, 4, bar_height, to_raylib_color(style_.scrollbar_bg));

    // Thumb
    float visible_ratio = static_cast<float>(visible_lines()) / output_.size();
    int thumb_height = std::max(static_cast<int>(visible_ratio * bar_height), 20);

    float scroll_ratio = static_cast<float>(scroll_offset_) / std::max(max_scroll(), 1);
    int thumb_y = bar_y + static_cast<int>((1 - scroll_ratio) * (bar_height - thumb_height));

    DrawRectangle(bar_x, thumb_y, 4, thumb_height, to_raylib_color(style_.scrollbar_fg));
}

// ============================================================================
// Ruby Bindings for GMR::Console
// ============================================================================

/// @module GMR::Console
/// @description In-game developer console with command registration and Ruby evaluation.
///   Provides a drop-down terminal for debugging, executing commands, and inspecting state.
///   Enabled in debug builds with Ruby eval; release builds support registered commands only.
/// @example GMR::Console.enable(height: 300, background: "#1a1a2e")
///   GMR::Console.register_command("spawn", "Spawn entity at position") do |*args|
///     x, y = args[0].to_i, args[1].to_i
///     spawn_entity(x, y)
///     "Spawned at #{x}, #{y}"
///   end

// Helper to parse color from Ruby hash
static void parse_color_from_hash(mrb_state* mrb, mrb_value hash, const char* key, uint8_t* out) {
    mrb_value color_key = mrb_symbol_value(mrb_intern_cstr(mrb, key));
    mrb_value color_val = mrb_hash_get(mrb, hash, color_key);

    if (mrb_nil_p(color_val)) return;

    // Support string hex colors like "#222" or "#222222"
    if (mrb_string_p(color_val)) {
        const char* str = mrb_str_to_cstr(mrb, color_val);
        if (str[0] == '#') {
            str++;
            size_t len = strlen(str);
            if (len == 3) {
                // Short form: #RGB -> #RRGGBB
                int r = 0, g = 0, b = 0;
                sscanf(str, "%1x%1x%1x", &r, &g, &b);
                out[0] = r * 17; out[1] = g * 17; out[2] = b * 17; out[3] = 255;
            } else if (len == 6) {
                int r = 0, g = 0, b = 0;
                sscanf(str, "%2x%2x%2x", &r, &g, &b);
                out[0] = r; out[1] = g; out[2] = b; out[3] = 255;
            } else if (len == 8) {
                int r = 0, g = 0, b = 0, a = 0;
                sscanf(str, "%2x%2x%2x%2x", &r, &g, &b, &a);
                out[0] = r; out[1] = g; out[2] = b; out[3] = a;
            }
        }
    }
    // Support array [r, g, b] or [r, g, b, a]
    else if (mrb_array_p(color_val)) {
        mrb_int len = RARRAY_LEN(color_val);
        if (len >= 3) {
            out[0] = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, color_val, 0)));
            out[1] = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, color_val, 1)));
            out[2] = static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, color_val, 2)));
            out[3] = (len > 3) ? static_cast<uint8_t>(mrb_fixnum(mrb_ary_ref(mrb, color_val, 3))) : 255;
        }
    }
}

/// @function enable
/// @description Enable the console with optional styling. Call once during initialization.
/// @param options [Hash] (optional) Styling options including:
///   - :height [Integer] Console height in pixels
///   - :width [Integer] Console width (0 for full screen width)
///   - :background [String, Array] Background color as "#RRGGBB" or [r, g, b, a]
///   - :foreground [String, Array] Default text color
///   - :prompt_color [String, Array] Input prompt color
///   - :result_color [String, Array] Result output color
///   - :error_color [String, Array] Error message color
///   - :font_size [Integer] Font size in pixels
///   - :position [Symbol] :top or :bottom
/// @returns [Module] self for chaining
/// @example GMR::Console.enable
/// @example GMR::Console.enable(height: 400, background: "#222233").allow_ruby_eval
static mrb_value mrb_console_enable(mrb_state* mrb, mrb_value self) {
    mrb_value opts = mrb_nil_value();
    mrb_get_args(mrb, "|o", &opts);  // Accept any object, check if hash

    auto& console = ConsoleModule::instance();

    // Initialize if needed
    if (!console.is_initialized()) {
        console.init(mrb);
    }

    // Apply styling options if provided
    if (!mrb_nil_p(opts) && mrb_hash_p(opts)) {
        ConsoleStyle style = console.style();

        // Colors
        parse_color_from_hash(mrb, opts, "background", style.bg_color);
        parse_color_from_hash(mrb, opts, "foreground", style.fg_color);
        parse_color_from_hash(mrb, opts, "prompt_color", style.prompt_color);
        parse_color_from_hash(mrb, opts, "input_color", style.input_color);
        parse_color_from_hash(mrb, opts, "result_color", style.result_color);
        parse_color_from_hash(mrb, opts, "error_color", style.error_color);
        parse_color_from_hash(mrb, opts, "warning_color", style.warning_color);
        parse_color_from_hash(mrb, opts, "info_color", style.info_color);
        parse_color_from_hash(mrb, opts, "border_color", style.border_color);

        // Layout
        mrb_value font_size = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "font_size")));
        if (!mrb_nil_p(font_size)) style.font_size = mrb_fixnum(font_size);

        mrb_value height = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "height")));
        if (!mrb_nil_p(height)) style.height = mrb_fixnum(height);

        mrb_value width = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "width")));
        if (!mrb_nil_p(width)) style.width = mrb_fixnum(width);

        // Font - accepts a Graphics::Font object
        mrb_value font_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "font")));
        if (!mrb_nil_p(font_val) && mrb_data_p(font_val)) {
            // Get the font data type from graphics module
            RClass* gmr = mrb_module_get(mrb, "GMR");
            RClass* graphics = mrb_class_get_under(mrb, gmr, "Graphics");
            RClass* font_class = mrb_class_get_under(mrb, graphics, "Font");
            if (mrb_obj_is_instance_of(mrb, font_val, font_class)) {
                // Extract FontHandle from Font object
                // FontData struct has handle as first member
                struct FontData { FontHandle handle; };
                auto* font_data = static_cast<FontData*>(DATA_PTR(font_val));
                if (font_data) {
                    style.font = font_data->handle;
                }
            }
        }

        mrb_value position = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_lit(mrb, "position")));
        if (mrb_symbol_p(position)) {
            const char* pos_name = mrb_sym_name(mrb, mrb_symbol(position));
            if (strcmp(pos_name, "top") == 0) style.position = 0;
            else if (strcmp(pos_name, "bottom") == 0) style.position = 1;
        }

        console.configure(style);
    }

    console.enable(true);
    return self;
}

/// @function disable
/// @description Disable the console entirely. Hides it if currently open.
/// @returns [Module] self for chaining
/// @example GMR::Console.disable
static mrb_value mrb_console_disable(mrb_state*, mrb_value self) {
    ConsoleModule::instance().enable(false);
    return self;
}

/// @function enabled?
/// @description Check if the console is enabled.
/// @returns [Boolean] true if the console is enabled
/// @example if GMR::Console.enabled?
///   puts "Console is available"
/// end
static mrb_value mrb_console_enabled(mrb_state*, mrb_value) {
    return mrb_bool_value(ConsoleModule::instance().is_enabled());
}

/// @function open?
/// @description Check if the console is currently visible/open.
/// @returns [Boolean] true if the console is open
/// @example pause_game if GMR::Console.open?
static mrb_value mrb_console_open(mrb_state*, mrb_value) {
    return mrb_bool_value(ConsoleModule::instance().is_open());
}

/// @function show
/// @description Open the console (slide it into view).
/// @returns [Module] self for chaining
/// @example GMR::Console.show
static mrb_value mrb_console_show(mrb_state*, mrb_value self) {
    ConsoleModule::instance().show();
    return self;
}

/// @function hide
/// @description Close the console (slide it out of view).
/// @returns [Module] self for chaining
/// @example GMR::Console.hide
static mrb_value mrb_console_hide(mrb_state*, mrb_value self) {
    ConsoleModule::instance().hide();
    return self;
}

/// @function toggle
/// @description Toggle the console open/closed.
/// @returns [Module] self for chaining
/// @example GMR::Console.toggle
static mrb_value mrb_console_toggle(mrb_state*, mrb_value self) {
    ConsoleModule::instance().toggle();
    return self;
}

/// @function println
/// @description Print a line of text to the console output.
/// @param text [String] The text to print
/// @param type [Symbol] (optional) Output type: :info, :input, :result, :error, :warning, :system
/// @returns [Module] self for chaining
/// @example GMR::Console.println("Player spawned", :info)
/// @example GMR::Console.println("Line 1").println("Line 2")
static mrb_value mrb_console_println(mrb_state* mrb, mrb_value self) {
    const char* text;
    mrb_sym type_sym = mrb_intern_lit(mrb, "info");
    mrb_get_args(mrb, "z|n", &text, &type_sym);

    OutputType type = OutputType::INFO;
    const char* type_name = mrb_sym_name(mrb, type_sym);
    if (strcmp(type_name, "input") == 0) type = OutputType::INPUT;
    else if (strcmp(type_name, "result") == 0) type = OutputType::RESULT;
    else if (strcmp(type_name, "error") == 0) type = OutputType::ERROR;
    else if (strcmp(type_name, "warning") == 0 || strcmp(type_name, "warn") == 0) type = OutputType::WARNING;
    else if (strcmp(type_name, "system") == 0) type = OutputType::SYSTEM;

    ConsoleModule::instance().println(text, type);
    return self;
}

/// @function clear
/// @description Clear all output from the console.
/// @returns [Module] self for chaining
/// @example GMR::Console.clear
static mrb_value mrb_console_clear(mrb_state*, mrb_value self) {
    ConsoleModule::instance().clear_output();
    return self;
}

// Store Ruby command blocks
static mrb_value g_ruby_commands;  // Hash of name -> proc

/// @function register_command
/// @description Register a custom console command. The block receives command arguments
///   as an array and should return a string result (or nil for no output).
/// @param name [String] Command name (case-insensitive)
/// @param description [String] (optional) Help text shown in command list
/// @yields [Array<String>] Block receives command arguments as strings
/// @returns [Module] self for chaining
/// @example GMR::Console.register_command("tp") { |a| "Teleported" }.register_command("spawn") { "Spawned" }
static mrb_value mrb_console_register_command(mrb_state* mrb, mrb_value self) {
    const char* name;
    const char* description = "";
    mrb_value block;
    mrb_get_args(mrb, "z|z&", &name, &description, &block);

    if (mrb_nil_p(block)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "register_command requires a block");
        return mrb_nil_value();
    }

    // Store the block in our Ruby-side hash
    if (mrb_nil_p(g_ruby_commands)) {
        g_ruby_commands = mrb_hash_new(mrb);
        mrb_gc_register(mrb, g_ruby_commands);
    }
    mrb_hash_set(mrb, g_ruby_commands, mrb_str_new_cstr(mrb, name), block);

    // Register the command in C++
    std::string cmd_name = name;
    std::string cmd_desc = description;

    ConsoleModule::instance().register_command(cmd_name, cmd_desc,
        [mrb, cmd_name](const std::vector<std::string>& args) -> std::string {
            mrb_value block = mrb_hash_get(mrb, g_ruby_commands, mrb_str_new_cstr(mrb, cmd_name.c_str()));
            if (mrb_nil_p(block)) {
                return "Command not found";
            }

            // Convert args to Ruby array
            mrb_value rb_args = mrb_ary_new_capa(mrb, static_cast<mrb_int>(args.size()));
            for (const auto& arg : args) {
                mrb_ary_push(mrb, rb_args, mrb_str_new_cstr(mrb, arg.c_str()));
            }

            // Call the block with args
            mrb_value result = mrb_funcall_with_block(mrb, block, mrb_intern_lit(mrb, "call"),
                                                      1, &rb_args, mrb_nil_value());

            if (mrb->exc) {
                mrb_value exc = mrb_obj_value(mrb->exc);
                mrb_value msg = mrb_inspect(mrb, exc);
                std::string error_str = mrb_string_p(msg)
                    ? std::string(RSTRING_PTR(msg), RSTRING_LEN(msg))
                    : "Error executing command";
                scripting::safe_clear_exception(mrb, "console command handler");
                return error_str;
            }

            if (mrb_nil_p(result)) {
                return "";
            }

            mrb_value str = mrb_funcall(mrb, result, "to_s", 0);
            return std::string(RSTRING_PTR(str), RSTRING_LEN(str));
        });

    return self;
}

/// @function unregister_command
/// @description Remove a previously registered console command.
/// @param name [String] Command name to remove
/// @returns [Module] self for chaining
/// @example GMR::Console.unregister_command("teleport")
static mrb_value mrb_console_unregister_command(mrb_state* mrb, mrb_value self) {
    const char* name;
    mrb_get_args(mrb, "z", &name);

    ConsoleModule::instance().unregister_command(name);

    // Remove from Ruby hash too
    if (!mrb_nil_p(g_ruby_commands)) {
        mrb_hash_delete_key(mrb, g_ruby_commands, mrb_str_new_cstr(mrb, name));
    }

    return self;
}

/// @function set_toggle_key
/// @description Set the keyboard key that toggles the console open/closed.
///   Default is backtick/grave (`) key.
/// @param key [Integer] Key code (use GMR::Input key constants)
/// @returns [Module] self for chaining
/// @example GMR::Console.set_toggle_key(GMR::Input::KEY_F12)
static mrb_value mrb_console_set_toggle_key(mrb_state* mrb, mrb_value self) {
    mrb_int key;
    mrb_get_args(mrb, "i", &key);
    ConsoleModule::instance().set_toggle_key(static_cast<int>(key));
    return self;
}

/// @function allow_ruby_eval
/// @description Enable or disable arbitrary Ruby code evaluation in the console.
/// @param allow [Boolean] (optional, default: true) true to allow Ruby eval, false to restrict to commands only
/// @returns [Module] self for chaining
/// @example GMR::Console.enable.allow_ruby_eval
/// @example GMR::Console.allow_ruby_eval(false)
static mrb_value mrb_console_allow_ruby_eval(mrb_state* mrb, mrb_value self) {
    mrb_bool allow = true;
    mrb_get_args(mrb, "|b", &allow);
    ConsoleModule::instance().allow_ruby_eval(allow);
    return self;
}

/// @function ruby_eval_allowed?
/// @description Check if Ruby code evaluation is allowed in the console.
/// @returns [Boolean] true if Ruby eval is enabled
/// @example if GMR::Console.ruby_eval_allowed?
///   puts "Full Ruby access enabled"
/// end
static mrb_value mrb_console_ruby_eval_allowed(mrb_state*, mrb_value) {
    return mrb_bool_value(ConsoleModule::instance().ruby_eval_allowed());
}

// ============================================================================
// Kernel method overrides for p, puts, print
// These output to both stdout AND the console
// ============================================================================

/// Override Kernel#puts to output to console and stdout
static mrb_value mrb_kernel_puts_override(mrb_state* mrb, mrb_value self) {
    mrb_value* argv;
    mrb_int argc;
    mrb_get_args(mrb, "*", &argv, &argc);

    auto& console = ConsoleModule::instance();

    if (argc == 0) {
        // puts with no args prints empty line
        printf("\n");
        if (console.is_initialized()) {
            console.println("", OutputType::INFO);
        }
    } else {
        for (mrb_int i = 0; i < argc; i++) {
            mrb_value str;
            if (mrb_array_p(argv[i])) {
                // For arrays, print each element on its own line
                mrb_int len = RARRAY_LEN(argv[i]);
                for (mrb_int j = 0; j < len; j++) {
                    mrb_value elem = mrb_ary_ref(mrb, argv[i], j);
                    str = mrb_funcall(mrb, elem, "to_s", 0);
                    printf("%.*s\n", (int)RSTRING_LEN(str), RSTRING_PTR(str));
                    if (console.is_initialized()) {
                        console.println(std::string(RSTRING_PTR(str), RSTRING_LEN(str)), OutputType::INFO);
                    }
                }
            } else {
                str = mrb_funcall(mrb, argv[i], "to_s", 0);
                printf("%.*s\n", (int)RSTRING_LEN(str), RSTRING_PTR(str));
                if (console.is_initialized()) {
                    console.println(std::string(RSTRING_PTR(str), RSTRING_LEN(str)), OutputType::INFO);
                }
            }
        }
    }

    return mrb_nil_value();
}

/// Override Kernel#print to output to console and stdout (no newline)
static mrb_value mrb_kernel_print_override(mrb_state* mrb, mrb_value self) {
    mrb_value* argv;
    mrb_int argc;
    mrb_get_args(mrb, "*", &argv, &argc);

    auto& console = ConsoleModule::instance();
    std::string combined;

    for (mrb_int i = 0; i < argc; i++) {
        mrb_value str = mrb_funcall(mrb, argv[i], "to_s", 0);
        printf("%.*s", (int)RSTRING_LEN(str), RSTRING_PTR(str));
        combined += std::string(RSTRING_PTR(str), RSTRING_LEN(str));
    }

    // For console, we still add as a line (console doesn't support partial lines)
    if (console.is_initialized() && !combined.empty()) {
        console.println(combined, OutputType::INFO);
    }

    return mrb_nil_value();
}

/// Override Kernel#p to output inspect format to console and stdout
static mrb_value mrb_kernel_p_override(mrb_state* mrb, mrb_value self) {
    mrb_value* argv;
    mrb_int argc;
    mrb_get_args(mrb, "*", &argv, &argc);

    auto& console = ConsoleModule::instance();

    for (mrb_int i = 0; i < argc; i++) {
        mrb_value str = mrb_funcall(mrb, argv[i], "inspect", 0);
        printf("%.*s\n", (int)RSTRING_LEN(str), RSTRING_PTR(str));
        if (console.is_initialized()) {
            console.println(std::string(RSTRING_PTR(str), RSTRING_LEN(str)), OutputType::INFO);
        }
    }

    // p returns the argument (or array of arguments if multiple)
    if (argc == 0) {
        return mrb_nil_value();
    } else if (argc == 1) {
        return argv[0];
    } else {
        return mrb_ary_new_from_values(mrb, argc, argv);
    }
}

void register_console_module(mrb_state* mrb) {
    // Initialize the Ruby commands hash
    g_ruby_commands = mrb_nil_value();

    // Get or create GMR module
    RClass* gmr = mrb_module_get(mrb, "GMR");

    // Create GMR::Console module
    RClass* console = mrb_define_module_under(mrb, gmr, "Console");

    // Configuration
    mrb_define_module_function(mrb, console, "enable", mrb_console_enable, MRB_ARGS_OPT(1));
    mrb_define_module_function(mrb, console, "disable", mrb_console_disable, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, console, "enabled?", mrb_console_enabled, MRB_ARGS_NONE());

    // State
    mrb_define_module_function(mrb, console, "open?", mrb_console_open, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, console, "show", mrb_console_show, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, console, "hide", mrb_console_hide, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, console, "toggle", mrb_console_toggle, MRB_ARGS_NONE());

    // Output
    mrb_define_module_function(mrb, console, "println", mrb_console_println, MRB_ARGS_ARG(1, 1));
    mrb_define_module_function(mrb, console, "clear", mrb_console_clear, MRB_ARGS_NONE());

    // Commands
    mrb_define_module_function(mrb, console, "register_command", mrb_console_register_command, MRB_ARGS_ARG(1, 1) | MRB_ARGS_BLOCK());
    mrb_define_module_function(mrb, console, "unregister_command", mrb_console_unregister_command, MRB_ARGS_REQ(1));

    // Configuration
    mrb_define_module_function(mrb, console, "set_toggle_key", mrb_console_set_toggle_key, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, console, "allow_ruby_eval", mrb_console_allow_ruby_eval, MRB_ARGS_OPT(1));
    mrb_define_module_function(mrb, console, "ruby_eval_allowed?", mrb_console_ruby_eval_allowed, MRB_ARGS_NONE());

    // Initialize the console module with mruby state
    ConsoleModule::instance().init(mrb);

    // Override Kernel methods to output to console as well as stdout
    RClass* kernel = mrb->kernel_module;
    mrb_define_method(mrb, kernel, "puts", mrb_kernel_puts_override, MRB_ARGS_ANY());
    mrb_define_method(mrb, kernel, "print", mrb_kernel_print_override, MRB_ARGS_ANY());
    mrb_define_method(mrb, kernel, "p", mrb_kernel_p_override, MRB_ARGS_ANY());
}

} // namespace console
} // namespace gmr
