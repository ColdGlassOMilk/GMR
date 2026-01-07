[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Utilities](../utilities/README.md) > **Window**

# GMR::Window

Window management and display settings.

## Table of Contents

- [Functions](#functions)
  - [actual_height](#actual_height)
  - [actual_width](#actual_width)
  - [clear_virtual_resolution](#clear_virtual_resolution)
  - [fullscreen=](#fullscreen)
  - [fullscreen?](#fullscreen)
  - [height](#height)
  - [monitor_count](#monitor_count)
  - [monitor_height](#monitor_height)
  - [monitor_name](#monitor_name)
  - [monitor_refresh_rate](#monitor_refresh_rate)
  - [monitor_width](#monitor_width)
  - [set_filter_bilinear](#set_filter_bilinear)
  - [set_filter_point](#set_filter_point)
  - [set_size](#set_size)
  - [set_title](#set_title)
  - [set_virtual_resolution](#set_virtual_resolution)
  - [toggle_fullscreen](#toggle_fullscreen)
  - [virtual_resolution?](#virtual_resolution)
  - [width](#width)

## Functions

<a id="width"></a>

### width

Get the logical width of the game screen. Returns virtual resolution width if set, otherwise the actual window width.

**Returns:** `Integer` - Screen width in pixels

**Example:**

```ruby
screen_w = GMR::Window.width
```

---

<a id="height"></a>

### height

Get the logical height of the game screen. Returns virtual resolution height if set, otherwise the actual window height.

**Returns:** `Integer` - Screen height in pixels

**Example:**

```ruby
screen_h = GMR::Window.height
```

---

<a id="actual_width"></a>

### actual_width

Get the actual window width in pixels, ignoring virtual resolution.

**Returns:** `Integer` - Actual window width

**Example:**

```ruby
real_w = GMR::Window.actual_width
```

---

<a id="actual_height"></a>

### actual_height

Get the actual window height in pixels, ignoring virtual resolution.

**Returns:** `Integer` - Actual window height

**Example:**

```ruby
real_h = GMR::Window.actual_height
```

---

<a id="set_size"></a>

### set_size

Set the window size. Has no effect in fullscreen mode.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `w` | `Integer` | Window width in pixels |
| `h` | `Integer` | Window height in pixels |

**Returns:** `nil`

**Example:**

```ruby
GMR::Window.set_size(1280, 720)
```

---

<a id="set_title"></a>

### set_title

Set the window title bar text.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `title` | `String` | The window title |

**Returns:** `nil`

**Example:**

```ruby
GMR::Window.set_title("My Awesome Game")
```

---

<a id="toggle_fullscreen"></a>

### toggle_fullscreen

Toggle between fullscreen and windowed mode.

**Returns:** `Boolean` - true

**Example:**

```ruby
GMR::Window.toggle_fullscreen
```

---

<a id="fullscreen"></a>

### fullscreen=

Set fullscreen mode on or off.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `fullscreen` | `Boolean` | true for fullscreen, false for windowed |

**Returns:** `Boolean` - The fullscreen state that was set

**Example:**

```ruby
GMR::Window.fullscreen = true
```

---

<a id="fullscreen"></a>

### fullscreen?

Check if the window is currently in fullscreen mode.

**Returns:** `Boolean` - true if fullscreen

**Example:**

```ruby
if GMR::Window.fullscreen?
  show_windowed_mode_button
end
```

---

<a id="set_virtual_resolution"></a>

### set_virtual_resolution

Set a virtual resolution for pixel-perfect rendering. The game renders to this resolution and scales to fit the window with letterboxing.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `w` | `Integer` | Virtual width in pixels |
| `h` | `Integer` | Virtual height in pixels |

**Returns:** `Boolean` - true

**Example:**

```ruby
# Render at 320x240 for retro-style game
  GMR::Window.set_virtual_resolution(320, 240)
```

---

<a id="clear_virtual_resolution"></a>

### clear_virtual_resolution

Disable virtual resolution and render directly at window size.

**Returns:** `Boolean` - true

**Example:**

```ruby
GMR::Window.clear_virtual_resolution
```

---

<a id="virtual_resolution"></a>

### virtual_resolution?

Check if virtual resolution is currently enabled.

**Returns:** `Boolean` - true if virtual resolution is active

**Example:**

```ruby
if GMR::Window.virtual_resolution?
  puts "Using virtual resolution"
end
```

---

<a id="set_filter_point"></a>

### set_filter_point

Set nearest-neighbor (point) filtering for virtual resolution scaling. Produces crisp, pixelated look. Only works when virtual resolution is enabled.

**Returns:** `nil`

**Example:**

```ruby
GMR::Window.set_filter_point  # For pixel art games
```

---

<a id="set_filter_bilinear"></a>

### set_filter_bilinear

Set bilinear filtering for virtual resolution scaling. Produces smoother, blended scaling. Only works when virtual resolution is enabled.

**Returns:** `nil`

**Example:**

```ruby
GMR::Window.set_filter_bilinear  # For smoother scaling
```

---

<a id="monitor_count"></a>

### monitor_count

Get the number of connected monitors.

**Returns:** `Integer` - Number of monitors

**Example:**

```ruby
count = GMR::Window.monitor_count
```

---

<a id="monitor_width"></a>

### monitor_width

Get the width of a specific monitor.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `index` | `Integer` | Monitor index (0-based) |

**Returns:** `Integer` - Monitor width in pixels

**Example:**

```ruby
w = GMR::Window.monitor_width(0)  # Primary monitor
```

---

<a id="monitor_height"></a>

### monitor_height

Get the height of a specific monitor.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `index` | `Integer` | Monitor index (0-based) |

**Returns:** `Integer` - Monitor height in pixels

**Example:**

```ruby
h = GMR::Window.monitor_height(0)
```

---

<a id="monitor_refresh_rate"></a>

### monitor_refresh_rate

Get the refresh rate of a specific monitor.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `index` | `Integer` | Monitor index (0-based) |

**Returns:** `Integer` - Refresh rate in Hz

**Example:**

```ruby
hz = GMR::Window.monitor_refresh_rate(0)
```

---

<a id="monitor_name"></a>

### monitor_name

Time and frame rate utilities. Provides delta time for frame-independent movement, elapsed time tracking, and FPS management.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `index` | `Integer` | Monitor index (0-based) |

**Returns:** `String` - Monitor name

**Example:**

```ruby
# Debug overlay with FPS and timing info
  class DebugOverlay
    def draw
      y = 10
      color = [0, 255, 0]

      # Show current FPS
      fps = GMR::Time.fps
      fps_color = fps < 30 ? [255, 0, 0] : fps < 55 ? [255, 255, 0] : [0, 255, 0]
      GMR::Graphics.draw_text("FPS: #{fps}", 10, y, 16, fps_color)
      y += 20

      # Show frame time
      frame_ms = (GMR::Time.delta * 1000).round(2)
      GMR::Graphics.draw_text("Frame: #{frame_ms}ms", 10, y, 16, color)
      y += 20

      # Show total elapsed time
      elapsed = GMR::Time.elapsed
      minutes = (elapsed / 60).to_i
      seconds = (elapsed % 60).to_i
      GMR::Graphics.draw_text("Time: #{minutes}:#{seconds.to_s.rjust(2, '0')}", 10, y, 16, color)
    end
  end
```

---

---

[Back to Utilities](README.md) | [Documentation Home](../../../README.md)
