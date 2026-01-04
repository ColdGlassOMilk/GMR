# GMR Enhanced Tilemap Demo - Technical Showcase
# Demonstrates ALL major engine capabilities

include GMR

# =============================================================================
# Constants
# =============================================================================
TILE_SIZE = 32
MAP_WIDTH = 60
MAP_HEIGHT = 40
SCREEN_WIDTH = 960
SCREEN_HEIGHT = 640

# Tile indices from tilemap.png (18 tiles wide, 32x32 each)
# Note: Columns 0-2 of rows 0-2 are the large tree sprite

# Row 0: Grass starts at column 3
GRASS_TILE = 3        # Plain grass (first grass tile)
GRASS_VAR1 = 4        # Grass with small details
GRASS_VAR2 = 5        # Grass variation
STONE_TILE = 6        # Dark cobblestone
STONE_LIGHT = 7       # Lighter cobblestone
WOOD_PLANK = 8        # Light wood plank floor
WOOD_PLANK2 = 9       # Wood plank variation
DIRT_TILE = 12        # Tan/beige floor

# Row 1: More grass variations
GRASS_FLOWER = 21     # Grass with yellow flowers
GRASS_DECOR = 22      # Grass with decoration

# Water tiles - the cyan/blue wavy water
# Row 3 (starts at 54), water is around columns 8-12
# Row 3, col 9 = 54 + 9 = 63
WATER_TILE = 124       # Main water tile (wavy blue pattern)
WATER_LIGHT = 118      # Lighter water variation
WATER_EDGE = 45       # Water edge/foam

# Obstacles
ROCK_TILE = 90        # Gray boulder (row 5)
TREE_TRUNK = 36       # Tree trunk (large tree, row 2 col 0-2)
FENCE_TILE = 75       # Wooden fence (row 4)

# Decorative
CHEST_TILE = 72       # Chest on grass (row 4)
SIGN_TILE = 73        # Wooden sign (row 4)
STUMP_TILE = 78       # Tree stump brown (row 4)
MUSHROOM_TILE = 39    # Grass with mushroom (row 2)

# Stairs/platforms
STAIRS_TILE = 51      # Gray stone stairs

# Player tile
PLAYER_TILE = 72      # Chest tile as placeholder

# Gem/collectible tile
GEM_TILE = 72         # Chest

# =============================================================================
# Player Class - Demonstrates Sprite API
# =============================================================================
class Player
  attr_accessor :x, :y, :speed, :in_water, :rotation
  attr_reader :sprite, :was_in_water, :hit_wall

  def initialize(texture, x, y, tile_size)
    @x = x.to_f
    @y = y.to_f
    @speed = 200
    @in_water = false
    @was_in_water = false
    @rotation = 0  # Degrees, 0 = facing up
    @hit_wall = false
    @tile_size = tile_size

    # Create sprite from tileset - use a distinct tile for the player
    player_tile = 72  # Pick a visible character-like tile
    tile_col = player_tile % 18
    tile_row = player_tile / 18
    @sprite = Sprite.new(texture)
    @sprite.source_rect = Rect.new(tile_col * tile_size, tile_row * tile_size, tile_size, tile_size)
    @sprite.origin = Vec2.new(tile_size / 2, tile_size / 2)
    @sprite.color = [255, 200, 100, 255]  # Orange tint to stand out
    update_sprite
  end

  # Required for Camera2D.follow
  def position
    Vec2.new(@x, @y)
  end

  def rotate(amount)
    @rotation = (@rotation + amount) % 360
  end

  def update(dt, map)
    @hit_wall = false
    @was_in_water = @in_water

    # Get input direction (relative to player facing)
    forward, strafe = 0, 0
    forward -= 1 if Input.action_down?(:move_up)    # W = forward
    forward += 1 if Input.action_down?(:move_down)  # S = backward
    strafe -= 1 if Input.action_down?(:move_left)   # A = strafe left
    strafe += 1 if Input.action_down?(:move_right)  # D = strafe right

    # Normalize diagonal movement
    if forward != 0 && strafe != 0
      forward *= 0.707
      strafe *= 0.707
    end

    # Convert rotation to radians for movement calculation
    # Sprite rotation: 0 = facing up visually (negative Y direction on screen)
    # Standard math: angle 0 = right (+X), angle 90 = down (+Y)
    # We want: rotation 0 = up (-Y), rotation 90 = right (+X)
    # So we offset by +90 degrees to convert from "up-is-zero" to standard math coords
    rad = (@rotation + 90) * Math::PI / 180

    # Forward direction (where the arrow points / where W moves you)
    # Negated because screen Y is inverted (down is positive)
    dir_x = -Math.cos(rad)
    dir_y = -Math.sin(rad)

    # Right direction (perpendicular, for strafing)
    right_x = Math.sin(rad)
    right_y = -Math.cos(rad)

    # W (forward=-1) moves forward, S (forward=+1) moves backward
    # A (strafe=-1) strafes left, D (strafe=+1) strafes right
    dx = -forward * dir_x + strafe * right_x
    dy = -forward * dir_y + strafe * right_y

    # Speed modifier in water
    current_speed = @in_water ? @speed * 0.5 : @speed

    # Try horizontal movement
    if dx != 0
      new_x = @x + dx * current_speed * dt
      tile_x = (new_x / @tile_size).to_i
      tile_y = (@y / @tile_size).to_i
      if map.solid?(tile_x, tile_y)
        @hit_wall = true
      else
        @x = new_x
      end
    end

    # Try vertical movement
    if dy != 0
      new_y = @y + dy * current_speed * dt
      tile_x = (@x / @tile_size).to_i
      tile_y = (new_y / @tile_size).to_i
      if map.solid?(tile_x, tile_y)
        @hit_wall = true
      else
        @y = new_y
      end
    end

    # Clamp to map bounds
    @x = clamp(@x, @tile_size, (MAP_WIDTH - 1) * @tile_size)
    @y = clamp(@y, @tile_size, (MAP_HEIGHT - 1) * @tile_size)

    # Check water state
    px = (@x / @tile_size).to_i
    py = (@y / @tile_size).to_i
    @in_water = map.water?(px, py)

    update_sprite
  end

  def update_sprite
    @sprite.x = @x
    @sprite.y = @y
    @sprite.rotation = @rotation

    # Color tint based on water state
    if @in_water
      # Blue tint and bobbing when in water
      bob = Math.sin(GMR::Time.elapsed * 6) * 2
      @sprite.y = @y + bob
      @sprite.color = [100, 180, 255, 255]
    else
      @sprite.color = [255, 255, 255, 255]
    end
  end

  def draw
    @sprite.draw
    # Draw a colored indicator with rotation
    size = 20
    color = @in_water ? [100, 180, 255] : [255, 180, 80]
    Graphics.draw_rect_rotated(@x.to_i, @y.to_i, size, size, @rotation, color)

    # Draw direction indicator (arrow pointing forward direction)
    # Use the same math as movement
    rad = (@rotation + 90) * Math::PI / 180
    arrow_len = 14
    # Arrow points where W will take you: forward direction (negated for screen coords)
    arrow_x = @x - Math.cos(rad) * arrow_len
    arrow_y = @y - Math.sin(rad) * arrow_len
    Graphics.draw_line_thick(@x.to_i, @y.to_i, arrow_x.to_i, arrow_y.to_i, 3, [40, 40, 40])
  end

  def just_entered_water?
    @in_water && !@was_in_water
  end
end

# =============================================================================
# Particle System - Demonstrates Graphics Primitives
# =============================================================================
class ParticleSystem
  def initialize
    @particles = []
  end

  def spawn(x, y, count, color, opts = {})
    speed_min = opts[:speed_min] || 50
    speed_max = opts[:speed_max] || 150
    life = opts[:life] || 1.0
    size_min = opts[:size_min] || 3
    size_max = opts[:size_max] || 8
    gravity = opts[:gravity] || 200

    count.times do
      angle = System.random_int(0, 360) * Math::PI / 180
      speed = System.random_int(speed_min, speed_max)
      @particles << {
        x: x.to_f,
        y: y.to_f,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed,
        life: life + System.random_float * 0.5,
        max_life: life + 0.5,
        color: color,
        size: System.random_int(size_min, size_max),
        gravity: gravity
      }
    end
  end

  def spawn_water_splash(x, y)
    spawn(x, y, 12, [100, 180, 255],
      speed_min: 30, speed_max: 100,
      life: 0.6, size_min: 2, size_max: 5,
      gravity: 150
    )
  end

  def spawn_collect(x, y)
    spawn(x, y, 20, [255, 220, 50],
      speed_min: 80, speed_max: 200,
      life: 0.8, size_min: 3, size_max: 7,
      gravity: 100
    )
  end

  def spawn_click(x, y)
    # Rainbow burst
    8.times do |i|
      hue = (i * 45) % 360
      color = hue_to_rgb(hue, 1.0, 1.0)
      spawn(x, y, 3, color,
        speed_min: 60, speed_max: 120,
        life: 0.7, size_min: 4, size_max: 8,
        gravity: 80
      )
    end
  end

  def update(dt)
    @particles.each do |p|
      p[:x] += p[:vx] * dt
      p[:y] += p[:vy] * dt
      p[:vy] += p[:gravity] * dt
      p[:life] -= dt
    end
    @particles.reject! { |p| p[:life] <= 0 }
  end

  def draw
    @particles.each do |p|
      alpha = (255 * (p[:life] / p[:max_life])).to_i
      alpha = clamp(alpha, 0, 255)
      color = [p[:color][0], p[:color][1], p[:color][2], alpha]
      Graphics.draw_circle(p[:x].to_i, p[:y].to_i, p[:size], color)
    end
  end

  def count
    @particles.size
  end
end

# =============================================================================
# Collectible Manager - Demonstrates Collision Detection
# =============================================================================
class CollectibleManager
  attr_reader :collected_count, :total_count

  def initialize(texture, tile_size)
    @texture = texture
    @tile_size = tile_size
    @collectibles = []
    @collected_count = 0
    @total_count = 0
    @bob_offset = 0
  end

  def spawn(map, count)
    @total_count = count
    count.times do
      attempts = 0
      loop do
        x = System.random_int(3, MAP_WIDTH - 3) * @tile_size + @tile_size / 2
        y = System.random_int(3, MAP_HEIGHT - 3) * @tile_size + @tile_size / 2
        tx = (x / @tile_size).to_i
        ty = (y / @tile_size).to_i

        # Don't spawn on solid or water tiles
        if !map.solid?(tx, ty) && !map.water?(tx, ty)
          sprite = Sprite.new(@texture)
          sprite.source_rect = Rect.new((GEM_TILE % 18) * @tile_size, (GEM_TILE / 18) * @tile_size, @tile_size, @tile_size)
          sprite.origin = Vec2.new(@tile_size / 2, @tile_size / 2)
          sprite.x = x
          sprite.y = y
          @collectibles << { sprite: sprite, x: x, y: y, collected: false, bob_phase: System.random_float * Math::PI * 2 }
          break
        end
        attempts += 1
        break if attempts > 100  # Prevent infinite loop
      end
    end
  end

  def update(dt, player, camera, particles)
    @bob_offset = GMR::Time.elapsed * 4

    @collectibles.each do |gem|
      next if gem[:collected]

      # Circle collision using GMR::Collision module
      if Collision.circle_overlap?(player.x, player.y, 14, gem[:x], gem[:y], 12)
        gem[:collected] = true
        @collected_count += 1
        camera.shake(strength: 2, duration: 0.15)
        particles.spawn_collect(gem[:x], gem[:y])
      end

      # Update sprite bobbing
      bob = Math.sin(@bob_offset + gem[:bob_phase]) * 4
      gem[:sprite].y = gem[:y] + bob

      # Pulsing scale effect
      scale = 1.0 + Math.sin(@bob_offset * 2 + gem[:bob_phase]) * 0.1
      gem[:sprite].scale_x = scale
      gem[:sprite].scale_y = scale

      # Color shimmer
      hue_shift = ((@bob_offset * 30 + gem[:bob_phase] * 30) % 60) - 30
      gem[:sprite].color = hue_to_rgb(50 + hue_shift, 0.8, 1.0) + [255]
    end
  end

  def draw
    @collectibles.each do |gem|
      gem[:sprite].draw unless gem[:collected]
    end
  end
end

# =============================================================================
# Day/Night Cycle System
# =============================================================================
class DayNightCycle
  NIGHT_COLOR = [60, 60, 100]
  DAWN_COLOR = [255, 200, 170]
  DAY_COLOR = [255, 255, 255]
  DUSK_COLOR = [255, 180, 140]

  def initialize(cycle_duration = 60.0)
    @cycle_duration = cycle_duration
    @time = 0.25  # Start at dawn
  end

  def update(dt)
    @time = (@time + dt / @cycle_duration) % 1.0
  end

  def tint
    if @time < 0.25
      # Night to dawn (0.0 - 0.25)
      t = @time / 0.25
      lerp_color(NIGHT_COLOR, DAWN_COLOR, ease_in_out(t))
    elsif @time < 0.5
      # Dawn to day (0.25 - 0.5)
      t = (@time - 0.25) / 0.25
      lerp_color(DAWN_COLOR, DAY_COLOR, ease_in_out(t))
    elsif @time < 0.75
      # Day to dusk (0.5 - 0.75)
      t = (@time - 0.5) / 0.25
      lerp_color(DAY_COLOR, DUSK_COLOR, ease_in_out(t))
    else
      # Dusk to night (0.75 - 1.0)
      t = (@time - 0.75) / 0.25
      lerp_color(DUSK_COLOR, NIGHT_COLOR, ease_in_out(t))
    end
  end

  def time_name
    case (@time * 4).to_i
    when 0 then "Night"
    when 1 then "Dawn"
    when 2 then "Day"
    when 3 then "Dusk"
    end
  end

  def time_value
    @time
  end
end

# =============================================================================
# Minimap Renderer
# =============================================================================
class Minimap
  SIZE = 140
  MARGIN = 10

  def initialize(map, tile_size)
    @map = map
    @tile_size = tile_size
    @scale_x = SIZE.to_f / (MAP_WIDTH * tile_size)
    @scale_y = SIZE.to_f / (MAP_HEIGHT * tile_size)
  end

  def draw(player, camera)
    x = SCREEN_WIDTH - SIZE - MARGIN
    y = MARGIN

    # Background with border
    Graphics.draw_rect(x - 3, y - 3, SIZE + 6, SIZE + 6, [0, 0, 0, 220])
    Graphics.draw_rect_outline(x - 3, y - 3, SIZE + 6, SIZE + 6, [100, 100, 100, 200])

    # Draw simplified terrain (sample every few tiles for performance)
    step = 2
    rect_size = (step * @tile_size * @scale_x).to_i + 1

    ty = 0
    while ty < MAP_HEIGHT
      tx = 0
      while tx < MAP_WIDTH
        mini_x = x + (tx * @tile_size * @scale_x).to_i
        mini_y = y + (ty * @tile_size * @scale_y).to_i

        if @map.water?(tx, ty)
          Graphics.draw_rect(mini_x, mini_y, rect_size, rect_size, [50, 100, 180, 200])
        elsif @map.solid?(tx, ty)
          Graphics.draw_rect(mini_x, mini_y, rect_size, rect_size, [80, 80, 80, 200])
        else
          Graphics.draw_rect(mini_x, mini_y, rect_size, rect_size, [60, 120, 60, 200])
        end
        tx += step
      end
      ty += step
    end

    # Camera viewport rectangle
    cam_x = x + (camera.target.x - SCREEN_WIDTH / 2 / camera.zoom) * @scale_x
    cam_y = y + (camera.target.y - SCREEN_HEIGHT / 2 / camera.zoom) * @scale_y
    cam_w = (SCREEN_WIDTH * @scale_x / camera.zoom).to_i
    cam_h = (SCREEN_HEIGHT * @scale_y / camera.zoom).to_i
    Graphics.draw_rect_outline(cam_x.to_i, cam_y.to_i, cam_w, cam_h, [255, 255, 255, 180])

    # Player position
    px = x + (player.x * @scale_x).to_i
    py = y + (player.y * @scale_y).to_i
    Graphics.draw_circle(px, py, 4, [255, 255, 0])
    Graphics.draw_circle_outline(px, py, 4, [0, 0, 0])

    # Label
    Graphics.draw_text("Minimap", x, y + SIZE + 5, 14, [180, 180, 180])
  end
end

# =============================================================================
# Main Game Functions
# =============================================================================
def init
  Window.set_title("GMR Enhanced Demo - Technical Showcase")
  Window.set_size(SCREEN_WIDTH, SCREEN_HEIGHT)

  # Enable virtual resolution for auto letterboxing
  Window.set_virtual_resolution(SCREEN_WIDTH, SCREEN_HEIGHT)
  Window.set_filter_point  # Crisp pixel-art style

  # Input mappings
  Input.map(:move_left, [:left, :a])
  Input.map(:move_right, [:right, :d])
  Input.map(:move_up, [:up, :w])
  Input.map(:move_down, [:down, :s])

  # Load tileset
  $tileset = Graphics::Texture.load("assets/tilemap.png")

  # Create tilemap
  $map = Graphics::Tilemap.new($tileset, TILE_SIZE, TILE_SIZE, MAP_WIDTH, MAP_HEIGHT)
  build_world
  define_tile_properties

  # Create player (center of map)
  $player = Player.new($tileset, MAP_WIDTH * TILE_SIZE / 2.0, MAP_HEIGHT * TILE_SIZE / 2.0, TILE_SIZE)

  # Create Camera2D with smooth following
  $camera = Camera2D.new
  $camera.target = $player.position
  $camera.offset = Vec2.new(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2)
  $camera.zoom = 1.0
  $camera.rotation = 0

  # Systems
  $particles = ParticleSystem.new
  $collectibles = CollectibleManager.new($tileset, TILE_SIZE)
  $collectibles.spawn($map, 25)
  $day_night = DayNightCycle.new(90)  # 90 second cycle
  $minimap = Minimap.new($map, TILE_SIZE)

  # State
  $mouse_world = Vec2.new(0, 0)
end

def build_world
  # Fill with grass base
  $map.fill(GRASS_TILE)

  # Add grass variety
  200.times do
    x = System.random_int(0, MAP_WIDTH - 1)
    y = System.random_int(0, MAP_HEIGHT - 1)
    variant = [GRASS_VAR1, GRASS_VAR2, GRASS_FLOWER, MUSHROOM_TILE][System.random_int(0, 3)]
    $map.set(x, y, variant)
  end

  # Stone ruins
  $map.fill_rect(8, 12, 6, 5, STONE_TILE)
  $map.fill_rect(9, 13, 4, 3, GRASS_TILE)

  $map.fill_rect(45, 8, 5, 4, STONE_LIGHT)
  $map.fill_rect(46, 9, 3, 2, GRASS_TILE)

  # Large stone fortress
  $map.fill_rect(3, 3, 8, 6, STONE_TILE)
  $map.fill_rect(4, 4, 6, 4, GRASS_TILE)

  # Wood platforms
  $map.fill_rect(25, 25, 6, 4, WOOD_PLANK)
  $map.fill_rect(50, 30, 4, 3, WOOD_PLANK2)

  # Dirt paths
  $map.fill_rect(14, 14, 25, 2, DIRT_TILE)
  $map.fill_rect(28, 10, 2, 20, DIRT_TILE)
  $map.fill_rect(10, 20, 15, 2, DIRT_TILE)

  # Water areas
  $map.fill_rect(38, 28, 12, 10, WATER_TILE)
  $map.fill_rect(5, 32, 6, 5, WATER_TILE)
  $map.fill_rect(52, 5, 5, 4, WATER_TILE)

  # Rocks
  rock_positions = [
    [20, 8], [22, 10], [35, 5], [50, 15], [12, 25],
    [40, 12], [55, 30], [15, 35], [30, 32], [48, 5],
    [8, 5], [52, 25], [18, 20], [42, 18], [33, 8],
    [15, 28], [44, 22], [25, 35], [38, 15], [10, 8]
  ]
  rock_positions.each { |pos| $map.set(pos[0], pos[1], ROCK_TILE) }

  # Trees
  tree_positions = [
    [3, 10], [25, 5], [50, 3], [55, 20], [10, 35],
    [35, 33], [20, 30], [45, 35], [5, 20], [52, 12],
    [30, 3], [40, 25], [15, 5], [58, 35], [2, 28],
    [48, 18], [22, 22], [36, 10], [12, 17], [55, 8]
  ]
  tree_positions.each { |pos| $map.set(pos[0], pos[1], TREE_TRUNK) }

  # Tree stumps (decorative, not solid)
  stump_positions = [
    [7, 15], [32, 20], [45, 12], [18, 32], [53, 28]
  ]
  stump_positions.each { |pos| $map.set(pos[0], pos[1], STUMP_TILE) }
end

def define_tile_properties
  # Solid tiles
  $map.define_tile(STONE_TILE, { solid: true })
  $map.define_tile(STONE_LIGHT, { solid: true })
  $map.define_tile(ROCK_TILE, { solid: true })
  $map.define_tile(TREE_TRUNK, { solid: true })

  # Water tiles
  $map.define_tile(WATER_TILE, { water: true })
  $map.define_tile(WATER_EDGE, { water: true })
end

def update(dt)
  # Hot reload safety
  unless $map && $player && $camera
    init
    return
  end

  return if console_open?

  # Update player
  $player.update(dt, $map)

  # Camera follows player with smoothing
  $camera.target = lerp_vec($camera.target, $player.position, 0.08)

  # Clamp camera to world bounds
  half_w = SCREEN_WIDTH / 2 / $camera.zoom
  half_h = SCREEN_HEIGHT / 2 / $camera.zoom
  $camera.target = Vec2.new(
    clamp($camera.target.x, half_w, MAP_WIDTH * TILE_SIZE - half_w),
    clamp($camera.target.y, half_h, MAP_HEIGHT * TILE_SIZE - half_h)
  )

  # Zoom with mouse wheel
  wheel = Input.mouse_wheel
  if wheel != 0
    $camera.zoom = clamp($camera.zoom + wheel * 0.15, 0.5, 2.5)
  end

  # Rotation with Q/E - rotates player, not camera
  if Input.key_down?(:q)
    $player.rotate(-120 * dt)
  end
  if Input.key_down?(:e)
    $player.rotate(120 * dt)
  end

  # Reset camera zoom with R
  if Input.key_pressed?(:r)
    $camera.zoom = 1.0
  end

  # Fullscreen toggle
  if Input.key_pressed?(:f11)
    Window.toggle_fullscreen
  end

  # Screen shake events
  if $player.just_entered_water?
    $camera.shake(strength: 6, duration: 0.25)
    $particles.spawn_water_splash($player.x, $player.y)
  end

  if $player.hit_wall
    $camera.shake(strength: 3, duration: 0.1)
  end

  # Mouse world position
  $mouse_world = $camera.screen_to_world(Vec2.new(Input.mouse_x, Input.mouse_y))

  # Mouse click to spawn particles
  if Input.mouse_pressed?(:left)
    $particles.spawn_click($mouse_world.x, $mouse_world.y)
  end

  # Update systems
  $particles.update(dt)
  $collectibles.update(dt, $player, $camera, $particles)
  $day_night.update(dt)
end

def draw
  unless $map && $player && $camera
    Graphics.clear([20, 20, 20])
    Graphics.draw_text("Initializing...", 10, 10, 24, [255, 100, 100])
    return
  end

  # Clear with dark color
  Graphics.clear([20, 30, 20])

  # Get day/night tint
  tint = $day_night.tint

  # Draw world with camera transform
  $camera.use do
    # Calculate visible tile region based on camera
    start_x = (($camera.target.x - SCREEN_WIDTH / 2 / $camera.zoom) / TILE_SIZE).to_i - 1
    start_y = (($camera.target.y - SCREEN_HEIGHT / 2 / $camera.zoom) / TILE_SIZE).to_i - 1
    end_x = start_x + (SCREEN_WIDTH / TILE_SIZE / $camera.zoom).to_i + 3
    end_y = start_y + (SCREEN_HEIGHT / TILE_SIZE / $camera.zoom).to_i + 3

    start_x = clamp(start_x, 0, MAP_WIDTH - 1)
    start_y = clamp(start_y, 0, MAP_HEIGHT - 1)
    end_x = clamp(end_x, 0, MAP_WIDTH)
    end_y = clamp(end_y, 0, MAP_HEIGHT)

    # Draw tilemap at world position (camera transforms it)
    # draw_region(draw_x, draw_y, tile_start_x, tile_start_y, tiles_wide, tiles_tall, color)
    $map.draw_region(
      start_x * TILE_SIZE, start_y * TILE_SIZE,
      start_x, start_y,
      end_x - start_x, end_y - start_y,
      tint
    )

    # Draw collectibles
    $collectibles.draw

    # Draw particles
    $particles.draw

    # Draw player
    $player.draw

    # Draw mouse cursor indicator in world space
    Graphics.draw_circle_outline($mouse_world.x.to_i, $mouse_world.y.to_i, 8, [255, 255, 255, 150])
  end

  # Draw UI (screen space, after camera)
  draw_ui

  # Draw minimap
  $minimap.draw($player, $camera)
end

def draw_ui
  # Main info panel
  panel_w = 320
  panel_h = 200
  Graphics.draw_rect(8, 8, panel_w, panel_h, [0, 0, 0, 180])
  Graphics.draw_rect_outline(8, 8, panel_w, panel_h, [80, 80, 80])

  y = 14
  line = 20

  # Title
  Graphics.draw_text("GMR Engine Technical Showcase", 14, y, 18, [255, 220, 100])
  y += line + 4

  # FPS
  fps_color = GMR::Time.fps >= 55 ? [100, 255, 100] : [255, 100, 100]
  Graphics.draw_text("FPS: #{GMR::Time.fps}", 14, y, 16, fps_color)
  y += line

  # Player position
  tile_x = ($player.x / TILE_SIZE).to_i
  tile_y = ($player.y / TILE_SIZE).to_i
  Graphics.draw_text("Player: #{$player.x.to_i}, #{$player.y.to_i} [tile #{tile_x},#{tile_y}]", 14, y, 16, [200, 200, 200])
  y += line

  # Tile info
  props = []
  props << "solid" if $map.solid?(tile_x, tile_y)
  props << "water" if $map.water?(tile_x, tile_y)
  props_text = props.empty? ? "walkable" : props.join(", ")
  Graphics.draw_text("Tile: #{props_text}", 14, y, 16, [180, 220, 180])
  y += line

  # Camera and player info
  Graphics.draw_text("Camera: zoom=#{$camera.zoom.round(2)}x | Player rot=#{$player.rotation.to_i}deg", 14, y, 16, [180, 180, 220])
  y += line

  # Collectibles
  Graphics.draw_text("Gems: #{$collectibles.collected_count}/#{$collectibles.total_count}", 14, y, 16, [255, 220, 100])
  y += line

  # Day/night
  Graphics.draw_text("Time: #{$day_night.time_name} (#{($day_night.time_value * 100).to_i}%)", 14, y, 16, [200, 180, 255])
  y += line

  # Particles
  Graphics.draw_text("Particles: #{$particles.count}", 14, y, 16, [255, 180, 180])
  y += line

  # Mouse world position
  mx = $mouse_world.x.to_i
  my = $mouse_world.y.to_i
  Graphics.draw_text("Mouse World: #{mx}, #{my}", 14, y, 16, [180, 180, 180])

  # Controls panel at bottom
  ctrl_y = SCREEN_HEIGHT - 65
  Graphics.draw_rect(8, ctrl_y, 600, 57, [0, 0, 0, 180])
  Graphics.draw_rect_outline(8, ctrl_y, 600, 57, [80, 80, 80])

  Graphics.draw_text("Controls:", 14, ctrl_y + 6, 16, [255, 220, 100])
  Graphics.draw_text("W/S: Forward/Back | A/D: Strafe | Q/E: Turn | Mouse Wheel: Zoom | R: Reset Zoom", 14, ctrl_y + 24, 14, [180, 180, 180])
  Graphics.draw_text("Left Click: Spawn Particles | F11: Fullscreen | `: Console", 14, ctrl_y + 40, 14, [180, 180, 180])

  # Fullscreen indicator
  if Window.fullscreen?
    Graphics.draw_text("[FULLSCREEN]", SCREEN_WIDTH - 120, 14, 14, [100, 255, 100])
  end
end

# Helper to lerp Vec2
def lerp_vec(a, b, t)
  Vec2.new(lerp(a.x, b.x, t), lerp(a.y, b.y, t))
end
