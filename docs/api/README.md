# GMR API Reference

Complete reference for the GMR Ruby API.

## Quick Links

| Module | Description |
|--------|-------------|
| [Graphics](graphics.md) | Drawing primitives, shapes, text |
| [Texture](texture.md) | Image loading and rendering |
| [Tilemap](tilemap.md) | Tile-based map system |
| [Input](input.md) | Keyboard, mouse, action mapping |
| [Audio](audio.md) | Sound loading and playback |
| [Window](window.md) | Window management |
| [Time](time.md) | Delta time, FPS |
| [System](system.md) | Random numbers, platform, utilities |
| [Collision](collision.md) | Collision detection helpers |

## Conventions

### Module Access

All GMR functionality is under the `GMR` namespace:

```ruby
include GMR  # Optional: allows calling without GMR:: prefix

# With include:
Graphics.clear([0, 0, 0])

# Without include:
GMR::Graphics.clear([0, 0, 0])
```

### Lifecycle Hooks

GMR calls these functions in your script:

```ruby
def init
  # Called once at startup
  # Load resources, initialize state
end

def update(dt)
  # Called every frame
  # dt = delta time in seconds
end

def draw
  # Called every frame after update
  # All rendering goes here
end
```

## Common Types

### Color

Colors are arrays of 3-4 integers (0-255):

```ruby
[255, 0, 0]       # Red (RGB)
[255, 0, 0, 128]  # Red, 50% transparent (RGBA)
[0, 0, 0]         # Black
[255, 255, 255]   # White
```

### KeyCode Symbols

Keyboard keys as symbols:

```ruby
# Letters
:a, :b, :c, ... :z

# Numbers
:zero, :one, :two, ... :nine

# Function keys
:f1, :f2, ... :f12

# Special keys
:space, :enter, :escape, :tab
:backspace, :delete, :insert
:up, :down, :left, :right
:home, :end, :page_up, :page_down
:left_shift, :right_shift
:left_control, :right_control
:left_alt, :right_alt
```

### MouseButton Symbols

```ruby
:left    # Left mouse button
:right   # Right mouse button
:middle  # Middle mouse button (scroll wheel)
```

## Coordinate System

- Origin (0, 0) is top-left corner
- X increases rightward
- Y increases downward
- All coordinates are in pixels unless noted

```
(0,0) ────────► X+
  │
  │
  │
  ▼
  Y+
```

## Error Handling

Most GMR functions fail silently or return `nil` on error. Use `System.error()` to report errors to the console:

```ruby
texture = Graphics::Texture.load("missing.png")
if texture.nil?
  System.error("Failed to load texture!")
end
```

## Performance Tips

1. **Load resources in `init`** - Don't load textures/sounds in `update` or `draw`
2. **Use tilemaps for large levels** - More efficient than individual tiles
3. **Draw in batches** - Similar draw calls are faster together
4. **Check `console_open?`** - Skip game input when console is open

```ruby
def update(dt)
  return if console_open?  # Don't process game input
  # ... game logic
end
```

## See Also

- [Getting Started](../getting-started.md) - First game tutorial
- [CLI Reference](../cli/README.md) - Build and run commands
- [Troubleshooting](../troubleshooting.md) - Common issues
