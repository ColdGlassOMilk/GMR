[GMR Docs](../../../../README.md) > [Engine](../../README.md) > [Graphics](README.md) > **Font**

# Graphics::Font

Custom font loading for text rendering. Supports TTF and OTF font files.

## Class Methods

### Font.load(path, size:)

Load a font from a TTF/OTF file.

**Parameters:**
- `path` (String) - Path to the font file (relative to assets folder)
- `size:` (Integer, optional) - Font size in pixels. Default: 32

**Returns:** Font object

**Raises:** RuntimeError if the file cannot be loaded

**Example:**
```ruby
@font = Graphics::Font.load("fonts/pixel.ttf", size: 24)
@large_font = Graphics::Font.load("fonts/title.ttf", size: 64)
```

**Notes:**
- Fonts are cached by path + size combination
- Loading the same font at the same size returns the cached instance
- Different sizes create separate font instances

---

## Instance Methods

### base_size

Get the base size this font was loaded at.

**Returns:** Integer - Base font size in pixels

**Example:**
```ruby
@font = Graphics::Font.load("fonts/pixel.ttf", size: 24)
puts @font.base_size  # => 24
```

---

### glyph_count

Get the number of glyphs (characters) available in this font.

**Returns:** Integer - Number of glyphs

**Example:**
```ruby
@font = Graphics::Font.load("fonts/pixel.ttf", size: 24)
puts @font.glyph_count  # => 95 (default ASCII set)
```

---

### release

Manually release this font reference. Decrements the internal reference count.

**Returns:** nil

**Example:**
```ruby
@font = Graphics::Font.load("fonts/pixel.ttf", size: 24)
# ... use the font ...
@font.release  # Decrement reference count
```

**Notes:**
- Fonts are reference-counted and shared
- Only unloaded when all references are released
- Generally not needed; fonts persist for the game lifetime

---

### ref_count

Get the current reference count for this font.

**Returns:** Integer - Number of active references

**Example:**
```ruby
font1 = Graphics::Font.load("fonts/pixel.ttf", size: 24)
font2 = Graphics::Font.load("fonts/pixel.ttf", size: 24)  # Same font, cached
puts font1.ref_count  # => 2
```

---

## Usage with draw_text

Use the `font:` keyword argument with `Graphics.draw_text`:

```ruby
@custom_font = Graphics::Font.load("fonts/custom.ttf", size: 32)

def draw
  # With custom font
  Graphics.draw_text("Hello!", 100, 100, 32, :white, font: @custom_font)

  # Without font argument uses default raylib font
  Graphics.draw_text("Default", 100, 150, 32, :yellow)
end
```

---

## Complete Example

```ruby
include GMR

def init
  # Load multiple fonts for different purposes
  @title_font = Graphics::Font.load("fonts/fancy.ttf", size: 48)
  @ui_font = Graphics::Font.load("fonts/clean.ttf", size: 16)
  @score = 0
end

def update(dt)
  @score += 1 if Input.key_pressed?(:space)
end

def draw
  Graphics.clear(:dark_gray)

  # Large decorative title
  Graphics.draw_text("MY AWESOME GAME", 200, 50, 48, :gold, font: @title_font)

  # Clean UI text
  Graphics.draw_text("Score: #{@score}", 10, 10, 16, :white, font: @ui_font)
  Graphics.draw_text("Press SPACE to score", 10, 30, 16, :gray, font: @ui_font)

  # FPS counter with default font
  Graphics.draw_text("#{Time.fps} FPS", 10, 560, 12, :lime)
end
```

---

## Supported Formats

- **TTF** - TrueType Font (recommended)
- **OTF** - OpenType Font

---

## See Also

- [Graphics](graphics.md) - Drawing module with draw_text
- [Graphics Guide](../../../../docs/graphics.md) - Complete graphics documentation

---

[Back](README.md) | [Documentation Home](../../../README.md)
