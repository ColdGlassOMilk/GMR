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

## Functions

<a id="clear"></a>

### clear(color)

Clear the screen with a solid color

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `color` | `Color` | The background color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.clear([20, 20, 40])
```

---

<a id="draw_rect"></a>

### draw_rect(x, y, w, h, color)

Draw a filled rectangle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Integer` | X position (left edge) |
| `y` | `Integer` | Y position (top edge) |
| `w` | `Integer` | Width in pixels |
| `h` | `Integer` | Height in pixels |
| `color` | `Color` | Fill color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_rect(100, 100, 50, 30, [255, 0, 0])
```

---

<a id="draw_rect_outline"></a>

### draw_rect_outline(x, y, w, h, color)

Draw a rectangle outline (not filled)

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Integer` | X position (left edge) |
| `y` | `Integer` | Y position (top edge) |
| `w` | `Integer` | Width in pixels |
| `h` | `Integer` | Height in pixels |
| `color` | `Color` | Outline color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_rect_outline(100, 100, 50, 30, [255, 255, 255])
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

### draw_line(x1, y1, x2, y2, color)

Draw a line between two points

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Integer` | Start X position |
| `y1` | `Integer` | Start Y position |
| `x2` | `Integer` | End X position |
| `y2` | `Integer` | End Y position |
| `color` | `Color` | Line color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_line(0, 0, 100, 100, [255, 255, 255])
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

### draw_circle(x, y, radius, color)

Draw a filled circle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Integer` | Center X position |
| `y` | `Integer` | Center Y position |
| `radius` | `Integer` | Circle radius in pixels |
| `color` | `Color` | Fill color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_circle(160, 120, 25, [100, 200, 255])
```

---

<a id="draw_circle_outline"></a>

### draw_circle_outline(x, y, radius, color)

Draw a circle outline

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Integer` | Center X position |
| `y` | `Integer` | Center Y position |
| `radius` | `Integer` | Circle radius in pixels |
| `color` | `Color` | Outline color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_circle_outline(160, 120, 25, [255, 255, 255])
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

### draw_triangle(x1, y1, x2, y2, x3, y3, color)

Draw a filled triangle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Float` | First vertex X |
| `y1` | `Float` | First vertex Y |
| `x2` | `Float` | Second vertex X |
| `y2` | `Float` | Second vertex Y |
| `x3` | `Float` | Third vertex X |
| `y3` | `Float` | Third vertex Y |
| `color` | `Color` | Fill color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_triangle(100, 50, 50, 150, 150, 150, [255, 0, 0])
```

---

<a id="draw_triangle_outline"></a>

### draw_triangle_outline(x1, y1, x2, y2, x3, y3, color)

Draw a triangle outline

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Float` | First vertex X |
| `y1` | `Float` | First vertex Y |
| `x2` | `Float` | Second vertex X |
| `y2` | `Float` | Second vertex Y |
| `x3` | `Float` | Third vertex X |
| `y3` | `Float` | Third vertex Y |
| `color` | `Color` | Outline color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_triangle_outline(100, 50, 50, 150, 150, 150, [255, 255, 255])
```

---

<a id="draw_text"></a>

### draw_text(text, x, y, size, color)

Draw text at a position

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `text` | `String` | The text to draw |
| `x` | `Integer` | X position (left edge) |
| `y` | `Integer` | Y position (top edge) |
| `size` | `Integer` | Font size in pixels |
| `color` | `Color` | Text color |

**Returns:** `nil`

**Example:**

```ruby
GMR::Graphics.draw_text("Hello!", 10, 10, 20, [255, 255, 255])
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
