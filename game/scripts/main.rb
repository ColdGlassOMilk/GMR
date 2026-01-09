include GMR

# MVP Platformer Demo: Procedural Level Generation with Proper Tile Placement

FRAME_WIDTH = 56
FRAME_HEIGHT = 56
COLUMNS = 7

# Virtual resolution (game renders at this size) - retro 16:9
VIRTUAL_WIDTH = 320
VIRTUAL_HEIGHT = 180

# Window size (actual window dimensions)
WINDOW_WIDTH = 960
WINDOW_HEIGHT = 540

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

# Camera Y offset to show ground at bottom of screen
CAMERA_OFFSET_Y = VIRTUAL_HEIGHT * 0.7

# Character hitbox offsets (relative to sprite position)
CHAR_HITBOX_OFFSET_X = 15
CHAR_HITBOX_OFFSET_Y = 8
CHAR_HITBOX_WIDTH = 26
CHAR_HITBOX_HEIGHT = 48

# === TILE INDEX DEFINITIONS ===
module Tiles
  GROUND_TOP_LEFT     = 0
  GROUND_TOP_CENTER   = 1
  GROUND_TOP_RIGHT    = 2
  GROUND_MID_LEFT     = 21
  GROUND_MID_CENTER   = 22
  GROUND_MID_RIGHT    = 23
  GROUND_BOT_LEFT     = 42
  GROUND_BOT_CENTER   = 43
  GROUND_BOT_RIGHT    = 44
  PLAT_TOP_LEFT       = 3
  PLAT_TOP_CENTER     = 4
  PLAT_TOP_RIGHT      = 5
  PLAT_MID_LEFT       = 24
  PLAT_MID_CENTER     = 25
  PLAT_MID_RIGHT      = 26
  PLAT_BOT_LEFT       = 45
  PLAT_BOT_CENTER     = 46
  PLAT_BOT_RIGHT      = 47
  COLUMN_TOP          = 63
  COLUMN_MID          = 84
  COLUMN_BOT          = 105
  THIN_LEFT           = 126
  THIN_CENTER         = 127
  THIN_RIGHT          = 128
  ALT_TOP_LEFT        = 6
  ALT_TOP_CENTER      = 7
  ALT_TOP_RIGHT       = 8
  INNER_TOP_LEFT      = 9
  INNER_TOP_RIGHT     = 11
  INNER_BOT_LEFT      = 51
  INNER_BOT_RIGHT     = 53
  DECO_GRASS_1        = 189
  DECO_GRASS_2        = 190
  DECO_ROCK_1         = 210
  DECO_ROCK_2         = 211
end

def init
  # === WINDOW SETUP (with method chaining) ===
  Window.set_size(WINDOW_WIDTH, WINDOW_HEIGHT)
        .set_virtual_resolution(VIRTUAL_WIDTH, VIRTUAL_HEIGHT)
        .set_filter_point  # Crisp pixel scaling

  # Enable the console with Ruby evaluation (Dev mode)
  Console.enable(height: 150).allow_ruby_eval

  # === INPUT MAPPING (with method chaining) ===
  Input.map(:move_left, [:left, :a])
       .map(:move_right, [:right, :d])
       .map(:jump, [:space, :up, :w])
  Input.on(:jump) { do_jump }

  # === CAMERA ===
  @camera = Graphics::Camera.new
  @camera.offset = Mathf::Vec2.new(VIRTUAL_WIDTH / 2.0, CAMERA_OFFSET_Y)
  @camera.zoom = 1.0

  # === PARALLAX BACKGROUNDS ===
  @bg1_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_1.png")
  @bg2_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_2.png")
  @bg3_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_3.png")

  # Scale backgrounds for retro resolution
  bg_scale = 1.5
  bg_width = 320.0 * bg_scale
  @bg_base_y = -40

  @bg1_sprites = []
  3.times do
    t = Transform2D.new(scale_x: bg_scale, scale_y: bg_scale)
    s = Graphics::Sprite.new(@bg1_tex, t)
    @bg1_sprites << { sprite: s, transform: t }
  end
  @bg1_width = bg_width
  @bg1_speed = 0.1

  @bg2_sprites = []
  3.times do
    t = Transform2D.new(scale_x: bg_scale, scale_y: bg_scale)
    s = Graphics::Sprite.new(@bg2_tex, t)
    @bg2_sprites << { sprite: s, transform: t }
  end
  @bg2_width = bg_width
  @bg2_speed = 0.3

  @bg3_sprites = []
  3.times do
    t = Transform2D.new(scale_x: bg_scale, scale_y: bg_scale)
    s = Graphics::Sprite.new(@bg3_tex, t)
    @bg3_sprites << { sprite: s, transform: t }
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

  # Create simple flat ground
  (0...MAP_WIDTH).each do |x|
    @tilemap.set(x, 19, Tiles::GROUND_TOP_CENTER)
    @tilemap.set(x, 20, Tiles::GROUND_MID_CENTER)
  end

  # === CHARACTER ===
  begin
    @char_tex = Graphics::Texture.load("assets/oak_woods/character/char_blue.png")
  rescue
    @char_tex = @tileset_tex
  end

  # Find spawn point
  spawn_x = 5
  pos_x = MAP_OFFSET_X + spawn_x * TILE_SIZE
  pos_y = MAP_OFFSET_Y + 18 * TILE_SIZE - FRAME_HEIGHT

  @sprite_transform = Transform2D.new(x: pos_x, y: pos_y)
  @sprite = Graphics::Sprite.new(@char_tex, @sprite_transform)
  @sprite.source_rect = Graphics::Rect.new(0, 0, FRAME_WIDTH, FRAME_HEIGHT)

  @velocity_y = 0.0
  @on_ground = false

  # === ANIMATION & STATE MACHINE ===
  begin
    # Use Animator for cleaner animation management
    @animator = Animation::Animator.new(@sprite,
      columns: COLUMNS,
      frame_width: FRAME_WIDTH,
      frame_height: FRAME_HEIGHT)

    @animator.add(:idle, frames: 0..5, fps: 8)
    @animator.add(:run, frames: 14..19, fps: 12)
    @animator.add(:jump_up, frames: 21..23, fps: 10, loop: false)
    @animator.add(:fall, frames: 24..26, fps: 10, loop: false)

    state_machine do
      state :idle do
        animate :idle
        on :move, :running
        on :jump, :jumping
      end

      state :running do
        animate :run
        on :stop, :idle
        on :jump, :jumping
      end

      state :jumping do
        animate :jump_up
        on :fall, :falling
        on :land, :idle
        on :land_moving, :running
      end

      state :falling do
        animate :fall
        on :land, :idle
        on :land_moving, :running
      end
    end
  rescue => e
    # Animation/state machine failed, game will run without it
  end
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

  @sprite_transform.x -= MOVE_SPEED * dt if moving_left
  @sprite_transform.x += MOVE_SPEED * dt if moving_right
  @sprite.flip_x = true if moving_left
  @sprite.flip_x = false if moving_right

  @sprite_transform.y += @velocity_y * dt

  check_tilemap_collision(moving)

  if @on_ground
    state_machine.trigger(moving ? :move : :stop)
  end

  # Update camera to follow player
  @camera.target = Mathf::Vec2.new(@sprite_transform.x + FRAME_WIDTH / 2.0, @sprite_transform.y + FRAME_HEIGHT / 2.0)
end

def check_tilemap_collision(moving)
  # Get hitbox in tilemap local coordinates
  local_x = @sprite_transform.x + CHAR_HITBOX_OFFSET_X - MAP_OFFSET_X
  local_y = @sprite_transform.y + CHAR_HITBOX_OFFSET_Y - MAP_OFFSET_Y

  # Use Collision module to resolve tilemap collision
  result = Collision.tilemap_resolve(
    @tilemap,
    local_x, local_y,
    CHAR_HITBOX_WIDTH, CHAR_HITBOX_HEIGHT,
    @velocity_x || 0.0, @velocity_y
  )

  # Apply resolved position (convert back to world coordinates)
  @sprite_transform.x = result.x + MAP_OFFSET_X - CHAR_HITBOX_OFFSET_X
  @sprite_transform.y = result.y + MAP_OFFSET_Y - CHAR_HITBOX_OFFSET_Y
  @velocity_y = result.vy

  # Update ground state
  if result.bottom?
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

  camera_x = @sprite_transform.x + FRAME_WIDTH / 2.0
  camera_y = @sprite_transform.y + FRAME_HEIGHT / 2.0

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
  sprites.each_with_index do |obj, i|
    obj[:transform].x = offset + (i - 1) * width
    obj[:transform].y = y_offset
    obj[:sprite].draw
  end
end
