# Input Module

Keyboard, mouse input, and action mapping system.

```ruby
include GMR
```

## Action Mapping

Map abstract actions to physical keys for cleaner, rebindable input.

### map

Define an action with one or more keys.

```ruby
Input.map(action, keys)
```

**Parameters:**
- `action` - Action name as symbol
- `keys` - Single key symbol or array of key symbols

**Example:**
```ruby
def init
  # Single key
  Input.map(:jump, :space)

  # Multiple keys (either works)
  Input.map(:move_left, [:left, :a])
  Input.map(:move_right, [:right, :d])
  Input.map(:move_up, [:up, :w])
  Input.map(:move_down, [:down, :s])

  # Action for attack
  Input.map(:attack, [:z, :j])
end
```

### action_down?

Check if any key mapped to an action is held.

```ruby
Input.action_down?(action)
```

**Returns:** `true` if any mapped key is currently held

**Example:**
```ruby
def update(dt)
  dx = 0
  dx -= 1 if Input.action_down?(:move_left)
  dx += 1 if Input.action_down?(:move_right)
  $player[:x] += dx * $player[:speed] * dt
end
```

### action_pressed?

Check if any key mapped to an action was just pressed this frame.

```ruby
Input.action_pressed?(action)
```

**Returns:** `true` if any mapped key was pressed this frame

**Example:**
```ruby
def update(dt)
  if Input.action_pressed?(:jump) && $player[:on_ground]
    $player[:velocity_y] = -JUMP_FORCE
  end
end
```

### action_released?

Check if any key mapped to an action was just released this frame.

```ruby
Input.action_released?(action)
```

**Example:**
```ruby
def update(dt)
  if Input.action_released?(:charge_attack)
    release_charged_attack()
  end
end
```

## Keyboard (Direct)

Direct key access when you need specific keys.

### key_down?

Check if a key is currently held.

```ruby
Input.key_down?(key)
Input.key_down?(keys)  # Array of keys
```

**Parameters:**
- `key` - Key symbol
- `keys` - Array of key symbols (returns true if ANY are held)

**Example:**
```ruby
# Single key
if Input.key_down?(:space)
  $player[:charging] = true
end

# Multiple keys (any)
if Input.key_down?([:left_shift, :right_shift])
  speed *= 2  # Sprint with either shift
end
```

### key_pressed?

Check if a key was just pressed this frame.

```ruby
Input.key_pressed?(key)
Input.key_pressed?(keys)
```

**Example:**
```ruby
if Input.key_pressed?(:escape)
  toggle_pause_menu()
end

# Any of these keys opens inventory
if Input.key_pressed?([:i, :tab])
  open_inventory()
end
```

### key_released?

Check if a key was just released this frame.

```ruby
Input.key_released?(key)
Input.key_released?(keys)
```

## Mouse

### mouse_x, mouse_y

Get current mouse position.

```ruby
x = Input.mouse_x
y = Input.mouse_y
```

**Example:**
```ruby
def update(dt)
  # Aim at mouse cursor
  $crosshair_x = Input.mouse_x
  $crosshair_y = Input.mouse_y
end
```

### mouse_down?

Check if a mouse button is held.

```ruby
Input.mouse_down?(button)
```

**Parameters:**
- `button` - `:left`, `:right`, or `:middle`

**Example:**
```ruby
if Input.mouse_down?(:left)
  fire_weapon()
end
```

### mouse_pressed?

Check if a mouse button was just clicked.

```ruby
Input.mouse_pressed?(button)
```

**Example:**
```ruby
if Input.mouse_pressed?(:left)
  select_unit_at(Input.mouse_x, Input.mouse_y)
end
```

### mouse_released?

Check if a mouse button was just released.

```ruby
Input.mouse_released?(button)
```

## Key Symbols Reference

### Letters
`:a` through `:z`

### Numbers
`:zero`, `:one`, `:two`, `:three`, `:four`, `:five`, `:six`, `:seven`, `:eight`, `:nine`

### Function Keys
`:f1` through `:f12`

### Arrow Keys
`:up`, `:down`, `:left`, `:right`

### Modifiers
`:left_shift`, `:right_shift`, `:left_control`, `:right_control`, `:left_alt`, `:right_alt`

### Special Keys
`:space`, `:enter`, `:escape`, `:tab`, `:backspace`, `:delete`, `:insert`, `:home`, `:end`, `:page_up`, `:page_down`

## Complete Example

```ruby
include GMR

def init
  Window.set_size(800, 600)

  # Set up action mappings
  Input.map(:move_left, [:left, :a])
  Input.map(:move_right, [:right, :d])
  Input.map(:move_up, [:up, :w])
  Input.map(:move_down, [:down, :s])
  Input.map(:fire, :space)
  Input.map(:pause, :escape)

  $player = { x: 400, y: 300, speed: 200 }
  $paused = false
end

def update(dt)
  return if console_open?

  # Toggle pause
  if Input.action_pressed?(:pause)
    $paused = !$paused
  end
  return if $paused

  # Movement using actions
  dx, dy = 0, 0
  dx -= 1 if Input.action_down?(:move_left)
  dx += 1 if Input.action_down?(:move_right)
  dy -= 1 if Input.action_down?(:move_up)
  dy += 1 if Input.action_down?(:move_down)

  # Normalize diagonal
  if dx != 0 && dy != 0
    dx *= 0.707
    dy *= 0.707
  end

  $player[:x] += dx * $player[:speed] * dt
  $player[:y] += dy * $player[:speed] * dt

  # Fire with keyboard or mouse
  if Input.action_pressed?(:fire) || Input.mouse_pressed?(:left)
    spawn_bullet($player[:x], $player[:y])
  end
end

def draw
  Graphics.clear([30, 30, 40])
  Graphics.draw_rect($player[:x] - 16, $player[:y] - 16, 32, 32, [100, 200, 255])

  if $paused
    Graphics.draw_text("PAUSED", 350, 280, 32, [255, 255, 255])
  end
end
```

## Tips

1. **Use action mapping** - Makes controls rebindable and code cleaner
2. **Check `console_open?`** - Skip game input when developer console is open
3. **`pressed?` vs `down?`** - Use `pressed?` for single actions (jump), `down?` for continuous (movement)
4. **Array syntax** - Accept multiple keys with `[:key1, :key2]` for accessibility

## See Also

- [Window](window.md) - Focus and fullscreen
- [API Overview](README.md) - Key symbol reference
