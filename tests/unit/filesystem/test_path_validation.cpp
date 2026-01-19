#include <catch2/catch_test_macros.hpp>
#include "gmr/filesystem/paths.hpp"

using namespace gmr::filesystem;

// ============================================================================
// is_valid_path() tests
// ============================================================================

TEST_CASE("is_valid_path rejects empty path", "[filesystem][security]") {
    REQUIRE_FALSE(is_valid_path(""));
}

TEST_CASE("is_valid_path rejects parent directory traversal", "[filesystem][security]") {
    SECTION("simple parent traversal") {
        REQUIRE_FALSE(is_valid_path(".."));
        REQUIRE_FALSE(is_valid_path("../secret.txt"));
    }

    SECTION("embedded parent traversal") {
        REQUIRE_FALSE(is_valid_path("foo/../../../etc/passwd"));
        REQUIRE_FALSE(is_valid_path("saves/../../private.txt"));
    }

    SECTION("multiple traversals") {
        REQUIRE_FALSE(is_valid_path("a/b/../../c/../../../d"));
    }
}

TEST_CASE("is_valid_path rejects absolute paths", "[filesystem][security]") {
    SECTION("Unix absolute paths") {
        REQUIRE_FALSE(is_valid_path("/etc/passwd"));
        REQUIRE_FALSE(is_valid_path("/"));
    }

    SECTION("Windows absolute paths") {
        REQUIRE_FALSE(is_valid_path("\\Windows\\System32"));
        REQUIRE_FALSE(is_valid_path("\\"));
    }

    SECTION("Windows drive letters") {
        REQUIRE_FALSE(is_valid_path("C:\\Windows"));
        REQUIRE_FALSE(is_valid_path("D:secret.txt"));
        REQUIRE_FALSE(is_valid_path("C:"));
    }
}

TEST_CASE("is_valid_path accepts valid relative paths", "[filesystem]") {
    SECTION("simple filenames") {
        REQUIRE(is_valid_path("save.txt"));
        REQUIRE(is_valid_path("config.json"));
    }

    SECTION("paths with subdirectories") {
        REQUIRE(is_valid_path("saves/game1.json"));
        REQUIRE(is_valid_path("deep/nested/folder/file.dat"));
    }

    SECTION("paths with dots in filename") {
        REQUIRE(is_valid_path("save.backup.txt"));
        REQUIRE(is_valid_path("file.tar.gz"));
    }

    SECTION("paths with underscores and hyphens") {
        REQUIRE(is_valid_path("save_game_01.dat"));
        REQUIRE(is_valid_path("my-save-file.json"));
    }
}

TEST_CASE("is_valid_path rejects paths with colons", "[filesystem][security]") {
    REQUIRE_FALSE(is_valid_path("file:name.txt"));
    REQUIRE_FALSE(is_valid_path("C:file.txt"));
    REQUIRE_FALSE(is_valid_path("data:text/plain"));
}

#ifdef _WIN32
TEST_CASE("is_valid_path rejects Windows invalid characters", "[filesystem][security][windows]") {
    SECTION("rejects angle brackets") {
        REQUIRE_FALSE(is_valid_path("file<name>.txt"));
        REQUIRE_FALSE(is_valid_path("<file>.txt"));
    }

    SECTION("rejects pipe character") {
        REQUIRE_FALSE(is_valid_path("file|name.txt"));
    }

    SECTION("rejects question mark") {
        REQUIRE_FALSE(is_valid_path("file?.txt"));
        REQUIRE_FALSE(is_valid_path("?file.txt"));
    }

    SECTION("rejects asterisk") {
        REQUIRE_FALSE(is_valid_path("file*.txt"));
        REQUIRE_FALSE(is_valid_path("*.txt"));
    }

    SECTION("rejects quotes") {
        REQUIRE_FALSE(is_valid_path("file\"name.txt"));
    }
}
#endif

// ============================================================================
// resolve_path() tests
// ============================================================================

#ifndef PLATFORM_WEB
TEST_CASE("resolve_path produces correct platform paths", "[filesystem]") {
    SECTION("assets root prepends game/assets/") {
        std::string result = resolve_path("sprite.png", Root::Assets);
        REQUIRE(result == "game/assets/sprite.png");
    }

    SECTION("data root prepends game/data/") {
        std::string result = resolve_path("save.json", Root::Data);
        REQUIRE(result == "game/data/save.json");
    }

    SECTION("preserves subdirectories") {
        std::string result = resolve_path("saves/slot1/data.json", Root::Data);
        REQUIRE(result == "game/data/saves/slot1/data.json");
    }

    SECTION("handles empty filename in subdirectory") {
        std::string result = resolve_path("saves/", Root::Data);
        REQUIRE(result == "game/data/saves/");
    }
}
#endif

// ============================================================================
// sanitize_path() tests
// ============================================================================

#ifndef PLATFORM_WEB
TEST_CASE("sanitize_path accepts paths within sandbox", "[filesystem][security]") {
    // Valid paths should return non-empty sanitized paths
    std::string resolved = resolve_path("save.txt", Root::Data);
    std::string sanitized = sanitize_path(resolved, Root::Data);
    REQUIRE_FALSE(sanitized.empty());
}

TEST_CASE("sanitize_path rejects paths that escape sandbox", "[filesystem][security]") {
    SECTION("rejects direct traversal escape") {
        // Manually craft a path that tries to escape
        std::string evil_path = "game/data/../../../etc/passwd";
        std::string sanitized = sanitize_path(evil_path, Root::Data);
        REQUIRE(sanitized.empty());
    }

    SECTION("rejects traversal from assets to parent") {
        std::string evil_path = "game/assets/../../../secret.txt";
        std::string sanitized = sanitize_path(evil_path, Root::Assets);
        REQUIRE(sanitized.empty());
    }

    SECTION("rejects cross-root traversal") {
        // Try to access assets from data root
        std::string evil_path = "game/data/../assets/private.png";
        std::string sanitized = sanitize_path(evil_path, Root::Data);
        REQUIRE(sanitized.empty());
    }
}

TEST_CASE("sanitize_path handles edge cases", "[filesystem][security]") {
    SECTION("allows internal traversal that stays in sandbox") {
        // This path stays within the sandbox: game/data/a/../b == game/data/b
        std::string path = "game/data/subdir/../file.txt";
        std::string sanitized = sanitize_path(path, Root::Data);
        // Should be allowed since it resolves within game/data/
        REQUIRE_FALSE(sanitized.empty());
    }
}
#endif
