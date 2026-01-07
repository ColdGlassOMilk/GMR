[GMR Docs](../../README.md) > [Types](../types/README.md) > **Rect**

# Rect

Rectangle for bounds and collision areas.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#h](#h)
  - [#initialize](#initialize)
  - [#to_s](#to_s)
  - [#w](#w)
  - [#x](#x)
  - [#y](#y)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new Rect with optional position and dimensions.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Float` | The x position (default: 0) |
| `y` | `Float` | The y position (default: 0) |
| `w` | `Float` | The width (default: 0) |
| `h` | `Float` | The height (default: 0) |

**Returns:** `Rect` - The new rectangle

**Example:**

```ruby
Rect.new(10, 20, 100, 50)   # x=10, y=20, w=100, h=50
```

---

<a id="x"></a>

### #x

Get the x position (left edge).

**Returns:** `Float` - The x position

**Example:**

```ruby
x = rect.x
```

---

<a id="y"></a>

### #y

Get the y position (top edge).

**Returns:** `Float` - The y position

**Example:**

```ruby
y = rect.y
```

---

<a id="w"></a>

### #w

Get the width.

**Returns:** `Float` - The width

**Example:**

```ruby
width = rect.w
```

---

<a id="h"></a>

### #h

Get the height.

**Returns:** `Float` - The height

**Example:**

```ruby
height = rect.h
```

---

<a id="to_s"></a>

### #to_s

Common math utility functions for game development. Provides lerp, clamp, smoothstep, and other interpolation helpers. Named "Mathf" to avoid conflict with Ruby's built-in Math module.

**Returns:** `String` - String in format "Rect(x, y, w, h)"

**Example:**

```ruby
# Smooth transitions
  t = elapsed / duration
  alpha = Mathf.smoothstep(0, 1, t) * 255
```

---

---

[Back to Types](README.md) | [Documentation Home](../../README.md)
