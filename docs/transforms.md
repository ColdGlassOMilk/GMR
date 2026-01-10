# Transforms

Transform2D is GMR's unified spatial transformation system. Every sprite requires a Transform2D that defines its position, rotation, scale, and origin.

## Creating Transforms

```ruby
# Basic transform at position
@transform = Transform2D.new(x: 100, y: 100)

# Full specification
@transform = Transform2D.new(
  x: 100,
  y: 100,
  rotation: 45,        # Degrees, clockwise
  scale_x: 2.0,
  scale_y: 2.0,
  origin_x: 16,        # Pivot point X
  origin_y: 16         # Pivot point Y
)
```

## Properties

### Position

```ruby
@transform.x = 200
@transform.y = 150

# Using Vec2
@transform.position = Mathf::Vec2.new(200, 150)
pos = @transform.position  # Returns Vec2
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

The origin is the point around which rotation and scaling occur:

```ruby
# Set origin to center of a 32x32 sprite
@transform.origin_x = 16
@transform.origin_y = 16

# Using Vec2
@transform.origin = Mathf::Vec2.new(16, 16)
```

For sprites, use `center_origin` to automatically set the origin to the texture center:

```ruby
@sprite.center_origin  # Sets origin on the transform
```

## Parent-Child Hierarchy

Transforms can be parented to create hierarchies. Child transforms inherit position, rotation, and scale from their parent.

```ruby
# Create a turret on a tank
@tank_transform = Transform2D.new(x: 400, y: 300)
@turret_transform = Transform2D.new(y: -20)  # Offset from tank
@gun_transform = Transform2D.new(x: 30)      # Offset from turret

# Set up hierarchy
@turret_transform.parent = @tank_transform
@gun_transform.parent = @turret_transform

# Now when tank moves, turret and gun follow
# When turret rotates, gun rotates with it
@tank_transform.x += 100
@turret_transform.rotation = 45  # Gun points at 45 degrees relative to turret
```

### World vs Local Coordinates

Local coordinates are relative to the parent. World coordinates are the final computed position.

```ruby
# Child at local position (10, 0)
@child = Transform2D.new(x: 10)
@child.parent = @parent  # Parent at (100, 100)

@child.x                 # 10 (local)
@child.world_position    # Vec2(110, 100) (world)

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

# Move in facing direction
@transform.x += forward.x * speed * dt
@transform.y += forward.y * speed * dt

# Strafe right
@transform.x += right.x * speed * dt
@transform.y += right.y * speed * dt
```

## Utility Functions

### Pixel Snapping

For pixel-perfect rendering:

```ruby
# Round position to nearest pixel
@transform.round_to_pixel!

# Snap to grid
@transform.snap_to_grid!(16)  # Snap to 16x16 grid
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
  # Get direction to target
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
    @transform = Transform2D.new(x: 400, y: 300)
    @sprite = Sprite.new(Texture.load("player.png"), @transform)

    # Weapon attached to player
    @weapon_transform = Transform2D.new(x: 20, y: -5)
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

## With Sprites

Every sprite requires a transform:

```ruby
@transform = Transform2D.new(x: 100, y: 100)
@texture = Texture.load("player.png")
@sprite = Sprite.new(@texture, @transform)

# The sprite uses the transform for all spatial properties
@transform.x += 50      # Sprite moves
@transform.rotation = 45 # Sprite rotates
@transform.scale_x = 2   # Sprite scales

# Access transform through sprite
@sprite.transform.y += 10
```

## Performance Notes

- World transforms are cached and only recomputed when dirty
- Modifying a parent marks all children as dirty
- Accessing `world_position`, `world_rotation`, or `world_scale` triggers recomputation if dirty
- For large hierarchies, minimize unnecessary parent changes

## See Also

- [Graphics](graphics.md) - Sprites and rendering
- [Camera](camera.md) - Camera transforms
- [Animation](animation.md) - Animating transform properties with tweens
- [API Reference](api/engine/scene/transform2d.md) - Complete Transform2D API
