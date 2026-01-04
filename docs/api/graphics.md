# Graphics Module

Drawing primitives, shapes, and text rendering.

```ruby
include GMR
```

## Screen Management

### clear

Clear the screen with a color.

```ruby
Graphics.clear(color)
```

**Parameters:**
- `color` - RGB or RGBA array

**Example:**
```ruby
def draw
  Graphics.clear([30, 30, 40])  # Dark blue-gray
end
```

## Rectangles

### draw_rect

Draw a filled rectangle.

```ruby
Graphics.draw_rect(x, y, width, height, color)
```

**Parameters:**
- `x`, `y` - Top-left corner position
- `width`, `height` - Rectangle dimensions
- `color` - RGB or RGBA array

**Example:**
```ruby
# Solid red square
Graphics.draw_rect(100, 100, 50, 50, [255, 0, 0])

# Semi-transparent blue
Graphics.draw_rect(200, 100, 50, 50, [0, 0, 255, 128])
```

### draw_rect_lines

Draw a rectangle outline.

```ruby
Graphics.draw_rect_lines(x, y, width, height, color)
```

**Example:**
```ruby
# White outline
Graphics.draw_rect_lines(100, 100, 50, 50, [255, 255, 255])
```

## Circles

### draw_circle

Draw a filled circle.

```ruby
Graphics.draw_circle(center_x, center_y, radius, color)
```

**Parameters:**
- `center_x`, `center_y` - Center position
- `radius` - Circle radius in pixels
- `color` - RGB or RGBA array

**Example:**
```ruby
# Yellow sun
Graphics.draw_circle(400, 100, 40, [255, 220, 50])
```

### draw_circle_lines

Draw a circle outline.

```ruby
Graphics.draw_circle_lines(center_x, center_y, radius, color)
```

**Example:**
```ruby
# Target ring
Graphics.draw_circle_lines(400, 300, 50, [255, 0, 0])
```

## Lines

### draw_line

Draw a line between two points.

```ruby
Graphics.draw_line(x1, y1, x2, y2, color)
```

**Parameters:**
- `x1`, `y1` - Start point
- `x2`, `y2` - End point
- `color` - RGB or RGBA array

**Example:**
```ruby
# Diagonal line
Graphics.draw_line(0, 0, 800, 600, [255, 255, 255])
```

### draw_line_ex

Draw a line with custom thickness.

```ruby
Graphics.draw_line_ex(x1, y1, x2, y2, thickness, color)
```

**Parameters:**
- `x1`, `y1` - Start point
- `x2`, `y2` - End point
- `thickness` - Line width in pixels
- `color` - RGB or RGBA array

**Example:**
```ruby
# Thick red line
Graphics.draw_line_ex(100, 100, 300, 200, 5, [255, 0, 0])
```

## Triangles

### draw_triangle

Draw a filled triangle.

```ruby
Graphics.draw_triangle(x1, y1, x2, y2, x3, y3, color)
```

**Parameters:**
- `x1`, `y1` - First vertex
- `x2`, `y2` - Second vertex
- `x3`, `y3` - Third vertex
- `color` - RGB or RGBA array

**Example:**
```ruby
# Green triangle
Graphics.draw_triangle(400, 100, 350, 200, 450, 200, [0, 255, 0])
```

### draw_triangle_lines

Draw a triangle outline.

```ruby
Graphics.draw_triangle_lines(x1, y1, x2, y2, x3, y3, color)
```

## Text

### draw_text

Draw text at a position.

```ruby
Graphics.draw_text(text, x, y, size, color)
```

**Parameters:**
- `text` - String to display
- `x`, `y` - Position (top-left of text)
- `size` - Font size in pixels
- `color` - RGB or RGBA array

**Example:**
```ruby
Graphics.draw_text("Score: #{$score}", 10, 10, 24, [255, 255, 255])
Graphics.draw_text("Game Over", 300, 250, 48, [255, 0, 0])
```

### measure_text

Get the width of text in pixels.

```ruby
width = Graphics.measure_text(text, size)
```

**Parameters:**
- `text` - String to measure
- `size` - Font size in pixels

**Returns:** Width in pixels (Integer)

**Example:**
```ruby
# Center text horizontally
text = "Hello World"
size = 32
width = Graphics.measure_text(text, size)
x = (800 - width) / 2
Graphics.draw_text(text, x, 100, size, [255, 255, 255])
```

## Drawing Order

Shapes are drawn in the order you call them. Later calls draw on top of earlier calls:

```ruby
def draw
  Graphics.clear([0, 0, 0])

  # Background layer
  Graphics.draw_rect(0, 0, 800, 600, [50, 50, 50])

  # Middle layer
  Graphics.draw_circle(400, 300, 100, [100, 100, 100])

  # Foreground (drawn on top)
  Graphics.draw_text("Hello", 350, 290, 24, [255, 255, 255])
end
```

## See Also

- [Texture](texture.md) - Image rendering
- [Tilemap](tilemap.md) - Tile-based maps
- [API Overview](README.md) - Color and coordinate conventions
