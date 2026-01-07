[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Graphics](../graphics/README.md) > **Sprite**

# Sprite

Drawable 2D sprite with built-in transform properties.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#alpha](#alpha)
  - [#alpha=](#alpha)
  - [#center_origin](#center_origin)
  - [#color](#color)
  - [#color=](#color)
  - [#count](#count)
  - [#draw](#draw)
  - [#flip_x](#flip_x)
  - [#flip_x=](#flip_x)
  - [#flip_y](#flip_y)
  - [#flip_y=](#flip_y)
  - [#height](#height)
  - [#initialize](#initialize)
  - [#origin](#origin)
  - [#origin=](#origin)
  - [#origin_x](#origin_x)
  - [#origin_x=](#origin_x)
  - [#origin_y](#origin_y)
  - [#origin_y=](#origin_y)
  - [#parent](#parent)
  - [#parent=](#parent)
  - [#play_animation](#play_animation)
  - [#position](#position)
  - [#position=](#position)
  - [#rotation](#rotation)
  - [#rotation=](#rotation)
  - [#scale_x](#scale_x)
  - [#scale_x=](#scale_x)
  - [#scale_y](#scale_y)
  - [#scale_y=](#scale_y)
  - [#source_rect](#source_rect)
  - [#source_rect=](#source_rect)
  - [#texture](#texture)
  - [#texture=](#texture)
  - [#width](#width)
  - [#x](#x)
  - [#x=](#x)
  - [#y](#y)
  - [#y=](#y)
  - [#z](#z)
  - [#z=](#z)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new Sprite from a texture with optional initial values.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `texture` | `Texture` | The texture to use for this sprite |
| `x` | `Float` | Initial X position (default: 0) |
| `y` | `Float` | Initial Y position (default: 0) |
| `rotation` | `Float` | Initial rotation in degrees (default: 0) |
| `scale_x` | `Float` | Initial X scale (default: 1.0) |
| `scale_y` | `Float` | Initial Y scale (default: 1.0) |
| `z` | `Float` | Explicit z-index for layering (default: nil, uses draw order) |
| `source_rect` | `Rect` | Region of texture to draw (default: entire texture) |

**Returns:** `Sprite` - The new sprite

**Example:**

```ruby
sprite = Sprite.new(spritesheet, source_rect: Rect.new(0, 0, 32, 32))
```

---

<a id="x"></a>

### #x

Get the X position of the sprite.

**Returns:** `Float` - The X position

**Example:**

```ruby
x_pos = sprite.x
```

---

<a id="y"></a>

### #y

Get the Y position of the sprite.

**Returns:** `Float` - The Y position

**Example:**

```ruby
y_pos = sprite.y
```

---

<a id="x"></a>

### #x=

Set the X position of the sprite.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The new X position |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.x += 5  # Move right by 5 pixels
```

---

<a id="y"></a>

### #y=

Set the Y position of the sprite.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The new Y position |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.y += 10  # Move down by 10 pixels
```

---

<a id="position"></a>

### #position

Get the position as a Vec2.

**Returns:** `Vec2` - The position vector

**Example:**

```ruby
pos = sprite.position
```

---

<a id="position"></a>

### #position=

Set the position using a Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | The new position vector |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
sprite.position = Vec2.new(100, 200)
```

---

<a id="rotation"></a>

### #rotation

Get the rotation in degrees.

**Returns:** `Float` - The rotation angle in degrees

**Example:**

```ruby
angle = sprite.rotation
```

---

<a id="rotation"></a>

### #rotation=

Set the rotation in degrees. Positive values rotate clockwise.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The new rotation angle in degrees |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.rotation += 90 * dt  # Rotate 90 degrees per second
```

---

<a id="scale_x"></a>

### #scale_x

Get the X scale factor.

**Returns:** `Float` - The X scale (1.0 = normal size)

**Example:**

```ruby
sx = sprite.scale_x
```

---

<a id="scale_y"></a>

### #scale_y

Get the Y scale factor.

**Returns:** `Float` - The Y scale (1.0 = normal size)

**Example:**

```ruby
sy = sprite.scale_y
```

---

<a id="scale_x"></a>

### #scale_x=

Set the X scale factor. Values greater than 1 stretch, less than 1 shrink.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The new X scale (1.0 = normal size) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.scale_x = 2.0  # Double width
```

---

<a id="scale_y"></a>

### #scale_y=

Set the Y scale factor. Values greater than 1 stretch, less than 1 shrink.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The new Y scale (1.0 = normal size) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.scale_y = 0.5  # Half height
```

---

<a id="origin_x"></a>

### #origin_x

Get the X origin (pivot point) for rotation and scaling.

**Returns:** `Float` - The X origin offset in pixels

**Example:**

```ruby
ox = sprite.origin_x
```

---

<a id="origin_y"></a>

### #origin_y

Get the Y origin (pivot point) for rotation and scaling.

**Returns:** `Float` - The Y origin offset in pixels

**Example:**

```ruby
oy = sprite.origin_y
```

---

<a id="origin_x"></a>

### #origin_x=

Set the X origin (pivot point) for rotation and scaling.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The X origin offset in pixels |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.origin_x = 16  # Pivot 16px from left
```

---

<a id="origin_y"></a>

### #origin_y=

Set the Y origin (pivot point) for rotation and scaling.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The Y origin offset in pixels |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.origin_y = 16  # Pivot 16px from top
```

---

<a id="origin"></a>

### #origin

Get the origin (pivot point) as a Vec2.

**Returns:** `Vec2` - The origin vector

**Example:**

```ruby
origin = sprite.origin
```

---

<a id="origin"></a>

### #origin=

Set the origin (pivot point) using a Vec2.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | The new origin vector |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
sprite.origin = Vec2.new(16, 16)
```

---

<a id="center_origin"></a>

### #center_origin

Set the origin to the center of the sprite, so it rotates and scales around its center. Uses texture dimensions or source_rect if set.

**Returns:** `Sprite` - self for chaining

**Example:**

```ruby
sprite = Sprite.new(tex).center_origin  # Method chaining
```

---

<a id="z"></a>

### #z

Get the explicit z-index for layering. Returns nil if using automatic draw order (the default). Higher z values render on top of lower values.

**Returns:** `Float, nil` - The z-index, or nil if using draw order

**Example:**

```ruby
if sprite.z.nil?
  puts "Using automatic draw order"
end
```

---

<a id="z"></a>

### #z=

Set an explicit z-index for layering, or nil to use draw order. By default (nil), sprites are layered by draw order - later drawn sprites appear on top. Setting an explicit z gives you precise control over layering.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float, nil` | The z-index (higher = on top), or nil for draw order |

**Returns:** `Float, nil` - The value that was set

**Example:**

```ruby
# Typical usage pattern
```

---

<a id="color"></a>

### #color

Get the color tint as an RGBA array. White [255,255,255,255] means no tint.

**Returns:** `Array<Integer>` - RGBA color array [r, g, b, a]

**Example:**

```ruby
r, g, b, a = sprite.color
```

---

<a id="color"></a>

### #color=

Set the color tint as an RGBA array. The sprite is multiplied by this color.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Array<Integer>` | RGBA color array [r, g, b] or [r, g, b, a] |

**Returns:** `Array<Integer>` - The value that was set

**Example:**

```ruby
sprite.color = [255, 255, 255, 128]  # 50% transparent
```

---

<a id="alpha"></a>

### #alpha

Get the alpha (opacity) as a float from 0.0 (invisible) to 1.0 (opaque).

**Returns:** `Float` - The alpha value

**Example:**

```ruby
a = sprite.alpha
```

---

<a id="alpha"></a>

### #alpha=

Set the alpha (opacity) from 0.0 (invisible) to 1.0 (opaque).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The new alpha value (0.0 to 1.0) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
sprite.alpha = 0    # Invisible
```

---

<a id="flip_x"></a>

### #flip_x

Check if the sprite is flipped horizontally.

**Returns:** `Boolean` - true if flipped horizontally

**Example:**

```ruby
if sprite.flip_x
  puts "Facing left"
end
```

---

<a id="flip_y"></a>

### #flip_y

Check if the sprite is flipped vertically.

**Returns:** `Boolean` - true if flipped vertically

**Example:**

```ruby
if sprite.flip_y
  puts "Flipped upside down"
end
```

---

<a id="flip_x"></a>

### #flip_x=

Set whether the sprite is flipped horizontally. Useful for facing direction.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Boolean` | true to flip horizontally |

**Returns:** `Boolean` - The value that was set

**Example:**

```ruby
sprite.flip_x = velocity.x < 0  # Face movement direction
```

---

<a id="flip_y"></a>

### #flip_y=

Set whether the sprite is flipped vertically.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Boolean` | true to flip vertically |

**Returns:** `Boolean` - The value that was set

**Example:**

```ruby
sprite.flip_y = true
```

---

<a id="texture"></a>

### #texture

Get the current texture.

**Returns:** `Texture` - The sprite's texture

**Example:**

```ruby
tex = sprite.texture
```

---

<a id="texture"></a>

### #texture=

Set the texture to draw.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Texture` | The new texture |

**Returns:** `Texture` - The value that was set

**Example:**

```ruby
sprite.texture = new_texture
```

---

<a id="source_rect"></a>

### #source_rect

Get the source rectangle (region of texture to draw). Returns nil if using the entire texture.

**Returns:** `Rect, nil` - The source rectangle, or nil if using full texture

**Example:**

```ruby
rect = sprite.source_rect
```

---

<a id="source_rect"></a>

### #source_rect=

Set the source rectangle to draw only part of the texture. Useful for sprite sheets and animations. Set to nil to draw the full texture.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Rect, nil` | The source rectangle, or nil for full texture |

**Returns:** `Rect, nil` - The value that was set

**Example:**

```ruby
@sprite.source_rect = nil  # Use full texture
```

---

<a id="width"></a>

### #width

Get the width of the sprite (from source_rect or texture).

**Returns:** `Integer` - The width in pixels

**Example:**

```ruby
w = sprite.width
```

---

<a id="height"></a>

### #height

Get the height of the sprite (from source_rect or texture).

**Returns:** `Integer` - The height in pixels

**Example:**

```ruby
h = sprite.height
```

---

<a id="parent"></a>

### #parent

Get the parent Transform2D. Returns nil if no parent is set.

**Returns:** `Transform2D, nil` - The parent transform, or nil if none

**Example:**

```ruby
parent = sprite.parent
```

---

<a id="parent"></a>

### #parent=

Set a Transform2D as the parent. The sprite will transform relative to the parent's world transform. Set to nil to remove the parent.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Transform2D, nil` | The parent transform, or nil to clear |

**Returns:** `Transform2D, nil` - The value that was set

**Example:**

```ruby
# Sprite follows a transform
  turret_base = Transform2D.new(x: 200, y: 200)
```

---

<a id="draw"></a>

### #draw

Queue the sprite for rendering. Sprites are drawn in z-order after all draw() calls complete. By default, draw order determines layering (later = on top). Set sprite.z to override with an explicit z-index.

**Returns:** `Sprite` - self for chaining

**Example:**

```ruby
# Method chaining
```

---

<a id="count"></a>

### #count

Get the total number of active sprites (class method).

**Returns:** `Integer` - The number of active sprites

**Example:**

```ruby
puts "Active sprites: #{Sprite.count}"
```

---

<a id="play_animation"></a>

### #play_animation

Convenience method to create and play a sprite animation. Creates a GMR::SpriteAnimation, calls play, and returns it for chaining.

**Returns:** `SpriteAnimation` - The animation instance (already playing)

**Example:**

```ruby
sprite.play_animation(frames: [0, 1, 2, 3], fps: 12)
```

---

---

[Back to Graphics](README.md) | [Documentation Home](../../../README.md)
