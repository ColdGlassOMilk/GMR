[GMR Docs](../../README.md) > [Types](../types/README.md) > **Vec2**

# Vec2

2D vector for positions and directions.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#*](#*)
  - [#+](#+)
  - [#-](#-)
  - [#/](#/)
  - [#initialize](#initialize)
  - [#to_a](#to_a)
  - [#to_s](#to_s)
  - [#x](#x)
  - [#y](#y)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new Vec2 with optional x and y values.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Float` | The x component (default: 0) |
| `y` | `Float` | The y component (default: 0) |

**Returns:** `Vec2` - The new vector

**Example:**

```ruby
Vec2.new(100, 200)    # (100, 200)
```

---

<a id="x"></a>

### #x

Get the x component.

**Returns:** `Float` - The x value

**Example:**

```ruby
x = vec.x
```

---

<a id="y"></a>

### #y

Get the y component.

**Returns:** `Float` - The y value

**Example:**

```ruby
y = vec.y
```

---

<a id="+"></a>

### #+

Add two vectors, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `other` | `Vec2` | The vector to add |

**Returns:** `Vec2` - A new vector with the sum

**Example:**

```ruby
result = Vec2.new(1, 2) + Vec2.new(3, 4)  # Vec2(4, 6)
```

---

<a id="-"></a>

### #-

Subtract two vectors, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `other` | `Vec2` | The vector to subtract |

**Returns:** `Vec2` - A new vector with the difference

**Example:**

```ruby
result = Vec2.new(5, 5) - Vec2.new(2, 1)  # Vec2(3, 4)
```

---

<a id="*"></a>

### #*

Multiply vector by a scalar, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `scalar` | `Float` | The scalar value |

**Returns:** `Vec2` - A new scaled vector

**Example:**

```ruby
result = Vec2.new(2, 3) * 2.0  # Vec2(4, 6)
```

---

<a id="/"></a>

### #/

Divide vector by a scalar, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `scalar` | `Float` | The scalar value (must not be zero) |

**Returns:** `Vec2` - A new scaled vector

**Example:**

```ruby
result = Vec2.new(10, 20) / 2.0  # Vec2(5, 10)
```

---

<a id="to_s"></a>

### #to_s

Convert to a string representation.

**Returns:** `String` - String in format "Vec2(x, y)"

**Example:**

```ruby
puts Vec2.new(1, 2).to_s  # "Vec2(1.00, 2.00)"
```

---

<a id="to_a"></a>

### #to_a

A 3D vector with x, y, and z components. Used for 3D positions, colors (RGB), and other 3-component values. Supports arithmetic operations.

**Returns:** `Array<Float>` - Array containing [x, y]

**Example:**

```ruby
# 3D position for parallax layers
  class ParallaxLayer
    def initialize(texture, depth)
```

---

---

[Back to Types](README.md) | [Documentation Home](../../README.md)
