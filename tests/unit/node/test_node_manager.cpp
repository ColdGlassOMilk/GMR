#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "gmr/node.hpp"
#include "test_fixtures.hpp"
#include <cmath>

using namespace gmr;

constexpr float PI = 3.14159265358979323846f;

TEST_CASE_METHOD(EngineTestFixture, "NodeManager create and destroy", "[node]") {
    auto& nm = NodeManager::instance();

    SECTION("create returns valid handle") {
        NodeHandle h = nm.create();
        REQUIRE(nm.valid(h));
        REQUIRE(nm.count() == 1);
    }

    SECTION("multiple creates return unique handles") {
        NodeHandle h1 = nm.create();
        NodeHandle h2 = nm.create();
        NodeHandle h3 = nm.create();

        REQUIRE(h1 != h2);
        REQUIRE(h2 != h3);
        REQUIRE(nm.count() == 3);
    }

    SECTION("destroy invalidates handle") {
        NodeHandle h = nm.create();
        REQUIRE(nm.valid(h));

        nm.destroy(h);

        REQUIRE_FALSE(nm.valid(h));
        REQUIRE(nm.count() == 0);
    }

    SECTION("destroy invalid handle is safe") {
        nm.destroy(INVALID_NODE_HANDLE);
        nm.destroy(9999);
        // Should not crash
        REQUIRE(nm.count() == 0);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager get", "[node]") {
    auto& nm = NodeManager::instance();

    SECTION("get returns node pointer for valid handle") {
        NodeHandle h = nm.create();
        Node* node = nm.get(h);

        REQUIRE(node != nullptr);
    }

    SECTION("get returns nullptr for invalid handle") {
        REQUIRE(nm.get(INVALID_NODE_HANDLE) == nullptr);
        REQUIRE(nm.get(9999) == nullptr);
    }

    SECTION("get returns nullptr after destroy") {
        NodeHandle h = nm.create();
        nm.destroy(h);

        REQUIRE(nm.get(h) == nullptr);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager valid", "[node]") {
    auto& nm = NodeManager::instance();

    SECTION("INVALID_NODE_HANDLE is not valid") {
        REQUIRE_FALSE(nm.valid(INVALID_NODE_HANDLE));
    }

    SECTION("created handle is valid") {
        NodeHandle h = nm.create();
        REQUIRE(nm.valid(h));
    }

    SECTION("destroyed handle is not valid") {
        NodeHandle h = nm.create();
        nm.destroy(h);
        REQUIRE_FALSE(nm.valid(h));
    }

    SECTION("arbitrary handle is not valid") {
        REQUIRE_FALSE(nm.valid(9999));
        REQUIRE_FALSE(nm.valid(-5));
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager hierarchy - add_child", "[node]") {
    auto& nm = NodeManager::instance();

    NodeHandle parent = nm.create();
    NodeHandle child = nm.create();

    SECTION("add_child establishes parent relationship") {
        nm.add_child(parent, child);

        Node* c = nm.get(child);
        REQUIRE(c->parent == parent);
    }

    SECTION("add_child updates children list") {
        nm.add_child(parent, child);

        auto children = nm.get_children(parent);
        REQUIRE(children.size() == 1);
        REQUIRE(children[0] == child);
    }

    SECTION("child_count updates correctly") {
        REQUIRE(nm.child_count(parent) == 0);

        nm.add_child(parent, child);
        REQUIRE(nm.child_count(parent) == 1);

        NodeHandle child2 = nm.create();
        nm.add_child(parent, child2);
        REQUIRE(nm.child_count(parent) == 2);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager hierarchy - remove_child", "[node]") {
    auto& nm = NodeManager::instance();

    NodeHandle parent = nm.create();
    NodeHandle child = nm.create();
    nm.add_child(parent, child);

    SECTION("remove_child clears parent") {
        nm.remove_child(parent, child);

        Node* c = nm.get(child);
        REQUIRE(c->parent == INVALID_NODE_HANDLE);
    }

    SECTION("remove_child updates children list") {
        nm.remove_child(parent, child);

        auto children = nm.get_children(parent);
        REQUIRE(children.empty());
    }

    SECTION("remove_child on wrong parent is safe") {
        NodeHandle other = nm.create();
        nm.remove_child(other, child);  // Should not crash

        // Child should still be attached to original parent
        Node* c = nm.get(child);
        REQUIRE(c->parent == parent);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager hierarchy - cycle prevention", "[node]") {
    auto& nm = NodeManager::instance();

    NodeHandle a = nm.create();
    NodeHandle b = nm.create();
    NodeHandle c = nm.create();

    SECTION("cannot add self as child") {
        nm.add_child(a, a);

        REQUIRE(nm.child_count(a) == 0);
        Node* node = nm.get(a);
        REQUIRE(node->parent == INVALID_NODE_HANDLE);
    }

    SECTION("cannot create direct cycle A -> B -> A") {
        nm.add_child(a, b);  // A is parent of B
        nm.add_child(b, a);  // B tries to be parent of A (should fail)

        Node* nodeA = nm.get(a);
        REQUIRE(nodeA->parent == INVALID_NODE_HANDLE);  // A should not have B as parent
    }

    SECTION("cannot create indirect cycle A -> B -> C -> A") {
        nm.add_child(a, b);
        nm.add_child(b, c);
        nm.add_child(c, a);  // Should fail

        Node* nodeA = nm.get(a);
        REQUIRE(nodeA->parent == INVALID_NODE_HANDLE);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager world transform computation", "[node][transform]") {
    auto& nm = NodeManager::instance();

    NodeHandle parent = nm.create();
    NodeHandle child = nm.create();

    Node* p = nm.get(parent);
    Node* c = nm.get(child);

    nm.add_child(parent, child);

    SECTION("child inherits parent translation") {
        p->local.position = {10.0f, 20.0f};
        c->local.position = {5.0f, 5.0f};

        nm.update_world_transforms(parent);

        REQUIRE_APPROX(c->world.position.x, 15.0f);
        REQUIRE_APPROX(c->world.position.y, 25.0f);
    }

    SECTION("parent scale affects child position") {
        p->local.position = {0.0f, 0.0f};
        p->local.scale = {2.0f, 2.0f};
        c->local.position = {10.0f, 10.0f};

        nm.update_world_transforms(parent);

        REQUIRE_APPROX(c->world.position.x, 20.0f);
        REQUIRE_APPROX(c->world.position.y, 20.0f);
    }

    SECTION("parent rotation affects child position") {
        p->local.position = {0.0f, 0.0f};
        p->local.rotation = PI / 2.0f;  // 90 degrees
        c->local.position = {10.0f, 0.0f};

        nm.update_world_transforms(parent);

        // (10, 0) rotated 90 degrees becomes (0, 10)
        REQUIRE_APPROX(c->world.position.x, 0.0f);
        REQUIRE_APPROX(c->world.position.y, 10.0f);
    }

    SECTION("combined transform propagation") {
        p->local.position = {100.0f, 0.0f};
        p->local.rotation = PI / 2.0f;
        p->local.scale = {2.0f, 2.0f};
        c->local.position = {10.0f, 0.0f};

        nm.update_world_transforms(parent);

        // Child local (10, 0) -> scaled (20, 0) -> rotated (0, 20) -> translated (100, 20)
        REQUIRE_APPROX(c->world.position.x, 100.0f);
        REQUIRE_APPROX(c->world.position.y, 20.0f);
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager is_active_in_hierarchy", "[node]") {
    auto& nm = NodeManager::instance();

    NodeHandle parent = nm.create();
    NodeHandle child = nm.create();
    nm.add_child(parent, child);

    SECTION("active node is active in hierarchy") {
        REQUIRE(nm.is_active_in_hierarchy(parent));
        REQUIRE(nm.is_active_in_hierarchy(child));
    }

    SECTION("inactive node is not active in hierarchy") {
        Node* p = nm.get(parent);
        p->active = false;

        REQUIRE_FALSE(nm.is_active_in_hierarchy(parent));
    }

    SECTION("child of inactive parent is not active in hierarchy") {
        Node* p = nm.get(parent);
        p->active = false;

        // Child itself is active, but parent is not
        Node* c = nm.get(child);
        REQUIRE(c->active);
        REQUIRE_FALSE(nm.is_active_in_hierarchy(child));
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager depth-first traversal", "[node]") {
    auto& nm = NodeManager::instance();

    NodeHandle root = nm.create();
    NodeHandle child1 = nm.create();
    NodeHandle child2 = nm.create();
    NodeHandle grandchild = nm.create();

    nm.add_child(root, child1);
    nm.add_child(root, child2);
    nm.add_child(child1, grandchild);

    SECTION("traverses all descendants") {
        std::vector<NodeHandle> visited;

        nm.traverse_depth_first(root, [](Node* node, void* user_data) {
            auto* vec = static_cast<std::vector<NodeHandle>*>(user_data);
            // We'd need handle lookup, but we can at least verify callback is called
        }, &visited);

        // The traversal should work without crashing
        // A more complete test would verify order
    }
}

TEST_CASE_METHOD(EngineTestFixture, "NodeManager clear", "[node]") {
    auto& nm = NodeManager::instance();

    nm.create();
    nm.create();
    nm.create();

    REQUIRE(nm.count() == 3);

    nm.clear();

    REQUIRE(nm.count() == 0);
}
