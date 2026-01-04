# Tilemap

API reference for Tilemap.

## Tilemap

TODO: Add description

### Class Methods

### new(tileset:, tile_width:, tile_height:, width:, height:)

Create a new tilemap with the specified dimensions. All tiles are initialized to -1 (empty/transparent).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| tileset | Texture | The tileset texture containing all tile graphics |
| tile_width | Integer | Width of each tile in pixels |
| tile_height | Integer | Height of each tile in pixels |
| width | Integer | Map width in tiles |
| height | Integer | Map height in tiles |

**Returns:** `Tilemap` - The new tilemap object

**Raises:**
- ArgumentError if dimensions are not positive

**Example:**
```ruby
tileset = GMR::Graphics::Texture.load("assets/tiles.png")
map = GMR::Graphics::Tilemap.new(tileset, 16, 16, 100, 50)
```

---

