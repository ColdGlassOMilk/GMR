# Texture Class

Loading and rendering images.

```ruby
include GMR
```

## Loading Textures

### Texture.load

Load an image file as a texture.

```ruby
texture = Graphics::Texture.load(path)
```

**Parameters:**
- `path` - Path to image file (relative to game folder)

**Returns:** Texture object or `nil` on failure

**Supported formats:** PNG, JPG, BMP, GIF

**Example:**
```ruby
def init
  $player_sprite = Graphics::Texture.load("assets/player.png")
  $tileset = Graphics::Texture.load("assets/tiles.png")
end
```

## Properties

### width

Get texture width in pixels.

```ruby
texture.width  # => Integer
```

### height

Get texture height in pixels.

```ruby
texture.height  # => Integer
```

**Example:**
```ruby
$sprite = Graphics::Texture.load("assets/hero.png")
puts "Sprite size: #{$sprite.width}x#{$sprite.height}"
```

## Drawing Methods

### draw

Draw texture at a position.

```ruby
texture.draw(x, y)
texture.draw(x, y, tint)
```

**Parameters:**
- `x`, `y` - Position (top-left corner)
- `tint` - Optional color tint (default: white/no tint)

**Example:**
```ruby
def draw
  # Normal drawing
  $sprite.draw(100, 100)

  # With red tint
  $sprite.draw(200, 100, [255, 100, 100])

  # Semi-transparent
  $sprite.draw(300, 100, [255, 255, 255, 128])
end
```

### draw_ex

Draw texture with rotation and scale.

```ruby
texture.draw_ex(x, y, rotation, scale, tint)
```

**Parameters:**
- `x`, `y` - Position (center of texture)
- `rotation` - Rotation in degrees (clockwise)
- `scale` - Scale factor (1.0 = normal size)
- `tint` - Color tint

**Example:**
```ruby
def draw
  # Rotate 45 degrees, double size
  $sprite.draw_ex(400, 300, 45, 2.0, [255, 255, 255])

  # Spin based on time
  angle = Time.elapsed * 90  # 90 degrees per second
  $sprite.draw_ex(200, 200, angle, 1.0, [255, 255, 255])
end
```

### draw_pro

Draw a portion of a texture with full control.

```ruby
texture.draw_pro(src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h, rotation, tint)
```

**Parameters:**
- `src_x`, `src_y` - Source rectangle position (in texture)
- `src_w`, `src_h` - Source rectangle size
- `dst_x`, `dst_y` - Destination position (on screen)
- `dst_w`, `dst_h` - Destination size (stretches/shrinks)
- `rotation` - Rotation in degrees
- `tint` - Color tint

**Example:**
```ruby
# Draw sprite from a spritesheet
# Source: 32x32 tile at position (64, 0) in spritesheet
# Destination: Draw at (100, 100), scaled to 64x64
$spritesheet.draw_pro(
  64, 0, 32, 32,    # Source rect
  100, 100, 64, 64, # Dest rect (2x scale)
  0,                # No rotation
  [255, 255, 255]   # No tint
)
```

## Spritesheet Animation Example

```ruby
FRAME_WIDTH = 32
FRAME_HEIGHT = 32
ANIMATION_SPEED = 0.1  # Seconds per frame

def init
  $spritesheet = Graphics::Texture.load("assets/player_walk.png")
  $frame = 0
  $frame_timer = 0
  $player_x = 100
  $player_y = 100
end

def update(dt)
  # Update animation
  $frame_timer += dt
  if $frame_timer >= ANIMATION_SPEED
    $frame_timer = 0
    $frame = ($frame + 1) % 4  # 4 frames of animation
  end
end

def draw
  Graphics.clear([40, 40, 40])

  # Draw current frame from spritesheet
  src_x = $frame * FRAME_WIDTH
  $spritesheet.draw_pro(
    src_x, 0, FRAME_WIDTH, FRAME_HEIGHT,
    $player_x, $player_y, FRAME_WIDTH, FRAME_HEIGHT,
    0, [255, 255, 255]
  )
end
```

## Tips

1. **Load in `init`** - Load textures once, not every frame
2. **Check for nil** - `Texture.load` returns `nil` if file not found
3. **Use spritesheets** - Combine multiple images into one file for efficiency
4. **Tint for effects** - Use tint for damage flash, selection highlight, etc.

```ruby
def init
  $sprite = Graphics::Texture.load("assets/player.png")
  unless $sprite
    System.error("Failed to load player sprite!")
  end
end
```

## See Also

- [Graphics](graphics.md) - Drawing primitives
- [Tilemap](tilemap.md) - Tile-based rendering
- [API Overview](README.md) - Color conventions
