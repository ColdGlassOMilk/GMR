#ifndef GMR_TILEMAP_MANAGER_HPP
#define GMR_TILEMAP_MANAGER_HPP

#include "gmr/types.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace gmr {

// Tile properties - common flags stored as bitfield for fast access
struct TileProperties {
    uint16_t flags = 0;
    int16_t damage = 0;      // For hazard tiles
    int16_t custom1 = 0;     // User-defined
    int16_t custom2 = 0;     // User-defined

    // Flag bit positions
    static constexpr uint16_t FLAG_SOLID    = 1 << 0;
    static constexpr uint16_t FLAG_HAZARD   = 1 << 1;
    static constexpr uint16_t FLAG_PLATFORM = 1 << 2;
    static constexpr uint16_t FLAG_LADDER   = 1 << 3;
    static constexpr uint16_t FLAG_WATER    = 1 << 4;
    static constexpr uint16_t FLAG_SLIPPERY = 1 << 5;

    bool solid() const    { return flags & FLAG_SOLID; }
    bool hazard() const   { return flags & FLAG_HAZARD; }
    bool platform() const { return flags & FLAG_PLATFORM; }
    bool ladder() const   { return flags & FLAG_LADDER; }
    bool water() const    { return flags & FLAG_WATER; }
    bool slippery() const { return flags & FLAG_SLIPPERY; }

    void set_solid(bool v)    { if (v) flags |= FLAG_SOLID;    else flags &= ~FLAG_SOLID; }
    void set_hazard(bool v)   { if (v) flags |= FLAG_HAZARD;   else flags &= ~FLAG_HAZARD; }
    void set_platform(bool v) { if (v) flags |= FLAG_PLATFORM; else flags &= ~FLAG_PLATFORM; }
    void set_ladder(bool v)   { if (v) flags |= FLAG_LADDER;   else flags &= ~FLAG_LADDER; }
    void set_water(bool v)    { if (v) flags |= FLAG_WATER;    else flags &= ~FLAG_WATER; }
    void set_slippery(bool v) { if (v) flags |= FLAG_SLIPPERY; else flags &= ~FLAG_SLIPPERY; }
};

// Tilemap data - stores tile indices for a grid
struct TilemapData {
    std::vector<int32_t> tiles;  // Tile indices (-1 = empty/transparent)
    std::unordered_map<int32_t, TileProperties> tile_properties;  // Per-tile-type properties
    int32_t width;               // Map width in tiles
    int32_t height;              // Map height in tiles
    int32_t tile_width;          // Width of each tile in pixels
    int32_t tile_height;         // Height of each tile in pixels
    TextureHandle tileset;       // Handle to the tileset texture

    TilemapData(int32_t w, int32_t h, int32_t tw, int32_t th, TextureHandle ts)
        : tiles(w * h, -1)  // Initialize all tiles to empty (-1)
        , width(w)
        , height(h)
        , tile_width(tw)
        , tile_height(th)
        , tileset(ts)
    {}

    // Get tile at position (returns -1 for empty or out of bounds)
    int32_t get(int32_t x, int32_t y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return -1;
        }
        return tiles[y * width + x];
    }

    // Set tile at position
    void set(int32_t x, int32_t y, int32_t tile_index) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            tiles[y * width + x] = tile_index;
        }
    }

    // Fill entire map with a tile index
    void fill(int32_t tile_index) {
        std::fill(tiles.begin(), tiles.end(), tile_index);
    }

    // Fill a rectangular region
    void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t tile_index) {
        for (int32_t ty = y; ty < y + h && ty < height; ++ty) {
            for (int32_t tx = x; tx < x + w && tx < width; ++tx) {
                if (tx >= 0 && ty >= 0) {
                    tiles[ty * width + tx] = tile_index;
                }
            }
        }
    }

    // Define properties for a tile type
    void define_tile(int32_t tile_index, const TileProperties& props) {
        tile_properties[tile_index] = props;
    }

    // Get properties for a tile type (returns nullptr if not defined)
    const TileProperties* get_tile_props(int32_t tile_index) const {
        auto it = tile_properties.find(tile_index);
        return (it != tile_properties.end()) ? &it->second : nullptr;
    }

    // Get properties for tile at map position (returns nullptr if not defined or empty)
    const TileProperties* get_props_at(int32_t x, int32_t y) const {
        int32_t tile_index = get(x, y);
        if (tile_index < 0) return nullptr;
        return get_tile_props(tile_index);
    }

    // Fast property checks at map position
    bool is_solid(int32_t x, int32_t y) const {
        auto* props = get_props_at(x, y);
        return props && props->solid();
    }

    bool is_hazard(int32_t x, int32_t y) const {
        auto* props = get_props_at(x, y);
        return props && props->hazard();
    }

    bool is_platform(int32_t x, int32_t y) const {
        auto* props = get_props_at(x, y);
        return props && props->platform();
    }

    bool is_ladder(int32_t x, int32_t y) const {
        auto* props = get_props_at(x, y);
        return props && props->ladder();
    }

    bool is_water(int32_t x, int32_t y) const {
        auto* props = get_props_at(x, y);
        return props && props->water();
    }

    bool is_slippery(int32_t x, int32_t y) const {
        auto* props = get_props_at(x, y);
        return props && props->slippery();
    }

    int16_t get_damage(int32_t x, int32_t y) const {
        auto* props = get_props_at(x, y);
        return props ? props->damage : 0;
    }
};

// TilemapManager - manages all tilemaps (they are created dynamically, not loaded from files)
class TilemapManager {
public:
    static TilemapManager& instance();

    // Create a new tilemap and return its handle
    TilemapHandle create(int32_t width, int32_t height, int32_t tile_width, int32_t tile_height, TextureHandle tileset);

    // Get tilemap by handle
    TilemapData* get(TilemapHandle handle);
    const TilemapData* get(TilemapHandle handle) const;

    // Check if handle is valid
    bool valid(TilemapHandle handle) const;

    // Destroy a tilemap
    void destroy(TilemapHandle handle);

    // Clear all tilemaps
    void clear();

    TilemapManager(const TilemapManager&) = delete;
    TilemapManager& operator=(const TilemapManager&) = delete;

private:
    TilemapManager() = default;

    std::vector<TilemapData*> tilemaps_;
    std::vector<TilemapHandle> free_handles_;  // Recycled handles
};

} // namespace gmr

#endif
