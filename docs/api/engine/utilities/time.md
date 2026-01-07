[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Utilities](../utilities/README.md) > **Time**

# GMR::Time

Frame timing and delta time access.

## Table of Contents

- [Functions](#functions)
  - [delta](#delta)
  - [elapsed](#elapsed)
  - [fps](#fps)
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

**Returns:** `nil`

**Example:**

```ruby
GMR::Time.set_target_fps(60)  # Lock to 60 FPS
```

---

---

[Back to Utilities](README.md) | [Documentation Home](../../../README.md)
