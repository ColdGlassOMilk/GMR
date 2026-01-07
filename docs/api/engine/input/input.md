[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Input](../input/README.md) > **Input**

# GMR::Input

Keyboard, mouse, and gamepad input handling.

## Table of Contents

- [Functions](#functions)
  - [action_down?](#action_down)
  - [action_pressed?](#action_pressed)
  - [action_released?](#action_released)
  - [char_pressed](#char_pressed)
  - [clear_mappings](#clear_mappings)
  - [current_context](#current_context)
  - [has_context?](#has_context)
  - [input](#input)
  - [input_context](#input_context)
  - [key_down?](#key_down)
  - [key_pressed](#key_pressed)
  - [key_pressed?](#key_pressed)
  - [key_released?](#key_released)
  - [map](#map)
  - [mouse_down?](#mouse_down)
  - [mouse_pressed?](#mouse_pressed)
  - [mouse_released?](#mouse_released)
  - [mouse_wheel](#mouse_wheel)
  - [mouse_x](#mouse_x)
  - [mouse_y](#mouse_y)
  - [off](#off)
  - [on](#on)
  - [pop_context](#pop_context)
  - [push_context](#push_context)
  - [set_context](#set_context)
  - [unmap](#unmap)

## Functions

<a id="mouse_x"></a>

### mouse_x

Get the mouse X position in virtual resolution coordinates. Automatically accounts for letterboxing when using virtual resolution.

**Returns:** `Integer` - Mouse X position

**Example:**

```ruby
x = GMR::Input.mouse_x
```

---

<a id="mouse_y"></a>

### mouse_y

Get the mouse Y position in virtual resolution coordinates. Automatically accounts for letterboxing when using virtual resolution.

**Returns:** `Integer` - Mouse Y position

**Example:**

```ruby
y = GMR::Input.mouse_y
```

---

<a id="mouse_down"></a>

### mouse_down?

Check if a mouse button is currently held down.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `button` | `Symbol, Integer` | The button to check (:left, :right, :middle, or constant) |

**Returns:** `Boolean` - true if the button is held

**Example:**

```ruby
if GMR::Input.mouse_down?(:left)
  player.aim
end
```

---

<a id="mouse_pressed"></a>

### mouse_pressed?

Check if a mouse button was just pressed this frame.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `button` | `Symbol, Integer` | The button to check (:left, :right, :middle, or constant) |

**Returns:** `Boolean` - true if the button was just pressed

**Example:**

```ruby
if GMR::Input.mouse_pressed?(:left)
  player.shoot
end
```

---

<a id="mouse_released"></a>

### mouse_released?

Check if a mouse button was just released this frame.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `button` | `Symbol, Integer` | The button to check (:left, :right, :middle, or constant) |

**Returns:** `Boolean` - true if the button was just released

**Example:**

```ruby
if GMR::Input.mouse_released?(:left)
  bow.release_arrow
end
```

---

<a id="mouse_wheel"></a>

### mouse_wheel

Get the mouse wheel movement this frame. Positive values indicate scrolling up/forward, negative values indicate scrolling down/backward.

**Returns:** `Float` - Wheel movement amount

**Example:**

```ruby
zoom += GMR::Input.mouse_wheel * 0.1
```

---

<a id="key_down"></a>

### key_down?

Check if a key is currently held down. Accepts a single key or an array of keys (returns true if any key in the array is held).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `key` | `Symbol, Integer, Array` | The key(s) to check (:space, :a, :left, etc.) |

**Returns:** `Boolean` - true if the key (or any key in array) is held

**Example:**

```ruby
if GMR::Input.key_down?([:a, :left])  # Either key works
  player.move_left
end
```

---

<a id="key_pressed"></a>

### key_pressed?

Check if a key was just pressed this frame. Accepts a single key or an array of keys (returns true if any key in the array was just pressed).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `key` | `Symbol, Integer, Array` | The key(s) to check |

**Returns:** `Boolean` - true if the key (or any key in array) was just pressed

**Example:**

```ruby
if GMR::Input.key_pressed?(:space)
  player.jump
end
```

---

<a id="key_released"></a>

### key_released?

Check if a key was just released this frame. Accepts a single key or an array of keys (returns true if any key in the array was just released).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `key` | `Symbol, Integer, Array` | The key(s) to check |

**Returns:** `Boolean` - true if the key (or any key in array) was just released

**Example:**

```ruby
if GMR::Input.key_released?(:shift)
  player.stop_running
end
```

---

<a id="key_pressed"></a>

### key_pressed

Get the key code of the last key pressed this frame. Useful for text input or detecting any key press.

**Returns:** `Integer, nil` - Key code, or nil if no key was pressed

**Example:**

```ruby
key = GMR::Input.key_pressed
  if key
    puts "Key code: #{key}"
  end
```

---

<a id="char_pressed"></a>

### char_pressed

Get the Unicode character code of the last character pressed this frame. Useful for text input fields. Returns the character, not the key code.

**Returns:** `Integer, nil` - Unicode character code, or nil if no character was pressed

**Example:**

```ruby
char = GMR::Input.char_pressed
  if char
```

---

<a id="map"></a>

### map

Map an action name to input bindings. Supports two forms: Traditional form maps a single action to keys directly. Block form allows defining multiple actions with a DSL.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `action` | `Symbol` | (optional) The action name for traditional form |
| `keys` | `Symbol, Array` | (optional) Key(s) to bind for traditional form |

**Returns:** `nil`

**Example:**

```ruby
# Block DSL form
  GMR::Input.map do |i|
    i.action :jump, key: :space
    i.action :attack, keys: [:z, :x], mouse: :left
  end
```

---

<a id="unmap"></a>

### unmap

Remove an action mapping by name.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `action` | `Symbol` | The action name to remove |

**Returns:** `nil`

**Example:**

```ruby
GMR::Input.unmap(:jump)
```

---

<a id="clear_mappings"></a>

### clear_mappings

Remove all action mappings.

**Returns:** `nil`

**Example:**

```ruby
GMR::Input.clear_mappings
```

---

<a id="action_down"></a>

### action_down?

Check if a mapped action is currently active (any bound input is held).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `action` | `Symbol` | The action name to check |

**Returns:** `Boolean` - true if the action is active

**Example:**

```ruby
if GMR::Input.action_down?(:move_left)
  player.x -= speed
end
```

---

<a id="action_pressed"></a>

### action_pressed?

Check if a mapped action was just triggered this frame.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `action` | `Symbol` | The action name to check |

**Returns:** `Boolean` - true if the action was just triggered

**Example:**

```ruby
if GMR::Input.action_pressed?(:jump)
  player.jump
end
```

---

<a id="action_released"></a>

### action_released?

Check if a mapped action was just released this frame.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `action` | `Symbol` | The action name to check |

**Returns:** `Boolean` - true if the action was just released

**Example:**

```ruby
if GMR::Input.action_released?(:charge_attack)
  player.release_charge
end
```

---

<a id="on"></a>

### on

Register a callback for an action. The callback fires when the action reaches the specified phase. Returns an ID for later removal with `off`.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `action` | `Symbol` | The action name to listen for |
| `when` | `Symbol` | (optional, default: :pressed) Phase: :pressed, :down, or :released |
| `context` | `Object` | (optional) Object to use as self in the callback block |

**Returns:** `Integer` - Callback ID for later removal

**Example:**

```ruby
# With phase and context
  id = GMR::Input.on(:attack, when: :released, context: player) do
    release_charge_attack
  end
  GMR::Input.off(id)  # Remove later
```

---

<a id="off"></a>

### off

Remove input callback(s). Pass an ID to remove a specific callback, or an action name to remove all callbacks for that action.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `id_or_action` | `Integer, Symbol` | Callback ID or action name |

**Returns:** `nil`

**Example:**

```ruby
GMR::Input.off(:jump)        # Remove all :jump callbacks
```

---

<a id="push_context"></a>

### push_context

Push a named input context onto the stack. Actions defined in this context become active. Previous contexts remain on the stack.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `name` | `Symbol` | The context name to push |

**Returns:** `nil`

**Example:**

```ruby
GMR::Input.push_context(:menu)
  # :menu actions are now active, game actions still on stack
```

---

<a id="pop_context"></a>

### pop_context

Pop the current input context from the stack, returning to the previous context.

**Returns:** `nil`

**Example:**

```ruby
GMR::Input.pop_context  # Return to previous context
```

---

<a id="set_context"></a>

### set_context

Replace the entire context stack with a single context. Clears the stack and sets the named context as the only active context.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `name` | `Symbol` | The context name to set |

**Returns:** `nil`

**Example:**

```ruby
GMR::Input.set_context(:gameplay)
```

---

<a id="current_context"></a>

### current_context

Get the name of the current active input context.

**Returns:** `Symbol, nil` - Current context name, or nil if no context is active

**Example:**

```ruby
context = GMR::Input.current_context
```

---

<a id="has_context"></a>

### has_context?

Check if a named input context exists (has been defined).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `name` | `Symbol` | The context name to check |

**Returns:** `Boolean` - true if the context exists

**Example:**

```ruby
if GMR::Input.has_context?(:menu)
  GMR::Input.push_context(:menu)
end
```

---

<a id="input"></a>

### input

Define global input actions using a verb-style DSL. Actions defined here are always available regardless of context.

**Returns:** `nil`

**Example:**

```ruby
input do |i|
  i.jump :space
  i.move_left [:a, :left]
  i.attack :z, mouse: :left
end
```

---

<a id="input_context"></a>

### input_context

Define input actions for a named context. Context-specific actions are only active when that context is pushed onto the stack.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `name` | `Symbol` | The context name |

**Returns:** `nil`

**Example:**

```ruby
input_context :menu do |i|
  i.confirm :enter
  i.cancel :escape
  i.navigate_up :up
  i.navigate_down :down
end
```

---

---

[Back to Input](README.md) | [Documentation Home](../../../README.md)
