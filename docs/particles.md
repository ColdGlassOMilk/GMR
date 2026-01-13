# Particles

GMR provides a data-driven particle system for visual effects like explosions, smoke, dust trails, and magic spells. Effects can be defined in JSON config files or declaratively in Ruby code, and support both textured sprites (from spritesheets) and colored circles.

## Quick Start

### Fire-and-Forget Effects

For one-shot effects like explosions or hit impacts:

```ruby
# Emit at a position
ParticleEmitter.emit("effects/explosion.json", position: Mathf::Vec2.new(x, y))

# Emit attached to a transform (follows the object)
ParticleEmitter.emit("effects/hit_sparks.json", transform: @enemy.transform)

# With completion callback
ParticleEmitter.emit("effects/death.json", position: pos) do
  spawn_pickup
end
```

### Managed Emitters

For continuous effects like smoke trails or running dust:

```ruby
def initialize
  @dust_emitter = ParticleEmitter.new("effects/run_dust.json")
end

def update(dt)
  # Update position manually
  @dust_emitter.position = Mathf::Vec2.new(foot_x, foot_y)

  # Control emission
  if running?
    @dust_emitter.start unless @dust_emitter.emitting?
  else
    @dust_emitter.stop if @dust_emitter.emitting?
  end
end
```

### Transform-Attached Emitters

For effects that automatically follow a game object:

```ruby
# Attach to transform on creation
@trail = ParticleEmitter.new("effects/magic_trail.json", @player.transform)
@trail.start

# Or attach later
@smoke = ParticleEmitter.new("effects/smoke.json")
@smoke.transform = @chimney.transform
@smoke.start
```

## ParticleEmitter API

### Class Methods

```ruby
# Fire-and-forget emission
ParticleEmitter.emit(config_path, position: vec2)
ParticleEmitter.emit(config_path, transform: transform2d)
ParticleEmitter.emit(config_path, position: vec2) { on_complete }

# Preload config (avoid loading hitches)
ParticleEmitter.preload("effects/explosion.json")

# Debug statistics
ParticleEmitter.emitter_count  # Number of active emitters
ParticleEmitter.alive_count    # Total alive particles
ParticleEmitter.total_count    # Total allocated particles
```

### Constructor

```ruby
# Create from JSON config file
emitter = ParticleEmitter.new("effects/smoke.json")

# Create attached to transform
emitter = ParticleEmitter.new("effects/trail.json", @player.transform)

# Create from Ruby Hash (declarative, no JSON file needed)
emitter = ParticleEmitter.new({
  texture: "particles.jpg",
  columns: 6,
  rows: 6,
  spawn_rate: 10,
  max_particles: 50,
  lifetime: { min: 0.5, max: 1.0 },
  start_size: { min: 0.3, max: 0.5 },
  end_size: 0,
  velocity_mode: "radial",
  speed: { min: 2.0, max: 4.0 }
})
```

The Hash constructor is useful for:
- Prototyping effects quickly without creating JSON files
- Dynamic effects with runtime-computed values
- Procedurally generated particle configurations

### Control Methods

```ruby
emitter.start    # Begin continuous emission
emitter.stop     # Stop emitting (existing particles continue)
emitter.pause    # Freeze all particle updates
emitter.resume   # Resume from pause
emitter.reset    # Clear all particles, reset state
emitter.burst    # Emit burst_count particles immediately
emitter.burst(50) # Emit specific number
```

### Properties

```ruby
# Position (only works when not attached to transform)
emitter.position = Mathf::Vec2.new(x, y)
pos = emitter.position

# Transform attachment
emitter.transform = @player.transform
emitter.transform = nil  # Detach
```

### State Queries

```ruby
emitter.emitting?       # Is spawning new particles?
emitter.alive?          # Has any alive particles?
emitter.active?         # Is not paused or destroyed?
emitter.particle_count  # Number of alive particles
```

### Draw Order Control

By default, particles render after all Ruby drawing. To control exactly when particles draw (e.g., behind the player but in front of the background):

```ruby
def draw
  @camera.use do
    @level.draw
    @dust_emitter.draw   # Draw dust behind player
    @player.draw
  end
end
```

### Callbacks

```ruby
# Called when all particles die (one-shot emitters)
emitter.on_complete { spawn_next_effect }
```

## JSON Effect Configuration

Effect files define particle behavior. Place them in your assets folder.

### Minimal Example

```json
{
  "name": "simple_burst",
  "burst_count": 20,
  "max_particles": 30,
  "lifetime": { "min": 0.5, "max": 1.0 },
  "speed": { "min": 2.0, "max": 5.0 },
  "start_size": { "min": 0.2, "max": 0.4 },
  "end_size": { "min": 0.0, "max": 0.0 }
}
```

### Complete Example

```json
{
  "name": "explosion",

  "spawn_rate": 0,
  "burst_count": 50,
  "burst_interval": 0,
  "max_particles": 100,

  "lifetime": { "min": 0.3, "max": 0.8 },

  "shape": "circle",
  "shape_radius": 0.5,

  "velocity_mode": "radial",
  "direction": 0,
  "spread": { "min": 0, "max": 0 },
  "speed": { "min": 3.0, "max": 8.0 },

  "gravity": { "x": 0, "y": 2.0 },
  "drag": 0.5,

  "start_size": { "min": 0.3, "max": 0.6 },
  "end_size": { "min": 0.0, "max": 0.1 },
  "size_easing": "out_quad",

  "start_rotation": { "min": 0, "max": 6.28 },
  "angular_velocity": { "min": -2, "max": 2 },

  "start_color": { "r": 255, "g": 200, "b": 50, "a": 255 },
  "end_color": { "r": 100, "g": 50, "b": 0, "a": 0 },
  "color_easing": "linear",

  "layer": 150,
  "z": 0,
  "world_space": true,
  "scaled": true
}
```

### Configuration Reference

#### Texture (Optional)

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `texture` | string | "" | Path to particle texture (empty = colored circles) |
| `spritesheet_cols` | int | 1 | Number of columns in spritesheet |
| `spritesheet_rows` | int | 1 | Number of rows in spritesheet |
| `frame_width` | float | auto | Pixel width per frame (auto-detected if 0) |
| `frame_height` | float | auto | Pixel height per frame (auto-detected if 0) |

When a texture is specified, each particle randomly selects a frame from the spritesheet on spawn. This is ideal for smoke puffs, dust clouds, or any effect with varied particle appearances.

#### Emission

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `name` | string | required | Effect identifier |
| `spawn_rate` | float | 0 | Particles per second (continuous) |
| `burst_count` | int | 0 | Particles per burst |
| `burst_interval` | float | 0 | Seconds between bursts (0 = burst on start only) |
| `max_particles` | int | 100 | Maximum particle pool size |

#### Lifetime

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `lifetime` | range | 1.0 | Particle lifetime in seconds |

#### Spawn Shape

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `shape` | string | "point" | Spawn shape (see below) |
| `shape_radius` | float | 1.0 | Radius for circle shapes |
| `shape_size` | vec2 | {1,1} | Size for rectangle/line shapes |

**Shape Types:**
- `point` - All particles spawn at emitter position
- `circle` - Random point inside circle
- `circle_edge` - Random point on circle edge
- `rectangle` - Random point inside rectangle
- `rectangle_edge` - Random point on rectangle edge
- `line` - Random point along horizontal line

#### Velocity

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `velocity_mode` | string | "directional" | How velocity is calculated |
| `direction` | float | 0 | Base direction in radians (0 = right) |
| `spread` | range | 0 | Angular spread in radians |
| `speed` | range | 1.0 | Initial speed |

**Velocity Modes:**
- `directional` - All particles move in `direction` with `spread`
- `radial` - Particles move away from spawn point
- `tangential` - Particles move perpendicular to radial
- `random` - Random direction

#### Physics

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `gravity` | vec2 | {0, 0} | Gravity acceleration |
| `drag` | float | 0 | Velocity damping (0-1) |

#### Size

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `start_size` | range | 1.0 | Initial size (world units) |
| `end_size` | range | 0.0 | Final size |
| `size_easing` | string | "linear" | Easing function |

#### Rotation

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `start_rotation` | range | 0 | Initial rotation in radians |
| `angular_velocity` | range | 0 | Rotation speed (radians/second) |

#### Color

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `start_color` | color | white | Initial color (RGBA 0-255) |
| `end_color` | color | white | Final color |
| `color_easing` | string | "linear" | Easing function |

#### Rendering

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `layer` | int | 150 | Render layer (EFFECTS layer) |
| `z` | float | 0 | Z-order within layer |
| `world_space` | bool | true | Particles independent of emitter movement |
| `scaled` | bool | true | Respect Time.scale |

### Range Values

Properties marked as "range" accept either a single value or min/max:

```json
"lifetime": 1.0
"lifetime": { "min": 0.5, "max": 1.5 }
```

### Easing Functions

Available for `size_easing` and `color_easing`:

- `linear`
- `in_quad`, `out_quad`, `in_out_quad`
- `in_cubic`, `out_cubic`, `in_out_cubic`
- `in_quart`, `out_quart`, `in_out_quart`
- `in_sine`, `out_sine`, `in_out_sine`
- `in_expo`, `out_expo`, `in_out_expo`
- `in_back`, `out_back`, `in_out_back`
- `in_elastic`, `out_elastic`, `in_out_elastic`
- `in_bounce`, `out_bounce`, `in_out_bounce`

## Common Effect Patterns

### Running Dust (Textured)

Using a 6x6 spritesheet of smoke/dust particles:

```json
{
  "name": "run_dust",
  "texture": "particles.jpg",
  "spritesheet_cols": 6,
  "spritesheet_rows": 6,

  "spawn_rate": 8,
  "max_particles": 30,

  "lifetime": { "min": 0.8, "max": 1.4 },

  "shape": "line",
  "shape_size": { "x": 0.3, "y": 0 },

  "velocity_mode": "directional",
  "direction": -1.5708,
  "spread": { "min": -0.6, "max": 0.6 },
  "speed": { "min": 0.8, "max": 1.5 },

  "gravity": { "x": 0, "y": -0.8 },
  "drag": 1.5,

  "start_size": { "min": 0.4, "max": 0.6 },
  "end_size": { "min": 1.2, "max": 1.8 },

  "start_color": { "r": 200, "g": 180, "b": 140, "a": 160 },
  "end_color": { "r": 180, "g": 160, "b": 120, "a": 0 },

  "world_space": true
}
```

### Running Dust (Simple)

Without textures, using colored circles:

```json
{
  "name": "run_dust",
  "spawn_rate": 15,
  "max_particles": 40,
  "lifetime": { "min": 0.3, "max": 0.5 },
  "shape": "line",
  "shape_size": { "x": 0.5, "y": 0 },
  "velocity_mode": "directional",
  "direction": -1.5708,
  "spread": { "min": -0.5, "max": 0.5 },
  "speed": { "min": 0.5, "max": 1.5 },
  "gravity": { "x": 0, "y": 1.0 },
  "drag": 0.3,
  "start_size": { "min": 0.15, "max": 0.25 },
  "end_size": { "min": 0.0, "max": 0.05 },
  "start_color": { "r": 160, "g": 140, "b": 100, "a": 200 },
  "end_color": { "r": 140, "g": 120, "b": 80, "a": 0 }
}
```

### Explosion

```json
{
  "name": "explosion",
  "burst_count": 50,
  "max_particles": 60,
  "lifetime": { "min": 0.2, "max": 0.6 },
  "shape": "circle",
  "shape_radius": 0.2,
  "velocity_mode": "radial",
  "speed": { "min": 5.0, "max": 12.0 },
  "gravity": { "x": 0, "y": 3.0 },
  "drag": 0.4,
  "start_size": { "min": 0.3, "max": 0.5 },
  "end_size": { "min": 0.0, "max": 0.0 },
  "size_easing": "out_quad",
  "start_color": { "r": 255, "g": 220, "b": 100, "a": 255 },
  "end_color": { "r": 200, "g": 50, "b": 0, "a": 0 }
}
```

### Magic Trail

```json
{
  "name": "magic_trail",
  "spawn_rate": 30,
  "max_particles": 50,
  "lifetime": { "min": 0.4, "max": 0.8 },
  "shape": "point",
  "velocity_mode": "random",
  "speed": { "min": 0.2, "max": 0.5 },
  "drag": 0.9,
  "start_size": { "min": 0.1, "max": 0.2 },
  "end_size": { "min": 0.0, "max": 0.0 },
  "start_color": { "r": 100, "g": 200, "b": 255, "a": 255 },
  "end_color": { "r": 50, "g": 100, "b": 200, "a": 0 },
  "world_space": true
}
```

### Smoke

```json
{
  "name": "smoke",
  "spawn_rate": 8,
  "max_particles": 30,
  "lifetime": { "min": 1.5, "max": 2.5 },
  "shape": "circle",
  "shape_radius": 0.2,
  "velocity_mode": "directional",
  "direction": -1.5708,
  "spread": { "min": -0.3, "max": 0.3 },
  "speed": { "min": 0.5, "max": 1.0 },
  "gravity": { "x": 0, "y": -0.5 },
  "drag": 0.1,
  "start_size": { "min": 0.3, "max": 0.5 },
  "end_size": { "min": 0.8, "max": 1.2 },
  "start_color": { "r": 80, "g": 80, "b": 80, "a": 150 },
  "end_color": { "r": 60, "g": 60, "b": 60, "a": 0 }
}
```

## Complete Example

```ruby
include GMR

class Player
  def initialize(x, y)
    @transform = Transform2D.new(x: x, y: y)
    @sprite = Sprite.new(Texture.load("player.png"), @transform)

    # Dust emitter for running
    @dust_emitter = ParticleEmitter.new("effects/run_dust.json")

    @on_ground = false
    @moving = false
  end

  def update(dt)
    # Update dust emitter position at feet
    foot_x = @transform.x + 16
    foot_y = @transform.y + 32
    @dust_emitter.position = Mathf::Vec2.new(foot_x, foot_y)

    # Emit dust only when running on ground
    if @on_ground && @moving
      @dust_emitter.start unless @dust_emitter.emitting?
    else
      @dust_emitter.stop if @dust_emitter.emitting?
    end
  end

  def take_damage
    # One-shot hit effect
    ParticleEmitter.emit("effects/hit_sparks.json",
      position: Mathf::Vec2.new(@transform.x + 16, @transform.y + 16))
  end

  def die
    # Death explosion with callback
    ParticleEmitter.emit("effects/death.json", transform: @transform) do
      SceneManager.load(GameOverScene.new)
    end
  end
end
```

## Performance Considerations

- **Pool Size**: Set `max_particles` to the expected maximum needed. Oversizing wastes memory; undersizing causes particles to not spawn.
- **Spawn Rate**: High spawn rates with long lifetimes can fill the pool quickly.
- **World Space**: `world_space: true` is slightly more expensive but produces better results for moving emitters.
- **Preloading**: Use `ParticleEmitter.preload` during scene load to avoid hitches.

## See Also

- [Animation](animation.md) - Tweens for property animation
- [Graphics](graphics.md) - Drawing and rendering
- [Transforms](transforms.md) - Transform2D for positioning
- [API Reference](api/engine/particle/README.md) - Complete Particle API
