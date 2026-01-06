[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Graphics](../graphics/README.md) > **Camera2D**

# Camera2D

2D camera for scrolling, zooming, and screen effects.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#bounds](#bounds)
  - [#bounds=](#bounds)
  - [#current](#current)
  - [#current=](#current)
  - [#follow](#follow)
  - [#initialize](#initialize)
  - [#offset](#offset)
  - [#offset=](#offset)
  - [#rotation](#rotation)
  - [#rotation=](#rotation)
  - [#screen_to_world](#screen_to_world)
  - [#shake](#shake)
  - [#target](#target)
  - [#target=](#target)
  - [#use](#use)
  - [#world_to_screen](#world_to_screen)
  - [#zoom](#zoom)
  - [#zoom=](#zoom)

## Instance Methods

<a id="initialize"></a>

### #initialize

Create a new Camera2D with optional initial values.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `target` | `Vec2` | World position the camera looks at (default: 0,0) |
| `offset` | `Vec2` | Screen position offset, typically screen center (default: 0,0) |
| `zoom` | `Float` | Zoom level, 1.0 = normal (default: 1.0) |
| `rotation` | `Float` | Rotation in degrees (default: 0) |

**Returns:** `Camera2D` - The new camera

**Example:**

```ruby
cam = Camera2D.new(offset: Vec2.new(400, 300))  # Center on 800x600 screen
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

Execute a block with this camera's transform applied. All drawing within the block will be transformed by the camera (position, zoom, rotation). The camera mode is automatically ended when the block completes.

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
