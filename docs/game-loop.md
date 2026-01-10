# Game Loop

GMR uses a simple, predictable game loop based on global hooks. This is the recommended approach for small games and prototypes.

## Global Hooks

Define these three functions at the top level of your script:

```ruby
include GMR

def init
  # Called once when the game starts (and on each hot-reload)
  @player_x = 400
  @player_y = 300
end

def update(dt)
  # Called every frame. dt = time since last frame in seconds
  @player_x += 100 * dt if Input.key_down?(:right)
  @player_x -= 100 * dt if Input.key_down?(:left)
end

def draw
  # Called every frame for rendering
  Graphics.clear(rgb(20, 20, 40))
  Graphics.draw_circle(@player_x, @player_y, 20, rgb(100, 200, 255))
end
```

Your entire game can live in `main.rb` with these three hooks.

## Frame Timing

### The `dt` Parameter

`update(dt)` receives the delta time - seconds elapsed since the last frame:

```ruby
def update(dt)
  # At 60 FPS, dt is approximately 0.0166
  # At 30 FPS, dt is approximately 0.0333

  speed = 200  # Pixels per second
  @x += speed * dt  # Frame-rate independent movement
end
```

Always multiply velocities and time-based values by `dt`. This ensures consistent behavior regardless of frame rate.

### Accessing Time Globally

The `Time` module provides additional timing information:

```ruby
def update(dt)
  puts Time.delta    # Same as dt parameter
  puts Time.fps      # Current frames per second
  puts Time.elapsed  # Total seconds since game start
end
```

### Time Scale

Slow down or speed up game time:

```ruby
Time.scale = 0.5   # Half speed (slow motion)
Time.scale = 2.0   # Double speed
Time.scale = 0.0   # Pause (dt becomes 0)
Time.scale = 1.0   # Normal speed
```

Time scale affects the `dt` value passed to `update`. Use this for pause menus, slow-motion effects, or speed-up power-ups.

## Execution Order

Each frame executes in this guaranteed order:

1. **Input Polling** - Engine captures current keyboard/mouse state
2. **`update(dt)`** - Your game logic runs
3. **`draw`** - Your rendering code runs
4. **Present** - Frame buffer displayed to screen

Input state is always fresh when `update` runs. All drawing happens after logic.

## The `init` Hook

`init` is called:
- Once when the game first starts
- Once on each hot-reload during development

Use `init` for setup that should run once:

```ruby
def init
  Window.set_title("My Game")
  Window.set_size(960, 540)

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w]
  end

  @player = Player.new(400, 300)
  @camera = Camera.new
end
```

### Preserving State Across Hot-Reload

Instance variables persist across hot-reloads. Use `||=` to keep existing state:

```ruby
def init
  # Always re-apply configuration
  Window.set_title("My Game")

  # Only create if doesn't exist (preserves state during development)
  @player ||= Player.new(400, 300)
  @score ||= 0
  @level ||= 1
end
```

This pattern lets you tweak code without losing your current game state.

## Pausing the Game

### Simple Pause Flag

For games using global hooks, use a boolean flag:

```ruby
def init
  @paused = false
end

def update(dt)
  if Input.key_pressed?(:escape)
    @paused = !@paused
  end

  return if @paused

  # Game logic only runs when not paused
  @player.update(dt)
end

def draw
  Graphics.clear(:dark_gray)
  @player.draw

  if @paused
    Graphics.draw_rect(0, 0, 960, 540, [0, 0, 0, 180])
    Graphics.draw_text("PAUSED", 400, 250, 48, :white)
  end
end
```

### Using Time Scale

Alternatively, set time scale to zero:

```ruby
def update(dt)
  if Input.key_pressed?(:escape)
    Time.scale = Time.scale == 0 ? 1.0 : 0.0
  end

  # With Time.scale = 0, dt is 0, so nothing moves
  @player.x += @speed * dt
end
```

## Console Integration

The developer console can pause your game. Check if it's open:

```ruby
def update(dt)
  return if Console.open?  # Skip game logic when console is active

  @player.update(dt)
end
```

This prevents input conflicts and lets you inspect game state without things moving.

## When to Use Scenes Instead

Global hooks work well for:
- Prototypes and experiments
- Game jam entries
- Single-screen games
- Learning GMR

Consider [Scene classes](scenes.md) when you need:
- Multiple distinct screens (title, gameplay, pause, game over)
- Clean separation between game states
- Pause menus that preserve game state
- Overlay systems (HUD, minimap)

## Complete Example

```ruby
include GMR

SPEED = 200
GRAVITY = 600
JUMP_FORCE = -300
GROUND_Y = 400

def init
  Window.set_size(960, 540)
  Window.set_title("Platformer")

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w, :up]
  end

  @x, @y = 100, GROUND_Y
  @vx, @vy = 0, 0
  @on_ground = true
end

def update(dt)
  return if Console.open?

  # Horizontal movement
  @vx = 0
  @vx -= SPEED if Input.action_down?(:move_left)
  @vx += SPEED if Input.action_down?(:move_right)

  # Jump
  if Input.action_pressed?(:jump) && @on_ground
    @vy = JUMP_FORCE
    @on_ground = false
  end

  # Gravity
  @vy += GRAVITY * dt unless @on_ground

  # Apply velocity
  @x += @vx * dt
  @y += @vy * dt

  # Ground collision
  if @y >= GROUND_Y
    @y = GROUND_Y
    @vy = 0
    @on_ground = true
  end
end

def draw
  Graphics.clear("#1e1e32")
  Graphics.draw_rect(0, GROUND_Y, 960, 140, "#3c3c50")
  Graphics.draw_circle(@x, @y, 20, :cyan)

  Graphics.draw_text("Arrows/WASD to move, Space to jump", 10, 10, 16, :white)
end
```

## See Also

- [Engine Model](engine-model.md) - Execution guarantees and invariants
- [Scenes](scenes.md) - Scene classes for larger games
- [Input](input.md) - Input handling details
- [Console](console.md) - Developer console usage
