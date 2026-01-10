# Platformer Tutorial

This tutorial walks through building a complete platformer using GMR's systems: input, state machine, animation, camera, and physics.

## What We'll Build

A player character with:
- WASD/arrow movement
- Jump with gravity
- State-based animation (idle, run, jump, fall)
- Camera following
- Ground collision

## Part 1: Basic Movement

Start with global hooks and simple movement:

```ruby
include GMR

SPEED = 180
GRAVITY = 600
JUMP_FORCE = -280
GROUND_Y = 400

def init
  Window.set_size(960, 540)

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
end
```

This gives us basic movement and jumping with a circle placeholder.

## Part 2: Add Sprite and Transform

Replace the circle with a sprite:

```ruby
include GMR

SPEED = 180
GRAVITY = 600
JUMP_FORCE = -280
GROUND_Y = 400

def init
  Window.set_size(960, 540)

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w, :up]
  end

  # Replace position variables with Transform2D
  @transform = Transform2D.new(x: 100, y: GROUND_Y)
  @vx, @vy = 0, 0
  @on_ground = true
  @facing = 1

  # Create sprite
  @texture = Texture.load("player.png")
  @sprite = Sprite.new(@texture, @transform)
  @sprite.center_origin
end

def update(dt)
  @vx = 0
  @vx -= SPEED if Input.action_down?(:move_left)
  @vx += SPEED if Input.action_down?(:move_right)

  # Track facing direction
  @facing = @vx.negative? ? -1 : 1 if @vx != 0

  if Input.action_pressed?(:jump) && @on_ground
    @vy = JUMP_FORCE
    @on_ground = false
  end

  @vy += GRAVITY * dt unless @on_ground

  @transform.x += @vx * dt
  @transform.y += @vy * dt

  if @transform.y >= GROUND_Y
    @transform.y = GROUND_Y
    @vy = 0
    @on_ground = true
  end

  # Flip sprite based on facing
  @sprite.flip_x = @facing < 0
end

def draw
  Graphics.clear("#1e1e32")
  Graphics.draw_rect(0, GROUND_Y, 960, 140, "#3c3c50")
  @sprite.draw
end
```

## Part 3: Add Animation

Add frame-based animation with state machine:

```ruby
include GMR

SPEED = 180
GRAVITY = 600
JUMP_FORCE = -280
GROUND_Y = 400

def init
  Window.set_size(960, 540)

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w, :up]
  end

  @transform = Transform2D.new(x: 100, y: GROUND_Y)
  @vx, @vy = 0, 0
  @on_ground = true
  @facing = 1

  @texture = Texture.load("player.png")
  @sprite = Sprite.new(@texture, @transform)
  @sprite.center_origin

  # Animation lookup for state machine
  @animations = {
    idle: SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8),
    run: SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
    jump: SpriteAnimation.new(@sprite, frames: 16..18, fps: 8, loop: false, columns: 8),
    fall: SpriteAnimation.new(@sprite, frames: 19..21, fps: 8, columns: 8)
  }

  # State machine
  state_machine do
    state :idle do
      animate :idle
      on :move, :run
      on :jump, :jump, if: -> { @on_ground }
      on :fall, :fall
    end

    state :run do
      animate :run
      on :stop, :idle
      on :jump, :jump, if: -> { @on_ground }
      on :fall, :fall
    end

    state :jump do
      animate :jump
      enter { @vy = JUMP_FORCE }
      on :peak, :fall
    end

    state :fall do
      animate :fall
      on :land, :idle
      on :land_moving, :run
    end
  end

  Console.enable
end

def update(dt)
  return if Console.open?

  # Input
  @vx = 0
  @vx -= SPEED if Input.action_down?(:move_left)
  @vx += SPEED if Input.action_down?(:move_right)
  @facing = @vx.negative? ? -1 : 1 if @vx != 0

  state_machine.trigger(:jump) if Input.action_pressed?(:jump)

  # Physics
  @vy += GRAVITY * dt unless @on_ground
  @transform.x += @vx * dt
  @transform.y += @vy * dt

  # Ground collision
  was_airborne = !@on_ground
  if @transform.y >= GROUND_Y
    @transform.y = GROUND_Y
    @vy = 0
    @on_ground = true
  else
    @on_ground = false
  end

  # State transitions
  state_machine.trigger(@vx != 0 ? :move : :stop)

  if !@on_ground
    if @vy > 0
      state_machine.trigger(:peak) if state_machine.state == :jump
      state_machine.trigger(:fall)
    end
  elsif was_airborne
    state_machine.trigger(@vx != 0 ? :land_moving : :land)
  end

  @sprite.flip_x = @facing < 0
end

def draw
  Graphics.clear("#1e1e32")
  Graphics.draw_rect(0, GROUND_Y, 960, 140, "#3c3c50")
  @sprite.draw

  Graphics.draw_text("State: #{state_machine.state}", 10, 10, 16, :white)
end
```

## Part 4: Add Camera

Add a camera that follows the player:

```ruby
include GMR

SPEED = 180
GRAVITY = 600
JUMP_FORCE = -280
GROUND_Y = 400

def init
  Window.set_size(960, 540)

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w, :up]
  end

  @transform = Transform2D.new(x: 100, y: GROUND_Y)
  @vx, @vy = 0, 0
  @on_ground = true
  @facing = 1

  @texture = Texture.load("player.png")
  @sprite = Sprite.new(@texture, @transform)
  @sprite.center_origin

  @animations = {
    idle: SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8),
    run: SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
    jump: SpriteAnimation.new(@sprite, frames: 16..18, fps: 8, loop: false, columns: 8),
    fall: SpriteAnimation.new(@sprite, frames: 19..21, fps: 8, columns: 8)
  }

  state_machine do
    state :idle do
      animate :idle
      on :move, :run
      on :jump, :jump, if: -> { @on_ground }
      on :fall, :fall
    end

    state :run do
      animate :run
      on :stop, :idle
      on :jump, :jump, if: -> { @on_ground }
      on :fall, :fall
    end

    state :jump do
      animate :jump
      enter { @vy = JUMP_FORCE }
      on :peak, :fall
    end

    state :fall do
      animate :fall
      on :land, :idle
      on :land_moving, :run
    end
  end

  # Camera setup
  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(480, 270)  # Screen center
  @camera.zoom = 1.0

  Console.enable
end

def update(dt)
  return if Console.open?

  @vx = 0
  @vx -= SPEED if Input.action_down?(:move_left)
  @vx += SPEED if Input.action_down?(:move_right)
  @facing = @vx.negative? ? -1 : 1 if @vx != 0

  state_machine.trigger(:jump) if Input.action_pressed?(:jump)

  @vy += GRAVITY * dt unless @on_ground
  @transform.x += @vx * dt
  @transform.y += @vy * dt

  was_airborne = !@on_ground
  if @transform.y >= GROUND_Y
    @transform.y = GROUND_Y
    @vy = 0
    @on_ground = true
  else
    @on_ground = false
  end

  state_machine.trigger(@vx != 0 ? :move : :stop)

  if !@on_ground
    if @vy > 0
      state_machine.trigger(:peak) if state_machine.state == :jump
      state_machine.trigger(:fall)
    end
  elsif was_airborne
    state_machine.trigger(@vx != 0 ? :land_moving : :land)
  end

  @sprite.flip_x = @facing < 0

  # Camera follows player
  @camera.target = Mathf::Vec2.new(@transform.x, @transform.y - 50)
end

def draw
  @camera.use do
    Graphics.clear("#1e1e32")
    Graphics.draw_rect(0, GROUND_Y, 2000, 200, "#3c3c50")
    @sprite.draw
  end

  # UI outside camera (fixed to screen)
  Graphics.draw_text("State: #{state_machine.state}", 10, 10, 16, :white)
end
```

## Part 5: Extract Player Class

For better organization, extract the player into a class:

```ruby
include GMR

class Player
  attr_reader :transform

  SPEED = 180
  GRAVITY = 600
  JUMP_FORCE = -280

  def initialize(x, y)
    @transform = Transform2D.new(x: x, y: y)
    @vx, @vy = 0, 0
    @on_ground = true
    @facing = 1

    @sprite = Sprite.new(Texture.load("player.png"), @transform)
    @sprite.center_origin

    @animations = {
      idle: SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8),
      run: SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
      jump: SpriteAnimation.new(@sprite, frames: 16..18, fps: 8, loop: false, columns: 8),
      fall: SpriteAnimation.new(@sprite, frames: 19..21, fps: 8, columns: 8)
    }

    setup_state_machine
  end

  def setup_state_machine
    state_machine do
      state :idle do
        animate :idle
        on :move, :run
        on :jump, :jump, if: -> { @on_ground }
        on :fall, :fall
      end

      state :run do
        animate :run
        on :stop, :idle
        on :jump, :jump, if: -> { @on_ground }
        on :fall, :fall
      end

      state :jump do
        animate :jump
        enter { @vy = JUMP_FORCE }
        on :peak, :fall
      end

      state :fall do
        animate :fall
        on :land, :idle
        on :land_moving, :run
      end
    end
  end

  def update(dt, ground_y)
    # Input
    @vx = 0
    @vx -= SPEED if Input.action_down?(:move_left)
    @vx += SPEED if Input.action_down?(:move_right)
    @facing = @vx.negative? ? -1 : 1 if @vx != 0

    state_machine.trigger(:jump) if Input.action_pressed?(:jump)

    # Physics
    @vy += GRAVITY * dt unless @on_ground
    @transform.x += @vx * dt
    @transform.y += @vy * dt

    # Ground collision
    was_airborne = !@on_ground
    if @transform.y >= ground_y
      @transform.y = ground_y
      @vy = 0
      @on_ground = true
    else
      @on_ground = false
    end

    # State transitions
    state_machine.trigger(@vx != 0 ? :move : :stop)

    if !@on_ground
      if @vy > 0
        state_machine.trigger(:peak) if state_machine.state == :jump
        state_machine.trigger(:fall)
      end
    elsif was_airborne
      state_machine.trigger(@vx != 0 ? :land_moving : :land)
    end

    @sprite.flip_x = @facing < 0
  end

  def draw
    @sprite.draw
  end

  def state
    state_machine.state
  end

  def position
    @transform.position
  end
end

# === GLOBAL HOOKS ===

GROUND_Y = 400

def init
  Window.set_size(960, 540)

  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w, :up]
  end

  @player = Player.new(100, GROUND_Y)

  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(480, 270)

  Console.enable
end

def update(dt)
  return if Console.open?

  @player.update(dt, GROUND_Y)
  @camera.follow(@player, smoothing: 0.1)
end

def draw
  @camera.use do
    Graphics.clear("#1e1e32")
    Graphics.draw_rect(0, GROUND_Y, 2000, 200, "#3c3c50")
    @player.draw
  end

  Graphics.draw_text("State: #{@player.state}", 10, 10, 16, :white)
  Graphics.draw_text("WASD to move, Space to jump", 10, 30, 14, :gray)
end
```

## Summary

This tutorial demonstrated:

1. **Game Loop** - Using global hooks (`init`, `update`, `draw`)
2. **Input** - Action mapping with the `input` block
3. **Transform2D** - Spatial properties for the player
4. **Sprite** - Texture loading and rendering
5. **Animation** - SpriteAnimation with spritesheet frames
6. **State Machine** - Managing player states (idle, run, jump, fall)
7. **Camera** - Following the player with smooth interpolation
8. **Console** - Debug console integration
9. **Class Extraction** - Organizing code into reusable classes

## Next Steps

- Add tilemap collision instead of flat ground
- Add enemies with AI state machines
- Add sound effects for jump and land
- Add particle effects for dust and impacts
- Implement multiple levels with scene transitions

## See Also

- [Game Loop](game-loop.md) - Global hooks pattern
- [Input](input.md) - Input system details
- [Graphics](graphics.md) - Drawing and sprites
- [Animation](animation.md) - Animation systems
- [State Machine](state-machine.md) - State machine DSL
- [Camera](camera.md) - Camera system
