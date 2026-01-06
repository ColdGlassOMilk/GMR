[GMR Docs](../../README.md) > [Types](../types/README.md) > **Vec3**

# Vec3

3D vector for positions and colors.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#*](#*)
  - [#+](#+)
  - [#-](#-)
  - [#/](#/)
  - [#initialize](#initialize)
  - [#to_s](#to_s)
  - [#x](#x)
  - [#y](#y)
  - [#z](#z)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new Vec3 with optional x, y, and z values.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Float` | The x component (default: 0) |
| `y` | `Float` | The y component (default: 0) |
| `z` | `Float` | The z component (default: 0) |

**Returns:** `Vec3` - The new vector

**Example:**

```ruby
Vec3.new(1, 2, 3)     # (1, 2, 3)
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

<a id="z"></a>

### #z

Get the z component.

**Returns:** `Float` - The z value

**Example:**

```ruby
z = vec.z
```

---

<a id="+"></a>

### #+

Add two vectors, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `other` | `Vec3` | The vector to add |

**Returns:** `Vec3` - A new vector with the sum

**Example:**

```ruby
result = Vec3.new(1, 2, 3) + Vec3.new(1, 1, 1)  # Vec3(2, 3, 4)
```

---

<a id="-"></a>

### #-

Subtract two vectors, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `other` | `Vec3` | The vector to subtract |

**Returns:** `Vec3` - A new vector with the difference

**Example:**

```ruby
result = Vec3.new(5, 5, 5) - Vec3.new(1, 2, 3)  # Vec3(4, 3, 2)
```

---

<a id="*"></a>

### #*

Multiply vector by a scalar, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `scalar` | `Float` | The scalar value |

**Returns:** `Vec3` - A new scaled vector

**Example:**

```ruby
result = Vec3.new(1, 2, 3) * 2.0  # Vec3(2, 4, 6)
```

---

<a id="/"></a>

### #/

Divide vector by a scalar, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `scalar` | `Float` | The scalar value (must not be zero) |

**Returns:** `Vec3` - A new scaled vector

**Example:**

```ruby
result = Vec3.new(10, 20, 30) / 10.0  # Vec3(1, 2, 3)
```

---

<a id="to_s"></a>

### #to_s

A rectangle with position (x, y) and dimensions (w, h). Used for bounds, source rectangles, collision areas, and UI layout.

**Returns:** `String` - String in format "Vec3(x, y, z)"

**Example:**

```ruby
puts rect.x, rect.y, rect.w, rect.h
```

---

---

[Back to Types](README.md) | [Documentation Home](../../README.md)
