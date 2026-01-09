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
  - [#layer](#layer)
  - [#layer=](#layer)
  - [#play_animation](#play_animation)
  - [#source_rect](#source_rect)
  - [#source_rect=](#source_rect)
  - [#texture](#texture)
  - [#texture=](#texture)
  - [#transform](#transform)
  - [#transform=](#transform)
  - [#width](#width)
  - [#z](#z)
  - [#z=](#z)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new sprite with a texture and transform.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `texture` | `Texture` | The texture to render |
| `transform` | `Transform2D` | The transform defining position, rotation, scale, and origin |

**Returns:** `Sprite` - The new sprite

**Example:**

```ruby

  transform = Transform2D.new(x: 100, y: 100, rotation: 45)
  sprite = Sprite.new(my_texture, transform)
```

---

<a id="transform"></a>

### #transform

Get the Transform2D handle associated with this sprite.

**Returns:** `Transform2D` - The sprite's transform

**Example:**

```ruby
t = sprite.transform
  t.x = 100
```

---

<a id="transform"></a>

### #transform=

Set the Transform2D for this sprite.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Transform2D` | The transform to use |

**Returns:** `Transform2D` - The value that was set

**Example:**

```ruby
sprite.transform = Transform2D.new(x: 100, y: 200)
```

---

<a id="center_origin"></a>

### #center_origin

Set the transform's origin to the center of the sprite, so it rotates and scales around its center. Uses texture dimensions or source_rect if set.

**Returns:** `Sprite` - self for chaining

**Example:**

```ruby
sprite = Sprite.new(tex, transform).center_origin  # Method chaining
```

---

<a id="layer"></a>

### #layer

Get the render layer. Layers control broad draw order categories. Lower values render first (background), higher values render last (foreground/UI). Returns a symbol for known layer values, or an integer for custom layers.

**Returns:** `Symbol, Integer` - The layer as a symbol (:background, :world, :entities, :effects, :ui, :debug) or integer

**Example:**

```ruby
sprite.layer           # => :entities (default)
```

---

<a id="layer"></a>

### #layer=

Set the render layer. Layers organize rendering into broad categories. Sprites in lower layers render first (appear behind), higher layers render last (appear in front). Within a layer, use z for fine-grained depth control.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Symbol, Integer` | Layer as symbol (:background, :world, :entities, :effects, :ui, :debug) or integer (0-255) |

**Returns:** `Symbol, Integer` - The value that was set

**Example:**

```ruby
# Layer organization
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
