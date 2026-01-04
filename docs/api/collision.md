# Collision Module

Collision detection helpers for common game scenarios.

```ruby
include GMR
```

All collision functions are under `GMR::Collision`.

## Distance

### distance

Calculate distance between two points.

```ruby
Collision.distance(x1, y1, x2, y2)
```

**Parameters:**
- `x1`, `y1` - First point
- `x2`, `y2` - Second point

**Returns:** Distance as Float

**Example:**
```ruby
dist = Collision.distance($player[:x], $player[:y], $enemy[:x], $enemy[:y])
if dist < 100
  # Enemy is close
end
```

## Point Tests

### point_in_rect?

Check if a point is inside a rectangle.

```ruby
Collision.point_in_rect?(px, py, rx, ry, rw, rh)
```

**Parameters:**
- `px`, `py` - Point position
- `rx`, `ry` - Rectangle top-left corner
- `rw`, `rh` - Rectangle dimensions

**Returns:** `true` if point is inside rectangle

**Example:**
```ruby
# Check if mouse is over a button
mx, my = Input.mouse_x, Input.mouse_y
if Collision.point_in_rect?(mx, my, button_x, button_y, button_w, button_h)
  # Mouse is hovering over button
  if Input.mouse_pressed?(:left)
    button_clicked()
  end
end
```

## Rectangle Collision

### rect_overlap?

Check if two rectangles overlap.

```ruby
Collision.rect_overlap?(x1, y1, w1, h1, x2, y2, w2, h2)
```

**Parameters:**
- `x1`, `y1`, `w1`, `h1` - First rectangle
- `x2`, `y2`, `w2`, `h2` - Second rectangle

**Returns:** `true` if rectangles overlap

**Example:**
```ruby
def check_player_enemy_collision
  $enemies.each do |enemy|
    if Collision.rect_overlap?(
      $player[:x], $player[:y], $player[:w], $player[:h],
      enemy[:x], enemy[:y], enemy[:w], enemy[:h]
    )
      player_hit(enemy)
    end
  end
end
```

## Circle Collision

### circle_overlap?

Check if two circles overlap.

```ruby
Collision.circle_overlap?(x1, y1, r1, x2, y2, r2)
```

**Parameters:**
- `x1`, `y1`, `r1` - First circle (center and radius)
- `x2`, `y2`, `r2` - Second circle (center and radius)

**Returns:** `true` if circles overlap

**Example:**
```ruby
# Bullet hitting enemy
$bullets.each do |bullet|
  $enemies.each do |enemy|
    if Collision.circle_overlap?(
      bullet[:x], bullet[:y], 5,
      enemy[:x], enemy[:y], enemy[:radius]
    )
      enemy[:health] -= bullet[:damage]
      bullet[:dead] = true
    end
  end
end
```

### circle_rect_overlap?

Check if a circle overlaps a rectangle.

```ruby
Collision.circle_rect_overlap?(cx, cy, cr, rx, ry, rw, rh)
```

**Parameters:**
- `cx`, `cy`, `cr` - Circle center and radius
- `rx`, `ry`, `rw`, `rh` - Rectangle

**Returns:** `true` if circle overlaps rectangle

**Example:**
```ruby
# Ball hitting paddle
if Collision.circle_rect_overlap?(
  $ball[:x], $ball[:y], $ball[:radius],
  $paddle[:x], $paddle[:y], $paddle[:w], $paddle[:h]
)
  $ball[:vy] = -$ball[:vy].abs  # Bounce up
end
```

## Tilemap Helpers

### rect_tiles

Get tile coordinates that a rectangle overlaps.

```ruby
tiles = Collision.rect_tiles(x, y, w, h, tile_size)
```

**Parameters:**
- `x`, `y`, `w`, `h` - Rectangle in world coordinates
- `tile_size` - Size of tiles in pixels

**Returns:** Array of `[tile_x, tile_y]` pairs

**Example:**
```ruby
# Check all tiles the player touches
TILE_SIZE = 32
tiles = Collision.rect_tiles(
  $player[:x], $player[:y],
  $player[:w], $player[:h],
  TILE_SIZE
)

tiles.each do |tile_x, tile_y|
  if $map.solid?(tile_x, tile_y)
    # Collision with solid tile
  end

  if $map.get_property(tile_x, tile_y, "damage")
    # Standing on damaging tile
    $player[:health] -= 1
  end
end
```

## Complete Example

```ruby
include GMR

TILE_SIZE = 32

def init
  Window.set_size(800, 600)

  # Player (rectangle)
  $player = { x: 400, y: 300, w: 32, h: 32, speed: 200 }

  # Enemies (circles)
  $enemies = 5.times.map do
    {
      x: System.random_int(100, 700),
      y: System.random_int(100, 500),
      radius: 20,
      dx: (System.random - 0.5) * 100,
      dy: (System.random - 0.5) * 100
    }
  end

  # Collectibles
  $coins = 10.times.map do
    {
      x: System.random_int(50, 750),
      y: System.random_int(50, 550),
      radius: 10,
      collected: false
    }
  end

  $score = 0
end

def update(dt)
  return if console_open?

  # Player movement
  dx, dy = 0, 0
  dx -= 1 if Input.key_down?(:left)
  dx += 1 if Input.key_down?(:right)
  dy -= 1 if Input.key_down?(:up)
  dy += 1 if Input.key_down?(:down)

  $player[:x] += dx * $player[:speed] * dt
  $player[:y] += dy * $player[:speed] * dt

  # Keep player in bounds
  $player[:x] = $player[:x].clamp(0, 800 - $player[:w])
  $player[:y] = $player[:y].clamp(0, 600 - $player[:h])

  # Player center (for circle collisions)
  px = $player[:x] + $player[:w] / 2
  py = $player[:y] + $player[:h] / 2
  player_radius = $player[:w] / 2

  # Enemy movement and collision
  $enemies.each do |e|
    e[:x] += e[:dx] * dt
    e[:y] += e[:dy] * dt

    # Bounce off walls
    if e[:x] < e[:radius] || e[:x] > 800 - e[:radius]
      e[:dx] = -e[:dx]
    end
    if e[:y] < e[:radius] || e[:y] > 600 - e[:radius]
      e[:dy] = -e[:dy]
    end

    # Check collision with player (circle vs circle)
    if Collision.circle_overlap?(px, py, player_radius, e[:x], e[:y], e[:radius])
      # Push player away
      dist = Collision.distance(px, py, e[:x], e[:y])
      if dist > 0
        push_x = (px - e[:x]) / dist * 5
        push_y = (py - e[:y]) / dist * 5
        $player[:x] += push_x
        $player[:y] += push_y
      end
    end
  end

  # Coin collection
  $coins.each do |coin|
    next if coin[:collected]

    if Collision.circle_overlap?(px, py, player_radius, coin[:x], coin[:y], coin[:radius])
      coin[:collected] = true
      $score += 10
    end
  end

  # Mouse hover detection
  $enemies.each do |e|
    e[:hovered] = Collision.point_in_rect?(
      Input.mouse_x, Input.mouse_y,
      e[:x] - e[:radius], e[:y] - e[:radius],
      e[:radius] * 2, e[:radius] * 2
    )
  end
end

def draw
  Graphics.clear([30, 35, 40])

  # Draw coins
  $coins.each do |coin|
    next if coin[:collected]
    Graphics.draw_circle(coin[:x], coin[:y], coin[:radius], [255, 220, 50])
  end

  # Draw enemies
  $enemies.each do |e|
    color = e[:hovered] ? [255, 150, 150] : [255, 80, 80]
    Graphics.draw_circle(e[:x], e[:y], e[:radius], color)
  end

  # Draw player
  Graphics.draw_rect($player[:x], $player[:y], $player[:w], $player[:h], [100, 200, 255])

  # HUD
  Graphics.draw_text("Score: #{$score}", 10, 10, 24, [255, 255, 255])
  Graphics.draw_text("Collect coins, avoid enemies!", 10, 570, 18, [200, 200, 200])
end
```

## Tips

1. **Use circle collision for characters** - More forgiving than rectangles
2. **Use rect collision for tiles** - Efficient for grid-based games
3. **`rect_tiles` for tilemap collision** - Check only relevant tiles
4. **Distance for range checks** - Aggro ranges, pickup radius, etc.

## See Also

- [Tilemap](tilemap.md) - Tile-based collision with `solid?`
- [Input](input.md) - Mouse position for point tests
- [Graphics](graphics.md) - Visualizing collision areas (debug)
