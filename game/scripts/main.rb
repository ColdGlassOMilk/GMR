# GMR Tilemap Demo
# Top-down explorer showcasing tilemap features

include GMR

# Constants
TILE_SIZE = 32
MAP_WIDTH = 60
MAP_HEIGHT = 40
SCREEN_WIDTH = 960
SCREEN_HEIGHT = 640

# Tile indices from tilemap.png (18 tiles wide, 32x32 each)
GRASS_TILE = 0       # Basic grass
GRASS_VAR1 = 1       # Grass with small detail
GRASS_VAR2 = 2       # Grass variant
GRASS_FLOWER = 3     # Grass with flowers
STONE_TILE = 6       # Dark stone/brick
STONE_LIGHT = 7      # Lighter stone
WOOD_PLANK = 9       # Wood planks
WOOD_PLANK2 = 10     # Wood planks variant
DIRT_TILE = 5        # Dirt/path
WATER_TILE = 57      # Still water (row 3, col 3)
WATER_EDGE = 56      # Water edge
ROCK_TILE = 93       # Boulder/rock (row 5)
TREE_TRUNK = 36      # Tree area (large tree occupies 36-37, 54-55)

def init
  Window.set_title("GMR Tilemap Demo")
  Window.set_size(SCREEN_WIDTH, SCREEN_HEIGHT)

  # Set up input action mappings
  Input.map(:move_left, [:left, :a])
  Input.map(:move_right, [:right, :d])
  Input.map(:move_up, [:up, :w])
  Input.map(:move_down, [:down, :s])

  # Load tileset texture
  $tileset = Graphics::Texture.load("assets/tilemap.png")

  # Create the tilemap
  $map = Graphics::Tilemap.new($tileset, TILE_SIZE, TILE_SIZE, MAP_WIDTH, MAP_HEIGHT)

  # Build the world
  build_world

  # Define tile properties
  define_tile_properties

  # Player starts in center of map
  $player = {
    x: MAP_WIDTH * TILE_SIZE / 2.0,
    y: MAP_HEIGHT * TILE_SIZE / 2.0,
    speed: 200,
    in_water: false
  }

  # Camera position
  $camera = { x: 0.0, y: 0.0 }

  # Update camera initially
  update_camera
end

def build_world
  # Fill entire map with grass base
  $map.fill(GRASS_TILE)

  # Add some grass variety
  150.times do
    x = System.random_int(0, MAP_WIDTH - 1)
    y = System.random_int(0, MAP_HEIGHT - 1)
    variant = [GRASS_VAR1, GRASS_VAR2, GRASS_FLOWER][System.random_int(0, 2)]
    $map.set(x, y, variant)
  end

  # Stone structure (like a small building/ruins)
  $map.fill_rect(8, 12, 6, 5, STONE_TILE)
  # Hollow out interior
  $map.fill_rect(9, 13, 4, 3, GRASS_TILE)

  # Another stone structure on the right
  $map.fill_rect(45, 8, 5, 4, STONE_LIGHT)
  $map.fill_rect(46, 9, 3, 2, GRASS_TILE)

  # Wood deck/platform
  $map.fill_rect(25, 25, 6, 4, WOOD_PLANK)

  # Dirt paths connecting areas
  # Horizontal path
  $map.fill_rect(14, 14, 25, 2, DIRT_TILE)
  # Vertical path
  $map.fill_rect(28, 10, 2, 20, DIRT_TILE)

  # Water pond (larger area)
  $map.fill_rect(38, 28, 10, 8, WATER_TILE)
  # Smaller water pool
  $map.fill_rect(5, 30, 5, 4, WATER_TILE)

  # Scatter rocks as obstacles
  rock_positions = [
    [20, 8], [22, 10], [35, 5], [50, 15], [12, 25],
    [40, 12], [55, 30], [15, 35], [30, 32], [48, 5],
    [8, 5], [52, 25], [18, 20], [42, 18]
  ]
  rock_positions.each do |pos|
    $map.set(pos[0], pos[1], ROCK_TILE)
  end

  # Place some "trees" (using the large tree trunk tile)
  tree_positions = [
    [3, 3], [25, 5], [50, 3], [55, 20], [10, 32],
    [35, 30], [20, 28], [45, 35], [5, 18], [52, 10]
  ]
  tree_positions.each do |pos|
    $map.set(pos[0], pos[1], TREE_TRUNK)
  end
end

def define_tile_properties
  # Solid tiles (player cannot walk through)
  $map.define_tile(STONE_TILE, { solid: true })
  $map.define_tile(STONE_LIGHT, { solid: true })
  $map.define_tile(ROCK_TILE, { solid: true })
  $map.define_tile(TREE_TRUNK, { solid: true })

  # Water tiles (special property)
  $map.define_tile(WATER_TILE, { water: true })
  $map.define_tile(WATER_EDGE, { water: true })

  # Wood is walkable (no properties needed, defaults to passable)
  # Grass variants are all walkable
  # Dirt path is walkable
end

def update(dt)
  # Check for reinitialization (hot reload safety)
  unless $map && $player && $camera
    init
    return
  end

  # Skip game input when console is open
  return if console_open?

  update_player(dt)
  update_camera
end

def update_player(dt)
  # Get input direction using action mappings
  dx, dy = 0, 0
  dx -= 1 if Input.action_down?(:move_left)
  dx += 1 if Input.action_down?(:move_right)
  dy -= 1 if Input.action_down?(:move_up)
  dy += 1 if Input.action_down?(:move_down)

  # Normalize diagonal movement
  if dx != 0 && dy != 0
    dx *= 0.707
    dy *= 0.707
  end

  # Calculate speed (slower in water)
  speed = $player[:in_water] ? $player[:speed] * 0.5 : $player[:speed]

  # Try to move (separate X and Y for sliding along walls)
  if dx != 0
    new_x = $player[:x] + dx * speed * dt
    tile_x = (new_x / TILE_SIZE).to_i
    tile_y = ($player[:y] / TILE_SIZE).to_i
    $player[:x] = new_x unless $map.solid?(tile_x, tile_y)
  end

  if dy != 0
    new_y = $player[:y] + dy * speed * dt
    tile_x = ($player[:x] / TILE_SIZE).to_i
    tile_y = (new_y / TILE_SIZE).to_i
    $player[:y] = new_y unless $map.solid?(tile_x, tile_y)
  end

  # Clamp to map bounds
  $player[:x] = [[$player[:x], TILE_SIZE].max, (MAP_WIDTH - 1) * TILE_SIZE].min
  $player[:y] = [[$player[:y], TILE_SIZE].max, (MAP_HEIGHT - 1) * TILE_SIZE].min

  # Check if player is in water
  px = ($player[:x] / TILE_SIZE).to_i
  py = ($player[:y] / TILE_SIZE).to_i
  $player[:in_water] = $map.water?(px, py)
end

def update_camera
  # Center camera on player
  $camera[:x] = $player[:x] - SCREEN_WIDTH / 2
  $camera[:y] = $player[:y] - SCREEN_HEIGHT / 2

  # Clamp camera to map bounds
  max_x = MAP_WIDTH * TILE_SIZE - SCREEN_WIDTH
  max_y = MAP_HEIGHT * TILE_SIZE - SCREEN_HEIGHT
  $camera[:x] = [[$camera[:x], 0].max, max_x].min
  $camera[:y] = [[$camera[:y], 0].max, max_y].min
end

def draw
  # Safety check for hot reload
  unless $map && $player && $camera
    Graphics.clear([20, 20, 20])
    Graphics.draw_text("Initializing...", 10, 10, 20, [255, 100, 100])
    return
  end

  # Dark green background (visible if tiles don't cover)
  Graphics.clear([34, 51, 34])

  # Calculate visible tile region
  start_tile_x = ($camera[:x] / TILE_SIZE).to_i
  start_tile_y = ($camera[:y] / TILE_SIZE).to_i
  tiles_wide = (SCREEN_WIDTH / TILE_SIZE) + 2
  tiles_tall = (SCREEN_HEIGHT / TILE_SIZE) + 2

  # Calculate draw offset for smooth scrolling
  draw_x = -($camera[:x] % TILE_SIZE).to_i
  draw_y = -($camera[:y] % TILE_SIZE).to_i

  # Draw only the visible portion of the map (efficient!)
  $map.draw_region(draw_x, draw_y, start_tile_x, start_tile_y, tiles_wide, tiles_tall)

  # Draw player
  draw_player

  # Draw UI overlay
  draw_ui
end

def draw_player
  # Convert player world position to screen position
  screen_x = ($player[:x] - $camera[:x]).to_i
  screen_y = ($player[:y] - $camera[:y]).to_i

  # Player size
  size = 24

  # Player color based on water state
  if $player[:in_water]
    # Blue tint when in water, with "bobbing" effect
    bob = (Math.sin(Time.elapsed * 6) * 2).to_i
    color = [100, 150, 220]
    Graphics.draw_rect(screen_x - size / 2, screen_y - size / 2 + bob, size, size - 4, color)
  else
    # Normal orange/yellow color
    color = [255, 180, 80]
    Graphics.draw_rect(screen_x - size / 2, screen_y - size / 2, size, size, color)
  end

  # Draw a little face/direction indicator
  Graphics.draw_rect(screen_x - 2, screen_y - 6, 4, 4, [50, 50, 50])
end

def draw_ui
  # Semi-transparent background for text readability
  Graphics.draw_rect(5, 5, 280, 100, [0, 0, 0, 150])

  # FPS
  Graphics.draw_text("FPS: #{GMR::Time.fps}", 10, 10, 22, [255, 255, 255])

  # Player tile position
  tile_x = ($player[:x] / TILE_SIZE).to_i
  tile_y = ($player[:y] / TILE_SIZE).to_i
  Graphics.draw_text("Position: #{tile_x}, #{tile_y}", 10, 35, 22, [200, 200, 200])

  # Current tile properties
  props = []
  props << "solid" if $map.solid?(tile_x, tile_y)
  props << "water" if $map.water?(tile_x, tile_y)
  props_text = props.empty? ? "walkable" : props.join(", ")
  Graphics.draw_text("Tile: #{props_text}", 10, 60, 22, [180, 220, 180])

  # World coordinates (smaller text)
  Graphics.draw_text("World: #{$player[:x].to_i}, #{$player[:y].to_i}", 10, 85, 18, [150, 150, 150])

  # Controls hint at bottom
  Graphics.draw_rect(5, SCREEN_HEIGHT - 35, 400, 30, [0, 0, 0, 150])
  Graphics.draw_text("WASD/Arrows: Move | `: Console | Water slows movement", 10, SCREEN_HEIGHT - 30, 18, [180, 180, 180])
end
