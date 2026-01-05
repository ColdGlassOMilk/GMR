#ifndef GMR_REPL_MULTILINE_DETECTOR_HPP
#define GMR_REPL_MULTILINE_DETECTOR_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <string>

namespace gmr {
namespace repl {

// Detects whether Ruby code is complete or requires more input
// Used for multi-line REPL input in Debug builds
class MultilineDetector {
public:
    // Check if the given code appears to be a complete Ruby expression
    // Returns true if complete, false if more input is needed
    static bool is_complete(const std::string& code);

private:
    // Parse state for tracking open constructs
    struct ParseState {
        int paren_depth = 0;     // ( )
        int bracket_depth = 0;   // [ ]
        int brace_depth = 0;     // { }
        int block_depth = 0;     // def/end, class/end, etc.

        bool in_single_string = false;
        bool in_double_string = false;
        bool in_heredoc = false;
        std::string heredoc_terminator;

        bool line_continues = false;  // backslash at EOL

        bool is_balanced() const {
            return paren_depth == 0 &&
                   bracket_depth == 0 &&
                   brace_depth == 0 &&
                   block_depth == 0 &&
                   !in_single_string &&
                   !in_double_string &&
                   !in_heredoc &&
                   !line_continues;
        }
    };

    // Check if a character is part of an identifier
    static bool is_identifier_char(char c);

    // Check if keyword at position is in modifier form (e.g., "x if y")
    static bool is_modifier_form(const std::string& code, size_t pos,
                                 const std::string& keyword);

    // Extract a word starting at position
    static std::string extract_word(const std::string& code, size_t pos);
};

} // namespace repl
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_REPL_MULTILINE_DETECTOR_HPP
