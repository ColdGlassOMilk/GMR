# Window

GMR provides window management and timing control through the `GMR::Window` and `GMR::Time` modules. Configure window size, fullscreen mode, VSync, virtual resolution, and frame timing.

## Window Configuration

### Setting Window Size

```ruby
# Set window dimensions
Window.set_size(1280, 720)

# Set window title
Window.set_title("My Game")
```

Window size changes take effect immediately on native platforms. On web, the canvas size is controlled by HTML/CSS.

### Querying Dimensions

```ruby
# Logical dimensions (what your code sees)
width = Window.width     # UI-scaled width
height = Window.height   # UI-scaled height (always 360 baseline)

# Actual screen dimensions in pixels
screen_w = Window.screen_width
screen_h = Window.screen_height

# Actual window dimensions (ignoring virtual resolution)
actual_w = Window.actual_width
actual_h = Window.actual_height
```

## Fullscreen Mode

```ruby
# Toggle fullscreen
Window.toggle_fullscreen

# Set fullscreen explicitly
Window.fullscreen = true
Window.fullscreen = false

# Check current state
if Window.fullscreen?
  puts "Running in fullscreen"
end
```

On native platforms, fullscreen uses the monitor's native resolution. On web, fullscreen requests the browser's fullscreen API.

## VSync (Vertical Synchronization)

VSync synchronizes frame presentation with the display's refresh rate to eliminate screen tearing. When enabled, the GPU waits for the monitor's vertical blank interval before presenting each frame.

### Enabling/Disabling VSync

```ruby
# Enable VSync (default)
Window.vsync = true

# Disable VSync for lower latency
Window.vsync = false

# Alternative setter
Window.set_vsync(true)

# Query current state
if Window.vsync?
  puts "VSync is enabled"
end
```

### VSync Behavior

| VSync State | Frame Pacing | Tearing | Latency |
|-------------|--------------|---------|---------|
| Enabled (default) | Locked to refresh rate | None | Higher |
| Disabled | Unlimited or target FPS | Possible | Lower |

**When VSync is enabled:**
- Frames are presented at the monitor's refresh rate (typically 60Hz)
- No screen tearing occurs
- Input latency may be slightly higher
- Target FPS is automatically set to 0 (unlimited) to avoid double-throttling

**When VSync is disabled:**
- Frames are presented as soon as they're ready
- Screen tearing may occur during fast motion
- Input latency is minimized
- Target FPS defaults to 60 as a software limiter

### Platform Notes

| Platform | VSync Mechanism |
|----------|-----------------|
| Windows | WGL swap interval |
| Linux | GLX swap interval |
| macOS | CGL swap interval |
| Web | Browser-controlled via requestAnimationFrame |

On web builds, VSync is handled automatically by the browser's `requestAnimationFrame` and cannot be disabled.

## Frame Timing

The `GMR::Time` module provides frame timing information and control.

### Delta Time

```ruby
def update(dt)
  # dt is the time since last frame in seconds
  # Typically 0.016 at 60 FPS
  @player.x += @speed * dt  # Frame-rate independent movement
end
```

Delta time is also accessible via:

```ruby
Time.delta  # Same as the dt parameter
```

### Elapsed Time

```ruby
# Total time since game started
Time.elapsed  # Returns seconds as float

# Example: Flash effect every 0.5 seconds
visible = (Time.elapsed % 1.0) < 0.5
```

### Frame Rate

```ruby
# Get current FPS
fps = Time.fps

# Set target frame rate
Time.set_target_fps(60)   # Limit to 60 FPS
Time.set_target_fps(144)  # Limit to 144 FPS
Time.set_target_fps(0)    # Unlimited (VSync controls pacing)
```

**Note:** When VSync is enabled, the display refresh rate takes precedence over the target FPS. Setting a target FPS below the refresh rate will still cap the frame rate, but setting it higher has no effect.

### Time Scale

Control the speed of game time for slow-motion, pause, or fast-forward effects:

```ruby
# Normal speed
Time.scale = 1.0

# Slow motion (half speed)
Time.scale = 0.5

# Pause
Time.scale = 0.0

# Fast forward (double speed)
Time.scale = 2.0

# Query current scale
current_scale = Time.scale
```

Timers created with `scaled: true` (the default) respect time scale. Physics and animations that use `dt` should also be affected.

## Virtual Resolution

Render to a fixed-size buffer and scale to fit the window. Ideal for pixel-art games that need crisp integer scaling.

### Enabling Virtual Resolution

```ruby
def init
  Window.set_size(960, 540)
  Window.set_virtual_resolution(320, 180)  # Render at 320x180
  Window.set_filter_point  # Crisp pixel scaling (nearest neighbor)
end
```

### Texture Filtering

| Method | Result | Use Case |
|--------|--------|----------|
| `Window.set_filter_point` | Crisp, pixelated | Pixel art, retro games |
| `Window.set_filter_bilinear` | Smooth, blurred | HD graphics, photos |

### Querying Virtual Resolution

```ruby
# Check if virtual resolution is enabled
if Window.virtual_resolution?
  puts "Rendering to virtual buffer"
end
```

### Clearing Virtual Resolution

```ruby
# Return to native resolution
Window.clear_virtual_resolution
```

### Complete Virtual Resolution Example

```ruby
include GMR

def init
  # 960x540 window, rendering at 320x180 (3x integer scale)
  Window.set_size(960, 540)
  Window.set_virtual_resolution(320, 180)
  Window.set_filter_point

  @camera = Camera2D.new
  @camera.view_height = 9
  @camera.viewport_size = Vec2.new(320, 180)  # Must match virtual res!
  @camera.offset = Vec2.new(160, 90)
end

def draw
  @camera.use do
    Graphics.clear(:black)
    # ... draw game at 320x180
  end
end
```

**Important:** When using virtual resolution with a camera, set `viewport_size` to match the virtual resolution, not the window size.

## Device Pixel Ratio (Web)

On high-DPI displays (Retina, 4K monitors), web browsers use a device pixel ratio greater than 1.0. GMR can render at the native device resolution or the CSS resolution.

```ruby
# Render at native device pixel ratio (sharper on Retina)
Window.use_native_dpr

# Render at CSS resolution (default, better performance)
Window.use_css_dpr

# Query current device pixel ratio
dpr = Window.device_pixel_ratio  # 1.0, 2.0, etc.

# Check if native DPR rendering is enabled
if Window.native_dpr?
  puts "Rendering at native resolution"
end
```

| Method | Resolution on 2x Retina | Performance |
|--------|-------------------------|-------------|
| `use_css_dpr` (default) | 1280x720 | Better |
| `use_native_dpr` | 2560x1440 | Lower |

## Monitor Information

Query connected monitors for multi-monitor setups or resolution selection:

```ruby
# Number of monitors
count = Window.monitor_count

# Monitor properties (0-indexed)
width = Window.monitor_width(0)
height = Window.monitor_height(0)
refresh_rate = Window.monitor_refresh_rate(0)  # Hz
name = Window.monitor_name(0)

# Example: List all monitors
Window.monitor_count.times do |i|
  puts "Monitor #{i}: #{Window.monitor_name(i)}"
  puts "  Resolution: #{Window.monitor_width(i)}x#{Window.monitor_height(i)}"
  puts "  Refresh: #{Window.monitor_refresh_rate(i)}Hz"
end
```

## Window Resize Handling

GMR calls `on_resize` when the window size changes:

```ruby
def on_resize(width, height)
  # Update camera viewport
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end
```

This is called automatically when:
- The user resizes the window
- Fullscreen is toggled
- The browser window changes size (web)

**Note:** `on_resize` is not called before `init`. Initialize your camera in `init` using `Window.width` and `Window.height`.

## Complete Example

```ruby
include GMR

def init
  Window.set_title("Window Demo")
  Window.set_size(1280, 720)

  # Enable VSync for smooth rendering
  Window.vsync = true

  # Setup camera
  @camera = Camera2D.new
  @camera.view_height = 9
  @camera.viewport_size = Vec2.new(Window.width, Window.height)
  @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)

  input do |i|
    i.toggle_fullscreen :f11
    i.toggle_vsync :v
  end
end

def on_resize(width, height)
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end

def update(dt)
  if Input.action_pressed?(:toggle_fullscreen)
    Window.toggle_fullscreen
  end

  if Input.action_pressed?(:toggle_vsync)
    Window.vsync = !Window.vsync?
  end
end

def draw
  @camera.use do
    Graphics.clear(:dark_gray)
    # ... draw game
  end

  # UI overlay
  Graphics.draw_text("FPS: #{Time.fps}", 10, 10, 16, :white)
  Graphics.draw_text("VSync: #{Window.vsync? ? 'ON' : 'OFF'}", 10, 30, 16, :white)
  Graphics.draw_text("Press F11 for fullscreen, V to toggle VSync", 10, 50, 16, :gray)
end
```

## Quick Reference

### GMR::Window

| Method | Description |
|--------|-------------|
| `set_size(w, h)` | Set window dimensions |
| `set_title(title)` | Set window title |
| `width` | Logical width (UI-scaled) |
| `height` | Logical height (360 baseline) |
| `screen_width` | Actual screen width in pixels |
| `screen_height` | Actual screen height in pixels |
| `toggle_fullscreen` | Toggle fullscreen mode |
| `fullscreen=` | Set fullscreen mode |
| `fullscreen?` | Check if fullscreen |
| `vsync=` | Enable/disable VSync |
| `vsync?` | Check if VSync is enabled |
| `set_vsync(enabled)` | Enable/disable VSync |
| `set_virtual_resolution(w, h)` | Enable virtual resolution |
| `clear_virtual_resolution` | Disable virtual resolution |
| `virtual_resolution?` | Check if virtual resolution is enabled |
| `set_filter_point` | Nearest-neighbor filtering |
| `set_filter_bilinear` | Bilinear filtering |
| `monitor_count` | Number of connected monitors |
| `monitor_width(i)` | Monitor width in pixels |
| `monitor_height(i)` | Monitor height in pixels |
| `monitor_refresh_rate(i)` | Monitor refresh rate in Hz |
| `monitor_name(i)` | Monitor name string |

### GMR::Time

| Method | Description |
|--------|-------------|
| `delta` | Time since last frame (seconds) |
| `elapsed` | Total time since start (seconds) |
| `fps` | Current frames per second |
| `set_target_fps(fps)` | Set target frame rate (0 = unlimited) |
| `scale` | Get current time scale |
| `scale=` | Set time scale (0 = pause, 1 = normal) |

## See Also

- [Camera](camera.md) - World-space rendering and resolution modes
- [Graphics](graphics.md) - Drawing and rendering
- [Engine Model](engine-model.md) - Frame execution order
- [Input](input.md) - Keyboard, mouse, gamepad input
