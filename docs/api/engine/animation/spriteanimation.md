[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Animation](../animation/README.md) > **SpriteAnimation**

# SpriteAnimation

Frame-based sprite animations.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#complete?](#complete)
  - [#count](#count)
  - [#fps](#fps)
  - [#fps=](#fps)
  - [#frame](#frame)
  - [#frame=](#frame)
  - [#initialize](#initialize)
  - [#loop=](#loop)
  - [#loop?](#loop)
  - [#on_complete](#on_complete)
  - [#on_frame_change](#on_frame_change)
  - [#pause](#pause)
  - [#play](#play)
  - [#playing?](#playing)
  - [#stop](#stop)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new sprite animation.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `sprite` | `Sprite` | The sprite to animate |

**Example:**

```ruby
attack = GMR::SpriteAnimation.new(sprite, frames: 4..7, fps: 15, loop: false)
```

---

<a id="play"></a>

### #play

Start or resume the animation.

**Returns:** `SpriteAnimation` - self for chaining

**Example:**

```ruby
anim.play
```

---

<a id="pause"></a>

### #pause

Pause the animation at the current frame.

**Returns:** `SpriteAnimation` - self for chaining

**Example:**

```ruby
anim.pause
```

---

<a id="stop"></a>

### #stop

Stop the animation and reset to the first frame.

**Returns:** `SpriteAnimation` - self for chaining

**Example:**

```ruby
anim.stop
```

---

<a id="on_complete"></a>

### #on_complete

Set a callback for when the animation finishes (non-looping only).

**Returns:** `SpriteAnimation` - self for chaining

**Example:**

```ruby
anim.on_complete { puts "Done!" }
```

---

<a id="on_frame_change"></a>

### #on_frame_change

Set a callback for each frame change. Receives frame index.

**Returns:** `SpriteAnimation` - self for chaining

**Example:**

```ruby
anim.on_frame_change { |frame| puts "Frame: #{frame}" }
```

---

<a id="playing"></a>

### #playing?

Check if the animation is currently playing.

**Returns:** `Boolean` - true if playing

---

<a id="complete"></a>

### #complete?

Check if the animation has completed (non-looping only).

**Returns:** `Boolean` - true if completed

---

<a id="frame"></a>

### #frame

Get the current frame index (from the frames array).

**Returns:** `Integer` - Current frame index

---

<a id="frame"></a>

### #frame=

Set the current frame index directly.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `index` | `Integer` | Frame index (into frames array) |

**Returns:** `Integer` - The frame index

---

<a id="fps"></a>

### #fps

Get the frames per second.

**Returns:** `Float` - FPS

---

<a id="fps"></a>

### #fps=

Set the frames per second.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | New FPS |

**Returns:** `Float` - The FPS value

---

<a id="loop"></a>

### #loop?

Check if the animation loops.

**Returns:** `Boolean` - true if looping

---

<a id="loop"></a>

### #loop=

Set whether the animation loops.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Boolean` | true to loop |

**Returns:** `Boolean` - The loop value

---

<a id="count"></a>

### #count

Get the number of active sprite animations.

**Returns:** `Integer` - Number of active animations

---

---

[Back to Animation](README.md) | [Documentation Home](../../../README.md)
