include GMR

# MVP Platformer Demo: Procedural Level Generation with Proper Tile Placement

FRAME_WIDTH = 56
FRAME_HEIGHT = 56
COLUMNS = 7

SCREEN_WIDTH = 800
SCREEN_HEIGHT = 600

GRAVITY = 800.0
MOVE_SPEED = 200.0
JUMP_FORCE = -400.0

# Tilemap configuration
# Tileset is 504x264 = 21 columns x 11 rows of 24x24 tiles
TILE_SIZE = 24
TILESET_COLS = 21
MAP_WIDTH = 80
MAP_HEIGHT = 25
MAP_OFFSET_X = -200.0
MAP_OFFSET_Y = 0.0

# Character hitbox offsets (relative to sprite position)
CHAR_HITBOX_OFFSET_X = 15
CHAR_HITBOX_OFFSET_Y = 8
CHAR_HITBOX_WIDTH = 26
CHAR_HITBOX_HEIGHT = 48

# === TILE INDEX DEFINITIONS ===
# Tileset layout analysis (24x24 tiles, 21 columns per row):
# Row 0: Platform tops with grass
# Row 1: Platform middles
# Row 2: Platform bottoms
# Rows 3-4: Vertical strips, columns
# Rows 5-6: Long horizontal platforms
# Rows 7-8: Slopes, decorations
# Rows 9-10: Small pieces

module Tiles
  # Main ground platform pieces (3x3 grid starting at row 0, col 0)
  GROUND_TOP_LEFT     = 0   # Row 0, Col 0 - grass corner top-left
  GROUND_TOP_CENTER   = 1   # Row 0, Col 1 - grass top middle
  GROUND_TOP_RIGHT    = 2   # Row 0, Col 2 - grass corner top-right
  GROUND_MID_LEFT     = 21  # Row 1, Col 0 - dirt left edge
  GROUND_MID_CENTER   = 22  # Row 1, Col 1 - dirt fill
  GROUND_MID_RIGHT    = 23  # Row 1, Col 2 - dirt right edge
  GROUND_BOT_LEFT     = 42  # Row 2, Col 0 - dirt bottom-left
  GROUND_BOT_CENTER   = 43  # Row 2, Col 1 - dirt bottom
  GROUND_BOT_RIGHT    = 44  # Row 2, Col 2 - dirt bottom-right

  # Floating platform pieces (3x3 grid starting at row 0, col 3)
  PLAT_TOP_LEFT       = 3   # Row 0, Col 3
  PLAT_TOP_CENTER     = 4   # Row 0, Col 4
  PLAT_TOP_RIGHT      = 5   # Row 0, Col 5
  PLAT_MID_LEFT       = 24  # Row 1, Col 3
  PLAT_MID_CENTER     = 25  # Row 1, Col 4
  PLAT_MID_RIGHT      = 26  # Row 1, Col 5
  PLAT_BOT_LEFT       = 45  # Row 2, Col 3
  PLAT_BOT_CENTER     = 46  # Row 2, Col 4
  PLAT_BOT_RIGHT      = 47  # Row 2, Col 5

  # Single-tile wide column/pillar pieces
  COLUMN_TOP          = 63  # Row 3, Col 0
  COLUMN_MID          = 84  # Row 4, Col 0
  COLUMN_BOT          = 105 # Row 5, Col 0

  # Thin horizontal platform (1 tile high)
  THIN_LEFT           = 126 # Row 6, Col 0
  THIN_CENTER         = 127 # Row 6, Col 1
  THIN_RIGHT          = 128 # Row 6, Col 2

  # Alternative grass tops (row 0, cols 6-8) - for variety
  ALT_TOP_LEFT        = 6
  ALT_TOP_CENTER      = 7
  ALT_TOP_RIGHT       = 8

  # Inner corners (where ground wraps around)
  INNER_TOP_LEFT      = 9   # Row 0, Col 9
  INNER_TOP_RIGHT     = 11  # Row 0, Col 11
  INNER_BOT_LEFT      = 51  # Row 2, Col 9
  INNER_BOT_RIGHT     = 53  # Row 2, Col 11

  # Small decorative pieces (row 9-10)
  DECO_GRASS_1        = 189 # Row 9
  DECO_GRASS_2        = 190
  DECO_ROCK_1         = 210 # Row 10
  DECO_ROCK_2         = 211
end

# Seeded random number generator for consistent level generation
class SeededRandom
  def initialize(seed)
    @seed = seed
    @current = seed
  end

  def next_int(max)
    # Linear congruential generator
    @current = (@current * 1103515245 + 12345) & 0x7FFFFFFF
    @current % max
  end

  def next_float
    next_int(10000) / 10000.0
  end

  def chance(probability)
    next_float < probability
  end

  def reset
    @current = @seed
  end
end

# Level generator that properly places tiles
class LevelGenerator
  def initialize(tilemap, seed = 12345)
    @tilemap = tilemap
    @rng = SeededRandom.new(seed)
    @width = MAP_WIDTH
    @height = MAP_HEIGHT
    # Track which cells have ground (for proper edge detection)
    @ground_map = Array.new(@height) { Array.new(@width, false) }
  end

  def generate
    @rng.reset
    clear_map

    # Generate ground floor
    generate_ground_floor

    # Generate floating platforms
    generate_platforms

    # Apply proper tile edges
    apply_tile_edges

    # Add decorations
    add_decorations
  end

  private

  def clear_map
    @height.times do |y|
      @width.times do |x|
        @tilemap.set(x, y, -1)
        @ground_map[y][x] = false
      end
    end
  end

  def generate_ground_floor
    # Ground starts at row 20 (leaving room for platforms above)
    ground_y = 20

    # Generate terrain with hills and gaps
    x = 0
    while x < @width
      # Random segment type
      segment_type = @rng.next_int(100)

      if segment_type < 60
        # Flat ground segment
        length = 5 + @rng.next_int(10)
        length = [@width - x, length].min

        length.times do |i|
          mark_ground_column(x + i, ground_y, @height - ground_y)
        end
        x += length

      elsif segment_type < 75
        # Gap (no ground)
        gap_width = 2 + @rng.next_int(3)
        x += gap_width

      elsif segment_type < 90
        # Hill/raised section
        hill_width = 4 + @rng.next_int(6)
        hill_height = 1 + @rng.next_int(3)
        hill_width = [@width - x, hill_width].min

        hill_width.times do |i|
          mark_ground_column(x + i, ground_y - hill_height, @height - (ground_y - hill_height))
        end
        x += hill_width

      else
        # Pit/lower section
        pit_width = 3 + @rng.next_int(4)
        pit_depth = 2 + @rng.next_int(2)
        pit_width = [@width - x, pit_width].min

        pit_width.times do |i|
          mark_ground_column(x + i, ground_y + pit_depth, @height - (ground_y + pit_depth))
        end
        x += pit_width
      end
    end
  end

  def generate_platforms
    # Generate floating platforms at various heights
    platform_count = 15 + @rng.next_int(10)

    platform_count.times do
      plat_x = @rng.next_int(@width - 6)
      plat_y = 5 + @rng.next_int(12)  # Platforms between y=5 and y=17
      plat_width = 3 + @rng.next_int(5)
      plat_height = 1 + @rng.next_int(2)

      # Check if space is clear
      next unless area_clear?(plat_x, plat_y, plat_width, plat_height)

      # Mark platform area
      plat_height.times do |dy|
        plat_width.times do |dx|
          mark_ground(plat_x + dx, plat_y + dy)
        end
      end
    end
  end

  def mark_ground_column(x, start_y, height)
    return if x < 0 || x >= @width
    height.times do |dy|
      y = start_y + dy
      mark_ground(x, y) if y >= 0 && y < @height
    end
  end

  def mark_ground(x, y)
    return if x < 0 || x >= @width || y < 0 || y >= @height
    @ground_map[y][x] = true
  end

  def ground?(x, y)
    return false if x < 0 || x >= @width || y < 0 || y >= @height
    @ground_map[y][x]
  end

  def area_clear?(x, y, w, h)
    # Check area plus 1-tile margin
    ((y - 1)..(y + h)).each do |cy|
      ((x - 1)..(x + w)).each do |cx|
        return false if ground?(cx, cy)
      end
    end
    true
  end

  def apply_tile_edges
    @height.times do |y|
      @width.times do |x|
        next unless ground?(x, y)

        # Determine neighbors
        above = ground?(x, y - 1)
        below = ground?(x, y + 1)
        left  = ground?(x - 1, y)
        right = ground?(x + 1, y)

        # Determine tile based on neighbors
        tile = select_tile(above, below, left, right, x, y)
        @tilemap.set(x, y, tile)
      end
    end
  end

  def select_tile(above, below, left, right, x, y)
    # Top edge (no ground above)
    if !above
      if !left && !right
        # Single column
        return Tiles::PLAT_TOP_CENTER
      elsif !left
        # Left edge of top
        return Tiles::GROUND_TOP_LEFT
      elsif !right
        # Right edge of top
        return Tiles::GROUND_TOP_RIGHT
      else
        # Middle of top
        return Tiles::GROUND_TOP_CENTER
      end
    end

    # Bottom edge (no ground below) - rare for platformers but handle it
    if !below
      if !left
        return Tiles::GROUND_BOT_LEFT
      elsif !right
        return Tiles::GROUND_BOT_RIGHT
      else
        return Tiles::GROUND_BOT_CENTER
      end
    end

    # Middle rows (has ground above and below)
    if !left && !right
      # Thin vertical column
      return Tiles::GROUND_MID_CENTER
    elsif !left
      # Left edge
      return Tiles::GROUND_MID_LEFT
    elsif !right
      # Right edge
      return Tiles::GROUND_MID_RIGHT
    else
      # Interior fill
      return Tiles::GROUND_MID_CENTER
    end
  end

  def add_decorations
    # Add small decorative elements on top of ground
    @width.times do |x|
      (@height - 1).times do |y|
        # Find top surfaces
        next unless ground?(x, y) && !ground?(x, y - 1)

        # Random chance to add decoration
        if @rng.chance(0.1)
          deco_tile = @rng.chance(0.5) ? Tiles::DECO_GRASS_1 : Tiles::DECO_GRASS_2
          # Decorations would go above, but we skip for now as it might overlap with gameplay
        end
      end
    end
  end
end

def init
  # === LEVEL SEED ===
  @level_seed = 42  # Change this to generate different levels

  # === INPUT MAPPING ===
  Input.map(:move_left, [:left, :a])
  Input.map(:move_right, [:right, :d])
  Input.map(:jump, [:space, :up, :w])
  Input.on(:jump) { do_jump }

  # === CAMERA ===
  @camera = Graphics::Camera2D.new
  @camera.offset = Mathf::Vec2.new(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0)
  @camera.zoom = 1.0

  # === PARALLAX BACKGROUNDS ===
  @bg1_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_1.png")
  @bg2_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_2.png")
  @bg3_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_3.png")

  bg_scale = 4.0
  bg_width = 320.0 * bg_scale
  @bg_base_y = -50

  @bg1_sprites = []
  3.times do
    s = Graphics::Sprite.new(@bg1_tex)
    s.scale_x = bg_scale
    s.scale_y = bg_scale
    @bg1_sprites << s
  end
  @bg1_width = bg_width
  @bg1_speed = 0.1

  @bg2_sprites = []
  3.times do
    s = Graphics::Sprite.new(@bg2_tex)
    s.scale_x = bg_scale
    s.scale_y = bg_scale
    @bg2_sprites << s
  end
  @bg2_width = bg_width
  @bg2_speed = 0.3

  @bg3_sprites = []
  3.times do
    s = Graphics::Sprite.new(@bg3_tex)
    s.scale_x = bg_scale
    s.scale_y = bg_scale
    @bg3_sprites << s
  end
  @bg3_width = bg_width
  @bg3_speed = 0.5

  # === TILEMAP ===
  @tileset_tex = Graphics::Texture.load("assets/oak_woods/oak_woods_tileset.png")
  @tilemap = Graphics::Tilemap.new(@tileset_tex, TILE_SIZE, TILE_SIZE, MAP_WIDTH, MAP_HEIGHT)

  # Define tile properties - all ground tiles are solid
  [
    Tiles::GROUND_TOP_LEFT, Tiles::GROUND_TOP_CENTER, Tiles::GROUND_TOP_RIGHT,
    Tiles::GROUND_MID_LEFT, Tiles::GROUND_MID_CENTER, Tiles::GROUND_MID_RIGHT,
    Tiles::GROUND_BOT_LEFT, Tiles::GROUND_BOT_CENTER, Tiles::GROUND_BOT_RIGHT,
    Tiles::PLAT_TOP_LEFT, Tiles::PLAT_TOP_CENTER, Tiles::PLAT_TOP_RIGHT,
    Tiles::PLAT_MID_LEFT, Tiles::PLAT_MID_CENTER, Tiles::PLAT_MID_RIGHT,
    Tiles::PLAT_BOT_LEFT, Tiles::PLAT_BOT_CENTER, Tiles::PLAT_BOT_RIGHT,
    Tiles::ALT_TOP_LEFT, Tiles::ALT_TOP_CENTER, Tiles::ALT_TOP_RIGHT,
    Tiles::THIN_LEFT, Tiles::THIN_CENTER, Tiles::THIN_RIGHT,
    Tiles::COLUMN_TOP, Tiles::COLUMN_MID, Tiles::COLUMN_BOT
  ].each do |tile_id|
    @tilemap.define_tile(tile_id, { solid: true })
  end

  # Generate level
  generator = LevelGenerator.new(@tilemap, @level_seed)
  generator.generate

  # === CHARACTER ===
  @char_tex = Graphics::Texture.load("assets/oak_woods/character/char_blue.png")
  @sprite = Graphics::Sprite.new(@char_tex)
  @sprite.source_rect = Graphics::Rect.new(0, 0, FRAME_WIDTH, FRAME_HEIGHT)

  # Find spawn point (first solid ground from left, at ground level area)
  spawn_x = find_spawn_point
  @sprite.x = MAP_OFFSET_X + spawn_x * TILE_SIZE
  @sprite.y = MAP_OFFSET_Y + 18 * TILE_SIZE - FRAME_HEIGHT

  @velocity_y = 0.0
  @on_ground = false

  @animator = Animation::Animator.new(@sprite,
    columns: COLUMNS,
    frame_width: FRAME_WIDTH,
    frame_height: FRAME_HEIGHT)

  @animator.add(:idle, frames: 0..5, fps: 8)
  @animator.add(:run, frames: 14..19, fps: 12)
  @animator.add(:jump_up, frames: 21..23, fps: 10, loop: false)
  @animator.add(:fall, frames: 24..26, fps: 10, loop: false)
  @animator.play(:idle)

  animator = @animator
  state_machine do
    state :idle do
      enter { animator.play(:idle) }
      on :move, :running
      on :jump, :jumping
    end

    state :running do
      enter { animator.play(:run) }
      on :stop, :idle
      on :jump, :jumping
    end

    state :jumping do
      enter { animator.play(:jump_up) }
      on :fall, :falling
      on :land, :idle
      on :land_moving, :running
    end

    state :falling do
      enter { animator.play(:fall) }
      on :land, :idle
      on :land_moving, :running
    end
  end
end

def find_spawn_point
  # Find first column with ground around y=19-20
  MAP_WIDTH.times do |x|
    (18..21).each do |y|
      if @tilemap.solid?(x, y)
        return x
      end
    end
  end
  5  # Default spawn
end

def do_jump
  return unless @on_ground
  @velocity_y = JUMP_FORCE
  @on_ground = false
  state_machine.trigger(:jump)
end

def update(dt)
  moving_left = Input.action_down?(:move_left)
  moving_right = Input.action_down?(:move_right)
  moving = moving_left || moving_right

  if !@on_ground
    @velocity_y += GRAVITY * dt
    state_machine.trigger(:fall) if @velocity_y > 0
  end

  @sprite.x -= MOVE_SPEED * dt if moving_left
  @sprite.x += MOVE_SPEED * dt if moving_right
  @sprite.flip_x = true if moving_left
  @sprite.flip_x = false if moving_right

  @sprite.y += @velocity_y * dt

  check_tilemap_collision(moving)

  if @on_ground
    state_machine.trigger(moving ? :move : :stop)
  end

  # Update camera to follow player
  @camera.target = Mathf::Vec2.new(@sprite.x + FRAME_WIDTH / 2.0, @sprite.y + FRAME_HEIGHT / 2.0)
end

def check_tilemap_collision(moving)
  # Get hitbox in tilemap local coordinates
  local_x = @sprite.x + CHAR_HITBOX_OFFSET_X - MAP_OFFSET_X
  local_y = @sprite.y + CHAR_HITBOX_OFFSET_Y - MAP_OFFSET_Y

  # Use Collision module to resolve tilemap collision
  result = Collision.tilemap_resolve(
    @tilemap,
    local_x, local_y,
    CHAR_HITBOX_WIDTH, CHAR_HITBOX_HEIGHT,
    @velocity_x || 0.0, @velocity_y
  )

  # Apply resolved position (convert back to world coordinates)
  @sprite.x = result[:x] + MAP_OFFSET_X - CHAR_HITBOX_OFFSET_X
  @sprite.y = result[:y] + MAP_OFFSET_Y - CHAR_HITBOX_OFFSET_Y
  @velocity_y = result[:vy]

  # Update ground state
  if result[:bottom]
    if !@on_ground
      @on_ground = true
      state_machine.trigger(moving ? :land_moving : :land)
    end
  else
    @on_ground = false
  end
end

def draw
  Graphics.clear([80, 120, 160])

  camera_x = @sprite.x + FRAME_WIDTH / 2.0
  camera_y = @sprite.y + FRAME_HEIGHT / 2.0

  base_y = MAP_OFFSET_Y + 18 * TILE_SIZE
  y_offset = camera_y - base_y

  draw_parallax_layer(@bg1_sprites, @bg1_width, @bg1_speed, @bg_base_y - y_offset * 0.05, camera_x)
  draw_parallax_layer(@bg2_sprites, @bg2_width, @bg2_speed, @bg_base_y - y_offset * 0.15, camera_x)
  draw_parallax_layer(@bg3_sprites, @bg3_width, @bg3_speed, @bg_base_y - y_offset * 0.25, camera_x)

  @camera.use do
    @tilemap.draw(MAP_OFFSET_X, MAP_OFFSET_Y)
    @sprite.draw
  end
end

def draw_parallax_layer(sprites, width, speed, y_offset, camera_x)
  offset = -camera_x * speed
  sprites.each_with_index do |s, i|
    s.x = offset + (i - 1) * width
    s.y = y_offset
    s.draw
  end
end
