[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Animation](../animation/README.md) > **Tween**

# Tween

Value interpolation over time with easing.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#active?](#active)
  - [#cancel](#cancel)
  - [#cancel_all](#cancel_all)
  - [#complete?](#complete)
  - [#count](#count)
  - [#from](#from)
  - [#on_complete](#on_complete)
  - [#on_update](#on_update)
  - [#pause](#pause)
  - [#paused?](#paused)
  - [#progress](#progress)
  - [#resume](#resume)
  - [#to](#to)

## Instance Methods

<a id="to"></a>

### from(target, property, start_value, duration:, delay:, ease:)

Create a tween that animates a property TO a target value. The tween starts from the property's current value.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `target` | `Object` | The object to animate (must have property getter and setter) |
| `property` | `Symbol` | The property to animate (:x, :y, :alpha, :rotation, etc.) |
| `end_value` | `Float` | The target value to animate to |

**Returns:** `Tween` - The tween instance for chaining

**Example:**

```ruby
# Smooth camera zoom for scope/aim mode
  def toggle_aim_mode
```

---

<a id="from"></a>

### #from

Create a tween that animates a property FROM a start value to current. Useful for "animate in" effects like fading in from transparent.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `target` | `Object` | The object to animate |
| `property` | `Symbol` | The property to animate |
| `start_value` | `Float` | The value to animate from |

**Returns:** `Tween` - The tween instance for chaining

**Example:**

```ruby
# Toast notification slides in from bottom
  class Toast
    def show(message)
```

---

<a id="on_complete"></a>

### #on_complete

Set a callback to invoke when the tween completes.

**Returns:** `Tween` - self for chaining

**Example:**

```ruby
# Sequence of tweens using on_complete for chaining
  class Chest
    def open
      # First: lid opens
      GMR::Tween.to(@lid, :rotation, -1.5, duration: 0.3, ease: :out_quad)
        .on_complete do
          # Second: spawn item and make it rise
```

---

<a id="on_update"></a>

### #on_update

Set a callback to invoke each frame during the tween. The callback receives (t, value) where t is normalized progress [0-1] and value is the current interpolated value.

**Returns:** `Tween` - self for chaining

**Example:**

```ruby
# Color cycling effect on update
  def start_rainbow_effect
```

---

<a id="cancel"></a>

### #cancel

Cancel the tween immediately. Does not invoke on_complete.

**Returns:** `nil`

**Example:**

```ruby
# Cancel movement when player hits wall
  class Player
    def move_to(target_x, target_y)
```

---

<a id="pause"></a>

### #pause

Pause the tween. Use resume to continue.

**Returns:** `Tween` - self for chaining

**Example:**

```ruby
# Pause all UI animations when game pauses
  class PauseMenu
    def show
```

---

<a id="resume"></a>

### #resume

Resume a paused tween.

**Returns:** `Tween` - self for chaining

**Example:**

```ruby
tween.resume
```

---

<a id="complete"></a>

### #complete?

Check if the tween has finished.

**Returns:** `Boolean` - true if completed

**Example:**

```ruby
if tween.complete?
  puts "Done!"
end
```

---

<a id="active"></a>

### #active?

Check if the tween is currently running (not paused, cancelled, or complete).

**Returns:** `Boolean` - true if active

**Example:**

```ruby
if tween.active?
  puts "Still animating..."
end
```

---

<a id="paused"></a>

### #paused?

Check if the tween is paused.

**Returns:** `Boolean` - true if paused

---

<a id="progress"></a>

### #progress

Get the current progress of the tween (0.0 to 1.0).

**Returns:** `Float` - Progress value

---

<a id="cancel_all"></a>

### #cancel_all

Cancel all active tweens.

**Returns:** `nil`

**Example:**

```ruby
# Clean slate when transitioning scenes
  class GameScene < GMR::Scene
    def unload
      # Cancel all tweens to prevent callbacks on destroyed objects
      GMR::Tween.cancel_all
      # Clean up resources
```

---

<a id="count"></a>

### #count

Get the number of active tweens.

**Returns:** `Integer` - Number of active tweens

**Example:**

```ruby
puts "Active tweens: #{GMR::Tween.count}"
```

---

---

[Back to Animation](README.md) | [Documentation Home](../../../README.md)
