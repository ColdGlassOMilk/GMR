[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Utilities](../utilities/README.md) > **Time**

# GMR::Time

Frame timing and delta time access.

## Table of Contents

- [Functions](#functions)
  - [delta](#delta)
  - [elapsed](#elapsed)
  - [fps](#fps)
  - [scale](#scale)
  - [scale=](#scale)
  - [set_target_fps](#set_target_fps)

## Functions

<a id="delta"></a>

### delta

Get the time elapsed since the last frame in seconds. Use this for frame-independent movement and animation.

**Returns:** `Float` - Delta time in seconds

**Example:**

```ruby
# Move at 100 pixels per second regardless of frame rate
  player.x += 100 * GMR::Time.delta
```

---

<a id="elapsed"></a>

### elapsed

Get the total time elapsed since the game started in seconds.

**Returns:** `Float` - Total elapsed time in seconds

**Example:**

```ruby
# Flash effect every 0.5 seconds
  visible = (GMR::Time.elapsed % 1.0) < 0.5
```

---

<a id="fps"></a>

### fps

Get the current frames per second.

**Returns:** `Integer` - Current FPS

**Example:**

```ruby
puts "FPS: #{GMR::Time.fps}"
```

---

<a id="set_target_fps"></a>

### set_target_fps

Set the target frame rate. The game will try to maintain this FPS. Set to 0 for unlimited frame rate.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `fps` | `Integer` | Target frames per second |

**Returns:** `Module` - self for chaining

**Example:**

```ruby
GMR::Time.set_target_fps(60)  # Lock to 60 FPS
```

---

<a id="scale"></a>

### scale

Get the current time scale. Affects how fast game time passes. 1.0 = normal speed, 0.5 = half speed (slow motion), 0.0 = paused.

**Returns:** `Float` - Current time scale

**Example:**

```ruby
current_scale = GMR::Time.scale
```

---

<a id="scale"></a>

### scale=

Set the time scale. Affects how fast game time passes. Timers created with `scaled: true` (default) will respect this. Set to 0.0 to pause, 0.5 for slow motion, 2.0 for fast forward.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `scale` | `Float` | The new time scale (must be >= 0) |

**Returns:** `Float` - The new time scale

**Example:**

```ruby
GMR::Time.scale = 1.0  # Normal speed
```

---

---

[Back to Utilities](README.md) | [Documentation Home](../../../README.md)
