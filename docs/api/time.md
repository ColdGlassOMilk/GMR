# Time Module

Delta time, elapsed time, and frame rate.

```ruby
include GMR
```

## Delta Time

### dt (in update)

Delta time is passed as a parameter to `update`:

```ruby
def update(dt)
  # dt = time since last frame in seconds
  # Typically ~0.016 for 60 FPS
end
```

**Always multiply movement by `dt`** for frame-rate independent motion:

```ruby
def update(dt)
  # Correct: moves 200 pixels per second regardless of FPS
  $player[:x] += 200 * dt

  # Wrong: moves 200 pixels per frame (varies with FPS)
  # $player[:x] += 200
end
```

## Elapsed Time

### elapsed

Get total time since game started.

```ruby
Time.elapsed
```

**Returns:** Time in seconds (Float)

**Example:**
```ruby
def draw
  # Pulsing effect using sine wave
  pulse = Math.sin(Time.elapsed * 3) * 0.5 + 0.5  # 0 to 1
  brightness = (pulse * 255).to_i
  Graphics.draw_circle(400, 300, 50, [brightness, brightness, brightness])
end

def update(dt)
  # Spawn enemy every 5 seconds
  if Time.elapsed > $next_spawn_time
    spawn_enemy()
    $next_spawn_time = Time.elapsed + 5
  end
end
```

## Frame Rate

### fps

Get current frames per second.

```ruby
Time.fps
```

**Returns:** Current FPS (Integer)

**Example:**
```ruby
def draw
  Graphics.draw_text("FPS: #{Time.fps}", 10, 10, 20, [255, 255, 255])
end
```

### set_target_fps

Set the target frame rate.

```ruby
Time.set_target_fps(fps)
```

**Parameters:**
- `fps` - Target frames per second

**Example:**
```ruby
def init
  Time.set_target_fps(60)  # Cap at 60 FPS
end
```

## Timing Patterns

### Animation Timer

```ruby
def init
  $frame = 0
  $frame_timer = 0
  $animation_speed = 0.1  # 10 FPS animation
end

def update(dt)
  $frame_timer += dt
  if $frame_timer >= $animation_speed
    $frame_timer = 0
    $frame = ($frame + 1) % 4  # 4 frames
  end
end
```

### Cooldown Timer

```ruby
def init
  $can_shoot = true
  $shoot_cooldown = 0
  SHOOT_DELAY = 0.25  # 4 shots per second
end

def update(dt)
  # Count down cooldown
  if $shoot_cooldown > 0
    $shoot_cooldown -= dt
    if $shoot_cooldown <= 0
      $can_shoot = true
    end
  end

  # Shoot with cooldown
  if Input.action_down?(:fire) && $can_shoot
    shoot()
    $can_shoot = false
    $shoot_cooldown = SHOOT_DELAY
  end
end
```

### Delay/Timer Class Pattern

```ruby
class Timer
  def initialize(duration)
    @duration = duration
    @remaining = 0
  end

  def start
    @remaining = @duration
  end

  def update(dt)
    @remaining -= dt if @remaining > 0
  end

  def done?
    @remaining <= 0
  end

  def progress
    1.0 - (@remaining / @duration)
  end
end

# Usage
def init
  $spawn_timer = Timer.new(2.0)
  $spawn_timer.start
end

def update(dt)
  $spawn_timer.update(dt)
  if $spawn_timer.done?
    spawn_enemy()
    $spawn_timer.start
  end
end
```

### Screen Shake Effect

```ruby
def init
  $shake_time = 0
  $shake_intensity = 0
end

def shake(intensity, duration)
  $shake_intensity = intensity
  $shake_time = duration
end

def update(dt)
  if $shake_time > 0
    $shake_time -= dt
  end
end

def draw
  offset_x, offset_y = 0, 0
  if $shake_time > 0
    offset_x = (rand * 2 - 1) * $shake_intensity
    offset_y = (rand * 2 - 1) * $shake_intensity
  end

  # Apply offset to all drawing
  draw_game(offset_x, offset_y)
end
```

## Complete Example

```ruby
include GMR

def init
  Window.set_size(800, 600)
  Time.set_target_fps(60)

  $player = { x: 400, y: 300 }
  $bullets = []
  $shoot_cooldown = 0
end

def update(dt)
  return if console_open?

  # Movement (frame-rate independent)
  speed = 300
  $player[:x] -= speed * dt if Input.key_down?(:left)
  $player[:x] += speed * dt if Input.key_down?(:right)
  $player[:y] -= speed * dt if Input.key_down?(:up)
  $player[:y] += speed * dt if Input.key_down?(:down)

  # Shooting with cooldown
  $shoot_cooldown -= dt if $shoot_cooldown > 0
  if Input.key_down?(:space) && $shoot_cooldown <= 0
    $bullets << { x: $player[:x], y: $player[:y], spawn_time: Time.elapsed }
    $shoot_cooldown = 0.1
  end

  # Update bullets
  $bullets.each { |b| b[:y] -= 500 * dt }
  $bullets.reject! { |b| b[:y] < 0 }
end

def draw
  Graphics.clear([20, 20, 30])

  # Player
  Graphics.draw_rect($player[:x] - 15, $player[:y] - 15, 30, 30, [100, 200, 255])

  # Bullets with age-based fading
  $bullets.each do |b|
    age = Time.elapsed - b[:spawn_time]
    alpha = [(1.0 - age) * 255, 255].min.to_i
    Graphics.draw_circle(b[:x], b[:y], 5, [255, 255, 100, alpha])
  end

  # HUD
  Graphics.draw_text("FPS: #{Time.fps}", 10, 10, 20, [255, 255, 255])
  Graphics.draw_text("Time: #{Time.elapsed.round(1)}s", 10, 35, 20, [200, 200, 200])
end
```

## See Also

- [Graphics](graphics.md) - Animation rendering
- [Input](input.md) - Input timing
- [API Overview](README.md) - Game loop structure
