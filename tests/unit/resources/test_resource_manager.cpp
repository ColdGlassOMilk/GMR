#include <catch2/catch_test_macros.hpp>
#include "gmr/resources/resource_manager.hpp"
#include "test_fixtures.hpp"
#include <string>
#include <vector>

using namespace gmr;

// Mock resource type for testing
struct MockResource {
    std::string path;
    int data = 0;
    bool loaded = true;

    MockResource() = default;
    MockResource(const std::string& p, int d = 0) : path(p), data(d) {}
};

// Mock resource manager that doesn't require raylib
class MockResourceManager : public ResourceManager<int32_t, MockResource> {
public:
    bool fail_next_load = false;
    int load_count = 0;
    std::vector<std::string> unloaded_paths;

protected:
    std::optional<MockResource> load_resource(const std::string& path) override {
        load_count++;
        if (fail_next_load) {
            fail_next_load = false;
            return std::nullopt;
        }
        return MockResource{path, load_count};
    }

    void unload_resource(MockResource& resource) override {
        unloaded_paths.push_back(resource.path);
        resource.loaded = false;
    }
};

TEST_CASE("ResourceManager load", "[resources]") {
    MockResourceManager rm;

    SECTION("first load creates resource with refcount 1") {
        auto h = rm.load("test.png");

        REQUIRE(rm.valid(h));
        REQUIRE(rm.get_ref_count(h) == 1);
    }

    SECTION("load returns valid resource data") {
        auto h = rm.load("test.png");
        MockResource* res = rm.get(h);

        REQUIRE(res != nullptr);
        REQUIRE(res->path == "test.png");
    }

    SECTION("loading same path returns same handle") {
        auto h1 = rm.load("test.png");
        auto h2 = rm.load("test.png");

        REQUIRE(h1 == h2);
    }

    SECTION("loading same path increments refcount") {
        auto h = rm.load("test.png");
        rm.load("test.png");

        REQUIRE(rm.get_ref_count(h) == 2);
    }

    SECTION("loading same path doesn't re-load resource") {
        rm.load("test.png");
        int count_after_first = rm.load_count;

        rm.load("test.png");

        REQUIRE(rm.load_count == count_after_first);  // No additional load
    }

    SECTION("loading different paths creates different handles") {
        auto h1 = rm.load("test1.png");
        auto h2 = rm.load("test2.png");

        REQUIRE(h1 != h2);
        REQUIRE(rm.count() == 2);
    }

    SECTION("load failure returns INVALID_HANDLE") {
        rm.fail_next_load = true;
        auto h = rm.load("missing.png");

        REQUIRE(h == INVALID_HANDLE);
    }
}

TEST_CASE("ResourceManager get", "[resources]") {
    MockResourceManager rm;

    SECTION("get returns resource for valid handle") {
        auto h = rm.load("test.png");
        MockResource* res = rm.get(h);

        REQUIRE(res != nullptr);
    }

    SECTION("get returns nullptr for invalid handle") {
        REQUIRE(rm.get(INVALID_HANDLE) == nullptr);
        REQUIRE(rm.get(9999) == nullptr);
    }

    SECTION("const get works correctly") {
        auto h = rm.load("test.png");
        const MockResourceManager& const_rm = rm;
        const MockResource* res = const_rm.get(h);

        REQUIRE(res != nullptr);
    }
}

TEST_CASE("ResourceManager valid", "[resources]") {
    MockResourceManager rm;

    SECTION("invalid handle is not valid") {
        REQUIRE_FALSE(rm.valid(INVALID_HANDLE));
    }

    SECTION("loaded handle is valid") {
        auto h = rm.load("test.png");
        REQUIRE(rm.valid(h));
    }

    SECTION("arbitrary handle is not valid") {
        REQUIRE_FALSE(rm.valid(9999));
        REQUIRE_FALSE(rm.valid(-5));
    }
}

TEST_CASE("ResourceManager release", "[resources]") {
    MockResourceManager rm;

    SECTION("release decrements refcount") {
        auto h = rm.load("test.png");
        rm.load("test.png");  // refcount = 2

        rm.release(h);

        REQUIRE(rm.get_ref_count(h) == 1);
    }

    SECTION("release at refcount 1 triggers unload") {
        auto h = rm.load("test.png");

        rm.release(h);

        REQUIRE(rm.unloaded_paths.size() == 1);
        REQUIRE(rm.unloaded_paths[0] == "test.png");
    }

    SECTION("release at refcount 0 is safe") {
        auto h = rm.load("test.png");
        rm.release(h);
        rm.release(h);  // Already at 0

        // Should not crash, unload should only happen once
        REQUIRE(rm.unloaded_paths.size() == 1);
    }

    SECTION("release invalid handle is safe") {
        rm.release(INVALID_HANDLE);
        rm.release(9999);
        // Should not crash
    }

    SECTION("released resource clears path cache") {
        rm.load("test.png");
        auto h1 = rm.load("test.png");
        rm.release(h1);
        rm.release(h1);  // Now unloaded

        // Loading same path should create new resource
        auto h2 = rm.load("test.png");

        REQUIRE(rm.load_count == 2);  // Should have loaded again
    }
}

TEST_CASE("ResourceManager add_ref", "[resources]") {
    MockResourceManager rm;

    SECTION("add_ref increments refcount") {
        auto h = rm.load("test.png");

        rm.add_ref(h);

        REQUIRE(rm.get_ref_count(h) == 2);
    }

    SECTION("add_ref on invalid handle is safe") {
        rm.add_ref(INVALID_HANDLE);
        rm.add_ref(9999);
        // Should not crash
    }
}

TEST_CASE("ResourceManager get_ref_count", "[resources]") {
    MockResourceManager rm;

    SECTION("returns 0 for invalid handle") {
        REQUIRE(rm.get_ref_count(INVALID_HANDLE) == 0);
        REQUIRE(rm.get_ref_count(9999) == 0);
    }

    SECTION("returns correct count for loaded resource") {
        auto h = rm.load("test.png");
        REQUIRE(rm.get_ref_count(h) == 1);

        rm.load("test.png");
        REQUIRE(rm.get_ref_count(h) == 2);

        rm.add_ref(h);
        REQUIRE(rm.get_ref_count(h) == 3);
    }
}

TEST_CASE("ResourceManager count", "[resources]") {
    MockResourceManager rm;

    SECTION("starts at 0") {
        REQUIRE(rm.count() == 0);
    }

    SECTION("increments with each unique load") {
        rm.load("test1.png");
        REQUIRE(rm.count() == 1);

        rm.load("test2.png");
        REQUIRE(rm.count() == 2);

        rm.load("test1.png");  // Same path, no new resource
        REQUIRE(rm.count() == 2);
    }
}

TEST_CASE("ResourceManager clear", "[resources]") {
    MockResourceManager rm;

    rm.load("test1.png");
    rm.load("test2.png");
    rm.load("test3.png");

    SECTION("clears all resources") {
        rm.clear();

        REQUIRE(rm.count() == 0);
    }

    SECTION("unloads all resources") {
        rm.clear();

        REQUIRE(rm.unloaded_paths.size() == 3);
    }

    SECTION("clears path cache") {
        rm.clear();
        rm.unloaded_paths.clear();

        // Loading after clear should create new resources
        auto h = rm.load("test1.png");

        REQUIRE(rm.valid(h));
        REQUIRE(rm.load_count == 4);  // 3 initial + 1 after clear
    }
}
