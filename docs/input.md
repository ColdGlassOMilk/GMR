# Input

GMR provides both raw input polling and an action mapping system. Action mapping is recommended for most games.

## Action Mapping

Map logical actions to physical inputs. This decouples game logic from specific keys and makes rebinding easier.

### Block DSL

```ruby
input do |i|
  i.move_left [:a, :left]
  i.move_right [:d, :right]
  i.jump [:space, :up, :w]
  i.attack :z, mouse: :left    # Keyboard + mouse binding
  i.pause :escape
end
```

### Method Chaining

```ruby
Input.map(:move_left, [:left, :a])
     .map(:move_right, [:right, :d])
     .map(:jump, [:space, :up, :w])
```

Both syntaxes are equivalent. Use whichever you prefer.

## Querying Actions

```ruby
def update(dt)
  # Check if action is currently held
  @player.x -= 200 * dt if Input.action_down?(:move_left)
  @player.x += 200 * dt if Input.action_down?(:move_right)

  # Check if action was just pressed this frame
  @player.jump if Input.action_pressed?(:jump)

  # Check if action was just released this frame
  @player.stop_charging if Input.action_released?(:attack)
end
```

| Method | Returns true when |
|--------|-------------------|
| `action_down?(:name)` | Action is currently held |
| `action_pressed?(:name)` | Action was pressed this frame |
| `action_released?(:name)` | Action was released this frame |

## Action Callbacks

Register callbacks for discrete events:

```ruby
Input.on(:pause) { toggle_pause }
Input.on(:jump) { @player.jump }
Input.on(:attack, when: :pressed) { @player.start_attack }
Input.on(:attack, when: :released) { @player.release_attack }
```

The `when` parameter defaults to `:pressed`. Options are `:pressed` and `:released`.

## Input Contexts

Switch between different control schemes for gameplay vs menus:

```ruby
# Define a context for menu navigation
input_context :menu do |i|
  i.confirm :enter
  i.cancel :escape
  i.nav_up :up
  i.nav_down :down
end

# Define default gameplay context
input do |i|
  i.move_left [:a, :left]
  i.move_right [:d, :right]
  i.jump [:space, :w]
  i.pause :escape
end
```

### Switching Contexts

```ruby
def open_pause_menu
  Input.push_context(:menu)
  SceneManager.push(PauseMenu.new)
end

def close_pause_menu
  Input.pop_context
  SceneManager.pop
end
```

Context management is stack-based. Push when entering a new mode, pop when leaving.

## Raw Input

When you need direct access to specific keys or mouse buttons.

### Keyboard

```ruby
# Single key
Input.key_down?(:space)           # Currently held
Input.key_pressed?(:enter)        # Just pressed this frame
Input.key_released?(:shift)       # Just released this frame

# Any of multiple keys
Input.key_down?([:a, :left])      # True if any key is down
```

### Available Key Symbols

Letters: `:a` through `:z`

Numbers: `:zero` through `:nine` (or `:num_0` through `:num_9`)

Arrows: `:up`, `:down`, `:left`, `:right`

Modifiers: `:shift`, `:ctrl`, `:alt`

Special: `:space`, `:enter`, `:escape`, `:tab`, `:backspace`

Function keys: `:f1` through `:f12`

### Mouse

```ruby
# Position (virtual resolution aware)
x = Input.mouse_x
y = Input.mouse_y

# Buttons: :left, :right, :middle
Input.mouse_down?(:left)
Input.mouse_pressed?(:right)
Input.mouse_released?(:middle)

# Scroll wheel (float, positive = up)
wheel = Input.mouse_wheel
```

Mouse position is automatically adjusted for virtual resolution. If your game runs at 960x540 but the window is larger, `mouse_x` and `mouse_y` return coordinates in game space.

### Text Input

For text fields and name entry:

```ruby
def update(dt)
  # Get the Unicode code point of the last character pressed
  char_code = Input.char_pressed
  if char_code > 0
    @text += char_code.chr
  end

  # Handle backspace separately
  if Input.key_pressed?(:backspace) && @text.length > 0
    @text = @text[0..-2]
  end
end
```

## Combining Inputs

### Multiple Keys for One Action

Pass an array to bind multiple keys:

```ruby
input do |i|
  i.jump [:space, :w, :up]  # Any of these triggers jump
end
```

### Keyboard + Mouse

Use the `mouse:` parameter:

```ruby
input do |i|
  i.attack :z, mouse: :left       # Z key or left mouse button
  i.secondary :x, mouse: :right   # X key or right mouse button
end
```

### Keyboard + Gamepad

Use the `gamepad:` parameter to bind controller buttons alongside keyboard:

```ruby
input do |i|
  i.jump [:space, :w], gamepad: :a           # Space/W or A button
  i.attack :z, gamepad: :x                   # Z key or X button
  i.pause :escape, gamepad: :start           # Escape or Start button
  i.dash :shift, gamepad: [:lb, :rb]         # Shift or either bumper
end
```

By default, gamepad bindings respond to any connected gamepad. To bind to a specific gamepad:

```ruby
input do |i|
  # Player 1 controls (gamepad 0 only)
  i.p1_jump :space, gamepad: :a, gamepad_index: 0

  # Player 2 controls (gamepad 1 only)
  i.p2_jump :enter, gamepad: :a, gamepad_index: 1
end
```

### Available Gamepad Button Symbols

Face buttons: `:a`, `:b`, `:x`, `:y`

D-pad: `:dpad_up`, `:dpad_down`, `:dpad_left`, `:dpad_right`

Shoulders: `:lb`, `:rb` (bumpers), `:lt`, `:rt` (triggers as buttons)

Thumbsticks: `:l3`, `:r3` (stick click)

Center: `:start`, `:select`, `:guide`

## Gamepad (Raw Access)

For direct gamepad access without action mapping, use the `Gamepad` module.

### Connection Status

```ruby
# Check how many gamepads are connected
num_gamepads = Gamepad.count

# Check specific gamepad
if Gamepad.connected?(0)
  puts "Gamepad 0: #{Gamepad.name(0)}"
end
```

### Button State

```ruby
# Check button state on gamepad 0
Gamepad.down?(0, :a)        # Currently held
Gamepad.pressed?(0, :b)     # Just pressed this frame
Gamepad.released?(0, :x)    # Just released this frame

# Check any connected gamepad
Gamepad.any_pressed?(:start)  # Start pressed on any gamepad
Gamepad.any_down?(:a)         # A button held on any gamepad
```

### Analog Sticks

```ruby
# Read analog stick with dead zone applied (-1.0 to 1.0)
move_x = Gamepad.axis(0, :left_x)
move_y = Gamepad.axis(0, :left_y)
aim_x = Gamepad.axis(0, :right_x)
aim_y = Gamepad.axis(0, :right_y)

# Read raw axis value (no dead zone)
raw_x = Gamepad.axis_raw(0, :left_x)
```

Available axes: `:left_x`, `:left_y`, `:right_x`, `:right_y`, `:left_trigger`, `:right_trigger`

### Dead Zone Configuration

```ruby
# Set inner dead zone (default: 0.15)
# Values below this threshold are treated as zero
Gamepad.dead_zone = 0.2

# Set outer dead zone (default: 0.95)
# Values above this threshold are treated as 1.0/-1.0
Gamepad.outer_dead_zone = 0.98
```

### Vibration / Rumble

```ruby
# Trigger rumble (0.0 to 1.0 intensity)
Gamepad.vibrate(0, left: 0.5, right: 0.3, duration: 0.2)

# Or with positional arguments
Gamepad.vibrate(0, 0.5, 0.5, 0.1)  # gamepad, left, right, duration
```

### Complete Gamepad Example

```ruby
def update(dt)
  return unless Gamepad.connected?(0)

  # Analog movement
  move_x = Gamepad.axis(0, :left_x)
  move_y = Gamepad.axis(0, :left_y)
  @player.move(move_x * @speed * dt, move_y * @speed * dt)

  # Analog aiming
  aim_x = Gamepad.axis(0, :right_x)
  aim_y = Gamepad.axis(0, :right_y)
  if aim_x.abs > 0.1 || aim_y.abs > 0.1
    @player.aim_direction = Math.atan2(aim_y, aim_x)
  end

  # Triggers as analog input
  shoot_power = Gamepad.axis(0, :right_trigger)
  @player.charge_shot(shoot_power) if shoot_power > 0.1

  # Button actions
  @player.jump if Gamepad.pressed?(0, :a)
  @player.dash if Gamepad.pressed?(0, :lb)

  # Rumble on hit
  if @player.took_damage?
    Gamepad.vibrate(0, left: 0.8, right: 0.4, duration: 0.15)
  end
end
```

## Example: Complete Input Setup

```ruby
include GMR

def init
  # Define all input mappings upfront
  input do |i|
    i.move_left [:a, :left]
    i.move_right [:d, :right]
    i.move_up [:w, :up]
    i.move_down [:s, :down]
    i.jump [:space]
    i.attack :z, mouse: :left
    i.interact :e
    i.pause :escape
  end

  # Define menu context
  input_context :menu do |i|
    i.confirm [:enter, :space]
    i.cancel :escape
    i.nav_up [:up, :w]
    i.nav_down [:down, :s]
  end

  # Register callbacks for discrete actions
  Input.on(:pause) { toggle_pause }

  @player = Player.new(400, 300)
  @paused = false
end

def update(dt)
  return if @paused

  # Continuous movement
  vx, vy = 0, 0
  vx -= 1 if Input.action_down?(:move_left)
  vx += 1 if Input.action_down?(:move_right)
  vy -= 1 if Input.action_down?(:move_up)
  vy += 1 if Input.action_down?(:move_down)

  @player.move(vx, vy, dt)

  # Discrete actions
  @player.jump if Input.action_pressed?(:jump)
  @player.attack if Input.action_pressed?(:attack)
  @player.interact if Input.action_pressed?(:interact)
end

def toggle_pause
  @paused = !@paused
  if @paused
    Input.push_context(:menu)
  else
    Input.pop_context
  end
end
```

## Input During Console

When the developer console is open, you typically want to ignore game input:

```ruby
def update(dt)
  return if Console.open?

  # Normal input handling
  @player.update(dt)
end
```

The console captures keyboard input for its own use. Checking `Console.open?` prevents your game from responding to keys meant for the console.

## See Also

- [Game Loop](game-loop.md) - Where to put input handling
- [Scenes](scenes.md) - Input contexts with scene transitions
- [Console](console.md) - Developer console interaction
