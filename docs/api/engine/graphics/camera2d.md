[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Graphics](../graphics/README.md) > **Camera2D**

# Camera2D

2D camera for world-space rendering, resolution-independent coordinates, scrolling, zooming, and screen effects.

## Quick Start

GMR supports two resolution modes. Choose based on your game style:

### Resolution-Independent Mode (HD Games)

Render at native window size. Width adapts to aspect ratio. **No letterboxing.**

```ruby
def init
  Window.set_size(1280, 720)

  @camera = Camera2D.new
  @camera.view_height = 9  # 9 world units visible vertically
  @camera.viewport_size = Vec2.new(Window.width, Window.height)
  @camera.offset = Vec2.new(Window.width / 2, Window.height / 2)
end

def on_resize(width, height)
  @camera.viewport_size = Vec2.new(width, height)
  @camera.offset = Vec2.new(width / 2, height / 2)
end
```

### Pixel-Perfect Mode (Retro Games)

Render to fixed-size texture. **Letterboxing when aspect ratios differ.**

```ruby
def init
  Window.set_size(960, 540)
  Window.set_virtual_resolution(320, 180)
  Window.set_filter_point

  @camera = Camera2D.new
  @camera.view_height = 9
  @camera.viewport_size = Vec2.new(320, 180)  # Must match virtual resolution!
  @camera.offset = Vec2.new(160, 90)
end
```

See [Camera Guide](../../../camera.md) for detailed documentation.

---

## Overview

The Camera2D class provides:
- **World-space coordinates**: Work in world units instead of pixels
- **Resolution independence**: Same game view at any resolution
- **Smooth following**: Camera follows targets with optional smoothing and deadzone
- **View configuration**: Control visible area via `view_height`
- **Screen effects**: Zoom, rotation, and screen shake

## Table of Contents

- [Constructor](#constructor)
- [View Configuration](#view-configuration)
  - [#view_height](#view_height)
  - [#view_height=](#view_height-1)
  - [#view_size](#view_size)
  - [#view_size=](#view_size-1)
  - [#viewport_size](#viewport_size)
  - [#viewport_size=](#viewport_size-1)
  - [#pixels_per_unit](#pixels_per_unit)
  - [#pixels_per_unit=](#pixels_per_unit-1)
  - [#effective_scale](#effective_scale)
  - [#visible_width](#visible_width)
  - [#visible_height](#visible_height)
  - [#visible_bounds](#visible_bounds)
- [Transform Properties](#transform-properties)
  - [#target](#target)
  - [#target=](#target-1)
  - [#offset](#offset)
  - [#offset=](#offset-1)
  - [#zoom](#zoom)
  - [#zoom=](#zoom-1)
  - [#rotation](#rotation)
  - [#rotation=](#rotation-1)
- [Following](#following)
  - [#follow](#follow)
  - [#bounds](#bounds)
  - [#bounds=](#bounds-1)
- [Screen Effects](#screen-effects)
  - [#shake](#shake)
- [Coordinate Conversion](#coordinate-conversion)
  - [#screen_to_world](#screen_to_world)
  - [#world_to_screen](#world_to_screen)
- [Rendering](#rendering)
  - [#use](#use)
  - [#begin](#begin)
  - [#end](#end)
- [Class Methods](#class-methods)
  - [.current](#current)
  - [.current=](#current-1)

---

## Constructor

<a id="initialize"></a>

### #initialize

Create a new Camera2D with optional initial values.

**Parameters:**

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `target` | `Vec2` | `(0, 0)` | World position the camera looks at |
| `offset` | `Vec2` | `(0, 0)` | Screen position where target appears |
| `zoom` | `Float` | `1.0` | Zoom level multiplier |
| `rotation` | `Float` | `0` | Rotation in degrees |

**Returns:** `Camera2D` - The new camera

**Example:**

```ruby
# Basic camera centered on screen
cam = Camera2D.new(offset: Vec2.new(Window.width / 2, Window.height / 2))

# Camera with initial view configuration
cam = Camera2D.new
cam.view_height = 9
cam.viewport_size = Vec2.new(Window.width, Window.height)
```

---

## View Configuration

<a id="view_height"></a>

### #view_height

Get the view height in world units (how many world units are visible vertically).

**Returns:** `Float` - The view height in world units

**Example:**

```ruby
height = camera.view_height  # => 9
```

---

<a id="view_height-1"></a>

### #view_height=

Set the view height in world units. This is the primary control for resolution-independent rendering. Setting this automatically recalculates `pixels_per_unit`.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | View height in world units (must be > 0) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
camera.view_height = 10.0  # 10 world units visible vertically
# Width is derived from aspect ratio automatically
```

---

<a id="view_size"></a>

### #view_size

Get the fixed view size in world units. Returns `nil` if using Unity-style (height-only) mode.

**Returns:** `Vec2, nil` - The view size in world units, or nil

**Example:**

```ruby
size = camera.view_size
if size
  puts "Fixed view: #{size.x} x #{size.y} world units"
else
  puts "Using height-only mode"
end
```

---

<a id="view_size-1"></a>

### #view_size=

Set a fixed view size in world units (retro-style rendering). This locks both width and height, using letterboxing if the aspect ratio doesn't match. Set to `nil` for Unity-style (height-only) mode.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2, nil` | View size in world units, or nil for Unity-style |

**Returns:** `Vec2, nil` - The value that was set

**Example:**

```ruby
# Fixed 16:9 view (retro mode)
camera.view_size = Vec2.new(16, 9)

# Switch back to Unity-style (derive width from aspect ratio)
camera.view_size = nil
```

---

<a id="viewport_size"></a>

### #viewport_size

Get the viewport dimensions in pixels.

**Returns:** `Vec2` - The viewport size in pixels

**Example:**

```ruby
viewport = camera.viewport_size
puts "Rendering at #{viewport.x}x#{viewport.y}"
```

---

<a id="viewport_size-1"></a>

### #viewport_size=

Set the viewport dimensions in pixels. This defines the camera's render target size. Setting this automatically recalculates `pixels_per_unit` to maintain the same view height, providing resolution independence.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | Viewport size in pixels |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
# Match window size
camera.viewport_size = Vec2.new(Window.width, Window.height)

# Retro resolution
camera.viewport_size = Vec2.new(320, 180)
```

---

<a id="pixels_per_unit"></a>

### #pixels_per_unit

Get the pixels-per-unit ratio. This is derived from `viewport_size.y / view_height`.

**Returns:** `Float` - Pixels per world unit

**Example:**

```ruby
ppu = camera.pixels_per_unit  # => 20 for 180p viewport with view_height=9
```

---

<a id="pixels_per_unit-1"></a>

### #pixels_per_unit=

Manually set the pixels-per-unit ratio. Prefer setting `view_height` instead for resolution independence.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | Pixels per world unit (must be > 0) |

**Returns:** `Float` - The value that was set

---

<a id="effective_scale"></a>

### #effective_scale

Get the effective scale factor (PPU * zoom). This is the final multiplier applied when converting world coordinates to screen pixels.

**Returns:** `Float` - The effective scale

**Example:**

```ruby
scale = camera.effective_scale  # => 48.0 for PPU=24 and zoom=2
```

---

<a id="visible_width"></a>

### #visible_width

Get the visible width in world units (affected by zoom).

**Returns:** `Float` - Visible world width

**Example:**

```ruby
width = camera.visible_width
```

---

<a id="visible_height"></a>

### #visible_height

Get the visible height in world units (affected by zoom).

**Returns:** `Float` - Visible world height

**Example:**

```ruby
height = camera.visible_height
```

---

<a id="visible_bounds"></a>

### #visible_bounds

Get the world-space bounds currently visible on screen.

**Returns:** `Rect` - The visible bounds in world coordinates

**Example:**

```ruby
bounds = camera.visible_bounds
# bounds.x, bounds.y = top-left corner
# bounds.width, bounds.height = visible area
```

---

## Transform Properties

<a id="target"></a>

### #target

Get the world position the camera is looking at.

**Returns:** `Vec2` - The camera's target position in world units

**Example:**

```ruby
target = camera.target
puts "Camera at #{target.x}, #{target.y}"
```

---

<a id="target-1"></a>

### #target=

Set the world position the camera looks at.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | Target position in world units |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
camera.target = player.position
camera.target = Vec2.new(50, 25)
```

---

<a id="offset"></a>

### #offset

Get the screen position offset (where the target appears on screen). Typically set to screen center for centered camera following.

**Returns:** `Vec2` - The camera's offset position in pixels

---

<a id="offset-1"></a>

### #offset=

Set the screen position offset. The target appears at this screen position.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | Offset position in pixels |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
# Center the target on screen
camera.offset = Vec2.new(Window.width / 2, Window.height / 2)

# Side-scroller: target in left third
camera.offset = Vec2.new(Window.width / 3, Window.height / 2)
```

---

<a id="zoom"></a>

### #zoom

Get the zoom level. 1.0 = normal, 2.0 = 2x magnification, 0.5 = zoomed out.

**Returns:** `Float` - The zoom level

---

<a id="zoom-1"></a>

### #zoom=

Set the zoom level. Affects the effective scale and visible area.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | Zoom level (must be > 0) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
camera.zoom = 2.0   # Zoom in (see half as much, things appear larger)
camera.zoom = 0.5   # Zoom out (see twice as much)
```

---

<a id="rotation"></a>

### #rotation

Get the camera rotation in degrees.

**Returns:** `Float` - The rotation angle

---

<a id="rotation-1"></a>

### #rotation=

Set the camera rotation in degrees.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | Rotation angle in degrees |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
camera.rotation = 45  # Tilt view 45 degrees
```

---

## Following

<a id="follow"></a>

### #follow

Configure the camera to follow a target object with optional smoothing and deadzone. The target must respond to `position` (returning Vec2) or have `x`/`y` methods. Call with `nil` to stop following.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `target` | `Object, nil` | Object to follow, or nil to stop |
| `smoothing` | `Float` | Smoothing factor 0-1 (0=instant, default: 0) |
| `deadzone` | `Rect` | Rectangle in world units where target can move freely |

**Returns:** `Camera2D` - self for chaining

**Example:**

```ruby
# Simple follow
camera.follow(@player)

# Smooth follow
camera.follow(@player, smoothing: 0.1)

# Smooth follow with deadzone
camera.follow(@player, smoothing: 0.08, deadzone: Rect.new(-2, -1, 4, 2))

# Stop following
camera.follow(nil)
```

---

<a id="bounds-1"></a>

### #bounds=

Set camera bounds to constrain movement within a world region. The camera will not show areas outside these bounds. Set to `nil` to remove bounds.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Rect, nil` | World bounds, or nil to remove |

**Returns:** `Rect, nil` - The value that was set

**Example:**

```ruby
# Constrain to level
camera.bounds = Rect.new(0, 0, 100, 50)  # World units

# Remove bounds
camera.bounds = nil
```

---

<a id="bounds"></a>

### #bounds

Get the current camera bounds. Returns nil if no bounds are set.

**Returns:** `Rect, nil` - The world bounds, or nil

---

## Screen Effects

<a id="shake"></a>

### #shake

Trigger a screen shake effect. The shake decays over the duration.

**Parameters:**

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `strength` | `Float` | `5.0` | Maximum shake offset in world units |
| `duration` | `Float` | `0.3` | Duration in seconds |
| `frequency` | `Float` | `30.0` | Shake oscillation frequency in Hz |

**Returns:** `Camera2D` - self for chaining

**Example:**

```ruby
camera.shake(strength: 0.3, duration: 0.2)
camera.shake(strength: 0.5, duration: 0.5, frequency: 20)
```

---

## Coordinate Conversion

<a id="screen_to_world"></a>

### #screen_to_world

Convert a screen position to world coordinates. Useful for mouse picking and click-to-move.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `position` | `Vec2` | Screen position in pixels |

**Returns:** `Vec2` - World position

**Example:**

```ruby
mouse_screen = Vec2.new(Input.mouse_x, Input.mouse_y)
mouse_world = camera.screen_to_world(mouse_screen)
```

---

<a id="world_to_screen"></a>

### #world_to_screen

Convert a world position to screen coordinates. Useful for placing UI elements relative to game objects.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `position` | `Vec2` | World position |

**Returns:** `Vec2` - Screen position in pixels

**Example:**

```ruby
screen_pos = camera.world_to_screen(enemy.position)
# Draw health bar at screen_pos
```

---

## Rendering

<a id="use"></a>

### #use

Execute a block with this camera's transform applied. All sprites and tilemaps drawn within the block will be rendered with the camera transform. This works correctly with z-ordering.

**Returns:** `Object` - The return value of the block

**Example:**

```ruby
camera.use do
  Graphics.clear(:black)
  @tilemap.draw(0, 0)
  @player.draw
end

# UI drawn outside (screen space)
Graphics.draw_text("Score: #{@score}", 10, 10, 16, :white)
```

---

<a id="begin"></a>

### #begin

Begin camera transform. All subsequent draws will use this camera until `end` is called. Prefer `use { }` block syntax when possible.

**Returns:** `Camera2D` - self for chaining

---

<a id="end"></a>

### #end

End camera transform. Should be called after `begin`.

**Returns:** `Camera2D` - self for chaining

---

## Class Methods

<a id="current-1"></a>

### .current=

Set the current active camera (class method). This camera will be used for sprite rendering and coordinate transformations.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Camera2D, nil` | Camera to make current, or nil to clear |

**Returns:** `Camera2D, nil` - The value that was set

**Example:**

```ruby
Camera2D.current = @main_camera
Camera2D.current = nil  # Clear
```

---

<a id="current"></a>

### .current

Get the current active camera (class method). Returns nil if no camera is set.

**Returns:** `Camera2D, nil` - The current camera, or nil

---

## Resolution Independence

The camera system provides resolution independence through the `view_height` property. Here's how the key values relate:

| Property | Description | Formula |
|----------|-------------|---------|
| `view_height` | World units visible vertically | Primary control |
| `viewport_size` | Render resolution in pixels | Set to match window/virtual resolution |
| `pixels_per_unit` | Pixels per world unit | `viewport_size.y / view_height` |
| `effective_scale` | Final pixel multiplier | `pixels_per_unit * zoom` |

**Example: Same Game View at Different Resolutions**

```ruby
# Low resolution (retro)
camera.view_height = 9
camera.viewport_size = Vec2.new(320, 180)
# PPU = 180 / 9 = 20

# High resolution (HD)
camera.view_height = 9
camera.viewport_size = Vec2.new(1920, 1080)
# PPU = 1080 / 9 = 120

# Both show the same 9 world units vertically - just at different quality
```

---

[Back to Graphics](README.md) | [Documentation Home](../../../README.md)
