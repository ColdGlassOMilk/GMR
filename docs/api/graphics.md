# GMR::Graphics

Drawing primitives, textures, and rendering.

## Functions

### clear(color)

Clear the screen with a solid color

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| color | Color | The background color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.clear([20, 20, 40])
```

---

### draw_rect(x, y, w, h, color)

Draw a filled rectangle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Integer | X position (left edge) |
| y | Integer | Y position (top edge) |
| w | Integer | Width in pixels |
| h | Integer | Height in pixels |
| color | Color | Fill color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_rect(100, 100, 50, 30, [255, 0, 0])
```

---

### draw_rect_outline(x, y, w, h, color)

Draw a rectangle outline (not filled)

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Integer | X position (left edge) |
| y | Integer | Y position (top edge) |
| w | Integer | Width in pixels |
| h | Integer | Height in pixels |
| color | Color | Outline color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_rect_outline(100, 100, 50, 30, [255, 255, 255])
```

---

### draw_rect_rotated(x, y, w, h, angle, color)

Draw a filled rectangle rotated around its center

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Float | X position (center) |
| y | Float | Y position (center) |
| w | Float | Width in pixels |
| h | Float | Height in pixels |
| angle | Float | Rotation angle in degrees |
| color | Color | Fill color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_rect_rotated(160, 120, 40, 20, 45.0, [0, 255, 0])
```

---

### draw_line(x1, y1, x2, y2, color)

Draw a line between two points

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x1 | Integer | Start X position |
| y1 | Integer | Start Y position |
| x2 | Integer | End X position |
| y2 | Integer | End Y position |
| color | Color | Line color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_line(0, 0, 100, 100, [255, 255, 255])
```

---

### draw_line_thick(x1, y1, x2, y2, thickness, color)

Draw a thick line between two points

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x1 | Float | Start X position |
| y1 | Float | Start Y position |
| x2 | Float | End X position |
| y2 | Float | End Y position |
| thickness | Float | Line thickness in pixels |
| color | Color | Line color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_line_thick(0, 0, 100, 100, 3.0, [255, 200, 100])
```

---

### draw_circle(x, y, radius, color)

Draw a filled circle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Integer | Center X position |
| y | Integer | Center Y position |
| radius | Integer | Circle radius in pixels |
| color | Color | Fill color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_circle(160, 120, 25, [100, 200, 255])
```

---

### draw_circle_outline(x, y, radius, color)

Draw a circle outline

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Integer | Center X position |
| y | Integer | Center Y position |
| radius | Integer | Circle radius in pixels |
| color | Color | Outline color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_circle_outline(160, 120, 25, [255, 255, 255])
```

---

### draw_circle_gradient(x, y, radius, inner_color, outer_color)

Draw a circle with a radial gradient from inner to outer color

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Integer | Center X position |
| y | Integer | Center Y position |
| radius | Integer | Circle radius in pixels |
| inner_color | Color | Color at center |
| outer_color | Color | Color at edge |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_circle_gradient(160, 120, 50, [255, 255, 255], [255, 0, 0, 0])
```

---

### draw_triangle(x1, y1, x2, y2, x3, y3, color)

Draw a filled triangle

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x1 | Float | First vertex X |
| y1 | Float | First vertex Y |
| x2 | Float | Second vertex X |
| y2 | Float | Second vertex Y |
| x3 | Float | Third vertex X |
| y3 | Float | Third vertex Y |
| color | Color | Fill color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_triangle(100, 50, 50, 150, 150, 150, [255, 0, 0])
```

---

### draw_triangle_outline(x1, y1, x2, y2, x3, y3, color)

Draw a triangle outline

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x1 | Float | First vertex X |
| y1 | Float | First vertex Y |
| x2 | Float | Second vertex X |
| y2 | Float | Second vertex Y |
| x3 | Float | Third vertex X |
| y3 | Float | Third vertex Y |
| color | Color | Outline color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_triangle_outline(100, 50, 50, 150, 150, 150, [255, 255, 255])
```

---

### draw_text(text, x, y, size, color)

Draw text at a position

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| text | String | The text to draw |
| x | Integer | X position (left edge) |
| y | Integer | Y position (top edge) |
| size | Integer | Font size in pixels |
| color | Color | Text color |

**Returns:** `nil`

**Example:**
```ruby
GMR::Graphics.draw_text("Hello!", 10, 10, 20, [255, 255, 255])
```

---

### measure_text(text, size)

A loaded image texture for drawing sprites and images

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| text | String | The text to measure |
| size | Integer | Font size in pixels |

**Returns:** `Integer` - Width in pixels

**Example:**
```ruby
width = GMR::Graphics.measure_text("Hello", 20)
```

---

## Texture

TODO: Add description

### Class Methods

### load

Load a texture from a file. Supports PNG, JPG, BMP, and other common formats.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| path | String | Path to the image file (relative to game root) |

**Returns:** `Texture` - The loaded texture object

**Raises:**
- RuntimeError if the file cannot be loaded

**Example:**
```ruby
sprite = GMR::Graphics::Texture.load("assets/player.png")
```

---

### Instance Methods

### width

Get the texture width in pixels

**Returns:** `Integer` - Width in pixels

**Example:**
```ruby
puts sprite.width
```

---

### height

Get the texture height in pixels

**Returns:** `Integer` - Height in pixels

**Example:**
```ruby
puts sprite.height
```

---

### draw

Draw the texture at a position, optionally with a color tint

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Integer | X position (left edge) |
| y | Integer | Y position (top edge) |
| color | Color | (optional, default: [255, 255, 255]) Color tint (multiplied with texture) |

**Returns:** `nil`

**Example:**
```ruby
sprite.draw(100, 100)
```

---

### draw_ex

Draw the texture with rotation and scaling

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| x | Float | X position |
| y | Float | Y position |
| rotation | Float | Rotation angle in degrees |
| scale | Float | Scale multiplier (1.0 = original size) |
| color | Color | (optional, default: [255, 255, 255]) Color tint |

**Returns:** `nil`

**Example:**
```ruby
sprite.draw_ex(160, 120, 45.0, 2.0)
```

---

### draw_pro

A tile-based map for efficient rendering of large worlds using a tileset texture

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| sx | Float | Source X (top-left of region) |
| sy | Float | Source Y (top-left of region) |
| sw | Float | Source width |
| sh | Float | Source height |
| dx | Float | Destination X (center) |
| dy | Float | Destination Y (center) |
| dw | Float | Destination width |
| dh | Float | Destination height |
| rotation | Float | Rotation angle in degrees |
| color | Color | (optional, default: [255, 255, 255]) Color tint |

**Returns:** `nil`

**Example:**
```ruby
sprite.draw_pro(0, 0, 32, 32, 160, 120, 64, 64, 0)
```

---

## Tilemap

TODO: Add description

### Class Methods

### new(tileset:, tile_width:, tile_height:, width:, height:)

Create a new tilemap with the specified dimensions. All tiles are initialized to -1 (empty/transparent).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| tileset | Texture | The tileset texture containing all tile graphics |
| tile_width | Integer | Width of each tile in pixels |
| tile_height | Integer | Height of each tile in pixels |
| width | Integer | Map width in tiles |
| height | Integer | Map height in tiles |

**Returns:** `Tilemap` - The new tilemap object

**Raises:**
- ArgumentError if dimensions are not positive

**Example:**
```ruby
tileset = GMR::Graphics::Texture.load("assets/tiles.png")
map = GMR::Graphics::Tilemap.new(tileset, 16, 16, 100, 50)
```

---

### Instance Methods

### tile_width

TODO: Add documentation

**Returns:** `unknown`

---

### tile_height

TODO: Add documentation

**Returns:** `unknown`

---

### get(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### set(arg1, arg2, arg3)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |
| arg3 | Integer |  |

**Returns:** `unknown`

---

### fill(arg1)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |

**Returns:** `unknown`

---

### fill_rect(arg1, arg2, arg3, arg4, arg5)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |
| arg3 | Integer |  |
| arg4 | Integer |  |
| arg5 | Integer |  |

**Returns:** `unknown`

---

### draw_region(arg1, arg2, arg3, arg4, arg5, arg6, [arg7])

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |
| arg3 | Integer |  |
| arg4 | Integer |  |
| arg5 | Integer |  |
| arg6 | Integer |  |
| arg7 | Array (optional) |  |

**Returns:** `unknown`

---

### define_tile(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Hash |  |

**Returns:** `unknown`

---

### tile_properties(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### tile_property(arg1, arg2, arg3)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |
| arg3 | Symbol |  |

**Returns:** `unknown`

---

### solid?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### wall?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | any |  |
| arg2 | any |  |

**Returns:** `unknown`

---

### hazard?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### platform?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### ladder?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### water?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### slippery?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

### damage(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| arg1 | Integer |  |
| arg2 | Integer |  |

**Returns:** `unknown`

---

