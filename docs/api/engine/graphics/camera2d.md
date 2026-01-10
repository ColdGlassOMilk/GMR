[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Graphics](../graphics/README.md) > **Camera2D**

# Camera2D

2D camera for scrolling, zooming, and screen effects.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#begin](#begin)
  - [#bounds](#bounds)
  - [#bounds=](#bounds)
  - [#current](#current)
  - [#current=](#current)
  - [#effective_scale](#effective_scale)
  - [#end](#end)
  - [#follow](#follow)
  - [#initialize](#initialize)
  - [#offset](#offset)
  - [#offset=](#offset)
  - [#pixels_per_unit](#pixels_per_unit)
  - [#pixels_per_unit=](#pixels_per_unit)
  - [#rotation](#rotation)
  - [#rotation=](#rotation)
  - [#screen_to_world](#screen_to_world)
  - [#shake](#shake)
  - [#target](#target)
  - [#target=](#target)
  - [#use](#use)
  - [#view_height](#view_height)
  - [#view_height=](#view_height)
  - [#view_size](#view_size)
  - [#view_size=](#view_size)
  - [#viewport_size](#viewport_size)
  - [#viewport_size=](#viewport_size)
  - [#visible_bounds](#visible_bounds)
  - [#visible_height](#visible_height)
  - [#visible_width](#visible_width)
  - [#world_to_screen](#world_to_screen)
  - [#zoom](#zoom)
  - [#zoom=](#zoom)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new Camera2D with optional initial values. Resolution-independent: Set view_height (world units visible vertically) and viewport_size (render resolution) - PPU is calculated automatically.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `target` | `Vec2` | World position the camera looks at (default: 0,0) |
| `offset` | `Vec2` | Screen position offset, typically screen center (default: 0,0) |
| `zoom` | `Float` | Zoom level, 1.0 = normal (default: 1.0) |
| `rotation` | `Float` | Rotation in degrees (default: 0) |
| `view_height` | `Float` | World units visible vertically (default: 7.5) |
| `view_size` | `Vec2` | Fixed view size in world units (retro mode with letterboxing) |
| `viewport_size` | `Vec2` | Render resolution in pixels (default: 320x180) |

**Returns:** `Camera2D` - The new camera

**Example:**

```ruby
# High-res rendering of same view
  cam = Camera2D.new(view_height: 7.5, viewport_size: Vec2.new(1920, 1080))
```

---

<a id="target"></a>

### #target

Get the world position the camera is looking at.

**Returns:** `Vec2` - The camera's target position

**Example:**

```ruby
target = camera.target
```

---

<a id="offset"></a>

### #offset

Get the screen position offset (where the target appears on screen). Typically set to screen center for centered camera following.

**Returns:** `Vec2` - The camera's offset position

**Example:**

```ruby
offset = camera.offset
```

---

<a id="zoom"></a>

### #zoom

Get the zoom level. 1.0 = normal, 2.0 = 2x magnification, 0.5 = zoomed out.

**Returns:** `Float` - The zoom level

**Example:**

```ruby
z = camera.zoom
```

---

<a id="rotation"></a>

### #rotation

Get the camera rotation in degrees.

**Returns:** `Float` - The rotation angle

**Example:**

```ruby
angle = camera.rotation
```

---

<a id="target"></a>

### #target=

Set the world position the camera looks at.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | The target position |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
camera.target = player.position
```

---

<a id="offset"></a>

### #offset=

Set the screen position offset. The target appears at this screen position. Set to screen center for centered following.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | The offset position |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
camera.offset = Vec2.new(400, 300)  # Center on 800x600 screen
```

---

<a id="zoom"></a>

### #zoom=

Set the zoom level. 1.0 = normal, 2.0 = 2x magnification, 0.5 = zoomed out.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The zoom level (must be > 0) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
camera.zoom = 0.5    # Zoom out (see more of the world)
```

---

<a id="rotation"></a>

### #rotation=

Set the camera rotation in degrees.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The rotation angle in degrees |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
camera.rotation += 10 * dt  # Rotate over time
```

---

<a id="pixels_per_unit"></a>

### #pixels_per_unit

Get the scale factor (screen pixels per world unit). This defines the relationship between world coordinates and screen pixels. At zoom=1.0, one world unit takes up this many pixels on screen.

**Returns:** `Float` - The pixels per unit value

**Example:**

```ruby
ppu = camera.pixels_per_unit
```

---

<a id="pixels_per_unit"></a>

### #pixels_per_unit=

Set the scale factor (screen pixels per world unit). Common values: - 100.0: Unity-style (default), 1 unit = 100 pixels - 16/24/32: Tile-based games, 1 unit = 1 tile - 1.0: 1:1 pixel mapping at zoom=1.0

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The pixels per unit value (must be > 0) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
camera.pixels_per_unit = 100.0 # Unity-style default
```

---

<a id="viewport_size"></a>

### #viewport_size

Get the viewport dimensions in pixels. This is the size of the render target (camera's "screen" size), used for aspect ratio and visible bounds.

**Returns:** `Vec2` - The viewport size in pixels

**Example:**

```ruby
size = camera.viewport_size
```

---

<a id="viewport_size"></a>

### #viewport_size=

Set the viewport dimensions in pixels. This defines the camera's render target size. Setting this automatically recalculates pixels_per_unit to maintain the same view_height, providing resolution independence.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2` | The viewport size in pixels |

**Returns:** `Vec2` - The value that was set

**Example:**

```ruby
camera.viewport_size = Vec2.new(1920, 1080) # HD (same view, higher quality)
```

---

<a id="visible_bounds"></a>

### #visible_bounds

Get the world-space bounds currently visible on screen. This is the rectangle of world coordinates that the camera can see.

**Returns:** `Rect` - The visible bounds in world coordinates

**Example:**

```ruby
end
```

---

<a id="visible_width"></a>

### #visible_width

Get the width of the visible world area (in world units).

**Returns:** `Float` - The visible width

**Example:**

```ruby
width = camera.visible_width
```

---

<a id="visible_height"></a>

### #visible_height

Get the height of the visible world area (in world units).

**Returns:** `Float` - The visible height

**Example:**

```ruby
height = camera.visible_height
```

---

<a id="effective_scale"></a>

### #effective_scale

Get the effective scale factor (pixels_per_unit * zoom). This is how many screen pixels one world unit currently occupies.

**Returns:** `Float` - The effective scale

**Example:**

```ruby
scale = camera.effective_scale
```

---

<a id="view_height"></a>

### #view_height

Get the view height in world units (how many world units are visible vertically). This is the primary control for resolution-independent rendering.

**Returns:** `Float` - The view height in world units

**Example:**

```ruby
height = camera.view_height
```

---

<a id="view_height"></a>

### #view_height=

Set the view height in world units (how many world units are visible vertically). Setting this automatically recalculates pixels_per_unit. Use this for Unity-style resolution independence where width is derived from aspect ratio.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | The view height in world units (must be > 0) |

**Returns:** `Float` - The value that was set

**Example:**

```ruby
camera.view_height = 10.0  # 10 world units tall
```

---

<a id="view_size"></a>

### #view_size

Get the fixed view size in world units (for retro-style rendering). Returns nil if using Unity-style (height-only) mode.

**Returns:** `Vec2, nil` - The view size in world units, or nil if not in retro mode

**Example:**

```ruby
size = camera.view_size
```

---

<a id="view_size"></a>

### #view_size=

Set a fixed view size in world units (for retro-style rendering). This locks both width and height, using letterboxing if the aspect ratio doesn't match. Set to nil to switch back to Unity-style (height-only) mode.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Vec2, nil` | The view size in world units, or nil for Unity-style mode |

**Returns:** `Vec2, nil` - The value that was set

**Example:**

```ruby
camera.view_size = nil  # Switch to Unity-style
```

---

<a id="follow"></a>

### #follow

Configure the camera to follow a target object with optional smoothing and deadzone. The target must respond to `position` (returning Vec2) or have `x`/`y` methods. Call with nil to stop following.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `target` | `Object, nil` | Object with position/x/y to follow, or nil to stop |
| `smoothing` | `Float` | Smoothing factor 0-1 (0=instant, 0.1=smooth, default: 0) |
| `deadzone` | `Rect` | Rectangle where target can move without camera moving |

**Returns:** `Camera2D` - self for chaining

**Example:**

```ruby
# Stop following
  camera.follow(nil)
```

---

<a id="bounds"></a>

### #bounds=

Set camera bounds to constrain movement within a world region. The camera will not show areas outside these bounds. Set to nil to remove bounds.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Rect, nil` | The world bounds, or nil to remove |

**Returns:** `Rect, nil` - The value that was set

**Example:**

```ruby
camera.bounds = nil  # No bounds
```

---

<a id="bounds"></a>

### #bounds

Get the current camera bounds. Returns nil if no bounds are set.

**Returns:** `Rect, nil` - The world bounds, or nil if unbounded

**Example:**

```ruby
rect = camera.bounds
```

---

<a id="shake"></a>

### #shake

Trigger a screen shake effect. The shake decays over the duration.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `strength` | `Float` | Maximum shake offset in pixels (default: 5.0) |
| `duration` | `Float` | How long the shake lasts in seconds (default: 0.3) |
| `frequency` | `Float` | Shake oscillation frequency in Hz (default: 30.0) |

**Returns:** `Camera2D` - self for chaining

**Example:**

```ruby
camera.shake(strength: 3, duration: 0.2, frequency: 20)
```

---

<a id="world_to_screen"></a>

### #world_to_screen

Convert a world position to screen coordinates. Useful for placing UI elements relative to game objects.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `position` | `Vec2` | The world position |

**Returns:** `Vec2` - The screen position

**Example:**

```ruby
health_bar_x = camera.world_to_screen(enemy.position).x
```

---

<a id="screen_to_world"></a>

### #screen_to_world

Convert a screen position to world coordinates. Useful for mouse picking and click-to-move.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `position` | `Vec2` | The screen position |

**Returns:** `Vec2` - The world position

**Example:**

```ruby
click_target = camera.screen_to_world(Vec2.new(mouse_x, mouse_y))
```

---

<a id="use"></a>

### #use

Execute a block with this camera's transform applied. All sprites drawn within the block will be rendered with the camera transform applied during the deferred rendering pass. This works correctly with z-ordering.

**Returns:** `Object` - The return value of the block

**Example:**

```ruby
# Nested cameras
  world_camera.use do
    draw_world()
  end
  # UI drawn outside camera (screen space)
  draw_ui()
```

---

<a id="begin"></a>

### #begin

Begin camera transform. All subsequent sprite draws will use this camera until `end` is called. Prefer `use { }` block syntax when possible.

**Returns:** `Camera2D` - self for chaining

**Example:**

```ruby
camera.begin
```

---

<a id="end"></a>

### #end

End camera transform. Should be called after `begin`.

**Returns:** `Camera2D` - self for chaining

---

<a id="current"></a>

### #current=

Set the current active camera (class method). This camera will be used for sprite rendering and coordinate transformations.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Camera2D, nil` | The camera to make current, or nil to clear |

**Returns:** `Camera2D, nil` - The value that was set

**Example:**

```ruby
Camera2D.current = nil  # No camera
```

---

<a id="current"></a>

### #current

Get the current active camera (class method). Returns nil if no camera is set.

**Returns:** `Camera2D, nil` - The current camera, or nil

**Example:**

```ruby
cam = Camera2D.current
```

---

---

[Back to Graphics](README.md) | [Documentation Home](../../../README.md)
