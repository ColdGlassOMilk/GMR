# Graphics

GMR provides drawing primitives, texture loading, sprites, and tilemaps through the `Graphics` module and related classes.

## Coordinate Systems

GMR uses two coordinate systems:

| Space | Use Case | Description |
|-------|----------|-------------|
| **Screen Space** | UI, HUD | Coordinates in logical pixels, auto-scaled |
| **World Space** | Game objects | Coordinates in world units, camera-transformed |

### Screen Space (UI)

Drawing outside of `camera.use` renders in screen space. UI elements are **automatically scaled** based on a 360p baseline, so your UI looks consistent at any resolution.

```ruby
def draw
  @camera.use do
    # ... world-space rendering
  end

  # Screen-space UI (outside camera.use)
  Graphics.draw_text("Score: #{@score}", 10, 10, 16, :white)
  Graphics.draw_rect(10, 30, 100, 10, :red)
end
```

| Render Height | Scale Factor | Size 16 becomes |
|---------------|--------------|-----------------|
| 180px | 0.5x | 8 pixels |
| 360px | 1.0x | 16 pixels |
| 720px | 2.0x | 32 pixels |
| 1080px | 3.0x | 48 pixels |

### World Space (Game Objects)

Sprites and tilemaps use world coordinates when rendered inside `camera.use`:

```ruby
def draw
  @camera.use do
    Graphics.clear(:black)
    @tilemap.draw(0, 0)  # World position (0, 0)
    @player.draw         # Uses transform's world position
  end
end
```

See [Camera](camera.md) for resolution modes (Resolution-Independent vs Pixel-Perfect).

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
x = (Window.width - width) / 2
Graphics.draw_text("Hello", x, 100, 20, :white)
```

## UI Auto-Scaling

Screen-space text and primitives use **automatic UI scaling** based on a 360p baseline. This means you design your UI at 360p measurements, and it scales proportionally to any resolution:

| Resolution | Scale Factor | Size 16 becomes |
|------------|--------------|-----------------|
| 128x128    | 0.36x        | ~6 pixels       |
| 320x180    | 0.5x         | 8 pixels        |
| 640x360    | 1.0x         | 16 pixels       |
| 1280x720   | 2.0x         | 32 pixels       |
| 1920x1080  | 3.0x         | 48 pixels       |

```ruby
# Works at any resolution - text stays proportionally sized
Graphics.draw_text("Score: 100", 10, 10, 16, :white)

# Position and size are all scaled together
Graphics.draw_rect(10, 30, 100, 10, :red)  # Health bar
```

## Custom Fonts

Load TTF/OTF fonts for custom typography. Fonts are cached and reference-counted like textures.

### Loading Fonts

```ruby
# Load a font at a specific size
@title_font = Graphics::Font.load("fonts/pixel.ttf", size: 32)
@ui_font = Graphics::Font.load("fonts/roboto.ttf", size: 16)

# Query font properties
puts @title_font.base_size    # => 32
puts @title_font.glyph_count  # => 95
```

The `size:` parameter determines the base resolution of the font. Larger sizes look better when scaled up but use more memory.

### Drawing with Custom Fonts

```ruby
# Use the font: keyword argument
Graphics.draw_text("Game Title", 100, 50, 32, :white, font: @title_font)
Graphics.draw_text("Score: #{@score}", 10, 10, 16, :cyan, font: @ui_font)

# Default font still works (no font: argument)
Graphics.draw_text("Default Font", 10, 40, 20, :yellow)
```

### Font Lifecycle

Fonts use reference counting. They're automatically cached by path and size:

```ruby
# These return the same font handle (cached)
font1 = Graphics::Font.load("fonts/pixel.ttf", size: 24)
font2 = Graphics::Font.load("fonts/pixel.ttf", size: 24)

# Different size = different font
font3 = Graphics::Font.load("fonts/pixel.ttf", size: 48)

# Manually release if needed (decrements ref count)
font1.release
```

### Complete Font Example

```ruby
include GMR

def init
  @title_font = Graphics::Font.load("fonts/title.ttf", size: 48)
  @body_font = Graphics::Font.load("fonts/body.ttf", size: 16)
  @score = 0
end

def draw
  Graphics.clear(:black)

  # Large title with custom font
  Graphics.draw_text("MY GAME", 320, 50, 48, :gold, font: @title_font)

  # UI text with different font
  Graphics.draw_text("Score: #{@score}", 10, 10, 16, :white, font: @body_font)
  Graphics.draw_text("Press SPACE to play", 10, 30, 16, :gray, font: @body_font)
end
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

Sprites combine a texture with spatial properties (via Transform2D) and rendering options. In world-space rendering, sprite size is determined by the texture dimensions and the asset pixels-per-unit (24 PPU by default).

### Creating Sprites

```ruby
# Load texture
@texture = Texture.load("player.png")

# Create transform for position in world units
@transform = Transform2D.new(x: 5.0, y: 3.0)

# Create sprite (texture and transform are required)
@sprite = Sprite.new(@texture, @transform)
```

### World-Space Sprite Sizing

Sprites are sized based on their texture dimensions and the asset pixels-per-unit (24 PPU by default):

| Texture Size | World Size |
|--------------|------------|
| 24x24 pixels | 1x1 world units |
| 48x48 pixels | 2x2 world units |
| 12x24 pixels | 0.5x1 world units |

This means a 24-pixel sprite perfectly covers a 24-pixel tilemap tile (both are 1 world unit).

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

# Spritesheet region (source rectangle in pixels)
@sprite.source_rect = Rect.new(0, 0, 32, 32)
```

### Drawing Sprites

```ruby
def draw
  @camera.use do
    Graphics.clear(:black)
    @sprite.draw  # Uses transform position, rotation, scale
  end
end
```

### Accessing the Transform

```ruby
# Move the sprite in world units
@sprite.transform.x += 5.0 * dt   # 5 world units per second
@sprite.transform.rotation += 90 * dt

# Scale the sprite
@sprite.transform.scale_x = 2.0  # Double width in world units

# Access directly
transform = @sprite.transform
puts "Position: #{transform.x}, #{transform.y}"
```

## Tilemaps

Tilemaps render grids of tiles from a tileset texture. Each tile occupies exactly 1 world unit.

### Creating a Tilemap

```ruby
@tilemap = Tilemap.new(
  Texture.load("tiles.png"),  # Tileset texture
  24, 24,                      # Tile width, height in pixels
  100, 50                      # Map width, height in tiles
)
```

### Tilemap World Size

A tilemap's world size is its tile count (since each tile = 1 world unit):

| Map Dimensions | World Size |
|----------------|------------|
| 100x50 tiles   | 100x50 world units |
| 20x15 tiles    | 20x15 world units |

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
  @camera.use do
    Graphics.clear(:black)
    @tilemap.draw(0, 0)  # Draw at world offset (0, 0)
  end
end
```

The tilemap is automatically culled to only render visible tiles based on the camera view.

## Render Order and Z-Ordering

Sprites and tilemaps support z-ordering for layered rendering. Lower z-values render first (behind), higher values render on top.

### Setting Z-Order

```ruby
# Via transform
@background.transform.z = -10
@player.transform.z = 0
@foreground.transform.z = 10

# Or during creation
@transform = Transform2D.new(x: 0, y: 0, z: 5)
```

### Parallax Layers

Use the `parallax` property for background layers that scroll at different speeds:

```ruby
# Background scrolls at 50% of camera movement
@bg_transform = Transform2D.new(parallax: 0.5)

# Foreground scrolls at 150% (faster than camera)
@fg_transform = Transform2D.new(parallax: 1.5)

# Default parallax is 1.0 (scrolls with camera)
```

See [Transforms](transforms.md) for more on parallax.

## Virtual Resolution (Pixel-Perfect Mode)

For retro-style games, render to a fixed-size texture and scale up. This enables **letterboxing** when aspect ratios don't match.

```ruby
def init
  Window.set_size(960, 540)
  Window.set_virtual_resolution(320, 180)  # Render at 320×180
  Window.set_filter_point  # Crisp pixel scaling

  @camera = Camera2D.new
  @camera.view_height = 9
  @camera.viewport_size = Vec2.new(320, 180)  # Must match!
  @camera.offset = Vec2.new(160, 90)
end
```

**Important:** When virtual resolution is enabled:
- `viewport_size` must match the virtual resolution
- The render texture is scaled to fit the window with letterboxing (black bars)
- Use `set_filter_point` for crisp pixels, `set_filter_bilinear` for smooth

### Switching Modes at Runtime

```ruby
def toggle_retro_mode
  @retro = !@retro

  if @retro
    # Pixel-Perfect: fixed resolution, letterboxing
    Window.set_virtual_resolution(320, 180)
    @camera.viewport_size = Vec2.new(320, 180)
    @camera.offset = Vec2.new(160, 90)
  else
    # Resolution-Independent: native size, no letterboxing
    Window.clear_virtual_resolution
    @camera.viewport_size = Vec2.new(Window.width, Window.height)
    @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
  end
end
```

See [Camera](camera.md) for full documentation on resolution modes.

## Complete Example

```ruby
include GMR

VIEW_HEIGHT = 9

def init
  Window.set_size(1280, 720)

  # Load textures
  @player_texture = Texture.load("player.png")
  @tileset = Texture.load("tiles.png")

  # Create player (position in world units)
  @player_transform = Transform2D.new(x: 5, y: 5)
  @player = Sprite.new(@player_texture, @player_transform)
  @player.center_origin

  # Create tilemap (60×34 tiles = 60×34 world units)
  @tilemap = Tilemap.new(@tileset, 24, 24, 60, 34)
  @tilemap.fill(0)
  @tilemap.fill_rect(10, 10, 5, 5, 1)
  @tilemap.define_tile(1, solid: true)

  # Setup camera (Resolution-Independent mode)
  @camera = Camera2D.new
  @camera.view_height = VIEW_HEIGHT
  @camera.viewport_size = Vec2.new(Window.width, Window.height)
  @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
end

def on_resize(width, height)
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end

def update(dt)
  speed = 5.0 * dt
  @player_transform.x -= speed if Input.key_down?(:left)
  @player_transform.x += speed if Input.key_down?(:right)
  @player_transform.y -= speed if Input.key_down?(:up)
  @player_transform.y += speed if Input.key_down?(:down)
end

def draw
  @camera.use do
    Graphics.clear("#1a1a2e")
    @tilemap.draw(0, 0)
    @player.draw
  end

  # UI (screen space, auto-scaled)
  Graphics.draw_text("Arrow keys to move", 10, 10, 16, :white)
end
```

## See Also

- [Camera](camera.md) - World-space rendering and resolution independence
- [Transforms](transforms.md) - Transform2D, parallax, and z-ordering
- [Animation](animation.md) - Sprite animation
- [API Reference](api/engine/graphics/README.md) - Complete Graphics API
