#include "gmr/repl/multiline_detector.hpp"

#if defined(GMR_DEBUG_ENABLED)

#include <cctype>
#include <cstring>

namespace gmr {
namespace repl {

// Block-opening keywords
static const char* BLOCK_OPENERS[] = {
    "def", "class", "module", "do", "if", "unless",
    "case", "while", "until", "for", "begin", nullptr
};

// Keywords that can be modifiers (appear after expression)
static const char* MODIFIER_KEYWORDS[] = {
    "if", "unless", "while", "until", "rescue", nullptr
};

bool MultilineDetector::is_identifier_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string MultilineDetector::extract_word(const std::string& code, size_t pos) {
    size_t end = pos;
    while (end < code.size() && is_identifier_char(code[end])) {
        end++;
    }
    // Also include ? and ! at end of method names
    if (end < code.size() && (code[end] == '?' || code[end] == '!')) {
        end++;
    }
    return code.substr(pos, end - pos);
}

bool MultilineDetector::is_modifier_form(const std::string& code, size_t pos,
                                          const std::string& keyword) {
    // Check if this keyword is being used as a modifier
    // Modifiers appear after an expression, not at line start

    // First check if this is even a modifier-capable keyword
    bool is_modifier_keyword = false;
    for (const char** kw = MODIFIER_KEYWORDS; *kw; kw++) {
        if (keyword == *kw) {
            is_modifier_keyword = true;
            break;
        }
    }
    if (!is_modifier_keyword) {
        return false;
    }

    // Look backward for expression content
    size_t check = pos;

    // Skip whitespace (but not newlines)
    while (check > 0 && code[check - 1] == ' ') {
        check--;
    }

    // If at line start or after specific characters, it's a block opener
    if (check == 0) {
        return false;  // At start of code
    }

    char prev = code[check - 1];

    // After these, it's a block opener, not modifier
    if (prev == '\n' || prev == ';' || prev == '(' || prev == '[' ||
        prev == '{' || prev == '=' || prev == ',') {
        return false;
    }

    // After these, it's a modifier
    if (prev == ')' || prev == ']' || prev == '}' ||
        prev == '"' || prev == '\'' ||
        std::isalnum(static_cast<unsigned char>(prev)) ||
        prev == '_' || prev == '?' || prev == '!') {
        return true;
    }

    return false;
}

bool MultilineDetector::is_complete(const std::string& code) {
    if (code.empty()) {
        return true;
    }

    ParseState state;
    size_t i = 0;
    size_t len = code.size();

    while (i < len) {
        char c = code[i];
        char next = (i + 1 < len) ? code[i + 1] : '\0';

        // Handle heredocs first (they span lines)
        if (state.in_heredoc) {
            // Find end of line
            size_t line_start = i;
            size_t line_end = code.find('\n', i);
            if (line_end == std::string::npos) {
                line_end = len;
            }

            // Extract line and trim whitespace
            std::string line = code.substr(line_start, line_end - line_start);
            size_t trim_start = 0;
            size_t trim_end = line.size();
            while (trim_start < trim_end && std::isspace(static_cast<unsigned char>(line[trim_start]))) {
                trim_start++;
            }
            while (trim_end > trim_start && std::isspace(static_cast<unsigned char>(line[trim_end - 1]))) {
                trim_end--;
            }
            std::string trimmed = line.substr(trim_start, trim_end - trim_start);

            if (trimmed == state.heredoc_terminator) {
                state.in_heredoc = false;
                state.heredoc_terminator.clear();
            }

            i = (line_end < len) ? line_end + 1 : len;
            continue;
        }

        // Skip comments (# to end of line)
        if (c == '#' && !state.in_single_string && !state.in_double_string) {
            while (i < len && code[i] != '\n') {
                i++;
            }
            continue;
        }

        // Handle single-quoted strings
        if (state.in_single_string) {
            if (c == '\\' && next != '\0') {
                i += 2;  // Skip escaped character
                continue;
            }
            if (c == '\'') {
                state.in_single_string = false;
            }
            i++;
            continue;
        }

        // Handle double-quoted strings
        if (state.in_double_string) {
            if (c == '\\' && next != '\0') {
                i += 2;  // Skip escaped character
                continue;
            }
            if (c == '"') {
                state.in_double_string = false;
            }
            i++;
            continue;
        }

        // Start of single-quoted string
        if (c == '\'') {
            state.in_single_string = true;
            i++;
            continue;
        }

        // Start of double-quoted string
        if (c == '"') {
            state.in_double_string = true;
            i++;
            continue;
        }

        // Check for heredoc start: <<IDENTIFIER, <<-IDENTIFIER, or <<~IDENTIFIER
        if (c == '<' && next == '<') {
            size_t hd_start = i + 2;

            // Skip - or ~ modifier
            if (hd_start < len && (code[hd_start] == '-' || code[hd_start] == '~')) {
                hd_start++;
            }

            // Check for quoted heredoc: <<'ID' or <<"ID"
            char quote_char = '\0';
            if (hd_start < len && (code[hd_start] == '\'' || code[hd_start] == '"')) {
                quote_char = code[hd_start];
                hd_start++;
            }

            // Extract identifier
            size_t hd_end = hd_start;
            while (hd_end < len && is_identifier_char(code[hd_end])) {
                hd_end++;
            }

            if (hd_end > hd_start) {
                state.heredoc_terminator = code.substr(hd_start, hd_end - hd_start);
                state.in_heredoc = true;

                // Skip closing quote if present
                if (quote_char && hd_end < len && code[hd_end] == quote_char) {
                    hd_end++;
                }

                i = hd_end;
                continue;
            }
        }

        // Brackets
        if (c == '(') { state.paren_depth++; i++; continue; }
        if (c == ')') { state.paren_depth--; i++; continue; }
        if (c == '[') { state.bracket_depth++; i++; continue; }
        if (c == ']') { state.bracket_depth--; i++; continue; }
        if (c == '{') { state.brace_depth++; i++; continue; }
        if (c == '}') { state.brace_depth--; i++; continue; }

        // Check for block keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string word = extract_word(code, i);
            size_t word_len = word.size();

            // Check for block openers
            for (const char** kw = BLOCK_OPENERS; *kw; kw++) {
                if (word == *kw) {
                    // Don't count modifier forms
                    if (!is_modifier_form(code, i, word)) {
                        state.block_depth++;
                    }
                    break;
                }
            }

            // Check for block closer
            if (word == "end") {
                state.block_depth--;
            }

            i += word_len;
            continue;
        }

        // Line continuation (backslash before newline)
        if (c == '\\' && next == '\n') {
            state.line_continues = true;
            i += 2;
            continue;
        }

        // Newline clears line continuation (unless preceded by backslash, handled above)
        if (c == '\n') {
            state.line_continues = false;
        }

        i++;
    }

    return state.is_balanced();
}

} // namespace repl
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
