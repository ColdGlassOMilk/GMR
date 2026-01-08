# <img src="game/assets/logo.png" height="75" align="middle" /> GMR - Game Middleware for Ruby

**A modern, cross-platform game framework combining the elegance of Ruby with the performance of C++.** Build games with hot-reloading Ruby scripts powered by mruby and raylib, deploy natively to Windows/Linux/macOS or compile to WebAssembly for the browser.

[Check out the live Demo on itch.io!](https://coldglassomilk.itch.io/gmr)

> **Early Stage Project:** GMR is in active development and not production-ready. The API bindings are currently MVP-level (graphics, input, audio basics), and advanced features like networking, physics, and extended audio are not yet implemented. That said, it's stable enough for experimentation, game jams, and small projects. Contributions welcome!

## Features

- **Instant Hot Reload** - Save your Ruby script, see changes immediately. No compile step, no restart, no lost game state
- **Live REPL Console** - Press backtick to open a console mid-game. Tweak variables, test functions, debug in real-time
- **One Codebase, Every Platform** - Write once, deploy to Windows, Linux, macOS, or the web via WebAssembly
- **Ruby Simplicity, Native Speed** - mruby bytecode execution with raylib's hardware-accelerated rendering
- **Batteries Included** - Graphics, tilemaps, audio, input mapping, collision detection - no external dependencies to wire up
- **Safe by Design** - Handle-based resource system keeps Ruby away from raw pointers and memory headaches
- **Modern CLI Tooling** - `gmrcli setup && gmrcli run` - that's it. JSON output for CI/IDE integration, presets for power users

---

## Quick Start

```bash
# Clone and setup
git clone https://github.com/ColdGlassOMilk/GMR
cd GMR
./bootstrap.sh

# Build and run
gmrcli dev
```

Edit `game/scripts/main.rb` - changes reload automatically!

### Minimal Example

A moving circle in 15 lines:

```ruby
include GMR

def init
  Window.set_title("My Game")
  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
  end
  @x, @y = 400, 300
end

def update(dt)
  speed = 200 * dt
  @x -= speed if Input.action_down?(:move_left)
  @x += speed if Input.action_down?(:move_right)
end

def draw
  Graphics.clear(:dark_gray)
  Graphics.draw_circle(@x, @y, 20, :cyan)
end
```

### Next Example: Animated Sprite

Building on the minimal example, add a sprite with animation:

```ruby
include GMR

def init
  Window.set_title("Animated Sprite")

  # Input mapping
  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
  end

  # Create transform (handles position, rotation, scale, origin)
  @transform = Transform2D.new(x: 400, y: 300)

  # Load texture and create sprite with transform
  @texture = Texture.load("assets/player.png")
  @sprite = Sprite.new(@texture, @transform)
  @sprite.center_origin  # Sets origin on the transform

  # Create animation (frames 0-3 from a spritesheet with 8 columns)
  @anim = SpriteAnimation.new(@sprite,
    frames: 0..3,
    fps: 8,
    columns: 8
  )
  @anim.play

  @facing = 1
end

def update(dt)
  speed = 200 * dt

  if Input.action_down?(:move_left)
    @transform.x -= speed
    @facing = -1
  end

  if Input.action_down?(:move_right)
    @transform.x += speed
    @facing = 1
  end

  @sprite.flip_x = @facing < 0
end

def draw
  Graphics.clear("#141428")
  @sprite.draw
end
```

This example introduces:
- **Transform2D** - Manages position, rotation, scale, and origin
- **Texture loading** - `Texture.load` (top-level alias for `Graphics::Texture`)
- **Sprite creation** - `Sprite.new(texture, transform)` requires a Transform2D
- **Sprite animation** - `SpriteAnimation` with spritesheet support
- **Sprite flipping** - `flip_x` for facing direction
- **Color formats** - hex strings like `"#141428"`

---

## Core Concepts

### Game Loop (Global Hooks)

The simplest way to build a game is with global lifecycle hooks. Define these functions at the top level of your script:

```ruby
include GMR

def init
  # Called once when the game starts (or script reloads)
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

This is the recommended approach for small games and prototypes. Your entire game can live in `main.rb` with these three hooks.

### Scene Classes (Optional)

For larger games, you can use Scene classes with the SceneManager for organized state management:

```ruby
include GMR

class GameScene < Scene
  def init
    # Called when this scene becomes active
    @player = Player.new(100, 100)
  end

  def update(dt)
    # Called every frame while this scene is on top
    @player.update(dt)
  end

  def draw
    # Called every frame for rendering
    Graphics.clear(:dark_gray)
    @player.draw
  end

  def unload
    # Called when this scene is removed from the stack
  end
end

# Start with this scene
SceneManager.load(GameScene.new)
```

### Scene Stack

When using SceneManager, scenes are managed as a stack. Only the top scene receives `update` and `draw` calls:

```ruby
SceneManager.load(TitleScene.new)     # Clear stack, set as only scene
SceneManager.push(PauseMenu.new)      # Pause current scene, show menu
SceneManager.pop                       # Remove top scene, resume previous

# Overlays render on top but don't pause the game
SceneManager.add_overlay(HUD.new)
SceneManager.remove_overlay(hud)
```

---

### Input

GMR supports both raw input polling and an action mapping system.

#### Action Mapping (Recommended)

Define logical actions mapped to physical inputs:

```ruby
# Method Chaining - fluent interface
Input.map(:move_left, [:left, :a])
     .map(:move_right, [:right, :d])
     .map(:jump, [:space, :up, :w])

# Block DSL - alternative syntax for defining multiple actions
input do |i|
  i.move_left [:a, :left]
  i.move_right [:d, :right]
  i.jump [:space, :up, :w]
  i.attack :z, mouse: :left    # Keyboard + mouse binding
  i.pause :escape
end

# Query actions in update
def update(dt)
  @player.x -= 200 * dt if Input.action_down?(:move_left)
  @player.x += 200 * dt if Input.action_down?(:move_right)
  @player.jump if Input.action_pressed?(:jump)
end

# Callbacks for discrete events (works with both styles)
Input.on(:pause) { toggle_pause }
Input.on(:jump) { do_jump }
Input.on(:attack, when: :pressed) { @player.attack }
```

#### Input Contexts

Switch between different control schemes (gameplay vs menus):

```ruby
# Define a menu context
input_context :menu do |i|
  i.confirm :enter
  i.cancel :escape
  i.nav_up :up
  i.nav_down :down
end

# Push context when opening menu
def open_pause_menu
  Input.push_context(:menu)
  SceneManager.push(PauseMenu.new)
end

# Pop context when closing
def close_pause_menu
  Input.pop_context
  SceneManager.pop
end
```

#### Raw Input

Direct input polling when you need it:

```ruby
# Keyboard
Input.key_down?(:space)           # Currently held
Input.key_pressed?(:enter)        # Just pressed this frame
Input.key_released?(:shift)       # Just released this frame
Input.key_down?([:a, :left])      # Any of these keys

# Mouse
Input.mouse_x, Input.mouse_y      # Position (virtual resolution aware)
Input.mouse_down?(:left)          # :left, :right, :middle
Input.mouse_pressed?(:right)
Input.mouse_wheel                 # Scroll delta (float)

# Text input
char = Input.char_pressed         # Unicode character code
key = Input.key_pressed           # Last key code
```

---

### Graphics

#### Drawing Primitives

```ruby
def draw
  Graphics.clear("#141428")                               # Hex color
  Graphics.draw_rect(10, 10, 100, 50, :red)               # Named color
  Graphics.draw_rect_outline(10, 10, 100, 50, :white)
  Graphics.draw_circle(200, 100, 30, rgb(0, 255, 0))      # rgb() helper
  Graphics.draw_circle_outline(200, 100, 30, :white)
  Graphics.draw_line(0, 0, 100, 100, :yellow)             # Line
  Graphics.draw_text("Score: #{@score}", 10, 10, 20, [255, 255, 255])  # Array still works
end
```

**Color Formats** - All drawing methods accept colors in multiple formats:
- Named symbols: `:red`, `:blue`, `:dark_gray`, `:transparent`, etc.
- Hex strings: `"#RGB"`, `"#RRGGBB"`, `"#RRGGBBAA"`
- Helper function: `rgb(r, g, b)` or `rgb(r, g, b, a)`
- Arrays: `[r, g, b]` or `[r, g, b, a]` (values 0-255)

#### Textures & Sprites

```ruby
# Load texture once
@texture = Texture.load("assets/player.png")

# Create transform for spatial properties
@transform = Transform2D.new(
  x: 100, y: 100,
  rotation: 0,         # Degrees
  scale_x: 1.0,
  scale_y: 1.0
)

# Create sprite with texture and transform
@sprite = Sprite.new(@texture, @transform)

# Modify transform properties
@transform.x = 200
@transform.y = 150
@transform.position = Mathf::Vec2.new(200, 150)
@transform.rotation = 45                 # Degrees
@transform.scale_x = 2.0
@transform.origin = Mathf::Vec2.new(16, 16)  # Pivot point

# Sprite rendering properties
@sprite.alpha = 0.8                      # 0.0 to 1.0
@sprite.color = :red                     # Tint (also accepts rgb(), hex, array)
@sprite.flip_x = true                    # Mirror horizontally
@sprite.center_origin                    # Sets origin to texture center
@sprite.source_rect = Rect.new(0, 0, 32, 32)  # Spritesheet region

# Draw sprite (queued for batch rendering)
@sprite.draw

# Access the sprite's transform
transform = @sprite.transform
puts "Position: #{transform.x}, #{transform.y}"
```

#### Tilemaps

```ruby
# Create tilemap from tileset texture
@tilemap = Tilemap.new(
  Texture.load("assets/tiles.png"),
  16, 16,    # Tile width, height
  100, 50    # Map width, height in tiles
)

# Set tiles
@tilemap.set(5, 3, 12)      # Set tile at (5,3) to tile index 12
@tilemap.fill(0)            # Fill entire map
@tilemap.fill_rect(0, 0, 10, 10, 5)  # Fill region

# Define tile properties for collision
@tilemap.define_tile(1, solid: true)
@tilemap.define_tile(5, solid: true, platform: true)
@tilemap.define_tile(10, hazard: true, damage: 1)

# Query tile properties
@tilemap.solid?(player_tile_x, player_tile_y)
@tilemap.hazard?(x, y)

# Draw (supports camera offset)
@tilemap.draw(0, 0)
```

#### Transform2D

Transform2D is the unified spatial transformation system. All sprites require a Transform2D to define their position, rotation, scale, and origin.

```ruby
# Create transform
@transform = Transform2D.new(
  x: 100, y: 100,
  rotation: 45,        # Degrees
  scale_x: 2.0,
  scale_y: 2.0,
  origin_x: 16,        # Pivot point
  origin_y: 16
)

# Modify properties
@transform.x = 200
@transform.y += 50
@transform.rotation += 90
@transform.scale_x = 1.5

# Parent-child hierarchy
@gun_transform = Transform2D.new(y: -20)  # Offset from parent
@gun_transform.parent = @turret_transform
# Now gun rotates with turret automatically!

# World space queries (accounts for parent hierarchy)
world_pos = @transform.world_position
world_rot = @transform.world_rotation    # Combined rotation
world_scale = @transform.world_scale     # Combined scale

# Direction vectors (useful for movement)
forward = @transform.forward    # Direction the transform is facing
right = @transform.right        # Perpendicular to forward
@transform.x += forward.x * speed * dt   # Move forward

# Utility functions
@transform.snap_to_grid!(16)    # Snap to 16x16 grid
@transform.round_to_pixel!      # Pixel-perfect positioning

# Interpolation (smooth transitions)
new_pos = Transform2D.lerp_position(start_pos, end_pos, t)
new_rot = Transform2D.lerp_rotation(0, 270, 0.5)  # Shortest path
new_scale = Transform2D.lerp_scale(Vec2.new(1, 1), Vec2.new(2, 2), t)
```

**Key Features:**
- **Unified System** - Single source of truth for all spatial transforms
- **Hierarchy Support** - Parent-child relationships with automatic propagation
- **Cached Performance** - World transforms computed once and cached
- **Matrix-Based** - Mathematically correct 2D affine transformations
- **Ruby-Safe** - Handle-based architecture prevents memory issues

---

### Camera

The Camera class provides smooth following, screen shake, and coordinate conversion:

```ruby
include GMR

def init
  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(480, 270)   # Screen center (half of 960x540)
  @camera.zoom = 2.0
end

def update(dt)
  # Follow player with built-in smoothing
  @camera.follow(@player, smoothing: 0.1)

  # Or with a deadzone (player can move without camera following)
  @camera.follow(@player,
    smoothing: 0.08,
    deadzone: Rect.new(-30, -20, 60, 40)
  )

  # Constrain camera to level bounds
  @camera.bounds = Rect.new(0, 0, @level.width, @level.height)
end

def draw
  # Everything inside the block uses camera transform
  @camera.use do
    @tilemap.draw(0, 0)
    @player.draw
    @enemies.each(&:draw)
  end

  # UI drawn outside camera.use is in screen coordinates
  Graphics.draw_text("Score: #{@score}", 10, 10, 20, :white)
end

# Screen shake on impact
def on_player_hit
  @camera.shake(strength: 5, duration: 0.2, frequency: 30)
end

# Zoom with mouse wheel
def update(dt)
  wheel = Input.mouse_wheel
  if wheel != 0
    @camera.zoom = Mathf.clamp(@camera.zoom + wheel * 0.25, 0.5, 4.0)
  end
end

# Coordinate conversion
world_pos = @camera.screen_to_world(Mathf::Vec2.new(Input.mouse_x, Input.mouse_y))
screen_pos = @camera.world_to_screen(@player.position)
```

---

### State Machine

The state machine DSL provides clean, declarative state management with automatic animation binding:

```ruby
include GMR

class Player
  def initialize(x, y)
    @x, @y = x, y
    @sprite = Sprite.new(Texture.load("assets/player.png"))
    @on_ground = true
    @stamina = 100

    # Animation lookup for state machine
    @animations = {
      idle: SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8),
      run: SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
      jump: SpriteAnimation.new(@sprite, frames: 16..18, fps: 10, loop: false, columns: 8),
      fall: SpriteAnimation.new(@sprite, frames: 19..21, fps: 8, columns: 8)
    }

    setup_state_machine
  end

  def setup_state_machine
    state_machine do
      state :idle do
        animate :idle                              # Auto-play animation on enter
        on :move, :run                             # Event -> target state
        on :jump, :jump, if: -> { @on_ground }     # Conditional transition
        on :fall, :fall
      end

      state :run do
        animate :run
        on :stop, :idle
        on :jump, :jump, if: -> { @on_ground && @stamina >= 10 }
        on :fall, :fall
      end

      state :jump do
        animate :jump
        enter do                                   # Callback on state entry
          @vy = -300
          @stamina -= 10
        end
        on :peak, :fall
      end

      state :fall do
        animate :fall
        on :land, :idle
        on :land_moving, :run
      end
    end
  end

  def update(dt)
    # Movement
    @vx = 0
    @vx -= 200 if Input.action_down?(:move_left)
    @vx += 200 if Input.action_down?(:move_right)

    # Trigger state machine events based on conditions
    state_machine.trigger(:move) if @vx != 0
    state_machine.trigger(:stop) if @vx == 0
    state_machine.trigger(:jump) if Input.action_pressed?(:jump)

    # Physics-based transitions
    state_machine.trigger(:fall) if !@on_ground && @vy > 0
    state_machine.trigger(:peak) if @vy >= 0 && state_machine.state == :jump

    if @on_ground && [:jump, :fall].include?(state_machine.state)
      state_machine.trigger(@vx != 0 ? :land_moving : :land)
    end
  end
end
```

#### State Machine DSL Reference

```ruby
state_machine do
  state :name do
    animate :animation_name           # Play animation from @animations hash
    enter { }                         # Block called on state entry
    exit { }                          # Block called on state exit
    on :event, :target_state          # Transition on event
    on :event, :target, if: -> { condition }  # Conditional transition
  end
end

# Instance methods
state_machine.state                   # Current state symbol
state_machine.state = :forced         # Force state change (bypasses transitions)
state_machine.trigger(:event)         # Trigger event, returns true if transitioned
state_machine.active?                 # Check if active
```

#### Input-Driven Transitions

For direct input-to-state binding:

```ruby
state_machine do
  state :idle do
    on_input :jump, :jumping                        # Transition on action press
    on_input :attack, :attacking, when: :pressed    # Specify input phase
    on_input :crouch, :crouching, when: :held       # While held
  end
end
```

---

### Animation

#### Sprite Animation

For direct control over frame-based animations:

```ruby
@sprite = Sprite.new(Texture.load("assets/player.png"))

# Create animation
@walk_anim = SpriteAnimation.new(@sprite,
  frames: 0..5,          # Frame indices (or array: [0, 1, 2, 1])
  fps: 12,               # Frames per second
  loop: true,            # Loop animation (default: true)
  frame_width: 32,       # Frame size (optional, inferred from source_rect)
  frame_height: 32,
  columns: 8             # Spritesheet columns
)

# Control
@walk_anim.play
@walk_anim.pause
@walk_anim.stop           # Stop and reset to first frame

# Callbacks
@walk_anim.on_complete { transition_to_idle }
@walk_anim.on_frame_change { |frame| spawn_dust if frame == 3 }

# State
@walk_anim.complete?      # True if non-looping animation finished
```

#### Animator (Animation Manager)

For managing multiple animations with transition rules:

```ruby
@animator = Animator.new(@sprite,
  frame_width: 48,
  frame_height: 48,
  columns: 8
)

# Define animations
@animator.add(:idle, frames: 0..3, fps: 6)
@animator.add(:run, frames: 8..13, fps: 12)
@animator.add(:attack, frames: 16..21, fps: 18, loop: false)
@animator.add(:hurt, frames: 24..26, fps: 10, loop: false)

# Transition rules
@animator.allow_transition(:idle, :run)
@animator.allow_transition(:run, :idle)
@animator.allow_transition(:idle, :attack)
@animator.allow_from_any(:hurt)           # Hurt can interrupt any animation

# Playback
@animator.play(:idle)
@animator.play(:attack, transition: :finish_current)  # Queue after current

# Query
@animator.current                        # Current animation name
@animator.playing?                       # Is any animation playing?
@animator.can_play?(:attack)             # Would transition be allowed?

# Callbacks
@animator.on_complete(:attack) { @can_attack_again = true }
```

---

### Tweens

Smooth property animations with easing:

```ruby
include GMR

# Tween sprite properties
Tween.to(@sprite, :alpha, 0.0, duration: 0.5, ease: :out_quad)

# Tween transform properties
Tween.to(@transform, :x, 200, duration: 0.3, ease: :out_back)
  .on_complete { puts "Arrived!" }

# Chained animations (squash/stretch on land)
def on_land
  Tween.to(@transform, :scale_y, 0.7, duration: 0.05, ease: :out_quad)
    .on_complete do
      Tween.to(@transform, :scale_y, 1.0, duration: 0.15, ease: :out_elastic)
    end
end

# Menu slide-in
def show_menu
  @menu_transform.x = -200
  Tween.to(@menu_transform, :x, 50, duration: 0.4, ease: :out_back)
end

# Rotate smoothly
Tween.to(@transform, :rotation, 360, duration: 2.0, ease: :in_out_quad)

# Tween control
@tween = Tween.to(@transform, :y, 100, duration: 1.0)
@tween.pause
@tween.resume
@tween.cancel
@tween.active?
```

#### Easing Functions

```ruby
# Linear
:linear

# Quadratic
:in_quad, :out_quad, :in_out_quad

# Cubic
:in_cubic, :out_cubic, :in_out_cubic

# Quartic / Quintic
:in_quart, :out_quart, :in_out_quart
:in_quint, :out_quint, :in_out_quint

# Sine (gentle)
:in_sine, :out_sine, :in_out_sine

# Exponential
:in_expo, :out_expo, :in_out_expo

# Circular
:in_circ, :out_circ, :in_out_circ

# Back (overshoot)
:in_back, :out_back, :in_out_back

# Elastic
:in_elastic, :out_elastic, :in_out_elastic

# Bounce
:in_bounce, :out_bounce, :in_out_bounce
```

---

## Complete Example: Platformer

Here's a complete, working game demonstrating input, state machine, animation, camera, and physics using global hooks:

```ruby
include GMR

# === CONSTANTS ===
SPEED = 180
GRAVITY = 600
JUMP_FORCE = -280
GROUND_Y = 400

# === GLOBAL HOOKS ===

def init
  # Window setup
  Window.set_size(960, 540)

  # Input mapping
  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w, :up]
  end

  # Player state
  @vx, @vy = 0, 0
  @on_ground = false
  @facing = 1

  # Transform and sprite setup
  @transform = Transform2D.new(x: 100, y: 300)
  @sprite = Sprite.new(Texture.load("assets/player.png"), @transform)
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
    end
  end

  # Camera
  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(480, 270)
  @camera.zoom = 1.0

  # Enable dev console
  Console.enable
end

def update(dt)
  return if Console.open?

  # === INPUT ===
  @vx = 0
  @vx -= SPEED if Input.action_down?(:move_left)
  @vx += SPEED if Input.action_down?(:move_right)
  @facing = @vx.negative? ? -1 : 1 if @vx != 0

  state_machine.trigger(:jump) if Input.action_pressed?(:jump)

  # === PHYSICS ===
  @vy += GRAVITY * dt unless @on_ground
  @transform.x += @vx * dt
  @transform.y += @vy * dt

  # Ground collision
  if @transform.y >= GROUND_Y
    @transform.y = GROUND_Y
    @vy = 0
    @on_ground = true
  else
    @on_ground = false
  end

  # === STATE MACHINE ===
  state_machine.trigger(@vx != 0 ? :move : :stop)

  if !@on_ground
    if @vy > 0
      state_machine.trigger(:peak) if state_machine.state == :jump
      state_machine.trigger(:fall)
    end
  elsif state_machine.state == :fall
    state_machine.trigger(:land)
  end

  # === UPDATE SPRITE ===
  @sprite.flip_x = @facing < 0

  # === CAMERA ===
  @camera.target = Mathf::Vec2.new(@transform.x, @transform.y - 50)
end

def draw
  @camera.use do
    Graphics.clear("#1e1e32")
    Graphics.draw_rect(0, GROUND_Y, 2000, 200, "#3c3c50")  # Ground
    @sprite.draw
  end

  # UI (outside camera)
  Graphics.draw_text("State: #{state_machine.state}", 10, 10, 16, :white)
end
```

### With a Player Class

For better organization, extract the player into a class:

```ruby
include GMR

class Player
  attr_reader :transform

  def initialize(x, y)
    @transform = Transform2D.new(x: x, y: y)
    @vx, @vy = 0, 0
    @on_ground = false
    @facing = 1

    @sprite = Sprite.new(Texture.load("assets/player.png"), @transform)
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
        enter { @vy = -280 }
        on :peak, :fall
      end

      state :fall do
        animate :fall
        on :land, :idle
      end
    end
  end

  def update(dt)
    # Input
    @vx = 0
    @vx -= 180 if Input.action_down?(:move_left)
    @vx += 180 if Input.action_down?(:move_right)
    @facing = @vx.negative? ? -1 : 1 if @vx != 0
    state_machine.trigger(:jump) if Input.action_pressed?(:jump)

    # Physics
    @vy += 600 * dt unless @on_ground
    @transform.x += @vx * dt
    @transform.y += @vy * dt

    if @transform.y >= 400
      @transform.y, @vy, @on_ground = 400, 0, true
    else
      @on_ground = false
    end

    # State transitions
    state_machine.trigger(@vx != 0 ? :move : :stop)
    if !@on_ground && @vy > 0
      state_machine.trigger(:peak) if state_machine.state == :jump
      state_machine.trigger(:fall)
    elsif @on_ground && state_machine.state == :fall
      state_machine.trigger(:land)
    end

    # Sprite
    @sprite.flip_x = @facing < 0
  end

  def draw
    @sprite.draw
  end

  def position
    @transform.position
  end
end

# === GLOBAL HOOKS ===

def init
  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.jump [:space, :w, :up]
  end

  @player = Player.new(100, 300)
  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(480, 270)
  @camera.follow(@player, smoothing: 0.1)
end

def update(dt)
  return if Console.open?
  @player.update(dt)
end

def draw
  @camera.use do
    Graphics.clear("#1e1e32")
    Graphics.draw_rect(0, 400, 2000, 200, "#3c3c50")
    @player.draw
  end
end
```

---

## Scene Stack & Overlays (SceneManager Pattern)

When using Scene classes with SceneManager, you can push pause menus or dialogs without destroying game state:

```ruby
include GMR

class GameScene < Scene
  def update(dt)
    return if Console.open?

    @player.update(dt)
    @enemies.each { |e| e.update(dt) }

    if Input.action_pressed?(:pause)
      SceneManager.push(PauseMenu.new)
    end
  end
end

class PauseMenu < Scene
  def init
    Input.push_context(:menu)
  end

  def update(dt)
    if Input.action_pressed?(:cancel)
      Input.pop_context
      SceneManager.pop
    end

    if Input.action_pressed?(:confirm)
      System.quit
    end
  end

  def draw
    # Semi-transparent overlay
    Graphics.draw_rect(0, 0, 960, 540, [0, 0, 0, 180])
    Graphics.draw_text("PAUSED", 400, 200, 48, [255, 255, 255])
    Graphics.draw_text("Press ESC to resume", 380, 280, 20, [180, 180, 180])
    Graphics.draw_text("Press ENTER to quit", 380, 310, 20, [180, 180, 180])
  end

  def unload
    Input.pop_context
  end
end

# Overlays render on top but don't pause the game
class MinimapOverlay < Scene
  def draw
    Graphics.draw_rect(10, 10, 150, 100, [0, 0, 0, 150])
    # Draw minimap contents...
  end
end

# In your init:
SceneManager.load(GameScene.new)
SceneManager.add_overlay(MinimapOverlay.new)
```

For simpler games using global hooks, you can manage pause state with a boolean flag instead of the SceneManager.

---

## Live Console

Press **`** (backtick) to open an interactive Ruby REPL mid-game:

```ruby
# Enable console with optional styling
Console.enable(
  height: 300,
  background: "#1a1a2e",
  font_size: 16
)

# Register custom commands
Console.register_command("tp", "Teleport to x,y") do |args|
  if args.length >= 2
    @player.x = args[0].to_f
    @player.y = args[1].to_f
    "Teleported to #{args[0]}, #{args[1]}"
  else
    "Usage: tp <x> <y>"
  end
end

Console.register_command("god", "Toggle invincibility") do
  @player.god_mode = !@player.god_mode
  "God mode: #{@player.god_mode ? 'ON' : 'OFF'}"
end

Console.register_command("spawn", "Spawn enemy at cursor") do
  @enemies << Enemy.new(Input.mouse_x, Input.mouse_y)
  "Spawned enemy at #{Input.mouse_x}, #{Input.mouse_y}"
end
```

In the console, you can execute any Ruby code:

```ruby
@player.x = 400                    # Modify game state
@player.health = 100               # Reset health
Time.scale = 0.5                   # Slow motion
@camera.zoom = 3.0                 # Zoom in
Tween.to(@camera, :zoom, 1.0, duration: 0.5)  # Animate back
System.quit                        # Exit game
```

Pause the game when console is open:

```ruby
def update(dt)
  return if Console.open?
  # Game logic...
end
```

---

## CLI

All GMR operations go through `gmrcli`. It outputs structured JSON by default, making it straightforward to integrate with editors, CI systems, and custom tooling. For terminal use, pass `-o text` for human-readable output, or use the `dev` command which defaults to text mode.

```bash
# Quick development (recommended)
gmrcli dev                # Build and run
gmrcli dev --clean        # Fresh build and run
gmrcli dev web            # Web build and local server

# Fine-grained control
gmrcli build debug        # Debug build only
gmrcli build release      # Optimized release build
gmrcli build web          # WebAssembly build only
gmrcli run                # Run existing build
gmrcli run web            # Start web server

# Project management
gmrcli setup              # Install dependencies
gmrcli new my-game        # Create new project
gmrcli docs               # Generate documentation
```

---

## API Quick Reference

All classes and modules live under the `GMR` namespace. Use `include GMR` at the top of your scripts to access them without the prefix.

**Top-Level Aliases**: After `include GMR`, these classes are available directly: `Sprite`, `Texture`, `Tilemap`, `Camera`, `Rect`, `Tween`, `Animator`, `SpriteAnimation`, `Transform2D`.

**Color Formats**: All color parameters accept: `:red` (symbols), `"#FF0000"` (hex), `rgb(255, 0, 0)` (helper), or `[255, 0, 0]` (arrays).

### GMR::Graphics

| Class/Module | Key Methods |
|--------------|-------------|
| `Graphics` | `clear(color)`, `draw_rect`, `draw_rect_outline`, `draw_circle`, `draw_circle_outline`, `draw_line`, `draw_line_thick`, `draw_text`, `measure_text`, `rgb(r, g, b, a)` |
| `Texture` | `.load(path)`, `draw(x, y)`, `draw_ex(x, y, rotation, scale)`, `width`, `height` |
| `Transform2D` | `.new(x:, y:, rotation:, scale_x:, scale_y:, origin_x:, origin_y:)`, `x`, `y`, `position`, `rotation`, `scale_x`, `scale_y`, `origin`, `parent`, `world_position`, `world_rotation`, `world_scale`, `forward`, `right`, `snap_to_grid!`, `round_to_pixel!`, `.lerp_position`, `.lerp_rotation`, `.lerp_scale` |
| `Sprite` | `.new(texture, transform)`, `transform`, `alpha`, `color`, `flip_x`, `flip_y`, `center_origin`, `source_rect`, `draw` |
| `Tilemap` | `.new(tileset, tw, th, w, h)`, `set(x, y, tile)`, `fill`, `fill_rect`, `draw`, `solid?`, `hazard?`, `platform?`, `define_tile` |
| `Camera` | `.new`, `target`, `offset`, `zoom`, `rotation`, `follow(obj, smoothing:, deadzone:)`, `bounds=`, `shake(strength:, duration:)`, `use { }`, `screen_to_world`, `world_to_screen` |
| `Rect` | `.new(x, y, w, h)`, `x`, `y`, `w`, `h` |

### GMR::Animation

| Class/Module | Key Methods |
|--------------|-------------|
| `Tween` | `.to(obj, :prop, value, duration:, ease:)`, `pause`, `resume`, `cancel`, `active?`, `on_complete { }`, `on_update { }` |
| `Animator` | `.new(sprite, **opts)`, `add(:name, frames:, fps:, loop:)`, `play(:name)`, `stop`, `current`, `playing?`, `allow_transition`, `allow_from_any`, `on_complete(:name) { }` |
| `SpriteAnimation` | `.new(sprite, frames:, fps:, loop:, columns:)`, `play`, `pause`, `stop`, `complete?`, `on_complete { }`, `on_frame_change { }` |
| `Animation::Ease` | `:linear`, `:in_quad`, `:out_quad`, `:in_out_quad`, `:out_back`, `:out_elastic`, `:out_bounce`, etc. |

### GMR::Audio

| Class/Module | Key Methods |
|--------------|-------------|
| `Audio::Sound` | `.load(path)`, `play`, `stop`, `volume=` |

### GMR::Input

| Module | Key Methods |
|--------|-------------|
| `Input` | `key_down?`, `key_pressed?`, `key_released?`, `mouse_x`, `mouse_y`, `mouse_down?`, `mouse_pressed?`, `mouse_wheel`, `action_down?`, `action_pressed?`, `action_released?`, `on(:action) { }`, `push_context`, `pop_context` |

### GMR::Core

| Class/Module | Key Methods |
|--------------|-------------|
| `Core::StateMachine` | `.attach`, `.count`, `state`, `state=`, `trigger(:event)`, `active?` |
| `Core::Node` | `add_child`, `remove_child`, `parent`, `children`, `local_position`, `world_position`, `active?` |

### GMR::Mathf

| Class/Module | Key Methods |
|--------------|-------------|
| `Mathf` | `lerp`, `inverse_lerp`, `clamp`, `wrap`, `smoothstep`, `remap`, `distance`, `distance_squared`, `sign`, `move_toward`, `deg_to_rad`, `rad_to_deg`, `random_int`, `random_float` |
| `Mathf::Vec2` | `.new(x, y)`, `x`, `y`, `+`, `-`, `*`, `/`, `to_a` |
| `Mathf::Vec3` | `.new(x, y, z)`, `x`, `y`, `z`, `+`, `-`, `*`, `/` |

### Top-Level Modules

| Module | Key Methods |
|--------|-------------|
| `Scene` | Base class: `init`, `update(dt)`, `draw`, `unload` |
| `SceneManager` | `load`, `push`, `pop`, `current`, `add_overlay`, `remove_overlay`, `has_overlay?` |
| `Console` | `enable(**opts)`, `disable`, `open?`, `show`, `hide`, `toggle`, `register_command`, `println` |
| `Window` | `set_size`, `set_title`, `fullscreen`, `width`, `height` |
| `Time` | `delta`, `fps`, `elapsed`, `scale`, `scale=` |
| `System` | `platform`, `quit`, `build_type`, `gpu_renderer` |
| `Collision` | `rect_overlap?`, `circle_overlap?`, `point_in_rect?`, `rect_tiles` |

---

## Documentation

**Reference Documentation** (auto-generated from source):

- [CLI Reference](docs/cli/README.md) - All `gmrcli` commands & options
- [API Reference](docs/api/README.md) - Complete Ruby API documentation

Run `gmrcli docs` to regenerate documentation after modifying source files.

---

## License

MIT License - completely free and open source, no royalties, no restrictions.

Copyright (c) 2026 GMR Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
