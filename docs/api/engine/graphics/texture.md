[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Graphics](../graphics/README.md) > **Texture**

# Texture

Loaded image textures for drawing sprites.

## Table of Contents

- [Class Methods](#class-methods)
  - [.load](#load)
- [Instance Methods](#instance-methods)
  - [#draw](#draw)
  - [#draw_ex](#draw_ex)
  - [#draw_pro](#draw_pro)
  - [#height](#height)
  - [#width](#width)

## Class Methods

<a id="load"></a>

### .load

Load a texture from a file. Supports PNG, JPG, BMP, and other common formats.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `path` | `String` | Path to the image file (relative to game root) |

**Returns:** `Texture` - The loaded texture object

**Raises:**
- RuntimeError if the file cannot be loaded

**Example:**

```ruby
sprite = GMR::Graphics::Texture.load("assets/player.png")
```

---

## Instance Methods

<a id="width"></a>

### #width

Get the texture width in pixels

**Returns:** `Integer` - Width in pixels

**Example:**

```ruby
puts sprite.width
```

---

<a id="height"></a>

### #height

Get the texture height in pixels

**Returns:** `Integer` - Height in pixels

**Example:**

```ruby
puts sprite.height
```

---

<a id="draw"></a>

### #draw

Draw the texture at a position, optionally with a color tint

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Integer` | X position (left edge) |
| `y` | `Integer` | Y position (top edge) |
| `color` | `Color` | (optional, default: [255, 255, 255]) Color tint (multiplied with texture) |

**Returns:** `nil`

**Example:**

```ruby
sprite.draw(100, 100)
```

---

<a id="draw_ex"></a>

### #draw_ex

Draw the texture with rotation and scaling

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Float` | X position |
| `y` | `Float` | Y position |
| `rotation` | `Float` | Rotation angle in degrees |
| `scale` | `Float` | Scale multiplier (1.0 = original size) |
| `color` | `Color` | (optional, default: [255, 255, 255]) Color tint |

**Returns:** `nil`

**Example:**

```ruby
sprite.draw_ex(160, 120, 45.0, 2.0)
```

---

<a id="draw_pro"></a>

### #draw_pro

A tile-based map for efficient rendering of large worlds using a tileset texture

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `sx` | `Float` | Source X (top-left of region) |
| `sy` | `Float` | Source Y (top-left of region) |
| `sw` | `Float` | Source width |
| `sh` | `Float` | Source height |
| `dx` | `Float` | Destination X (center) |
| `dy` | `Float` | Destination Y (center) |
| `dw` | `Float` | Destination width |
| `dh` | `Float` | Destination height |
| `rotation` | `Float` | Rotation angle in degrees |
| `color` | `Color` | (optional, default: [255, 255, 255]) Color tint |

**Returns:** `nil`

**Example:**

```ruby
sprite.draw_pro(0, 0, 32, 32, 160, 120, 64, 64, 0)
```

---

---

[Back to Graphics](README.md) | [Documentation Home](../../../README.md)
