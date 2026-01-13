# Spatial Queries

The `SpatialHash` module provides efficient spatial queries for finding entities by position. Instead of checking every entity (O(n)), spatial hashing uses a grid to achieve near-constant-time lookups.

## When to Use

Use spatial queries when you need to:
- Find all enemies within attack range
- Check what entities are under the mouse cursor
- Find the nearest entity to a position
- Implement efficient collision between dynamic entities

## Basic Usage

### Adding Entities

```ruby
# Register entities with their bounds
SpatialHash.add(@enemy, bounds: @enemy.hitbox)
SpatialHash.add(@player, bounds: [@player.x, @player.y, 16, 16])
SpatialHash.add(@item, x: 100, y: 200, w: 8, h: 8)
```

Bounds can be specified as:
- A `Rect` object (anything with `x`, `y`, `width`/`w`, `height`/`h` methods)
- An array `[x, y, w, h]`
- Individual `x:`, `y:`, `w:`, `h:` keyword arguments

### Updating Position

When an entity moves, update its position in the spatial hash:

```ruby
def update(dt)
  @enemy.x += @velocity_x * dt
  @enemy.y += @velocity_y * dt

  # Update spatial hash after movement
  SpatialHash.update(@enemy, bounds: @enemy.hitbox)
end
```

### Removing Entities

```ruby
# Remove when entity is destroyed
SpatialHash.remove(@enemy)

# Clear all entities (e.g., on scene change)
SpatialHash.clear
```

## Querying

### Rectangle Query

Find all entities overlapping a rectangular area:

```ruby
# Get all entities in a region
entities = SpatialHash.query_rect(100, 100, 50, 50)

# Attack everything in a blast radius
SpatialHash.query_rect(@explosion.x - 32, @explosion.y - 32, 64, 64).each do |entity|
  entity.take_damage(50) if entity.respond_to?(:take_damage)
end
```

### Circle Query

Find all entities overlapping a circle:

```ruby
# Find enemies within attack range
nearby_enemies = SpatialHash.query_circle(@player.x, @player.y, 100)

# Attract coins within magnet range
SpatialHash.query_circle(@player.x, @player.y, 50).each do |entity|
  entity.move_toward(@player) if entity.is_a?(Coin)
end
```

### Point Query

Find all entities containing a specific point:

```ruby
# What did the player click on?
clicked = SpatialHash.query_point(Input.mouse_x, Input.mouse_y)
clicked.each { |entity| entity.select if entity.respond_to?(:select) }
```

### Nearest Query

Find the single nearest entity to a point:

```ruby
# Turret targeting
target = SpatialHash.nearest(@turret.x, @turret.y, max_distance: 200)
@turret.aim_at(target) if target

# Find nearest health pickup
health = SpatialHash.nearest(@player.x, @player.y, max_distance: 100)
```

## Configuration

### Cell Size

The spatial hash divides the world into a grid. The cell size affects performance:
- **Smaller cells**: Faster queries, but more memory and slower updates
- **Larger cells**: Slower queries, but less memory and faster updates

The default is 64 world units. Set based on typical entity size and query radius:

```ruby
# For games with small entities and frequent queries
SpatialHash.cell_size = 32

# For games with large entities or sparse worlds
SpatialHash.cell_size = 128

# Read current setting
puts SpatialHash.cell_size
```

A good rule of thumb: cell size should be roughly equal to or slightly larger than your typical query radius.

## Complete Example

```ruby
class GameScene < Scene
  def init
    @enemies = GameArray.new
    @projectiles = GameArray.new

    # Spawn enemies
    10.times do
      enemy = Enemy.new(Random.float(0, 40), Random.float(0, 22.5))
      @enemies << enemy
      SpatialHash.add(enemy, bounds: enemy.hitbox)
    end
  end

  def update(dt)
    # Update enemies
    @enemies.each_alive do |enemy|
      enemy.update(dt)
      SpatialHash.update(enemy, bounds: enemy.hitbox)
    end

    # Update projectiles
    @projectiles.each_alive do |proj|
      proj.update(dt)

      # Check for hits using spatial query
      SpatialHash.query_circle(proj.x, proj.y, proj.radius).each do |entity|
        next unless entity.is_a?(Enemy) && entity.alive?
        entity.take_damage(proj.damage)
        proj.destroy
        break
      end
    end

    # Find nearest enemy for auto-aim
    if @player.auto_aim_enabled
      target = SpatialHash.nearest(@player.x, @player.y, max_distance: 200)
      @player.aim_at(target) if target
    end
  end

  def on_entity_destroyed(entity)
    SpatialHash.remove(entity)
  end

  def cleanup
    SpatialHash.clear
  end
end
```

## Debug Info

```ruby
# Get number of tracked entities
puts "Entities in spatial hash: #{SpatialHash.count}"
```

## Performance Tips

1. **Update only when moving**: Don't call `SpatialHash.update` every frame for static entities.

2. **Remove destroyed entities promptly**: Keep the spatial hash clean by removing entities in their `on_destroy` hook.

3. **Use appropriate cell size**: Profile different cell sizes if you have performance issues.

4. **Filter query results**: Query results include all entities with overlapping bounds. Filter by type or other criteria:

```ruby
# Only damage enemies, not allies
SpatialHash.query_circle(x, y, radius)
  .select { |e| e.is_a?(Enemy) }
  .each { |e| e.take_damage(10) }
```

## See Also

- [Utilities](utilities.md) - Destroyable pattern for cleanup
- [Debug](debug.md) - Visualizing spatial queries during development
