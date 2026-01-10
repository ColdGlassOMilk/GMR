[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Utilities](../utilities/README.md) > **Window**

# GMR::Window

Window management, display settings, and virtual resolution control.

## Overview

The Window module provides:
- Window size and title configuration
- Fullscreen toggle
- Virtual resolution for retro-style rendering
- Texture filtering for scaling quality
- Multi-monitor support

## Table of Contents

- [Size and Resolution](#size-and-resolution)
  - [width](#width)
  - [height](#height)
  - [actual_width](#actual_width)
  - [actual_height](#actual_height)
  - [set_size](#set_size)
- [Virtual Resolution](#virtual-resolution)
  - [set_virtual_resolution](#set_virtual_resolution)
  - [clear_virtual_resolution](#clear_virtual_resolution)
  - [virtual_resolution?](#virtual_resolution)
  - [set_filter_point](#set_filter_point)
  - [set_filter_bilinear](#set_filter_bilinear)
- [Display Settings](#display-settings)
  - [set_title](#set_title)
  - [toggle_fullscreen](#toggle_fullscreen)
  - [fullscreen=](#fullscreen)
  - [fullscreen?](#fullscreen-1)
- [Monitor Information](#monitor-information)
  - [monitor_count](#monitor_count)
  - [monitor_width](#monitor_width)
  - [monitor_height](#monitor_height)
  - [monitor_refresh_rate](#monitor_refresh_rate)
  - [monitor_name](#monitor_name)

---

## Size and Resolution

<a id="width"></a>

### width

Get the logical width of the game screen. Returns virtual resolution width if set, otherwise the actual window width.

**Returns:** `Integer` - Screen width in pixels

**Example:**

```ruby
screen_w = GMR::Window.width
# With virtual resolution 320x180: returns 320
# Without virtual resolution: returns actual window width
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
# Always returns the true window size, even with virtual resolution enabled
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

**Returns:** `Module` - self for chaining

**Example:**

```ruby
GMR::Window.set_size(1280, 720).set_title("My Game")
```

---

## Virtual Resolution

Virtual resolution enables **Pixel-Perfect mode** - render at a fixed low resolution and scale up to the window. The render texture is scaled with **letterboxing** (black bars) when aspect ratios don't match.

> **Note:** For games that should fill the entire screen without letterboxing, don't use virtual resolution. Instead, use Resolution-Independent mode by setting `camera.viewport_size` to the actual window size. See [Camera](../../../camera.md) for details.

<a id="set_virtual_resolution"></a>

### set_virtual_resolution

Enable Pixel-Perfect mode. Creates a fixed-size render texture that is scaled to fit the window with letterboxing.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `w` | `Integer` | Virtual width in pixels |
| `h` | `Integer` | Virtual height in pixels |

**Returns:** `Module` - self for chaining

**Behavior:**
- Creates a render texture at the specified size
- All drawing goes to this texture
- Texture is scaled to fit the window, maintaining aspect ratio
- **Black bars appear** if window aspect ratio differs from virtual resolution
- `Window.width` and `Window.height` return the virtual resolution

**Example:**

```ruby
def init
  Window.set_size(960, 540)
  Window.set_virtual_resolution(320, 180)
  Window.set_filter_point  # Crisp pixel scaling

  # Camera viewport MUST match virtual resolution
  @camera = Camera2D.new
  @camera.view_height = 9
  @camera.viewport_size = Vec2.new(320, 180)
  @camera.offset = Vec2.new(160, 90)
end
```

---

<a id="clear_virtual_resolution"></a>

### clear_virtual_resolution

Disable virtual resolution and switch to Resolution-Independent mode. Renders directly at window size with **no letterboxing**.

**Returns:** `Module` - self for chaining

**Behavior:**
- Releases the render texture
- Drawing goes directly to the window
- `Window.width` and `Window.height` return the actual window size
- Camera width adapts to window aspect ratio (no black bars)

**Example:**

```ruby
def toggle_retro_mode
  @retro = !@retro

  if @retro
    Window.set_virtual_resolution(320, 180)
    @camera.viewport_size = Vec2.new(320, 180)
    @camera.offset = Vec2.new(160, 90)
  else
    Window.clear_virtual_resolution
    @camera.viewport_size = Vec2.new(Window.width, Window.height)
    @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
  end
end
```

---

<a id="virtual_resolution"></a>

### virtual_resolution?

Check if virtual resolution is currently enabled.

**Returns:** `Boolean` - true if virtual resolution is active

**Example:**

```ruby
if Window.virtual_resolution?
  puts "Rendering at #{Window.width}x#{Window.height} virtual"
  puts "Actual window: #{Window.actual_width}x#{Window.actual_height}"
end
```

---

<a id="set_filter_point"></a>

### set_filter_point

Set nearest-neighbor (point) filtering for virtual resolution scaling. Produces crisp, pixelated look. Only works when virtual resolution is enabled.

**Returns:** `Module` - self for chaining

**Example:**

```ruby
Window.set_virtual_resolution(320, 180).set_filter_point
# Pixels stay sharp when scaled up
```

---

<a id="set_filter_bilinear"></a>

### set_filter_bilinear

Set bilinear filtering for virtual resolution scaling. Produces smoother, blended scaling. Only works when virtual resolution is enabled.

**Returns:** `Module` - self for chaining

**Example:**

```ruby
Window.set_virtual_resolution(320, 180).set_filter_bilinear
# Smoother but slightly blurry scaling
```

---

## Display Settings

<a id="set_title"></a>

### set_title

Set the window title bar text.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `title` | `String` | The window title |

**Returns:** `Module` - self for chaining

**Example:**

```ruby
Window.set_title("My Awesome Game")
```

---

<a id="toggle_fullscreen"></a>

### toggle_fullscreen

Toggle between fullscreen and windowed mode.

**Returns:** `Boolean` - true

**Example:**

```ruby
# Toggle on F11
if Input.key_pressed?(:f11)
  Window.toggle_fullscreen
end
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
Window.fullscreen = true  # Go fullscreen
Window.fullscreen = false # Return to windowed
```

---

<a id="fullscreen-1"></a>

### fullscreen?

Check if the window is currently in fullscreen mode.

**Returns:** `Boolean` - true if fullscreen

**Example:**

```ruby
if Window.fullscreen?
  puts "Running in fullscreen"
end
```

---

## Monitor Information

<a id="monitor_count"></a>

### monitor_count

Get the number of connected monitors.

**Returns:** `Integer` - Number of monitors

**Example:**

```ruby
count = Window.monitor_count
puts "#{count} monitor(s) detected"
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
w = Window.monitor_width(0)  # Primary monitor width
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
h = Window.monitor_height(0)
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
hz = Window.monitor_refresh_rate(0)
puts "Monitor refresh rate: #{hz}Hz"
```

---

<a id="monitor_name"></a>

### monitor_name

Get the name of a specific monitor.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `index` | `Integer` | Monitor index (0-based) |

**Returns:** `String` - Monitor name

**Example:**

```ruby
name = Window.monitor_name(0)
puts "Primary monitor: #{name}"
```

---

## Virtual Resolution Workflow

Here's a complete example of using virtual resolution with dynamic toggling:

```ruby
include GMR

RETRO_WIDTH = 320
RETRO_HEIGHT = 180
WINDOW_WIDTH = 960
WINDOW_HEIGHT = 540

def init
  Window.set_size(WINDOW_WIDTH, WINDOW_HEIGHT)

  @retro_mode = true
  @camera = Camera.new

  apply_resolution_mode
end

def apply_resolution_mode
  if @retro_mode
    Window.set_virtual_resolution(RETRO_WIDTH, RETRO_HEIGHT)
           .set_filter_point
    @camera.viewport_size = Vec2.new(RETRO_WIDTH, RETRO_HEIGHT)
    @camera.offset = Vec2.new(RETRO_WIDTH / 2, RETRO_HEIGHT / 2)
  else
    Window.clear_virtual_resolution
    @camera.viewport_size = Vec2.new(Window.width, Window.height)
    @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
  end
end

def update(dt)
  if Input.key_pressed?(:r)
    @retro_mode = !@retro_mode
    apply_resolution_mode
  end
end

# Handle window resize (native mode only)
def on_resize(width, height)
  return if @retro_mode
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end
```

---

[Back to Utilities](README.md) | [Documentation Home](../../../README.md)
