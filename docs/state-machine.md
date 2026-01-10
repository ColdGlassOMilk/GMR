# State Machine

GMR provides a lightweight state machine DSL for managing game logic. State machines are useful for player controllers, AI, UI flows, and any system with distinct modes of behavior.

## Basic Structure

```ruby
include GMR

class Player
  def initialize
    setup_state_machine
  end

  def setup_state_machine
    state_machine do
      state :idle do
        on :move, :running
        on :jump, :jumping
      end

      state :running do
        on :stop, :idle
        on :jump, :jumping
      end

      state :jumping do
        on :land, :idle
      end
    end
  end
end
```

## State Definition

### Basic States

```ruby
state_machine do
  state :idle do
    # State configuration here
  end

  state :running do
    # ...
  end
end
```

The first state defined is the initial state.

### Transitions

Define what events cause state changes:

```ruby
state :idle do
  on :move, :running        # :move event transitions to :running
  on :jump, :jumping        # :jump event transitions to :jumping
  on :attack, :attacking
end
```

### Conditional Transitions

Add conditions that must be true for a transition to occur:

```ruby
state :idle do
  on :jump, :jumping, if: -> { @on_ground }
  on :attack, :attacking, if: -> { @stamina >= 10 }
end
```

The condition is a lambda evaluated in the context of the object.

### Entry and Exit Callbacks

```ruby
state :jumping do
  enter do
    @vy = -300           # Apply jump velocity
    @stamina -= 10       # Cost stamina
    @jump_sound.play     # Play sound
  end

  exit do
    @can_double_jump = false
  end

  on :land, :idle
end
```

`enter` is called when transitioning into the state. `exit` is called when leaving.

## Animation Binding

The `animate` directive automatically plays animations on state entry.

### With Animator

```ruby
class Player
  def initialize
    @sprite = Sprite.new(Texture.load("player.png"), @transform)

    # Create animator
    @animator = Animator.new(@sprite,
      columns: 8,
      frame_width: 32,
      frame_height: 48
    )

    @animator.add(:idle, frames: 0..3, fps: 6)
    @animator.add(:run, frames: 8..13, fps: 12)
    @animator.add(:jump, frames: 16..18, fps: 10, loop: false)

    setup_state_machine
  end

  def setup_state_machine
    state_machine do
      state :idle do
        animate :idle    # Calls @animator.play(:idle)
        on :move, :running
      end

      state :running do
        animate :run     # Calls @animator.play(:run)
        on :stop, :idle
      end

      state :jumping do
        animate :jump    # Calls @animator.play(:jump)
        on :land, :idle
      end
    end
  end
end
```

### With @animations Hash

Alternatively, use a hash of SpriteAnimation objects:

```ruby
class Player
  def initialize
    @sprite = Sprite.new(Texture.load("player.png"), @transform)

    @animations = {
      idle: SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8),
      run: SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
      jump: SpriteAnimation.new(@sprite, frames: 16..18, fps: 10, loop: false, columns: 8)
    }

    setup_state_machine
  end

  def setup_state_machine
    state_machine do
      state :idle do
        animate :idle    # Plays @animations[:idle]
        on :move, :running
      end
    end
  end
end
```

The state machine checks for `@animator` first, then falls back to `@animations`.

## Triggering Events

Trigger events in your update loop:

```ruby
def update(dt)
  # Gather input
  @vx = 0
  @vx -= 200 if Input.action_down?(:move_left)
  @vx += 200 if Input.action_down?(:move_right)

  # Trigger events based on conditions
  state_machine.trigger(:move) if @vx != 0
  state_machine.trigger(:stop) if @vx == 0
  state_machine.trigger(:jump) if Input.action_pressed?(:jump)

  # Physics-based events
  if @on_ground && state_machine.state == :jumping
    state_machine.trigger(:land)
  end

  if !@on_ground && @vy > 0
    state_machine.trigger(:fall)
  end
end
```

`trigger` returns `true` if the transition occurred, `false` if blocked (no matching transition or condition failed).

## Querying State

```ruby
state_machine.state          # Current state symbol (:idle, :running, etc.)
state_machine.active?        # Is the state machine running?

# Common pattern
if state_machine.state == :attacking
  # Can't move while attacking
  return
end
```

## Forcing State Changes

Bypass transitions and directly set state:

```ruby
state_machine.state = :idle  # Force to idle, ignoring transitions
```

Use sparingly. This skips transition checks and won't call enter/exit callbacks properly.

## Input-Driven Transitions

Bind transitions directly to input actions:

```ruby
state_machine do
  state :idle do
    on_input :jump, :jumping                        # Transition on action press
    on_input :attack, :attacking, when: :pressed    # Explicit press
    on_input :crouch, :crouching, when: :held       # While held
  end
end
```

| `when` value | Behavior |
|--------------|----------|
| `:pressed` (default) | Transition when action is first pressed |
| `:held` | Transition while action is held |
| `:released` | Transition when action is released |

## Complete Player Example

```ruby
include GMR

class Player
  attr_reader :transform

  def initialize(x, y)
    @transform = Transform2D.new(x: x, y: y)
    @vx, @vy = 0, 0
    @on_ground = true
    @facing = 1
    @stamina = 100

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
        on :jump, :jump, if: -> { @on_ground && @stamina >= 10 }
        on :fall, :fall
      end

      state :jump do
        animate :jump
        enter do
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
    # Movement input
    @vx = 0
    @vx -= 200 if Input.action_down?(:move_left)
    @vx += 200 if Input.action_down?(:move_right)
    @facing = @vx.negative? ? -1 : 1 if @vx != 0

    # Jump input
    state_machine.trigger(:jump) if Input.action_pressed?(:jump)

    # Physics
    @vy += 600 * dt unless @on_ground
    @transform.x += @vx * dt
    @transform.y += @vy * dt

    # Ground check
    if @transform.y >= 400  # Ground level
      @transform.y = 400
      @vy = 0
      was_airborne = !@on_ground
      @on_ground = true

      if was_airborne
        state_machine.trigger(@vx != 0 ? :land_moving : :land)
      end
    else
      @on_ground = false
    end

    # Movement state transitions
    state_machine.trigger(@vx != 0 ? :move : :stop)

    # Air state transitions
    if !@on_ground
      if @vy > 0
        state_machine.trigger(:peak) if state_machine.state == :jump
        state_machine.trigger(:fall)
      end
    end

    # Update sprite
    @sprite.flip_x = @facing < 0

    # Regenerate stamina
    @stamina = [@stamina + 20 * dt, 100].min if @on_ground
  end

  def draw
    @sprite.draw
  end
end
```

## Common Patterns

### AI State Machine

```ruby
class Enemy
  def setup_state_machine
    state_machine do
      state :patrol do
        enter { pick_patrol_point }
        on :see_player, :chase
        on :reached_point, :wait
      end

      state :wait do
        enter { @wait_timer = 2.0 }
        on :timeout, :patrol
        on :see_player, :chase
      end

      state :chase do
        on :lost_player, :search
        on :in_range, :attack
      end

      state :attack do
        enter { start_attack_animation }
        on :attack_done, :chase
      end

      state :search do
        enter { @search_timer = 3.0 }
        on :see_player, :chase
        on :timeout, :patrol
      end
    end
  end
end
```

### Menu State Machine

```ruby
class MainMenu
  def setup_state_machine
    state_machine do
      state :main do
        on :select_play, :play_selected
        on :select_options, :options
        on :select_quit, :quit_confirm
      end

      state :options do
        on :back, :main
        on :select_audio, :audio_options
        on :select_video, :video_options
      end

      state :quit_confirm do
        on :confirm, :quitting
        on :cancel, :main
      end

      state :quitting do
        enter { System.quit }
      end
    end
  end
end
```

## Best Practices

1. **Keep states focused** - Each state should represent one clear behavior
2. **Use conditions for guards** - Don't create states just for validation
3. **Trigger events, don't force states** - Let the state machine manage transitions
4. **Name events by what happened** - `:landed`, `:took_damage`, `:player_spotted`
5. **Name states by what the entity is doing** - `:idle`, `:running`, `:attacking`

## See Also

- [Animation](animation.md) - Animation integration
- [Input](input.md) - Input-driven transitions
- [Game Loop](game-loop.md) - Where to trigger events
- [API Reference](api/engine/state-machine/statemachine.md) - Complete State Machine API
