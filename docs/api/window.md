# Window Module

Window management, display settings, and virtual resolution.

```ruby
include GMR
```

## Window Properties

### set_title

Set the window title.

```ruby
Window.set_title(title)
```

**Parameters:**
- `title` - Window title string

**Example:**
```ruby
def init
  Window.set_title("My Awesome Game")
end
```

### set_size

Set the window dimensions.

```ruby
Window.set_size(width, height)
```

**Parameters:**
- `width`, `height` - Window size in pixels

**Example:**
```ruby
def init
  Window.set_size(1280, 720)
end
```

### width, height

Get current window dimensions.

```ruby
w = Window.width
h = Window.height
```

**Example:**
```ruby
def draw
  # Center something on screen
  center_x = Window.width / 2
  center_y = Window.height / 2
  Graphics.draw_circle(center_x, center_y, 50, [255, 255, 255])
end
```

## Fullscreen

### set_fullscreen

Enable or disable fullscreen mode.

```ruby
Window.set_fullscreen(enabled)
```

**Parameters:**
- `enabled` - `true` for fullscreen, `false` for windowed

**Example:**
```ruby
def init
  Input.map(:toggle_fullscreen, :f11)
end

def update(dt)
  if Input.action_pressed?(:toggle_fullscreen)
    $fullscreen = !$fullscreen
    Window.set_fullscreen($fullscreen)
  end
end
```

### fullscreen?

Check if window is fullscreen.

```ruby
Window.fullscreen?
```

**Returns:** `true` if fullscreen

## Virtual Resolution

Render at a fixed resolution and scale to window size. Great for pixel art games.

### set_virtual_size

Set the virtual rendering resolution.

```ruby
Window.set_virtual_size(width, height)
```

**Parameters:**
- `width`, `height` - Virtual resolution in pixels

When set, all drawing happens at this resolution and is scaled to fit the window.

**Example:**
```ruby
def init
  # Pixel art game at 320x240
  Window.set_size(960, 720)          # Window is 3x size
  Window.set_virtual_size(320, 240)  # Render at low res
end

def draw
  # Draw as if screen is 320x240
  # Everything is automatically scaled up
  Graphics.draw_rect(0, 0, 320, 240, [30, 30, 40])
  Graphics.draw_rect(100, 100, 16, 16, [255, 0, 0])
end
```

### virtual_width, virtual_height

Get the virtual resolution (or actual resolution if not set).

```ruby
vw = Window.virtual_width
vh = Window.virtual_height
```

**Example:**
```ruby
def draw
  # Works whether virtual resolution is set or not
  center_x = Window.virtual_width / 2
  center_y = Window.virtual_height / 2
end
```

## Complete Example

```ruby
include GMR

def init
  Window.set_title("Window Demo")
  Window.set_size(800, 600)

  # Optional: pixel art mode
  # Window.set_virtual_size(400, 300)

  Input.map(:fullscreen, :f11)
  Input.map(:quit, :escape)

  $fullscreen = false
end

def update(dt)
  return if console_open?

  if Input.action_pressed?(:fullscreen)
    $fullscreen = !$fullscreen
    Window.set_fullscreen($fullscreen)
  end

  if Input.action_pressed?(:quit)
    System.quit
  end
end

def draw
  Graphics.clear([40, 40, 50])

  # Display window info
  info = [
    "Window: #{Window.width}x#{Window.height}",
    "Virtual: #{Window.virtual_width}x#{Window.virtual_height}",
    "Fullscreen: #{Window.fullscreen?}",
    "",
    "F11: Toggle fullscreen",
    "ESC: Quit"
  ]

  info.each_with_index do |line, i|
    Graphics.draw_text(line, 20, 20 + i * 25, 20, [255, 255, 255])
  end
end
```

## Tips

1. **Set size in `init`** - Configure window before game loop starts
2. **Virtual resolution** - Use for consistent pixel-perfect rendering
3. **Aspect ratio** - Virtual resolution maintains aspect ratio, letterboxing if needed
4. **Fullscreen toggle** - Let players switch with F11 or similar

## See Also

- [Graphics](graphics.md) - Drawing functions
- [Input](input.md) - Input handling
- [System](system.md) - Quit function
