# Debug Drawing

The `Debug` module provides drawing utilities for visualizing collision bounds, positions, and game state during development. All debug draws render on the `DEBUG_OVERLAY` layer, appearing on top of all game content.

## Enabling/Disabling

```ruby
# Check if debug drawing is enabled
if Debug.enabled?
  puts "Debug mode is on"
end

# Disable all debug drawing (e.g., for release builds)
Debug.enabled = false

# Re-enable
Debug.enabled = true
```

## Conditional Drawing

Use `when_enabled` to wrap debug drawing code that should only run when debug mode is on:

```ruby
def draw
  @sprite.draw

  Debug.when_enabled do
    draw_hitbox
    draw_velocity_vector
    draw_state_label
  end
end
```

This is more efficient than checking `Debug.enabled?` for each draw call.

## Drawing Functions

### Rectangles

```ruby
# Outline (default: green)
Debug.draw_rect(x, y, width, height)
Debug.draw_rect(x, y, width, height, :red)
Debug.draw_rect(x, y, width, height, color: :blue)

# Filled (default: green with 50% alpha)
Debug.draw_rect_filled(x, y, width, height)
Debug.draw_rect_filled(x, y, width, height, color: [255, 0, 0, 128])
```

### Circles

```ruby
# Outline (default: red)
Debug.draw_circle(x, y, radius)
Debug.draw_circle(x, y, radius, :yellow)

# Filled (default: red with 50% alpha)
Debug.draw_circle_filled(x, y, radius)
Debug.draw_circle_filled(x, y, radius, color: :green)
```

### Lines

```ruby
# Default: yellow
Debug.draw_line(x1, y1, x2, y2)
Debug.draw_line(x1, y1, x2, y2, :cyan)
```

### Arrows

Useful for visualizing directions and velocities:

```ruby
# Default: magenta
Debug.draw_arrow(start_x, start_y, end_x, end_y)
Debug.draw_arrow(@x, @y, @x + @vx, @y + @vy, :orange)
```

### Points and Crosses

```ruby
# Small cross marker (default: white)
Debug.draw_point(x, y)
Debug.draw_point(x, y, :cyan)

# Larger cross with custom size (default: cyan)
Debug.draw_cross(x, y, size)
Debug.draw_cross(@spawn_x, @spawn_y, 0.5, :yellow)
```

### Text

```ruby
# Default: white, small fixed-size font
Debug.draw_text("HP: #{@health}", x, y)
Debug.draw_text("State: #{@state}", x, y - 1, :green)
```

## Color Formats

Debug functions accept colors in multiple formats:

```ruby
# Symbol (named color)
Debug.draw_rect(x, y, w, h, :red)
Debug.draw_rect(x, y, w, h, :dark_green)

# Array [r, g, b] or [r, g, b, a]
Debug.draw_circle(x, y, r, [255, 128, 0])      # Orange
Debug.draw_circle(x, y, r, [255, 0, 0, 128])   # Red with 50% alpha

# Hash with color: key
Debug.draw_line(x1, y1, x2, y2, color: :yellow)
```

## Complete Example

```ruby
class Player
  include Destroyable

  def draw
    @sprite.draw

    Debug.when_enabled do
      # Hitbox
      Debug.draw_rect(@hitbox.x, @hitbox.y, @hitbox.w, @hitbox.h, :green)

      # Position marker
      Debug.draw_cross(@transform.x, @transform.y, 0.2, :cyan)

      # Velocity vector
      Debug.draw_arrow(
        @transform.x, @transform.y,
        @transform.x + @velocity.x * 0.5,
        @transform.y + @velocity.y * 0.5,
        :yellow
      )

      # Attack range
      Debug.draw_circle(@transform.x, @transform.y, @attack_range, :red)

      # State label
      Debug.draw_text(@state.to_s, @transform.x, @transform.y - 1.5)
    end
  end
end

class Enemy
  def draw_debug
    Debug.draw_rect_filled(@x, @y, @w, @h, color: [255, 0, 0, 64])
    Debug.draw_rect(@x, @y, @w, @h, :red)
    Debug.draw_text("HP: #{@health}", @x, @y - 0.5)
  end
end

# In main draw
def draw
  Graphics.clear(:dark_gray)

  @player.draw
  @enemies.each(&:draw)

  Debug.when_enabled do
    @enemies.each(&:draw_debug)

    # Spatial hash visualization
    SpatialHash.query_rect(0, 0, 40, 22.5).each do |entity|
      Debug.draw_point(entity.x, entity.y, :white)
    end
  end
end
```

## Toggle with Key

A common pattern is to toggle debug drawing with a key:

```ruby
def update(dt)
  if Input.key_pressed?(:f3)
    Debug.enabled = !Debug.enabled?
  end
end
```

Or using the input system:

```ruby
input do |i|
  i.toggle_debug :f3
end

Input.on(:toggle_debug) { Debug.enabled = !Debug.enabled? }
```

## Render Layer

All debug draws are queued to the `DEBUG_OVERLAY` layer (layer 250), which renders after:
- Background (0)
- World (50)
- Entities (100)
- Effects (150)
- UI (200)

This ensures debug visualizations are always visible on top of game content.

## See Also

- [Graphics](graphics.md) - Regular drawing functions
- [Spatial Queries](spatial.md) - Visualizing spatial query results
- [Console](console.md) - Runtime debugging with the REPL console
