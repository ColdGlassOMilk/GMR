# Camera

The Camera class provides world-space rendering, resolution-independent coordinates, smooth following, screen shake, and zoom control for 2D games.

## Resolution Modes

GMR supports two resolution modes for different game styles. Choose based on your needs:

| Mode | Best For | Aspect Ratio | Black Bars |
|------|----------|--------------|------------|
| **Resolution-Independent** | HD games, modern art | Adapts to screen | Never |
| **Pixel-Perfect** | Retro games, pixel art | Fixed | Yes (letterbox) |

### Resolution-Independent Mode (Recommended for HD)

Render at native window resolution. The camera adapts width to the screen's aspect ratio while maintaining a fixed view height. **No letterboxing, no black bars.**

```ruby
def init
  Window.set_size(1280, 720)
  # NO virtual resolution - render at native size

  @camera = Camera2D.new
  @camera.view_height = 9  # Always see 9 world units tall
  @camera.viewport_size = Vec2.new(Window.width, Window.height)
  @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
end

def on_resize(width, height)
  # Camera adapts to new aspect ratio
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end
```

**How aspect ratio affects visible world:**

| Screen | Aspect | Visible World (view_height = 9) |
|--------|--------|--------------------------------|
| 1920×1080 | 16:9 | 16 × 9 world units |
| 1280×1024 | 5:4 | 11.25 × 9 world units |
| 2560×1080 | 21:9 | 21.33 × 9 world units |
| 800×600 | 4:3 | 12 × 9 world units |

Players with ultrawide monitors see more of the world horizontally. Height is always consistent.

### Pixel-Perfect Mode (Recommended for Retro)

Render to a fixed-size texture and scale up to the window. **Letterboxing (black bars) when aspect ratios don't match.** Pixels stay crisp and sharp.

```ruby
def init
  Window.set_size(960, 540)
  Window.set_virtual_resolution(320, 180)  # Fixed render target
  Window.set_filter_point  # Crisp pixel scaling

  @camera = Camera2D.new
  @camera.view_height = 9  # 180 / 9 = 20 PPU
  @camera.viewport_size = Vec2.new(320, 180)  # Must match virtual resolution!
  @camera.offset = Vec2.new(160, 90)
end

# No on_resize needed - virtual resolution is fixed
```

**Important:** When using virtual resolution:
- `viewport_size` must match the virtual resolution exactly
- The virtual resolution texture is scaled to fit the window with letterboxing
- Use `set_filter_point` for crisp pixels or `set_filter_bilinear` for smooth scaling

---

## World-Space Coordinates

GMR uses world units instead of pixels for game logic. This separates your game mechanics from rendering resolution.

### Key Concepts

| Concept | Description |
|---------|-------------|
| **World Units** | Abstract units for game logic. A character might be 1 unit tall. |
| **View Height** | How many world units are visible vertically. |
| **Viewport Size** | Render resolution in pixels. |
| **Pixels Per Unit (PPU)** | Derived: `viewport_size.y / view_height`. Converts world to pixels. |

### The PPU Formula

```
PPU = viewport_size.y / view_height
```

| Viewport | View Height | PPU | Result |
|----------|-------------|-----|--------|
| 180px | 9 | 20 | 20px sprite = 1 world unit |
| 720px | 9 | 80 | 80px sprite = 1 world unit |
| 1080px | 9 | 120 | 120px sprite = 1 world unit |

The same game logic works at any resolution - only the visual quality changes.

---

## Basic Setup

```ruby
include GMR

def init
  @camera = Camera2D.new

  # Configure view (how many world units to show)
  @camera.view_height = 9

  # Set viewport to match your render target
  @camera.viewport_size = Vec2.new(Window.width, Window.height)

  # Center the camera offset on screen
  @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)

  @player = Player.new(0, 0)  # Position in world units
end

def draw
  # Everything inside camera.use is in world coordinates
  @camera.use do
    @tilemap.draw(0, 0)
    @player.draw
  end

  # UI drawn outside camera.use is in screen space
  Graphics.draw_text("Score: #{@score}", 10, 10, 20, :white)
end
```

---

## Camera Properties

### Target

The world position the camera looks at:

```ruby
@camera.target = Vec2.new(5.0, 3.0)  # World coordinates

# Follow player position
@camera.target = @player.transform.position
```

### Offset

Screen position (in pixels) where the target appears:

```ruby
# Target appears at screen center
@camera.offset = Vec2.new(Window.width / 2, Window.height / 2)

# Target appears in left third of screen (for side-scrollers)
@camera.offset = Vec2.new(Window.width / 3, Window.height / 2)
```

### Zoom

Multiplies the effective scale. Affects how much of the world is visible:

```ruby
@camera.zoom = 2.0   # Zoom in (see half as much)
@camera.zoom = 0.5   # Zoom out (see twice as much)
@camera.zoom = 1.0   # Default
```

### Rotation

```ruby
@camera.rotation = 45  # Rotate view 45 degrees
```

---

## Following a Target

### Basic Follow

```ruby
def update(dt)
  # Snap camera to player position
  @camera.target = @player.transform.position
end
```

### Smooth Follow

```ruby
def init
  @camera.follow(@player, smoothing: 0.1)
end
```

The `smoothing` parameter controls how quickly the camera catches up:
- `0.0` = Instant snap (no smoothing)
- `0.1` = Responsive follow
- `0.5` = Smooth, slightly laggy
- `0.9` = Very smooth, noticeable lag

### Follow with Deadzone

A deadzone allows the player to move without the camera following until they reach the edge:

```ruby
@camera.follow(@player,
  smoothing: 0.08,
  deadzone: Rect.new(-2, -1, 4, 2)  # World units
)
```

### Compatible Objects

The `follow` method works with any object that has:
- A `transform` property returning a Transform2D, or
- A `position` property returning a Vec2, or
- Both `x` and `y` properties

```ruby
@camera.follow(@sprite)        # Has transform
@camera.follow(@player)        # Has position method
@camera.follow(nil)            # Stop following
```

---

## Camera Bounds

Constrain the camera to level boundaries (in world units):

```ruby
def init
  @camera = Camera2D.new
  @camera.view_height = 9

  # Level is 100x50 world units
  @camera.bounds = Rect.new(0, 0, 100, 50)
end
```

When bounds are set, the camera will not scroll past the edges, preventing empty space from showing outside your level.

---

## Screen Shake

Add impact feedback with screen shake:

```ruby
def on_player_hit
  @camera.shake(strength: 0.2, duration: 0.2, frequency: 30)
end

def on_explosion
  @camera.shake(strength: 0.5, duration: 0.5)
end
```

| Parameter | Description |
|-----------|-------------|
| `strength` | Maximum shake offset in world units |
| `duration` | How long the shake lasts in seconds |
| `frequency` | Shake oscillations per second (default: 30) |

---

## Coordinate Conversion

Convert between screen and world coordinates:

### Screen to World

```ruby
def update(dt)
  # Get mouse position in world coordinates
  mouse_screen = Vec2.new(Input.mouse_x, Input.mouse_y)
  mouse_world = @camera.screen_to_world(mouse_screen)

  # Position crosshair at mouse world position
  @crosshair.transform.x = mouse_world.x
  @crosshair.transform.y = mouse_world.y
end
```

### World to Screen

```ruby
def draw
  # Draw UI element at player's screen position
  screen_pos = @camera.world_to_screen(@player.transform.position)

  # Draw health bar above player (outside camera.use block)
  Graphics.draw_rect(screen_pos.x - 20, screen_pos.y - 40, 40, 5, :red)
end
```

---

## Handling Window Resize

Update the camera when the window is resized (Resolution-Independent mode only):

```ruby
def on_resize(width, height)
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end
```

The `on_resize` callback is automatically called by the engine when the window is resized.

---

## Dynamic Mode Switching

Switch between retro and native rendering at runtime:

```ruby
RETRO_WIDTH = 320
RETRO_HEIGHT = 180

def toggle_resolution
  @retro_mode = !@retro_mode

  if @retro_mode
    # Switch to Pixel-Perfect mode
    Window.set_virtual_resolution(RETRO_WIDTH, RETRO_HEIGHT)
    @camera.viewport_size = Vec2.new(RETRO_WIDTH, RETRO_HEIGHT)
    @camera.offset = Vec2.new(RETRO_WIDTH / 2, RETRO_HEIGHT / 2)
  else
    # Switch to Resolution-Independent mode
    Window.clear_virtual_resolution
    @camera.viewport_size = Vec2.new(Window.width, Window.height)
    @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
  end
end
```

---

## Complete Example

```ruby
include GMR

VIEW_HEIGHT = 9
PLAYER_SPEED = 5.0
LEVEL_SIZE = Vec2.new(100, 50)

def init
  Window.set_size(960, 540)
  Window.set_title("Camera Demo")

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.move_up [:w, :up]
    i.move_down [:s, :down]
  end

  # Create player at world position
  @player_transform = Transform2D.new(x: 10, y: 5)
  @player = Sprite.new(Texture.load("player.png"), @player_transform)
  @player.center_origin

  # Setup camera
  @camera = Camera2D.new
  @camera.view_height = VIEW_HEIGHT
  @camera.viewport_size = Vec2.new(Window.width, Window.height)
  @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
  @camera.bounds = Rect.new(0, 0, LEVEL_SIZE.x, LEVEL_SIZE.y)

  # Smooth follow with deadzone
  @camera.follow(@player, smoothing: 0.1, deadzone: Rect.new(-1, -0.5, 2, 1))
end

def on_resize(width, height)
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end

def update(dt)
  if Input.action_down?(:move_left)
    @player_transform.x -= PLAYER_SPEED * dt
  end
  if Input.action_down?(:move_right)
    @player_transform.x += PLAYER_SPEED * dt
  end
  if Input.action_down?(:move_up)
    @player_transform.y -= PLAYER_SPEED * dt
  end
  if Input.action_down?(:move_down)
    @player_transform.y += PLAYER_SPEED * dt
  end

  # Zoom with mouse wheel
  wheel = Input.mouse_wheel
  if wheel != 0
    @camera.zoom = Mathf.clamp(@camera.zoom + wheel * 0.1, 0.5, 3.0)
  end
end

def draw
  @camera.use do
    Graphics.clear("#1e1e32")
    @tilemap.draw(0, 0)
    @player.draw
  end

  # UI (screen space)
  Graphics.draw_text("WASD to move, scroll to zoom", 10, 10, 16, :white)
  Graphics.draw_text("View: #{@camera.visible_width.round(1)} x #{@camera.visible_height.round(1)}", 10, 30, 16, :white)
end
```

---

## API Summary

### View Configuration

| Method | Description |
|--------|-------------|
| `view_height` | How many world units visible vertically |
| `viewport_size` | Render resolution in pixels |
| `pixels_per_unit` | PPU (derived from viewport_size.y / view_height) |
| `effective_scale` | Final scale factor (PPU × zoom) |
| `visible_width` | World width visible (affected by zoom) |
| `visible_height` | World height visible (affected by zoom) |
| `visible_bounds` | Visible world rectangle |

### Transform

| Method | Description |
|--------|-------------|
| `target` | World position camera looks at |
| `offset` | Screen position where target appears |
| `zoom` | Zoom multiplier |
| `rotation` | Rotation in degrees |

### Following

| Method | Description |
|--------|-------------|
| `follow(target, smoothing:, deadzone:)` | Configure automatic following |
| `bounds` | World bounds for camera clamping |

### Effects

| Method | Description |
|--------|-------------|
| `shake(strength:, duration:, frequency:)` | Trigger screen shake |

### Coordinate Conversion

| Method | Description |
|--------|-------------|
| `screen_to_world(vec2)` | Convert screen to world position |
| `world_to_screen(vec2)` | Convert world to screen position |

### Rendering

| Method | Description |
|--------|-------------|
| `use { }` | Execute block with camera transform |
| `begin` / `end` | Manual camera control |
| `Camera2D.current` | Get/set active camera |

---

## See Also

- [Graphics](graphics.md) - Drawing and rendering
- [Transforms](transforms.md) - Transform2D and parallax
- [Window API](api/engine/utilities/window.md) - Virtual resolution settings
- [Camera2D API](api/engine/graphics/camera2d.md) - Complete API reference
