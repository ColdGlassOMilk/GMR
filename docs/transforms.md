# Transforms

Transform2D is GMR's unified spatial transformation system. Every sprite requires a Transform2D that defines its position, rotation, scale, origin, and rendering properties like z-order and parallax.

## Creating Transforms

```ruby
# Basic transform at position (in world units)
@transform = Transform2D.new(x: 5.0, y: 3.0)

# Full specification
@transform = Transform2D.new(
  x: 5.0,
  y: 3.0,
  z: 0,              # Z-order for layering
  rotation: 45,      # Degrees, clockwise
  scale_x: 2.0,
  scale_y: 2.0,
  origin_x: 0.5,     # Pivot point X (in world units)
  origin_y: 0.5,     # Pivot point Y (in world units)
  parallax: 1.0      # Parallax factor (1.0 = scrolls with camera)
)
```

## World-Space Coordinates

Transform positions are in **world units**, not pixels. This provides resolution independence:

```ruby
# Position in world units
@transform = Transform2D.new(x: 10.5, y: 7.0)

# Move 5 world units per second
@transform.x += 5.0 * dt
```

The camera's `view_height` and `pixels_per_unit` determine how world units map to screen pixels. See [Camera](camera.md) for details.

## Properties

### Position

```ruby
@transform.x = 10.0    # World units
@transform.y = 5.0

# Using Vec2
@transform.position = Mathf::Vec2.new(10.0, 5.0)
pos = @transform.position  # Returns Vec2
```

### Z-Order

Z-order controls rendering order. Lower values render first (behind), higher values render on top:

```ruby
# Background behind player
@bg_transform.z = -10
@player_transform.z = 0
@fg_transform.z = 10

# Set during creation
@transform = Transform2D.new(x: 0, y: 0, z: 5)
```

### Rotation

```ruby
@transform.rotation = 45      # Set to 45 degrees
@transform.rotation += 90     # Rotate 90 degrees clockwise

# Rotation is in degrees, clockwise positive
```

### Scale

```ruby
@transform.scale_x = 2.0      # Double width
@transform.scale_y = 0.5      # Half height

# Uniform scale
@transform.scale_x = 2.0
@transform.scale_y = 2.0
```

### Origin (Pivot Point)

The origin is the point around which rotation and scaling occur (in world units for the sprite's texture):

```ruby
# Set origin to center of a sprite that's 1 world unit square
@transform.origin_x = 0.5
@transform.origin_y = 0.5

# Using Vec2
@transform.origin = Mathf::Vec2.new(0.5, 0.5)
```

For sprites, use `center_origin` to automatically set the origin to the texture center:

```ruby
@sprite.center_origin  # Sets origin on the transform
```

### Parallax

Parallax controls how the transform scrolls relative to the camera. Use this for depth effects in side-scrollers and top-down games:

```ruby
# Background scrolls slower (appears farther away)
@bg_transform.parallax = 0.5   # 50% of camera movement

# Foreground scrolls faster (appears closer)
@fg_transform.parallax = 1.5   # 150% of camera movement

# Default: scrolls with camera
@player_transform.parallax = 1.0
```

#### Parallax Examples

```ruby
# Sky layer - barely moves
@sky_transform = Transform2D.new(parallax: 0.1, z: -100)

# Far mountains
@mountains_transform = Transform2D.new(parallax: 0.3, z: -80)

# Near trees
@trees_transform = Transform2D.new(parallax: 0.7, z: -20)

# Game objects - scroll with camera
@player_transform = Transform2D.new(parallax: 1.0, z: 0)

# Rain overlay - scrolls faster for depth effect
@rain_transform = Transform2D.new(parallax: 1.2, z: 50)
```

## Parent-Child Hierarchy

Transforms can be parented to create hierarchies. Child transforms inherit position, rotation, and scale from their parent.

```ruby
# Create a turret on a tank
@tank_transform = Transform2D.new(x: 10.0, y: 5.0)
@turret_transform = Transform2D.new(y: -0.5)  # Offset from tank in world units
@gun_transform = Transform2D.new(x: 1.0)      # Offset from turret

# Set up hierarchy
@turret_transform.parent = @tank_transform
@gun_transform.parent = @turret_transform

# Now when tank moves, turret and gun follow
# When turret rotates, gun rotates with it
@tank_transform.x += 5.0 * dt
@turret_transform.rotation = 45  # Gun points at 45 degrees relative to turret
```

### World vs Local Coordinates

Local coordinates are relative to the parent. World coordinates are the final computed position.

```ruby
# Child at local position (1, 0) world units
@child = Transform2D.new(x: 1.0)
@child.parent = @parent  # Parent at (10, 5)

@child.x                 # 1.0 (local)
@child.world_position    # Vec2(11.0, 5.0) (world)

# World rotation combines parent and child
@parent.rotation = 30
@child.rotation = 15
@child.world_rotation    # 45 (30 + 15)

# World scale multiplies
@parent.scale_x = 2.0
@child.scale_x = 1.5
@child.world_scale       # Vec2(3.0, 1.0)
```

## Direction Vectors

Get the direction the transform is facing:

```ruby
# Forward vector (direction of rotation)
forward = @transform.forward

# Right vector (perpendicular to forward)
right = @transform.right

# Move in facing direction (world units per second)
@transform.x += forward.x * speed * dt
@transform.y += forward.y * speed * dt

# Strafe right
@transform.x += right.x * speed * dt
@transform.y += right.y * speed * dt
```

## Utility Functions

### Pixel Snapping

For pixel-perfect rendering (when using virtual resolution):

```ruby
# Round position to nearest pixel
@transform.round_to_pixel!

# Snap to grid (in world units)
@transform.snap_to_grid!(1.0)  # Snap to 1 world unit grid
```

### Interpolation

Smooth transitions between values:

```ruby
# Interpolate position
new_pos = Transform2D.lerp_position(start_pos, end_pos, t)
# t = 0.0 returns start_pos, t = 1.0 returns end_pos

# Interpolate rotation (shortest path)
new_rot = Transform2D.lerp_rotation(0, 270, 0.5)  # Returns 315 or -45

# Interpolate scale
new_scale = Transform2D.lerp_scale(
  Mathf::Vec2.new(1, 1),
  Mathf::Vec2.new(2, 2),
  t
)
```

## Common Patterns

### Following a Target

```ruby
def update(dt)
  # Get direction to target (in world units)
  dx = @target.transform.x - @transform.x
  dy = @target.transform.y - @transform.y

  # Normalize and move
  dist = Math.sqrt(dx * dx + dy * dy)
  if dist > 0
    @transform.x += (dx / dist) * @speed * dt
    @transform.y += (dy / dist) * @speed * dt
  end
end
```

### Facing Movement Direction

```ruby
def update(dt)
  if @vx != 0 || @vy != 0
    @transform.rotation = Math.atan2(@vy, @vx) * 180 / Math::PI
  end
end
```

### Orbiting

```ruby
def update(dt)
  @angle += @orbit_speed * dt
  @transform.x = @center_x + Math.cos(@angle) * @radius
  @transform.y = @center_y + Math.sin(@angle) * @radius
end
```

### Attached Weapon

```ruby
class Player
  def initialize
    @transform = Transform2D.new(x: 10.0, y: 5.0)
    @sprite = Sprite.new(Texture.load("player.png"), @transform)

    # Weapon attached to player
    @weapon_transform = Transform2D.new(x: 0.8, y: -0.2)  # Offset in world units
    @weapon_transform.parent = @transform
    @weapon = Sprite.new(Texture.load("weapon.png"), @weapon_transform)
  end

  def aim_at(target_x, target_y)
    # Calculate angle from player to target
    world_pos = @transform.world_position
    dx = target_x - world_pos.x
    dy = target_y - world_pos.y
    @weapon_transform.rotation = Math.atan2(dy, dx) * 180 / Math::PI
  end
end
```

### Parallax Background Layer

```ruby
def create_parallax_layer(texture, parallax_factor, z_order)
  transform = Transform2D.new(
    x: 0, y: 0,
    z: z_order,
    parallax: parallax_factor
  )
  Sprite.new(texture, transform)
end

# Create layered background
@sky = create_parallax_layer(Texture.load("sky.png"), 0.1, -100)
@mountains = create_parallax_layer(Texture.load("mountains.png"), 0.3, -80)
@trees = create_parallax_layer(Texture.load("trees.png"), 0.6, -40)
```

## With Sprites

Every sprite requires a transform:

```ruby
@transform = Transform2D.new(x: 5.0, y: 3.0)
@texture = Texture.load("player.png")
@sprite = Sprite.new(@texture, @transform)

# The sprite uses the transform for all spatial properties
@transform.x += 2.0 * dt      # Sprite moves (2 world units/sec)
@transform.rotation = 45       # Sprite rotates
@transform.scale_x = 2         # Sprite scales

# Access transform through sprite
@sprite.transform.y += 1.0 * dt
```

## Performance Notes

- World transforms are cached and only recomputed when dirty
- Modifying a parent marks all children as dirty
- Accessing `world_position`, `world_rotation`, or `world_scale` triggers recomputation if dirty
- For large hierarchies, minimize unnecessary parent changes
- Parallax calculations happen during rendering, not during transform updates

## See Also

- [Graphics](graphics.md) - Sprites and rendering
- [Camera](camera.md) - World-space configuration and resolution independence
- [Animation](animation.md) - Animating transform properties with tweens
- [API Reference](api/engine/scene/transform2d.md) - Complete Transform2D API
