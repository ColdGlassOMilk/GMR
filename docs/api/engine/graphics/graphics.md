[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Graphics](../graphics/README.md) > **Graphics**

# GMR::Graphics

Drawing primitives, textures, and rendering.

## Table of Contents

- [Functions](#functions)
  - [clear](#clear)
  - [draw_circle](#draw_circle)
  - [draw_circle_gradient](#draw_circle_gradient)
  - [draw_circle_outline](#draw_circle_outline)
  - [draw_line](#draw_line)
  - [draw_line_thick](#draw_line_thick)
  - [draw_rect](#draw_rect)
  - [draw_rect_outline](#draw_rect_outline)
  - [draw_rect_rotated](#draw_rect_rotated)
  - [draw_text](#draw_text)
  - [draw_triangle](#draw_triangle)
  - [draw_triangle_outline](#draw_triangle_outline)
  - [measure_text](#measure_text)
  - [rgb](#rgb)

## Functions

<a id="rgb"></a>

### rgb

Create a color array from RGB(A) values. Convenience helper.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `r` | `Integer` | Red component (0-255) |
| `g` | `Integer` | Green component (0-255) |
| `b` | `Integer` | Blue component (0-255) |
| `a` | `Integer` | Alpha component (0-255, default: 255) |

**Returns:** `Array<Integer>` - Color array [r, g, b, a]

**Example:**

```ruby
sprite.color = rgb(255, 100, 100)
```

---

<a id="clear"></a>

### clear(color)

Clear the screen with a solid color

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `color` | `Color` | The background color (array, hex string, or named symbol) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.clear(:dark_gray)
```

---

<a id="draw_rect"></a>

### draw_rect(transform, width, height, color)

Draw a filled rectangle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_x` | `Transform2D|Integer` | Transform2D object OR X position (left edge) |
| `width_or_y` | `Float|Integer` | Width in pixels (if transform) OR Y position (top edge) |
| `height_or_w` | `Float|Integer` | Height in pixels (if transform) OR Width in pixels |
| `color_or_h` | `Color|Integer` | Fill color (if transform) OR Height in pixels |
| `color` | `Color` | Fill color (if using x,y,w,h) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_rect(100, 100, 50, 30, [255, 0, 0])  # Legacy x,y,w,h
```

---

<a id="draw_rect_outline"></a>

### draw_rect_outline(transform, width, height, color)

Draw a rectangle outline (not filled)

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_x` | `Transform2D|Integer` | Transform2D object OR X position (left edge) |
| `width_or_y` | `Float|Integer` | Width in pixels (if transform) OR Y position (top edge) |
| `height_or_w` | `Float|Integer` | Height in pixels (if transform) OR Width in pixels |
| `color_or_h` | `Color|Integer` | Outline color (if transform) OR Height in pixels |
| `color` | `Color` | Outline color (if using x,y,w,h) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_rect_outline(100, 100, 50, 30, [255, 255, 255])  # Legacy x,y,w,h
```

---

<a id="draw_rect_rotated"></a>

### draw_rect_rotated(x, y, w, h, angle, color)

Draw a filled rectangle rotated around its center

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Float` | X position (center) |
| `y` | `Float` | Y position (center) |
| `w` | `Float` | Width in pixels |
| `h` | `Float` | Height in pixels |
| `angle` | `Float` | Rotation angle in degrees |
| `color` | `Color` | Fill color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_rect_rotated(160, 120, 40, 20, 45.0, [0, 255, 0])
```

---

<a id="draw_line"></a>

### draw_line(transform, x1, y1, x2, y2, color, thickness=1.0)

Draw a line between two points

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_x1` | `Transform2D|Integer` | Transform2D object OR Start X position |
| `x1_or_y1` | `Float|Integer` | Local X1 (if transform) OR Start Y position |
| `y1_or_x2` | `Float|Integer` | Local Y1 (if transform) OR End X position |
| `x2_or_y2` | `Float|Integer` | Local X2 (if transform) OR End Y position |
| `y2_or_color` | `Float|Color` | Local Y2 (if transform) OR Line color |
| `color_or_thickness` | `Color|Float` | Line color (if transform) OR Optional thickness |
| `thickness` | `Float` | Optional line thickness (default: 1.0, transform API only) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_line(0, 0, 100, 100, :red, 3.0)  # Legacy with thickness
```

---

<a id="draw_line_thick"></a>

### draw_line_thick(x1, y1, x2, y2, thickness, color)

Draw a thick line between two points

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Float` | Start X position |
| `y1` | `Float` | Start Y position |
| `x2` | `Float` | End X position |
| `y2` | `Float` | End Y position |
| `thickness` | `Float` | Line thickness in pixels |
| `color` | `Color` | Line color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_line_thick(0, 0, 100, 100, 3.0, [255, 200, 100])
```

---

<a id="draw_circle"></a>

### draw_circle(transform, radius, color)

Draw a filled circle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_x` | `Transform2D|Integer` | Transform2D object OR Center X position |
| `radius_or_y` | `Float|Integer` | Radius in pixels (if transform) OR Center Y position |
| `color_or_radius` | `Color|Integer` | Fill color (if transform) OR Radius in pixels |
| `color` | `Color` | Fill color (if using x,y,radius) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_circle(160, 120, 25, [100, 200, 255])  # Legacy x,y,radius
```

---

<a id="draw_circle_outline"></a>

### draw_circle_outline(transform, radius, color)

Draw a circle outline

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_x` | `Transform2D|Integer` | Transform2D object OR Center X position |
| `radius_or_y` | `Float|Integer` | Radius in pixels (if transform) OR Center Y position |
| `color_or_radius` | `Color|Integer` | Outline color (if transform) OR Radius in pixels |
| `color` | `Color` | Outline color (if using x,y,radius) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_circle_outline(160, 120, 25, [255, 255, 255])  # Legacy x,y,radius
```

---

<a id="draw_circle_gradient"></a>

### draw_circle_gradient(x, y, radius, inner_color, outer_color)

Draw a circle with a radial gradient from inner to outer color

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Integer` | Center X position |
| `y` | `Integer` | Center Y position |
| `radius` | `Integer` | Circle radius in pixels |
| `inner_color` | `Color` | Color at center |
| `outer_color` | `Color` | Color at edge |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_circle_gradient(160, 120, 50, [255, 255, 255], [255, 0, 0, 0])
```

---

<a id="draw_triangle"></a>

### draw_triangle(transform, x1, y1, x2, y2, x3, y3, color)

Draw a filled triangle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_x1` | `Transform2D|Float` | Transform2D object OR First vertex X |
| `x1_or_y1` | `Float` | Local X1 (if transform) OR First vertex Y |
| `y1_or_x2` | `Float` | Local Y1 (if transform) OR Second vertex X |
| `x2_or_y2` | `Float` | Local X2 (if transform) OR Second vertex Y |
| `y2_or_x3` | `Float` | Local Y2 (if transform) OR Third vertex X |
| `x3_or_y3` | `Float` | Local X3 (if transform) OR Third vertex Y |
| `y3_or_color` | `Float|Color` | Local Y3 (if transform) OR Fill color |
| `color` | `Color` | Fill color (if using transform) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_triangle(100, 50, 50, 150, 150, 150, [255, 0, 0])  # Legacy world coords
```

---

<a id="draw_triangle_outline"></a>

### draw_triangle_outline(transform, x1, y1, x2, y2, x3, y3, color)

Draw a triangle outline

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_x1` | `Transform2D|Float` | Transform2D object OR First vertex X |
| `x1_or_y1` | `Float` | Local X1 (if transform) OR First vertex Y |
| `y1_or_x2` | `Float` | Local Y1 (if transform) OR Second vertex X |
| `x2_or_y2` | `Float` | Local X2 (if transform) OR Second vertex Y |
| `y2_or_x3` | `Float` | Local Y2 (if transform) OR Third vertex X |
| `x3_or_y3` | `Float` | Local X3 (if transform) OR Third vertex Y |
| `y3_or_color` | `Float|Color` | Local Y3 (if transform) OR Outline color |
| `color` | `Color` | Outline color (if using transform) |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_triangle_outline(100, 50, 50, 150, 150, 150, [255, 255, 255])  # Legacy world coords
```

---

<a id="draw_text"></a>

### draw_text(transform, text, size, color, font:)

Draw text at a position, optionally with a custom font.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `transform_or_text` | `Transform2D|String` | Transform2D object OR The text to draw |
| `text_or_x` | `String|Integer` | Text content (if transform) OR X position (left edge) |
| `size_or_y` | `Integer` | Font size (if transform) OR Y position (top edge) |
| `color_or_size` | `Color|Integer` | Text color (if transform) OR Font size in pixels |
| `color` | `Color` | Text color (if using x,y) |
| `font:` | `Font` | (Optional) Custom font loaded via `Graphics::Font.load` |

**Returns:** `nil`

**Example:**

```ruby
# Default font
GMR::Graphics.draw_text("Hello!", 10, 10, 20, [255, 255, 255])

# Custom font
@font = Graphics::Font.load("fonts/pixel.ttf", size: 24)
GMR::Graphics.draw_text("Custom!", 10, 40, 24, :white, font: @font)
```

---

<a id="measure_text"></a>

### measure_text(text, size)

A loaded image texture for drawing sprites and images

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `text` | `String` | The text to measure |
| `size` | `Integer` | Font size in pixels |

**Returns:** `Integer` - Width in pixels

**Example:**

```ruby
width = GMR::Graphics.measure_text("Hello", 20)
```

---

---

[Back to Graphics](README.md) | [Documentation Home](../../../README.md)
