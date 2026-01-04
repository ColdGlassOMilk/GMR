# Tilemap Class

Efficient tile-based map system for levels, backgrounds, and terrain.

```ruby
include GMR
```

## Creating Tilemaps

### new

Create a new tilemap.

```ruby
tilemap = Graphics::Tilemap.new(texture, tile_w, tile_h, map_w, map_h)
```

**Parameters:**
- `texture` - Tileset texture (spritesheet of tiles)
- `tile_w`, `tile_h` - Size of each tile in pixels
- `map_w`, `map_h` - Map dimensions in tiles

**Example:**
```ruby
def init
  $tileset = Graphics::Texture.load("assets/tilemap.png")
  $map = Graphics::Tilemap.new($tileset, 32, 32, 50, 40)
end
```

## Setting Tiles

### set

Set a single tile.

```ruby
tilemap.set(x, y, tile_index)
```

**Parameters:**
- `x`, `y` - Tile position
- `tile_index` - Index in tileset (0-based, left-to-right, top-to-bottom)

**Example:**
```ruby
# Tileset is 10 tiles wide
# Index 0 = top-left tile
# Index 10 = first tile of second row
$map.set(5, 5, 0)   # Grass tile
$map.set(6, 5, 15)  # Water tile
```

### fill

Fill entire map with one tile.

```ruby
tilemap.fill(tile_index)
```

**Example:**
```ruby
$map.fill(0)  # Fill with grass
```

### fill_rect

Fill a rectangular area.

```ruby
tilemap.fill_rect(x, y, width, height, tile_index)
```

**Parameters:**
- `x`, `y` - Top-left corner (in tiles)
- `width`, `height` - Area size (in tiles)
- `tile_index` - Tile to fill with

**Example:**
```ruby
# Create a lake
$map.fill_rect(10, 10, 8, 6, WATER_TILE)

# Stone walls
$map.fill_rect(20, 5, 10, 1, WALL_TILE)  # Top wall
$map.fill_rect(20, 10, 10, 1, WALL_TILE) # Bottom wall
```

### get

Get the tile at a position.

```ruby
tile_index = tilemap.get(x, y)
```

**Returns:** Tile index or `-1` if out of bounds

**Example:**
```ruby
current_tile = $map.get(player_tile_x, player_tile_y)
if current_tile == LAVA_TILE
  take_damage()
end
```

## Tile Properties

Define custom properties for tile types (solid, water, etc.).

### define_tile

Set properties for a tile type.

```ruby
tilemap.define_tile(tile_index, properties)
```

**Parameters:**
- `tile_index` - Which tile to define
- `properties` - Hash of property names to values

**Example:**
```ruby
# Define tile behaviors
$map.define_tile(WALL_TILE, { solid: true })
$map.define_tile(WATER_TILE, { water: true, slow: true })
$map.define_tile(LAVA_TILE, { solid: false, damage: 10 })
```

### solid?

Check if a tile is solid (blocking).

```ruby
tilemap.solid?(x, y)
```

**Returns:** `true` if tile has `solid: true` property

**Example:**
```ruby
def can_move_to?(tile_x, tile_y)
  !$map.solid?(tile_x, tile_y)
end
```

### get_property

Get any custom property value.

```ruby
value = tilemap.get_property(x, y, property_name)
```

**Parameters:**
- `x`, `y` - Tile position
- `property_name` - Property name as string

**Returns:** Property value or `nil`

**Example:**
```ruby
damage = $map.get_property(px, py, "damage")
if damage
  $player[:health] -= damage
end

# Check for slow terrain
if $map.get_property(px, py, "slow")
  speed *= 0.5
end
```

### water?

Convenience method to check for water tiles.

```ruby
tilemap.water?(x, y)
```

**Returns:** `true` if tile has `water: true` property

## Drawing

### draw

Draw the entire tilemap.

```ruby
tilemap.draw(offset_x, offset_y)
```

**Parameters:**
- `offset_x`, `offset_y` - Drawing offset (for scrolling)

**Example:**
```ruby
def draw
  # Draw map with camera offset
  $map.draw(-$camera_x, -$camera_y)
end
```

### draw_region

Draw only a portion of the tilemap (efficient for large maps).

```ruby
tilemap.draw_region(draw_x, draw_y, start_tile_x, start_tile_y, tiles_wide, tiles_tall)
```

**Parameters:**
- `draw_x`, `draw_y` - Screen position to start drawing
- `start_tile_x`, `start_tile_y` - First tile to draw
- `tiles_wide`, `tiles_tall` - Number of tiles to draw

**Example:**
```ruby
def draw
  # Calculate visible region
  start_x = ($camera_x / TILE_SIZE).to_i
  start_y = ($camera_y / TILE_SIZE).to_i

  # Extra tiles for smooth scrolling
  visible_w = (SCREEN_WIDTH / TILE_SIZE) + 2
  visible_h = (SCREEN_HEIGHT / TILE_SIZE) + 2

  # Pixel offset for smooth scrolling
  offset_x = -($camera_x % TILE_SIZE)
  offset_y = -($camera_y % TILE_SIZE)

  $map.draw_region(offset_x, offset_y, start_x, start_y, visible_w, visible_h)
end
```

## Complete Example

```ruby
include GMR

TILE_SIZE = 32
MAP_W, MAP_H = 50, 40

# Tile indices
GRASS = 0
WALL = 1
WATER = 2

def init
  Window.set_size(800, 600)

  $tileset = Graphics::Texture.load("assets/tiles.png")
  $map = Graphics::Tilemap.new($tileset, TILE_SIZE, TILE_SIZE, MAP_W, MAP_H)

  # Fill with grass
  $map.fill(GRASS)

  # Add walls around edge
  MAP_W.times { |x| $map.set(x, 0, WALL); $map.set(x, MAP_H-1, WALL) }
  MAP_H.times { |y| $map.set(0, y, WALL); $map.set(MAP_W-1, y, WALL) }

  # Add a pond
  $map.fill_rect(10, 10, 5, 4, WATER)

  # Define properties
  $map.define_tile(WALL, { solid: true })
  $map.define_tile(WATER, { water: true })

  $player = { x: 5, y: 5 }
  $camera = { x: 0, y: 0 }
end

def update(dt)
  return if console_open?

  dx, dy = 0, 0
  dx -= 1 if Input.key_down?(:left)
  dx += 1 if Input.key_down?(:right)
  dy -= 1 if Input.key_down?(:up)
  dy += 1 if Input.key_down?(:down)

  new_x = $player[:x] + dx
  new_y = $player[:y] + dy

  # Collision check
  unless $map.solid?(new_x, new_y)
    $player[:x] = new_x
    $player[:y] = new_y
  end

  # Center camera on player
  $camera[:x] = $player[:x] * TILE_SIZE - 400
  $camera[:y] = $player[:y] * TILE_SIZE - 300
end

def draw
  Graphics.clear([0, 0, 0])
  $map.draw(-$camera[:x], -$camera[:y])

  # Draw player
  px = $player[:x] * TILE_SIZE - $camera[:x]
  py = $player[:y] * TILE_SIZE - $camera[:y]
  Graphics.draw_rect(px + 4, py + 4, 24, 24, [255, 200, 50])
end
```

## Tips

1. **Use `draw_region`** for large maps - Only draws visible tiles
2. **Define properties once** in `init` - Don't redefine every frame
3. **Tile indices** count left-to-right, top-to-bottom in tileset
4. **Multiple layers** - Create multiple tilemaps for background/foreground

## See Also

- [Texture](texture.md) - Loading tileset images
- [Collision](collision.md) - Collision helpers
- [Graphics](graphics.md) - Drawing primitives
