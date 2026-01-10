# Graphics

GMR provides drawing primitives, texture loading, sprites, and tilemaps through the `Graphics` module and related classes.

## Color Formats

All drawing methods accept colors in multiple formats:

| Format | Example |
|--------|---------|
| Named symbol | `:red`, `:blue`, `:dark_gray`, `:transparent` |
| Hex string | `"#FF0000"`, `"#F00"`, `"#FF0000AA"` |
| Helper function | `rgb(255, 0, 0)`, `rgb(255, 0, 0, 128)` |
| Array | `[255, 0, 0]`, `[255, 0, 0, 128]` |

Alpha defaults to 255 (fully opaque) if omitted. RGB values are 0-255.

### Named Colors

Common named colors: `:white`, `:black`, `:red`, `:green`, `:blue`, `:yellow`, `:cyan`, `:magenta`, `:orange`, `:purple`, `:pink`, `:brown`, `:gray`, `:dark_gray`, `:light_gray`, `:transparent`

### The `rgb()` Helper

```ruby
rgb(255, 0, 0)        # Red, fully opaque
rgb(255, 0, 0, 128)   # Red, 50% transparent
```

## Clearing the Screen

Always clear at the start of your `draw` function:

```ruby
def draw
  Graphics.clear(:dark_gray)          # Named color
  Graphics.clear("#141428")           # Hex color
  Graphics.clear(rgb(20, 20, 40))     # RGB helper
  # ... draw your game
end
```

## Drawing Primitives

### Rectangles

```ruby
# Filled rectangle
Graphics.draw_rect(x, y, width, height, color)
Graphics.draw_rect(10, 10, 100, 50, :red)

# Outline only
Graphics.draw_rect_outline(x, y, width, height, color)
Graphics.draw_rect_outline(10, 10, 100, 50, :white)
```

### Circles

```ruby
# Filled circle
Graphics.draw_circle(x, y, radius, color)
Graphics.draw_circle(200, 100, 30, rgb(0, 255, 0))

# Outline only
Graphics.draw_circle_outline(x, y, radius, color)
Graphics.draw_circle_outline(200, 100, 30, :white)
```

### Lines

```ruby
# Basic line
Graphics.draw_line(x1, y1, x2, y2, color)
Graphics.draw_line(0, 0, 100, 100, :yellow)

# Thick line
Graphics.draw_line_thick(x1, y1, x2, y2, thickness, color)
Graphics.draw_line_thick(0, 0, 100, 100, 3, :yellow)
```

### Text

```ruby
Graphics.draw_text(text, x, y, font_size, color)
Graphics.draw_text("Score: #{@score}", 10, 10, 20, :white)

# Measure text before drawing (for centering)
width = Graphics.measure_text("Hello", 20)
x = (960 - width) / 2  # Center on 960-wide screen
Graphics.draw_text("Hello", x, 100, 20, :white)
```

## Textures

Textures are images loaded from files. Load once and reuse.

```ruby
# Load texture (cached by path)
@texture = Texture.load("player.png")

# Draw at position
@texture.draw(100, 100)

# Draw with rotation and scale
@texture.draw_ex(100, 100, rotation, scale)
@texture.draw_ex(100, 100, 45, 2.0)  # 45 degrees, 2x size

# Query dimensions
width = @texture.width
height = @texture.height
```

Textures are cached. Multiple `Texture.load` calls with the same path return the same handle.

## Sprites

Sprites combine a texture with spatial properties (via Transform2D) and rendering options.

### Creating Sprites

```ruby
# Load texture
@texture = Texture.load("player.png")

# Create transform for position/rotation/scale
@transform = Transform2D.new(x: 100, y: 100)

# Create sprite (texture and transform are required)
@sprite = Sprite.new(@texture, @transform)
```

### Sprite Properties

```ruby
# Transparency (0.0 = invisible, 1.0 = opaque)
@sprite.alpha = 0.8

# Tint color (multiplied with texture colors)
@sprite.color = :red
@sprite.color = rgb(255, 200, 200)

# Horizontal/vertical flip
@sprite.flip_x = true
@sprite.flip_y = false

# Set origin to center (for rotation around center)
@sprite.center_origin

# Spritesheet region (source rectangle)
@sprite.source_rect = Rect.new(0, 0, 32, 32)
```

### Drawing Sprites

```ruby
def draw
  Graphics.clear(:black)
  @sprite.draw  # Uses transform position, rotation, scale
end
```

### Accessing the Transform

```ruby
# Move the sprite by modifying its transform
@sprite.transform.x += 100 * dt
@sprite.transform.rotation += 90 * dt

# Or access directly
transform = @sprite.transform
puts "Position: #{transform.x}, #{transform.y}"
```

## Tilemaps

Tilemaps render grids of tiles from a tileset texture.

### Creating a Tilemap

```ruby
@tilemap = Tilemap.new(
  Texture.load("tiles.png"),  # Tileset texture
  16, 16,                      # Tile width, height in pixels
  100, 50                      # Map width, height in tiles
)
```

### Setting Tiles

```ruby
# Set single tile (x, y, tile_index)
@tilemap.set(5, 3, 12)

# Fill entire map with one tile
@tilemap.fill(0)

# Fill rectangular region
@tilemap.fill_rect(0, 0, 10, 10, 5)
```

Tile indices count left-to-right, top-to-bottom in the tileset texture.

### Tile Properties

Define properties for collision detection:

```ruby
# Define tile 1 as solid
@tilemap.define_tile(1, solid: true)

# Platform tile (solid from above only)
@tilemap.define_tile(5, solid: true, platform: true)

# Hazard tile with damage
@tilemap.define_tile(10, hazard: true, damage: 1)
```

### Querying Tiles

```ruby
# Check tile properties at position
if @tilemap.solid?(tile_x, tile_y)
  # Handle collision
end

if @tilemap.hazard?(tile_x, tile_y)
  @player.take_damage(@tilemap.damage_at(tile_x, tile_y))
end
```

### Drawing Tilemaps

```ruby
def draw
  Graphics.clear(:black)
  @tilemap.draw(0, 0)  # Draw at offset (0, 0)
end
```

For camera support, pass the camera offset:

```ruby
@camera.use do
  @tilemap.draw(0, 0)
end
```

## Render Order

Drawing happens in the order you call draw methods. Later draws appear on top:

```ruby
def draw
  Graphics.clear(:black)

  # Background layer (drawn first, appears behind)
  @background.draw

  # Tilemap
  @tilemap.draw(0, 0)

  # Game objects
  @enemies.each(&:draw)
  @player.draw

  # Foreground layer
  @foreground.draw

  # UI (drawn last, appears on top)
  Graphics.draw_text("Score: #{@score}", 10, 10, 20, :white)
end
```

## Complete Example

```ruby
include GMR

def init
  # Load textures
  @player_texture = Texture.load("player.png")
  @tileset = Texture.load("tiles.png")

  # Create player sprite
  @player_transform = Transform2D.new(x: 400, y: 300)
  @player = Sprite.new(@player_texture, @player_transform)
  @player.center_origin

  # Create tilemap
  @tilemap = Tilemap.new(@tileset, 16, 16, 60, 34)
  @tilemap.fill(0)  # Floor tile
  @tilemap.fill_rect(10, 10, 5, 5, 1)  # Some walls

  @tilemap.define_tile(1, solid: true)
end

def update(dt)
  speed = 200 * dt
  @player_transform.x -= speed if Input.key_down?(:left)
  @player_transform.x += speed if Input.key_down?(:right)
  @player_transform.y -= speed if Input.key_down?(:up)
  @player_transform.y += speed if Input.key_down?(:down)
end

def draw
  Graphics.clear("#1a1a2e")

  # Draw tilemap
  @tilemap.draw(0, 0)

  # Draw player
  @player.draw

  # Draw UI
  Graphics.draw_text("Arrow keys to move", 10, 10, 16, :white)
end
```

## See Also

- [Transforms](transforms.md) - Transform2D details
- [Camera](camera.md) - Camera and scrolling
- [Animation](animation.md) - Sprite animation
- [API Reference](api/engine/graphics/README.md) - Complete Graphics API
