# System Module

Random numbers, platform detection, error handling, and utilities.

```ruby
include GMR
```

## Random Numbers

### random

Get a random float between 0 and 1.

```ruby
System.random
```

**Returns:** Float from 0.0 (inclusive) to 1.0 (exclusive)

**Example:**
```ruby
# Random chance (30%)
if System.random < 0.3
  spawn_power_up()
end

# Random float in range
def random_range(min, max)
  min + System.random * (max - min)
end

# Random position
x = random_range(0, 800)
```

### random_int

Get a random integer in a range.

```ruby
System.random_int(min, max)
```

**Parameters:**
- `min` - Minimum value (inclusive)
- `max` - Maximum value (inclusive)

**Returns:** Integer from min to max

**Example:**
```ruby
# Random die roll (1-6)
roll = System.random_int(1, 6)

# Random spawn position
x = System.random_int(0, 800)
y = System.random_int(0, 600)

# Random enemy type (0, 1, or 2)
enemy_type = System.random_int(0, 2)
```

## Platform Detection

### platform

Get the current platform.

```ruby
System.platform
```

**Returns:** String - `"windows"`, `"linux"`, `"macos"`, or `"web"`

**Example:**
```ruby
def init
  case System.platform
  when "web"
    # Web-specific setup
    $save_enabled = false
  when "windows"
    # Windows-specific setup
  end
end
```

## Error Handling

### error

Log an error message to the developer console.

```ruby
System.error(message)
```

**Parameters:**
- `message` - Error message string

**Example:**
```ruby
def load_level(name)
  data = load_file("levels/#{name}.dat")
  if data.nil?
    System.error("Failed to load level: #{name}")
    return false
  end
  true
end
```

## Application Control

### quit

Exit the game.

```ruby
System.quit
```

**Example:**
```ruby
def update(dt)
  if Input.key_pressed?(:escape)
    if $in_menu
      System.quit
    else
      $in_menu = true
    end
  end
end
```

## Developer Console

### console_open?

Check if the developer console is open.

```ruby
console_open?
```

**Returns:** `true` if console is visible

**Note:** This is a global function, not under `System.`

**Example:**
```ruby
def update(dt)
  # Don't process game input while console is open
  return if console_open?

  # Normal game input
  process_player_input(dt)
end
```

The developer console is opened with the backtick key (`` ` ``).

## Complete Example

```ruby
include GMR

def init
  Window.set_size(800, 600)
  Window.set_title("System Demo")

  # Platform-specific setup
  puts "Running on: #{System.platform}"

  # Spawn random enemies
  $enemies = []
  10.times do
    $enemies << {
      x: System.random_int(50, 750),
      y: System.random_int(50, 550),
      type: System.random_int(0, 2),
      speed: 50 + System.random * 100
    }
  end

  $score = 0
end

def update(dt)
  return if console_open?

  # Quit on escape
  if Input.key_pressed?(:escape)
    System.quit
  end

  # Move enemies randomly
  $enemies.each do |e|
    if System.random < 0.02  # 2% chance per frame to change direction
      e[:dx] = System.random * 2 - 1
      e[:dy] = System.random * 2 - 1
    end
    e[:x] += (e[:dx] || 0) * e[:speed] * dt
    e[:y] += (e[:dy] || 0) * e[:speed] * dt

    # Bounce off edges
    if e[:x] < 0 || e[:x] > 800
      e[:dx] = -(e[:dx] || 0)
      e[:x] = e[:x].clamp(0, 800)
    end
    if e[:y] < 0 || e[:y] > 600
      e[:dy] = -(e[:dy] || 0)
      e[:y] = e[:y].clamp(0, 600)
    end
  end

  # Random score event
  if System.random < 0.01
    $score += System.random_int(1, 10)
  end
end

def draw
  Graphics.clear([30, 30, 40])

  # Draw enemies with type-based colors
  colors = [
    [255, 100, 100],  # Type 0: Red
    [100, 255, 100],  # Type 1: Green
    [100, 100, 255]   # Type 2: Blue
  ]

  $enemies.each do |e|
    color = colors[e[:type]]
    Graphics.draw_circle(e[:x], e[:y], 15, color)
  end

  # HUD
  Graphics.draw_text("Score: #{$score}", 10, 10, 24, [255, 255, 255])
  Graphics.draw_text("Platform: #{System.platform}", 10, 40, 18, [180, 180, 180])
  Graphics.draw_text("ESC: Quit | `: Console", 10, 570, 18, [150, 150, 150])
end
```

## Tips

1. **Use `random_int` for discrete choices** - Selecting from arrays, tile types, etc.
2. **Use `random` for continuous values** - Speeds, positions, chances
3. **Check platform for compatibility** - Some features may vary by platform
4. **Always check `console_open?`** - Prevent accidental input while debugging

## See Also

- [Input](input.md) - Input handling with console check
- [Window](window.md) - Platform display settings
- [API Overview](README.md) - Error handling conventions
