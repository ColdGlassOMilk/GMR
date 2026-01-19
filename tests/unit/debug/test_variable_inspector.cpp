// Debug Variable Inspector Tests
// Tests frame-aware expression evaluation and variable inspection
// These tests require GMR_DEBUG_ENABLED (Debug builds only)

#include <catch2/catch_test_macros.hpp>
#include "test_fixtures.hpp"

#if defined(GMR_DEBUG_ENABLED)

#include "gmr/debug/variable_inspector.hpp"
#include "gmr/debug/json_protocol.hpp"
#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/variable.h>

using namespace gmr::debug;

// Test fixture that creates and manages an mruby state
struct MrubyTestFixture {
    mrb_state* mrb;

    MrubyTestFixture() {
        mrb = mrb_open();
        REQUIRE(mrb != nullptr);
    }

    ~MrubyTestFixture() {
        if (mrb) {
            mrb_close(mrb);
        }
    }

    // Helper to execute Ruby code and leave the VM in a specific state
    mrb_value eval(const char* code) {
        return mrb_load_string(mrb, code);
    }

    // Helper to check if execution succeeded
    bool check_no_exception() {
        if (mrb->exc) {
            mrb->exc = nullptr;
            return false;
        }
        return true;
    }
};

TEST_CASE_METHOD(MrubyTestFixture, "json_escape escapes special characters", "[debug][json]") {
    SECTION("escapes quotes") {
        std::string result = json_escape("hello \"world\"");
        REQUIRE(result == "hello \\\"world\\\"");
    }

    SECTION("escapes backslashes") {
        std::string result = json_escape("path\\to\\file");
        REQUIRE(result == "path\\\\to\\\\file");
    }

    SECTION("escapes newlines and tabs") {
        std::string result = json_escape("line1\nline2\ttab");
        REQUIRE(result == "line1\\nline2\\ttab");
    }

    SECTION("handles empty string") {
        std::string result = json_escape("");
        REQUIRE(result == "");
    }

    SECTION("handles plain string unchanged") {
        std::string result = json_escape("hello world");
        REQUIRE(result == "hello world");
    }
}

TEST_CASE_METHOD(MrubyTestFixture, "serialize_value handles basic types", "[debug][serialize]") {
    SerializeContext ctx;

    SECTION("serializes nil") {
        mrb_value val = mrb_nil_value();
        std::string result = serialize_value(mrb, val, ctx);
        REQUIRE(result.find("\"type\":\"nil\"") != std::string::npos);
        REQUIRE(result.find("\"value\":\"nil\"") != std::string::npos);
    }

    SECTION("serializes true") {
        mrb_value val = mrb_true_value();
        std::string result = serialize_value(mrb, val, ctx);
        REQUIRE(result.find("\"type\":\"Boolean\"") != std::string::npos);
        REQUIRE(result.find("\"value\":\"true\"") != std::string::npos);
    }

    SECTION("serializes false") {
        mrb_value val = mrb_false_value();
        std::string result = serialize_value(mrb, val, ctx);
        REQUIRE(result.find("\"type\":\"Boolean\"") != std::string::npos);
        REQUIRE(result.find("\"value\":\"false\"") != std::string::npos);
    }

    SECTION("serializes integers") {
        mrb_value val = mrb_fixnum_value(42);
        std::string result = serialize_value(mrb, val, ctx);
        REQUIRE(result.find("\"type\":\"Integer\"") != std::string::npos);
        REQUIRE(result.find("\"value\":\"42\"") != std::string::npos);
    }

    SECTION("serializes strings") {
        mrb_value val = mrb_str_new_cstr(mrb, "hello");
        std::string result = serialize_value(mrb, val, ctx);
        REQUIRE(result.find("\"type\":\"String\"") != std::string::npos);
        REQUIRE(result.find("\"value\":\"hello\"") != std::string::npos);
    }

    SECTION("serializes symbols") {
        mrb_value val = mrb_symbol_value(mrb_intern_cstr(mrb, "test_sym"));
        std::string result = serialize_value(mrb, val, ctx);
        REQUIRE(result.find("\"type\":\"Symbol\"") != std::string::npos);
        REQUIRE(result.find(":test_sym") != std::string::npos);
    }
}

TEST_CASE_METHOD(MrubyTestFixture, "serialize_value handles collections", "[debug][serialize]") {
    SerializeContext ctx;

    SECTION("serializes arrays") {
        mrb_value arr = mrb_ary_new(mrb);
        mrb_ary_push(mrb, arr, mrb_fixnum_value(1));
        mrb_ary_push(mrb, arr, mrb_fixnum_value(2));
        mrb_ary_push(mrb, arr, mrb_fixnum_value(3));

        std::string result = serialize_value(mrb, arr, ctx);
        REQUIRE(result.find("\"type\":\"Array\"") != std::string::npos);
        REQUIRE(result.find("3 elements") != std::string::npos);
        REQUIRE(result.find("\"elements\":[") != std::string::npos);
    }

    SECTION("serializes hashes") {
        mrb_value hash = mrb_hash_new(mrb);
        mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_cstr(mrb, "key")),
                     mrb_fixnum_value(123));

        std::string result = serialize_value(mrb, hash, ctx);
        REQUIRE(result.find("\"type\":\"Hash\"") != std::string::npos);
        REQUIRE(result.find("1 pairs") != std::string::npos);
    }
}

TEST_CASE_METHOD(MrubyTestFixture, "serialize_value respects depth limit", "[debug][serialize]") {
    SerializeContext ctx;
    ctx.max_depth = 2;

    // Create nested arrays: [[[[1]]]]
    mrb_value inner = mrb_ary_new(mrb);
    mrb_ary_push(mrb, inner, mrb_fixnum_value(1));

    for (int i = 0; i < 5; ++i) {
        mrb_value outer = mrb_ary_new(mrb);
        mrb_ary_push(mrb, outer, inner);
        inner = outer;
    }

    std::string result = serialize_value(mrb, inner, ctx);
    REQUIRE(result.find("<max depth>") != std::string::npos);
}

TEST_CASE_METHOD(MrubyTestFixture, "evaluate_expression handles invalid inputs", "[debug][evaluate]") {
    SECTION("returns error for null mrb_state") {
        std::string result = evaluate_expression(nullptr, "1 + 1", 0);
        REQUIRE(result.find("\"type\":\"Error\"") != std::string::npos);
        REQUIRE(result.find("Invalid expression") != std::string::npos);
    }

    SECTION("returns error for empty expression") {
        std::string result = evaluate_expression(mrb, "", 0);
        REQUIRE(result.find("\"type\":\"Error\"") != std::string::npos);
        REQUIRE(result.find("Invalid expression") != std::string::npos);
    }
}

TEST_CASE_METHOD(MrubyTestFixture, "evaluate_expression requires valid call frame", "[debug][evaluate]") {
    // The new frame-aware evaluate_expression requires a valid call frame
    // A fresh mruby state has no call frame, so it should return an error

    SECTION("returns error when no call frame exists") {
        std::string result = evaluate_expression(mrb, "2 + 3", 0);
        // Should return error because there's no valid frame
        REQUIRE(result.find("\"type\":\"Error\"") != std::string::npos);
        REQUIRE(result.find("Invalid frame index") != std::string::npos);
    }

    SECTION("returns error for negative frame index") {
        std::string result = evaluate_expression(mrb, "1 + 1", -1);
        REQUIRE(result.find("\"type\":\"Error\"") != std::string::npos);
        REQUIRE(result.find("Invalid frame index") != std::string::npos);
    }

    SECTION("returns error for large frame index") {
        std::string result = evaluate_expression(mrb, "1 + 1", 999);
        REQUIRE(result.find("\"type\":\"Error\"") != std::string::npos);
        REQUIRE(result.find("Invalid frame index") != std::string::npos);
    }
}

TEST_CASE_METHOD(MrubyTestFixture, "get_stack_trace_json returns valid JSON", "[debug][stack]") {
    SECTION("handles empty/minimal call stack") {
        std::string result = get_stack_trace_json(mrb);
        // Should be a JSON array (possibly empty)
        REQUIRE(result[0] == '[');
        REQUIRE(result[result.length() - 1] == ']');
    }
}

TEST_CASE_METHOD(MrubyTestFixture, "get_locals_json returns valid JSON", "[debug][locals]") {
    SECTION("handles frame 0 with no locals") {
        std::string result = get_locals_json(mrb, 0);
        // Should be a JSON object
        REQUIRE(result[0] == '{');
        REQUIRE(result[result.length() - 1] == '}');
    }
}

TEST_CASE("make_variables_response generates correct JSON", "[debug][protocol]") {
    std::string variables = "{\"x\":{\"type\":\"Integer\",\"value\":\"42\"}}";
    std::string result = make_variables_response(variables);

    REQUIRE(result.find("\"type\":\"variables_response\"") != std::string::npos);
    REQUIRE(result.find("\"variables\":") != std::string::npos);
    REQUIRE(result.find(variables) != std::string::npos);
}

TEST_CASE("make_evaluate_response generates correct JSON", "[debug][protocol]") {
    SECTION("success response") {
        std::string result_value = "{\"type\":\"Integer\",\"value\":\"42\"}";
        std::string result = make_evaluate_response(true, result_value);

        REQUIRE(result.find("\"type\":\"evaluate_response\"") != std::string::npos);
        REQUIRE(result.find("\"success\":true") != std::string::npos);
        REQUIRE(result.find("\"result\":") != std::string::npos);
    }

    SECTION("failure response") {
        std::string result_value = "{\"type\":\"Error\",\"value\":\"oops\"}";
        std::string result = make_evaluate_response(false, result_value);

        REQUIRE(result.find("\"success\":false") != std::string::npos);
    }
}

TEST_CASE("parse_command parses evaluate command", "[debug][protocol]") {
    std::string json = R"({"type":"evaluate","expression":"x + 1","frame_id":2})";
    DebugCommand cmd = parse_command(json);

    REQUIRE(cmd.type == CommandType::EVALUATE);
    REQUIRE(cmd.expression == "x + 1");
    REQUIRE(cmd.frame_id == 2);
}

TEST_CASE("parse_command parses get_variables command", "[debug][protocol]") {
    std::string json = R"({"type":"get_variables","frame_id":1})";
    DebugCommand cmd = parse_command(json);

    REQUIRE(cmd.type == CommandType::GET_VARIABLES);
    REQUIRE(cmd.frame_id == 1);
}

TEST_CASE("parse_command parses breakpoint commands", "[debug][protocol]") {
    SECTION("set_breakpoint") {
        std::string json = R"({"type":"set_breakpoint","file":"test.rb","line":42})";
        DebugCommand cmd = parse_command(json);

        REQUIRE(cmd.type == CommandType::SET_BREAKPOINT);
        REQUIRE(cmd.file == "test.rb");
        REQUIRE(cmd.line == 42);
    }

    SECTION("clear_breakpoint") {
        std::string json = R"({"type":"clear_breakpoint","file":"test.rb","line":42})";
        DebugCommand cmd = parse_command(json);

        REQUIRE(cmd.type == CommandType::CLEAR_BREAKPOINT);
    }
}

TEST_CASE("parse_command parses stepping commands", "[debug][protocol]") {
    SECTION("continue") {
        DebugCommand cmd = parse_command(R"({"type":"continue"})");
        REQUIRE(cmd.type == CommandType::CONTINUE);
    }

    SECTION("step_over") {
        DebugCommand cmd = parse_command(R"({"type":"step_over"})");
        REQUIRE(cmd.type == CommandType::STEP_OVER);
    }

    SECTION("step_into") {
        DebugCommand cmd = parse_command(R"({"type":"step_into"})");
        REQUIRE(cmd.type == CommandType::STEP_INTO);
    }

    SECTION("step_out") {
        DebugCommand cmd = parse_command(R"({"type":"step_out"})");
        REQUIRE(cmd.type == CommandType::STEP_OUT);
    }
}

#else // GMR_DEBUG_ENABLED not defined

// Placeholder test when debug is disabled
TEST_CASE("Debug tests skipped (Release build)", "[debug][skip]") {
    WARN("Debug variable inspector tests require GMR_DEBUG_ENABLED (Debug build)");
    REQUIRE(true);  // Always pass
}

#endif // GMR_DEBUG_ENABLED
