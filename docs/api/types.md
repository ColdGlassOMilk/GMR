# Vec2

2D vector for positions and directions.

## Vec2

TODO: Add description

### Instance Methods

### initialize

Create a new Vec2 with optional x and y values.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Float | The x component (default: 0) |
| y | Float | The y component (default: 0) |

**Returns:** `Vec2` - The new vector

**Example:**
```ruby
Vec2.new(100, 200)    # (100, 200)
```

---

### x

Get the x component.

**Returns:** `Float` - The x value

**Example:**
```ruby
x = vec.x
```

---

### y

Get the y component.

**Returns:** `Float` - The y value

**Example:**
```ruby
y = vec.y
```

---

### +

Add two vectors, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| other | Vec2 | The vector to add |

**Returns:** `Vec2` - A new vector with the sum

**Example:**
```ruby
result = Vec2.new(1, 2) + Vec2.new(3, 4)  # Vec2(4, 6)
```

---

### -

Subtract two vectors, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| other | Vec2 | The vector to subtract |

**Returns:** `Vec2` - A new vector with the difference

**Example:**
```ruby
result = Vec2.new(5, 5) - Vec2.new(2, 1)  # Vec2(3, 4)
```

---

### *

Multiply vector by a scalar, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scalar | Float | The scalar value |

**Returns:** `Vec2` - A new scaled vector

**Example:**
```ruby
result = Vec2.new(2, 3) * 2.0  # Vec2(4, 6)
```

---

### /

Divide vector by a scalar, returning a new Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scalar | Float | The scalar value (must not be zero) |

**Returns:** `Vec2` - A new scaled vector

**Example:**
```ruby
result = Vec2.new(10, 20) / 2.0  # Vec2(5, 10)
```

---

### to_s

A 3D vector with x, y, and z components. Used for 3D positions, colors (RGB), and other 3-component values. Supports arithmetic operations.

**Returns:** `String` - String in format "Vec2(x, y)"

**Example:**
```ruby
v3 = v * 0.5                 # Scale
```

---

## Vec3

TODO: Add description

### Instance Methods

### initialize

Create a new Vec3 with optional x, y, and z values.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Float | The x component (default: 0) |
| y | Float | The y component (default: 0) |
| z | Float | The z component (default: 0) |

**Returns:** `Vec3` - The new vector

**Example:**
```ruby
Vec3.new(1, 2, 3)     # (1, 2, 3)
```

---

### x

Get the x component.

**Returns:** `Float` - The x value

**Example:**
```ruby
x = vec.x
```

---

### y

Get the y component.

**Returns:** `Float` - The y value

**Example:**
```ruby
y = vec.y
```

---

### z

Get the z component.

**Returns:** `Float` - The z value

**Example:**
```ruby
z = vec.z
```

---

### +

Add two vectors, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| other | Vec3 | The vector to add |

**Returns:** `Vec3` - A new vector with the sum

**Example:**
```ruby
result = Vec3.new(1, 2, 3) + Vec3.new(1, 1, 1)  # Vec3(2, 3, 4)
```

---

### -

Subtract two vectors, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| other | Vec3 | The vector to subtract |

**Returns:** `Vec3` - A new vector with the difference

**Example:**
```ruby
result = Vec3.new(5, 5, 5) - Vec3.new(1, 2, 3)  # Vec3(4, 3, 2)
```

---

### *

Multiply vector by a scalar, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scalar | Float | The scalar value |

**Returns:** `Vec3` - A new scaled vector

**Example:**
```ruby
result = Vec3.new(1, 2, 3) * 2.0  # Vec3(2, 4, 6)
```

---

### /

Divide vector by a scalar, returning a new Vec3.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scalar | Float | The scalar value (must not be zero) |

**Returns:** `Vec3` - A new scaled vector

**Example:**
```ruby
result = Vec3.new(10, 20, 30) / 10.0  # Vec3(1, 2, 3)
```

---

### to_s

A rectangle with position (x, y) and dimensions (w, h). Used for bounds, source rectangles, collision areas, and UI layout.

**Returns:** `String` - String in format "Vec3(x, y, z)"

**Example:**
```ruby
puts rect.x, rect.y, rect.w, rect.h
```

---

## Rect

TODO: Add description

### Instance Methods

### initialize

Create a new Rect with optional position and dimensions.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Float | The x position (default: 0) |
| y | Float | The y position (default: 0) |
| w | Float | The width (default: 0) |
| h | Float | The height (default: 0) |

**Returns:** `Rect` - The new rectangle

**Example:**
```ruby
Rect.new(10, 20, 100, 50)   # x=10, y=20, w=100, h=50
```

---

### x

Get the x position (left edge).

**Returns:** `Float` - The x position

**Example:**
```ruby
x = rect.x
```

---

### y

Get the y position (top edge).

**Returns:** `Float` - The y position

**Example:**
```ruby
y = rect.y
```

---

### w

Get the width.

**Returns:** `Float` - The width

**Example:**
```ruby
width = rect.w
```

---

### h

Get the height.

**Returns:** `Float` - The height

**Example:**
```ruby
height = rect.h
```

---

### to_s

Convert to a string representation.

**Returns:** `String` - String in format "Rect(x, y, w, h)"

**Example:**
```ruby
puts Rect.new(0, 0, 32, 32).to_s  # "Rect(0.00, 0.00, 32.00, 32.00)"
```

---

