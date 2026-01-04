# Transform2D

2D transform with position, rotation, scale, and hierarchy.

## Transform2D

TODO: Add description

### Instance Methods

### initialize

Create a new Transform2D with optional initial values.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Float | Initial X position (default: 0) |
| y | Float | Initial Y position (default: 0) |
| rotation | Float | Initial rotation in degrees (default: 0) |
| scale_x | Float | Initial X scale (default: 1.0) |
| scale_y | Float | Initial Y scale (default: 1.0) |
| origin_x | Float | Pivot point X (default: 0) |
| origin_y | Float | Pivot point Y (default: 0) |

**Returns:** `Transform2D` - The new transform

**Example:**
```ruby
t = Transform2D.new(x: 100, y: 50, rotation: 45)
```

---

### x

Get the X position of the transform.

**Returns:** `Float` - The X position

**Example:**
```ruby
x_pos = transform.x
```

---

### y

Get the Y position of the transform.

**Returns:** `Float` - The Y position

**Example:**
```ruby
y_pos = transform.y
```

---

### position

Get the position as a Vec2.

**Returns:** `Vec2` - The position vector

**Example:**
```ruby
pos = transform.position
```

---

### rotation

Get the rotation in degrees.

**Returns:** `Float` - The rotation angle in degrees

**Example:**
```ruby
angle = transform.rotation
```

---

### scale_x

Get the X scale factor.

**Returns:** `Float` - The X scale (1.0 = normal size)

**Example:**
```ruby
sx = transform.scale_x
```

---

### scale_y

Get the Y scale factor.

**Returns:** `Float` - The Y scale (1.0 = normal size)

**Example:**
```ruby
sy = transform.scale_y
```

---

### origin_x

Get the X origin (pivot point) for rotation and scaling.

**Returns:** `Float` - The X origin offset in pixels

**Example:**
```ruby
ox = transform.origin_x
```

---

### origin_y

Get the Y origin (pivot point) for rotation and scaling.

**Returns:** `Float` - The Y origin offset in pixels

**Example:**
```ruby
oy = transform.origin_y
```

---

### x=

Set the X position of the transform.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The new X position |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
transform.x += 5  # Move right by 5 pixels
```

---

### y=

Set the Y position of the transform.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The new Y position |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
transform.y += 10  # Move down by 10 pixels
```

---

### position=

Set the position using a Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Vec2 | The new position vector |

**Returns:** `Vec2` - The value that was set

**Example:**
```ruby
transform.position = player.position  # Copy another position
```

---

### rotation=

Set the rotation in degrees. Positive values rotate clockwise.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The new rotation angle in degrees |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
transform.rotation += 90 * dt  # Rotate 90 degrees per second
```

---

### scale_x=

Set the X scale factor. Values greater than 1 stretch horizontally, less than 1 shrink. Negative values flip horizontally.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The new X scale (1.0 = normal size) |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
transform.scale_x = -1.0  # Flip horizontally
```

---

### scale_y=

Set the Y scale factor. Values greater than 1 stretch vertically, less than 1 shrink. Negative values flip vertically.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The new Y scale (1.0 = normal size) |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
transform.scale_y = -1.0  # Flip vertically
```

---

### origin_x=

Set the X origin (pivot point) for rotation and scaling. The origin is the point around which the transform rotates and scales.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The X origin offset in pixels from top-left |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
transform.origin_x = 16  # Pivot 16px from left edge
```

---

### origin_y=

Set the Y origin (pivot point) for rotation and scaling. The origin is the point around which the transform rotates and scales.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The Y origin offset in pixels from top-left |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
transform.origin_y = 16  # Pivot 16px from top edge
```

---

### parent

Get the parent transform. Returns nil if no parent is set. When a transform has a parent, its position, rotation, and scale are relative to the parent's world transform.

**Returns:** `Transform2D, nil` - The parent transform, or nil if none

**Example:**
```ruby
if transform.parent
  puts "Has a parent!"
end
```

---

### parent=

Set the parent transform for hierarchical transformations. When parented, this transform's position, rotation, and scale become relative to the parent. Set to nil to remove the parent.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Transform2D, nil | The parent transform, or nil to clear |

**Returns:** `Transform2D, nil` - The value that was set

**Example:**
```ruby
transform.parent = nil  # Remove parent
```

---

### world_position

Get the final world position after applying all parent transforms. For transforms without a parent, this equals the local position. For parented transforms, this returns the actual screen position after parent transformations.

**Returns:** `Vec2` - The world position after parent hierarchy composition

**Example:**
```ruby
# Get world position of a child transform
  parent = Transform2D.new(x: 100, y: 100)
  parent.rotation = 90
  child = Transform2D.new(x: 50, y: 0)
  child.parent = parent
  pos = child.world_position  # Position after rotation by parent
```

---

