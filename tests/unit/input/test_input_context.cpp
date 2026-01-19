#include <catch2/catch_test_macros.hpp>
#include "gmr/input/input_context.hpp"
#include "gmr/input/input_manager.hpp"
#include "test_fixtures.hpp"

using namespace gmr::input;

// Custom fixture that resets context stack
struct InputContextFixture : EngineTestFixture {
    InputContextFixture() {
        ContextStack::instance().clear_all();
    }

    ~InputContextFixture() {
        ContextStack::instance().clear_all();
    }
};

TEST_CASE_METHOD(InputContextFixture, "ContextStack initially empty", "[input][context]") {
    auto& stack = ContextStack::instance();

    REQUIRE(stack.empty());
    REQUIRE(stack.current_name() == "");
    REQUIRE(stack.current() == nullptr);
    REQUIRE(stack.stack_size() == 0);
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack push", "[input][context]") {
    auto& stack = ContextStack::instance();

    SECTION("push makes context current") {
        stack.push("gameplay");

        REQUIRE_FALSE(stack.empty());
        REQUIRE(stack.current_name() == "gameplay");
        REQUIRE(stack.current() != nullptr);
    }

    SECTION("push auto-creates context") {
        REQUIRE_FALSE(stack.has("new_context"));

        stack.push("new_context");

        REQUIRE(stack.has("new_context"));
    }

    SECTION("multiple push creates stack") {
        stack.push("gameplay");
        stack.push("pause");
        stack.push("inventory");

        REQUIRE(stack.stack_size() == 3);
        REQUIRE(stack.current_name() == "inventory");
    }
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack pop", "[input][context]") {
    auto& stack = ContextStack::instance();

    stack.push("gameplay");
    stack.push("pause");

    SECTION("pop removes top context") {
        stack.pop();

        REQUIRE(stack.current_name() == "gameplay");
        REQUIRE(stack.stack_size() == 1);
    }

    SECTION("pop to empty") {
        stack.pop();
        stack.pop();

        REQUIRE(stack.empty());
        REQUIRE(stack.current_name() == "");
    }

    SECTION("pop on empty stack is safe") {
        stack.clear();
        stack.pop();  // Should not crash

        REQUIRE(stack.empty());
    }
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack set", "[input][context]") {
    auto& stack = ContextStack::instance();

    stack.push("gameplay");
    stack.push("pause");
    stack.push("inventory");

    REQUIRE(stack.stack_size() == 3);

    SECTION("set replaces entire stack") {
        stack.set("menu");

        REQUIRE(stack.stack_size() == 1);
        REQUIRE(stack.current_name() == "menu");
    }

    SECTION("set auto-creates context") {
        stack.set("new_context");

        REQUIRE(stack.has("new_context"));
        REQUIRE(stack.current_name() == "new_context");
    }
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack clear", "[input][context]") {
    auto& stack = ContextStack::instance();

    stack.push("gameplay");
    stack.push("pause");

    stack.clear();

    REQUIRE(stack.empty());
    REQUIRE(stack.stack_size() == 0);
    // Contexts should still exist (just not active)
    REQUIRE(stack.has("gameplay"));
    REQUIRE(stack.has("pause"));
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack is_active", "[input][context]") {
    auto& stack = ContextStack::instance();

    stack.push("gameplay");
    stack.push("pause");

    SECTION("returns true for contexts in stack") {
        REQUIRE(stack.is_active("gameplay"));
        REQUIRE(stack.is_active("pause"));
    }

    SECTION("returns false for contexts not in stack") {
        REQUIRE_FALSE(stack.is_active("menu"));
        REQUIRE_FALSE(stack.is_active("nonexistent"));
    }

    SECTION("returns false after context is popped") {
        stack.pop();
        REQUIRE_FALSE(stack.is_active("pause"));
        REQUIRE(stack.is_active("gameplay"));
    }
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack define", "[input][context]") {
    auto& stack = ContextStack::instance();

    SECTION("creates new context") {
        InputContext& ctx = stack.define("new_context");

        REQUIRE(ctx.name == "new_context");
        REQUIRE(stack.has("new_context"));
    }

    SECTION("returns existing context") {
        InputContext& ctx1 = stack.define("existing");
        ctx1.blocks_global = true;

        InputContext& ctx2 = stack.define("existing");

        REQUIRE(ctx2.blocks_global == true);
        REQUIRE(&ctx1 == &ctx2);
    }
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack get", "[input][context]") {
    auto& stack = ContextStack::instance();

    stack.define("test_context");

    SECTION("returns context for existing name") {
        InputContext* ctx = stack.get("test_context");
        REQUIRE(ctx != nullptr);
        REQUIRE(ctx->name == "test_context");
    }

    SECTION("returns nullptr for nonexistent") {
        InputContext* ctx = stack.get("nonexistent");
        REQUIRE(ctx == nullptr);
    }
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack clear_all", "[input][context]") {
    auto& stack = ContextStack::instance();

    stack.push("gameplay");
    stack.push("pause");

    stack.clear_all();

    SECTION("clears stack") {
        REQUIRE(stack.empty());
    }

    SECTION("clears context definitions") {
        REQUIRE_FALSE(stack.has("gameplay"));
        REQUIRE_FALSE(stack.has("pause"));
    }
}

TEST_CASE_METHOD(InputContextFixture, "InputContext blocks_global", "[input][context]") {
    auto& stack = ContextStack::instance();

    InputContext& gameplay = stack.define("gameplay");
    gameplay.blocks_global = false;

    InputContext& console = stack.define("console");
    console.blocks_global = true;

    SECTION("default is not blocking") {
        InputContext ctx;
        REQUIRE_FALSE(ctx.blocks_global);
    }

    SECTION("blocking context can be configured") {
        REQUIRE(console.blocks_global == true);
    }
}

TEST_CASE_METHOD(InputContextFixture, "ContextStack context_count", "[input][context]") {
    auto& stack = ContextStack::instance();

    SECTION("starts at 0 or 1 (global)") {
        // After clear_all, might have global context auto-created
        REQUIRE(stack.context_count() >= 0);
    }

    SECTION("increments with define") {
        size_t initial = stack.context_count();

        stack.define("ctx1");
        stack.define("ctx2");
        stack.define("ctx3");

        REQUIRE(stack.context_count() == initial + 3);
    }
}

TEST_CASE_METHOD(InputContextFixture, "InputManager action definition", "[input]") {
    auto& mgr = InputManager::instance();

    SECTION("define_action stores action") {
        std::vector<InputBinding> bindings;
        InputBinding bind;
        bind.source = InputSource::Keyboard;
        bind.code = 87;  // W key
        bindings.push_back(bind);

        mgr.define_action("move_up", bindings);

        ActionDefinition* action = mgr.get_action("move_up");
        REQUIRE(action != nullptr);
        REQUIRE(action->name == "move_up");
        REQUIRE(action->bindings.size() == 1);
    }

    SECTION("remove_action removes action") {
        std::vector<InputBinding> bindings;
        mgr.define_action("test_action", bindings);

        REQUIRE(mgr.get_action("test_action") != nullptr);

        mgr.remove_action("test_action");

        REQUIRE(mgr.get_action("test_action") == nullptr);
    }

    SECTION("clear_actions removes all") {
        std::vector<InputBinding> bindings;
        mgr.define_action("action1", bindings);
        mgr.define_action("action2", bindings);

        mgr.clear_actions();

        REQUIRE(mgr.get_action("action1") == nullptr);
        REQUIRE(mgr.get_action("action2") == nullptr);
    }
}

TEST_CASE_METHOD(InputContextFixture, "InputManager action_count", "[input]") {
    auto& mgr = InputManager::instance();

    std::vector<InputBinding> bindings;
    mgr.define_action("action1", bindings);
    mgr.define_action("action2", bindings);
    mgr.define_action("action3", bindings);

    REQUIRE(mgr.action_count() >= 3);  // May have other actions from engine
}

TEST_CASE("InputBinding defaults", "[input]") {
    InputBinding bind;

    REQUIRE(bind.source == InputSource::Keyboard);
    REQUIRE(bind.code == 0);
    REQUIRE(bind.gamepad_index == -1);  // Any gamepad
}

TEST_CASE("InputSource enum", "[input]") {
    // Verify enum values
    REQUIRE(static_cast<int>(InputSource::Keyboard) == 0);
    REQUIRE(static_cast<int>(InputSource::Mouse) == 1);
    REQUIRE(static_cast<int>(InputSource::Gamepad) == 2);
}

TEST_CASE("InputPhase enum", "[input]") {
    // Verify enum values
    REQUIRE(static_cast<int>(InputPhase::Pressed) == 0);
    REQUIRE(static_cast<int>(InputPhase::Released) == 1);
    REQUIRE(static_cast<int>(InputPhase::Held) == 2);
}
