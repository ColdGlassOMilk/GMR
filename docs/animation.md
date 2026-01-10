# Animation

GMR provides three animation systems: SpriteAnimation for frame-based sprite animation, Animator for managing multiple animations with transitions, and Tweens for animating any property.

## SpriteAnimation

SpriteAnimation cycles through frames of a spritesheet.

### Creating Animations

```ruby
@sprite = Sprite.new(Texture.load("player.png"), @transform)

@walk_anim = SpriteAnimation.new(@sprite,
  frames: 0..5,          # Frame indices (range or array)
  fps: 12,               # Frames per second
  loop: true,            # Loop animation (default: true)
  columns: 8             # Columns in spritesheet
)
```

Frame indices count left-to-right, top-to-bottom in the spritesheet.

### Optional Parameters

```ruby
@anim = SpriteAnimation.new(@sprite,
  frames: [0, 1, 2, 1],  # Array for custom sequences
  fps: 10,
  loop: false,           # Play once and stop
  frame_width: 32,       # Frame size (optional, inferred from source_rect)
  frame_height: 32,
  columns: 8
)
```

### Playback Control

```ruby
@anim.play           # Start playing
@anim.pause          # Pause at current frame
@anim.stop           # Stop and reset to first frame
```

### Callbacks

```ruby
# Called when non-looping animation completes
@anim.on_complete { transition_to_idle }

# Called each time the frame changes
@anim.on_frame_change { |frame| spawn_dust if frame == 3 }
```

### State Queries

```ruby
@anim.complete?      # True if non-looping animation finished
```

## Animator

Animator manages multiple named animations with transition rules.

### Setup

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
```

### Transition Rules

Control which animations can interrupt which:

```ruby
# Allow specific transitions
@animator.allow_transition(:idle, :run)
@animator.allow_transition(:run, :idle)
@animator.allow_transition(:idle, :attack)

# Allow from any animation
@animator.allow_from_any(:hurt)  # Hurt can interrupt anything
```

### Playback

```ruby
@animator.play(:idle)

# Queue to play after current finishes
@animator.play(:attack, transition: :finish_current)
```

### Queries

```ruby
@animator.current        # Current animation name (:idle, :run, etc.)
@animator.playing?       # Is any animation playing?
@animator.can_play?(:attack)  # Would transition be allowed?
```

### Callbacks

```ruby
@animator.on_complete(:attack) { @can_attack_again = true }
```

## Tweens

Tweens smoothly animate any numeric property over time.

### Basic Usage

```ruby
# Tween a sprite's alpha to 0 over 0.5 seconds
Tween.to(@sprite, :alpha, 0.0, duration: 0.5, ease: :out_quad)

# Tween transform position
Tween.to(@transform, :x, 200, duration: 0.3, ease: :out_back)
```

### Callbacks

```ruby
Tween.to(@transform, :x, 200, duration: 0.3)
  .on_complete { puts "Arrived!" }
  .on_update { |value| puts "Current: #{value}" }
```

### Chained Animations

```ruby
# Squash and stretch on landing
def on_land
  Tween.to(@transform, :scale_y, 0.7, duration: 0.05, ease: :out_quad)
    .on_complete do
      Tween.to(@transform, :scale_y, 1.0, duration: 0.15, ease: :out_elastic)
    end
end
```

### Tween Control

```ruby
@tween = Tween.to(@transform, :y, 100, duration: 1.0)

@tween.pause
@tween.resume
@tween.cancel
@tween.active?
```

### Common Patterns

```ruby
# Fade out and destroy
def fade_out
  Tween.to(@sprite, :alpha, 0.0, duration: 0.3)
    .on_complete { destroy }
end

# Menu slide in
def show_menu
  @menu_transform.x = -200
  Tween.to(@menu_transform, :x, 50, duration: 0.4, ease: :out_back)
end

# Pulsing effect
def pulse
  Tween.to(@transform, :scale_x, 1.2, duration: 0.3, ease: :in_out_quad)
    .on_complete do
      Tween.to(@transform, :scale_x, 1.0, duration: 0.3, ease: :in_out_quad)
        .on_complete { pulse }  # Loop
    end
end

# Rotate continuously
Tween.to(@transform, :rotation, 360, duration: 2.0, ease: :linear)
  .on_complete { @transform.rotation = 0; rotate_again }
```

## Easing Functions

Easing functions control the rate of change over time.

### Linear

```ruby
:linear  # Constant speed
```

### Quadratic

```ruby
:in_quad      # Start slow, accelerate
:out_quad     # Start fast, decelerate
:in_out_quad  # Slow at both ends
```

### Cubic

```ruby
:in_cubic, :out_cubic, :in_out_cubic
```

### Quartic / Quintic

```ruby
:in_quart, :out_quart, :in_out_quart
:in_quint, :out_quint, :in_out_quint
```

### Sine

Gentle, natural-feeling curves:

```ruby
:in_sine, :out_sine, :in_out_sine
```

### Exponential

Very dramatic acceleration/deceleration:

```ruby
:in_expo, :out_expo, :in_out_expo
```

### Circular

```ruby
:in_circ, :out_circ, :in_out_circ
```

### Back (Overshoot)

Pulls back before moving, or overshoots the target:

```ruby
:in_back      # Pulls back first
:out_back     # Overshoots then settles
:in_out_back  # Both
```

### Elastic

Springy, bouncy motion:

```ruby
:in_elastic, :out_elastic, :in_out_elastic
```

### Bounce

Ball-bounce effect:

```ruby
:in_bounce, :out_bounce, :in_out_bounce
```

### Choosing Easing

| Use Case | Recommended Easing |
|----------|-------------------|
| UI elements appearing | `:out_back`, `:out_elastic` |
| UI elements disappearing | `:in_quad`, `:in_back` |
| Character movement | `:out_quad`, `:in_out_quad` |
| Impacts, hits | `:out_elastic`, `:out_bounce` |
| Camera transitions | `:in_out_quad`, `:in_out_sine` |
| Fades | `:linear`, `:out_quad` |

## Integration with State Machines

The state machine DSL can automatically play animations on state entry. See [State Machine](state-machine.md) for details.

```ruby
state_machine do
  state :idle do
    animate :idle  # Plays animation on state entry
  end

  state :run do
    animate :run
  end
end
```

## Complete Example

```ruby
include GMR

def init
  @transform = Transform2D.new(x: 400, y: 300)
  @texture = Texture.load("player.png")
  @sprite = Sprite.new(@texture, @transform)
  @sprite.center_origin

  # Setup animator
  @animator = Animator.new(@sprite,
    frame_width: 32,
    frame_height: 32,
    columns: 8
  )

  @animator.add(:idle, frames: 0..3, fps: 6)
  @animator.add(:run, frames: 8..13, fps: 12)
  @animator.add(:jump, frames: 16..18, fps: 10, loop: false)

  @animator.allow_transition(:idle, :run)
  @animator.allow_transition(:run, :idle)
  @animator.allow_transition(:idle, :jump)
  @animator.allow_transition(:run, :jump)
  @animator.allow_from_any(:idle)  # Can return to idle from anywhere

  @animator.play(:idle)

  @vx = 0
end

def update(dt)
  @vx = 0
  @vx -= 200 if Input.key_down?(:left)
  @vx += 200 if Input.key_down?(:right)

  @transform.x += @vx * dt

  # Switch animations based on movement
  if @vx != 0
    @animator.play(:run) if @animator.can_play?(:run)
  else
    @animator.play(:idle) if @animator.can_play?(:idle)
  end

  # Jump with squash effect
  if Input.key_pressed?(:space)
    @animator.play(:jump)
    # Squash before jump
    Tween.to(@transform, :scale_y, 0.8, duration: 0.05, ease: :out_quad)
      .on_complete do
        Tween.to(@transform, :scale_y, 1.2, duration: 0.1, ease: :out_quad)
          .on_complete do
            Tween.to(@transform, :scale_y, 1.0, duration: 0.2, ease: :out_elastic)
          end
      end
  end
end

def draw
  Graphics.clear("#1e1e32")
  @sprite.draw
end
```

## See Also

- [State Machine](state-machine.md) - Animation integration with states
- [Graphics](graphics.md) - Sprites and rendering
- [Transforms](transforms.md) - Animating transform properties
- [API Reference](api/engine/animation/README.md) - Complete Animation API
