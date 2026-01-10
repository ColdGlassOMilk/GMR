# Camera

The Camera class provides smooth following, screen shake, zoom control, and coordinate conversion for scrolling games.

## Basic Setup

```ruby
include GMR

def init
  @camera = Camera.new

  # Set the screen center as the camera offset
  # This keeps the target centered on screen
  @camera.offset = Mathf::Vec2.new(480, 270)  # Half of 960x540

  @player = Player.new(100, 100)
end

def draw
  # Everything inside camera.use is transformed
  @camera.use do
    @tilemap.draw(0, 0)
    @player.draw
    @enemies.each(&:draw)
  end

  # UI drawn outside camera.use stays fixed on screen
  Graphics.draw_text("Score: #{@score}", 10, 10, 20, :white)
end
```

## Camera Properties

### Target

The world position the camera looks at:

```ruby
@camera.target = Mathf::Vec2.new(400, 300)

# Or directly set to player position
@camera.target = @player.transform.position
```

### Offset

Screen position where the target appears:

```ruby
# Target appears at screen center
@camera.offset = Mathf::Vec2.new(480, 270)

# Target appears in left third of screen
@camera.offset = Mathf::Vec2.new(320, 270)
```

### Zoom

```ruby
@camera.zoom = 2.0   # 2x zoom (things appear larger)
@camera.zoom = 0.5   # 0.5x zoom (things appear smaller)
@camera.zoom = 1.0   # Default (1:1 pixel mapping)
```

### Rotation

```ruby
@camera.rotation = 45  # Rotate view 45 degrees
```

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
def update(dt)
  # Smooth following with interpolation
  @camera.follow(@player, smoothing: 0.1)
end
```

The `smoothing` parameter controls how quickly the camera catches up:
- `0.1` = Smooth, laggy follow
- `0.5` = Medium responsiveness
- `1.0` = Instant snap (no smoothing)

### Follow with Deadzone

A deadzone allows the player to move without the camera following until they reach the edge:

```ruby
def update(dt)
  @camera.follow(@player,
    smoothing: 0.08,
    deadzone: Rect.new(-30, -20, 60, 40)
  )
end
```

The deadzone is a rectangle centered on the camera's target position. The camera only moves when the player exits this zone.

### Following Any Object

The `follow` method works with any object that has:
- A `transform` property returning a Transform2D, or
- A `position` property returning a Vec2, or
- Both `x` and `y` properties

```ruby
# All of these work
@camera.follow(@sprite)           # Has transform
@camera.follow(@player)           # Has position method
@camera.follow(@some_object)      # Has x and y
```

## Camera Bounds

Constrain the camera to level boundaries:

```ruby
def init
  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(480, 270)

  # Prevent camera from showing outside level bounds
  @camera.bounds = Rect.new(0, 0, @level.width, @level.height)
end
```

When bounds are set, the camera will not scroll past the edges. This prevents showing empty space outside your level.

## Screen Shake

Add impact feedback with screen shake:

```ruby
def on_player_hit
  @camera.shake(strength: 5, duration: 0.2, frequency: 30)
end

def on_explosion
  @camera.shake(strength: 10, duration: 0.5)
end
```

| Parameter | Description |
|-----------|-------------|
| `strength` | Maximum shake offset in pixels |
| `duration` | How long the shake lasts in seconds |
| `frequency` | Shake oscillations per second (optional) |

Shake is additive - multiple shakes can overlap for bigger effects.

## Coordinate Conversion

Convert between screen and world coordinates:

### Screen to World

Where in the game world is the mouse pointing?

```ruby
def update(dt)
  # Get mouse position in world coordinates
  mouse_screen = Mathf::Vec2.new(Input.mouse_x, Input.mouse_y)
  mouse_world = @camera.screen_to_world(mouse_screen)

  # Now mouse_world.x and mouse_world.y are in game coordinates
  @crosshair.transform.x = mouse_world.x
  @crosshair.transform.y = mouse_world.y
end
```

### World to Screen

Where on screen does this world position appear?

```ruby
def draw
  # Draw UI element at player's screen position
  screen_pos = @camera.world_to_screen(@player.transform.position)

  # Draw health bar above player (outside camera.use block)
  Graphics.draw_rect(screen_pos.x - 20, screen_pos.y - 40, 40, 5, :red)
end
```

## Zoom Control

### Mouse Wheel Zoom

```ruby
def update(dt)
  wheel = Input.mouse_wheel
  if wheel != 0
    @camera.zoom = Mathf.clamp(@camera.zoom + wheel * 0.25, 0.5, 4.0)
  end
end
```

### Zoom to Point

Zoom while keeping a specific point stationary:

```ruby
def zoom_to_mouse(zoom_delta)
  # Get mouse world position before zoom
  mouse_screen = Mathf::Vec2.new(Input.mouse_x, Input.mouse_y)
  world_before = @camera.screen_to_world(mouse_screen)

  # Apply zoom
  @camera.zoom = Mathf.clamp(@camera.zoom + zoom_delta, 0.5, 4.0)

  # Get mouse world position after zoom
  world_after = @camera.screen_to_world(mouse_screen)

  # Adjust camera to keep mouse point stationary
  @camera.target.x += world_before.x - world_after.x
  @camera.target.y += world_before.y - world_after.y
end
```

## Complete Example

```ruby
include GMR

def init
  Window.set_size(960, 540)

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.move_up [:w, :up]
    i.move_down [:s, :down]
  end

  # Create player
  @player_transform = Transform2D.new(x: 400, y: 300)
  @player = Sprite.new(Texture.load("player.png"), @player_transform)
  @player.center_origin

  # Setup camera
  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(480, 270)
  @camera.zoom = 2.0
  @camera.bounds = Rect.new(0, 0, 2000, 1500)  # Level size

  # Create some world content
  @tilemap = create_tilemap()
end

def update(dt)
  # Player movement
  speed = 150 * dt
  @player_transform.x -= speed if Input.action_down?(:move_left)
  @player_transform.x += speed if Input.action_down?(:move_right)
  @player_transform.y -= speed if Input.action_down?(:move_up)
  @player_transform.y += speed if Input.action_down?(:move_down)

  # Camera follows player smoothly
  @camera.follow(@player,
    smoothing: 0.1,
    deadzone: Rect.new(-50, -30, 100, 60)
  )

  # Zoom with mouse wheel
  wheel = Input.mouse_wheel
  if wheel != 0
    @camera.zoom = Mathf.clamp(@camera.zoom + wheel * 0.25, 1.0, 4.0)
  end
end

def draw
  @camera.use do
    Graphics.clear("#1e1e32")
    @tilemap.draw(0, 0)
    @player.draw

    # Draw mouse cursor in world space
    mouse_world = @camera.screen_to_world(
      Mathf::Vec2.new(Input.mouse_x, Input.mouse_y)
    )
    Graphics.draw_circle(mouse_world.x, mouse_world.y, 5, :yellow)
  end

  # UI (fixed to screen)
  Graphics.draw_text("WASD to move, scroll to zoom", 10, 10, 16, :white)
  Graphics.draw_text("Zoom: #{@camera.zoom.round(1)}x", 10, 30, 16, :white)
end
```

## Multiple Cameras

For split-screen or minimap, create multiple cameras:

```ruby
def init
  @main_camera = Camera.new
  @main_camera.offset = Mathf::Vec2.new(480, 270)

  @minimap_camera = Camera.new
  @minimap_camera.zoom = 0.1  # Zoomed out for overview
end

def draw
  # Main view
  @main_camera.use do
    # Draw game world
  end

  # Minimap in corner (you'd also set a viewport/scissor for proper minimap)
  @minimap_camera.use do
    # Draw simplified world view
  end
end
```

## See Also

- [Graphics](graphics.md) - Drawing and rendering
- [Transforms](transforms.md) - Transform2D details
- [API Reference](api/engine/graphics/camera2d.md) - Complete Camera API
