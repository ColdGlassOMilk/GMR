include GMR

# MVP Platformer Demo: World-Space Coordinates
# This demo uses world units where 1 unit = 1 tile = 24 pixels

# === WORLD-SPACE CONFIGURATION ===
# view_height: how many world units are visible vertically (PPU is calculated automatically)
# At 180px viewport height with 7.5 view_height: PPU = 180/7.5 = 24
VIEW_HEIGHT = 7.5

# Sprite frame dimensions (in pixels for texture sampling)
FRAME_WIDTH_PX = 56
FRAME_HEIGHT_PX = 56
COLUMNS = 7

# Asset PPU: pixels per unit at which assets are designed (24px = 1 world unit)
ASSET_PPU = 24.0

# Sprite frame dimensions in world units
FRAME_WIDTH = FRAME_WIDTH_PX / ASSET_PPU   # ~2.33 units
FRAME_HEIGHT = FRAME_HEIGHT_PX / ASSET_PPU # ~2.33 units

# Virtual resolution for retro mode
RETRO_WIDTH = 128
RETRO_HEIGHT = 128

# Window size (actual window dimensions)
WINDOW_WIDTH = 960
WINDOW_HEIGHT = 540

# Native mode uses actual window size (no virtual resolution)

# Physics in world units per second
GRAVITY = 33.33        # ~800 pixels/sec^2 / 24 = 33.33 units/sec^2
MOVE_SPEED = 8.33      # ~200 pixels/sec / 24 = 8.33 units/sec
JUMP_FORCE = -16.67    # ~-400 pixels/sec / 24 = -16.67 units/sec

# Audio
SFX_VOLUME = 0.5
MUSIC_VOLUME = 0.7

# Tilemap configuration (in tiles/world units)
# Tileset is 504x264 = 21 columns x 11 rows of 24x24 pixel tiles
TILE_SIZE_PX = 24      # Pixel size for texture sampling
TILE_SIZE = 1.0        # World size: 1 tile = 1 world unit
TILESET_COLS = 21
MAP_WIDTH = 80         # Map width in tiles
MAP_HEIGHT = 25        # Map height in tiles
MAP_OFFSET_X = -8.33   # -200 pixels / 24 = -8.33 units
MAP_OFFSET_Y = 0.0

# Camera Y offset to show ground at bottom of screen (percentage of virtual height)
CAMERA_OFFSET_Y_PERCENT = 0.7

# Camera smoothing and deadzone (deadzone now in world units)
CAMERA_SMOOTHING = 0.85
CAMERA_DEADZONE_WIDTH = 1.0    # ~24 pixels / 24 = 1 unit
CAMERA_DEADZONE_HEIGHT = 0.5   # ~12 pixels / 24 = 0.5 units

# Parallax layer configuration (world units)
# Background textures are 320x180 pixels, which at 24 ASSET_PPU = 13.33 x 7.5 world units
# We'll use the actual texture size for proper tiling
BG_LAYER_WIDTH = 320.0 / ASSET_PPU   # ~13.33 world units per panel
BG_LAYER_Y = -1.67                          # Y position in world units (-40 pixels / 24)
BG_PARALLAX_1 = 0.1                         # Distant sky layer (moves slowest)
BG_PARALLAX_2 = 0.3                         # Middle tree layer
BG_PARALLAX_3 = 0.5                         # Foreground bushes (moves fastest)

# Screen shake settings (now in world units)
ATTACK_SHAKE_STRENGTH = 0.25   # 6 pixels / 24 = 0.25 units
ATTACK_SHAKE_DURATION = 0.30

# Character hitbox (in world units)
CHAR_HITBOX_OFFSET_X = 0.625   # 15 pixels / 24
CHAR_HITBOX_OFFSET_Y = 0.33    # 8 pixels / 24
CHAR_HITBOX_WIDTH = 1.08       # 26 pixels / 24
CHAR_HITBOX_HEIGHT = 2.0       # 48 pixels / 24

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
  # === RESOLUTION MODE ===
  @retro_mode = false  # Start in native mode (no virtual resolution)

  # === WINDOW SETUP (with method chaining) ===
  Window.set_size(WINDOW_WIDTH, WINDOW_HEIGHT)
        .set_filter_point  # Crisp pixel scaling

  # Start in native mode - use actual window dimensions
  @virtual_width = Window.width
  @virtual_height = Window.height

  # === FONTS ===
  # Load font at size appropriate for HD resolution (will be scaled down for retro mode)
  @custom_font = Graphics::Font.load("fonts/Ubuntu-Regular.ttf", size: 48)

  # Enable the console with settings for current resolution
  # Height and font_size are in virtual resolution pixels
  configure_console

  # === AUDIO ===
  @jump_sound = Audio::Sound.load("sfx/jump.mp3", volume: SFX_VOLUME)
  @attack_sound = Audio::Sound.load("sfx/sword_swing.mp3", volume: SFX_VOLUME + 1)
  @music = Audio::Music.load("music/jungle.mp3", volume: MUSIC_VOLUME, loop: true)
  @music.play

  # === INPUT MAPPING (with method chaining) ===
  Input.map(:move_left, [:left, :a])
       .map(:move_right, [:right, :d])
       .map(:jump, [:space, :up, :w])
       .map(:attack, [:z, :x])          # Attack with Z or X keys
       .map(:toggle_resolution, [:r])   # Toggle retro/regular resolution
  Input.on(:jump) { do_jump }
  Input.on(:attack) { do_attack }       # Attack callback
  Input.on(:toggle_resolution) { toggle_resolution }

  # === CAMERA ===
  # Configure camera with world-space settings
  # - viewport_size: render target dimensions in pixels
  # - view_height: how many world units are visible vertically (PPU auto-calculated)
  @camera = Graphics::Camera.new(
    viewport_size: Mathf::Vec2.new(@virtual_width, @virtual_height),
    view_height: VIEW_HEIGHT
  )
  @camera.offset = Mathf::Vec2.new(@virtual_width / 2.0, @virtual_height * CAMERA_OFFSET_Y_PERCENT)
  @camera.zoom = 1.0

  # Configure camera deadzone in world units (centered on camera target)
  # Deadzone rect: x/y are offsets from center, width/height are in world units
  deadzone = Graphics::Rect.new(0, 0, CAMERA_DEADZONE_WIDTH, CAMERA_DEADZONE_HEIGHT)

  # === PARALLAX BACKGROUNDS ===
  # Load background textures
  @bg1_tex = Graphics::Texture.load("oak_woods/background/background_layer_1.png")
  @bg2_tex = Graphics::Texture.load("oak_woods/background/background_layer_2.png")
  @bg3_tex = Graphics::Texture.load("oak_woods/background/background_layer_3.png")

  # Create parallax background sprites - rendered in world space with parallax factors
  # The player is around y=16-17, ground at y=19. We want backgrounds above ground.
  #
  # PARALLAX Y-POSITION CALCULATION:
  # The parallax formula adds: y += camera_target_y * (1 - parallax)
  # So to have the background appear at visual_y when camera is at target_y:
  #   initial_y = visual_y - target_y * (1 - parallax)
  #
  # For backgrounds we want them positioned so their bottom edge is hidden behind ground
  # With camera target around y = 16
  target_y = 16.0
  visual_y = 10.0  # Move backgrounds DOWN so bottom edge is behind ground

  # Scale backgrounds up to fill more vertical space
  bg_scale = 1.5

  bg1_y = visual_y - target_y * (1.0 - BG_PARALLAX_1)
  bg2_y = visual_y - target_y * (1.0 - BG_PARALLAX_2)
  bg3_y = visual_y - target_y * (1.0 - BG_PARALLAX_3)

  @bg1_sprites = create_parallax_layer(@bg1_tex, BG_PARALLAX_1, bg1_y, -30.0, bg_scale)
  @bg2_sprites = create_parallax_layer(@bg2_tex, BG_PARALLAX_2, bg2_y, -30.0, bg_scale)
  @bg3_sprites = create_parallax_layer(@bg3_tex, BG_PARALLAX_3, bg3_y, -30.0, bg_scale)

  # === TILEMAP ===
  # Tilemap uses pixel dimensions for texture sampling
  @tileset_tex = Graphics::Texture.load("oak_woods/oak_woods_tileset.png")
  @tilemap = Graphics::Tilemap.new(@tileset_tex, TILE_SIZE_PX, TILE_SIZE_PX, MAP_WIDTH, MAP_HEIGHT)

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

  # Build boundary walls and ground using fill_rect for cleaner code
  # Wall thickness: 5 tiles on each side to ensure parallax edges are hidden
  wall_width = 10
  playable_start = wall_width
  playable_end = MAP_WIDTH - wall_width - 1


  # Ground - top surface and fill below
  @tilemap.fill_rect(playable_start, 19, playable_end - playable_start + 1, 1, Tiles::GROUND_TOP_CENTER)
  @tilemap.fill_rect(playable_start, 20, playable_end - playable_start + 1, MAP_HEIGHT - 20, Tiles::GROUND_MID_CENTER)

  # Left wall - solid fill then inner edge
  @tilemap.fill_rect(0, 0, wall_width, MAP_HEIGHT, Tiles::GROUND_MID_CENTER)
  # @tilemap.fill_rect(wall_width - 1, 0, 1, MAP_HEIGHT, 24)

  # Right wall - solid fill then inner edge
  @tilemap.fill_rect(MAP_WIDTH - wall_width, 0, wall_width, MAP_HEIGHT, Tiles::GROUND_MID_CENTER)
  # @tilemap.fill_rect(MAP_WIDTH - wall_width, MAP_HEIGHT, 1, 19, Tiles::GROUND_MID_LEFT)  # Inner edge column

  # === CHARACTER ===
  begin
    @char_tex = Graphics::Texture.load("oak_woods/character/char_blue.png")
  rescue
    @char_tex = @tileset_tex
  end

  # Find spawn point
  spawn_x = wall_width + 5
  pos_x = MAP_OFFSET_X + spawn_x * TILE_SIZE
  pos_y = MAP_OFFSET_Y + 18 * TILE_SIZE - FRAME_HEIGHT

  @sprite_transform = Transform2D.new(x: pos_x, y: pos_y)
  @sprite = Graphics::Sprite.new(@char_tex, @sprite_transform)
  # Source rect uses pixel values for texture sampling
  @sprite.source_rect = Graphics::Rect.new(0, 0, FRAME_WIDTH_PX, FRAME_HEIGHT_PX)

  @velocity_x = 0.0
  @velocity_y = 0.0
  @on_ground = false

  # Create a camera target that returns the center of the sprite
  # The camera.follow() system expects an object with x() and y() methods
  @camera_target = Object.new
  def @camera_target.x
    $sprite_transform.x + FRAME_WIDTH / 2.0
  end
  def @camera_target.y
    $sprite_transform.y + FRAME_HEIGHT / 2.0
  end
  $sprite_transform = @sprite_transform  # Store for camera target access

  # Initialize camera position to player center
  @camera.target = Mathf::Vec2.new(@camera_target.x, @camera_target.y)

  # Setup camera to follow player center with smoothing and deadzone
  @camera.follow(@camera_target, smoothing: CAMERA_SMOOTHING, deadzone: deadzone)

  # === ANIMATION & STATE MACHINE ===
  begin
    # Use Animator for cleaner animation management
    # Animator uses pixel dimensions for texture sampling
    @animator = Animation::Animator.new(@sprite,
      columns: COLUMNS,
      frame_width: FRAME_WIDTH_PX,
      frame_height: FRAME_HEIGHT_PX)

    # Core movement animations (REFINED for perfect timing)
    @animator.add(:idle, frames: 0..5, fps: 6, loop: true)           # Slower, calmer breathing
    @animator.add(:run, frames: 14..19, fps: 12, loop: true)         # Fast run cycle

    # Jump sequence (IMPROVED with better timing)
    @animator.add(:jump_up, frames: 22..23, fps: 15, loop: false)    # Faster ascent
    @animator.add(:fall, frames: 24..26, fps: 12, loop: false)       # Smooth descent

    # Attack animations (NEW - Primary slash attack)
    @animator.add(:attack, frames: 8..12, fps: 18, loop: false)     # Fast, snappy attack

    # Damage/death animations (NEW)
    @animator.add(:hurt, frames: 43..47, fps: 15, loop: false)       # Hit reaction
    @animator.add(:death, frames: 50..57, fps: 10, loop: false)      # Dramatic death

    # Animation completion callbacks - auto-return to idle after animations finish
    @animator.on_complete(:attack) do
      state_machine.trigger(:attack_finish) if state_machine.state == :attacking
    end

    @animator.on_complete(:hurt) do
      state_machine.trigger(:hurt_finish) if state_machine.state == :hurt
    end

    @animator.on_complete(:death) do
      puts "Game Over!"  # Death animation complete
    end

    state_machine do
      state :idle do
        animate :idle
        on :move, :running
        on :jump, :jumping
        on :attack, :attacking       # NEW: Attack from idle
      end

      state :running do
        animate :run
        on :stop, :idle
        on :jump, :jumping
        on :attack, :attacking       # NEW: Attack while running
      end

      state :jumping do
        animate :jump_up
        on :fall, :falling
        on :land, :idle
        on :land_moving, :running
        on :attack, :attacking       # NEW: Aerial attack
      end

      state :falling do
        animate :fall
        on :land, :idle
        on :land_moving, :running
        on :attack, :attacking       # NEW: Aerial attack
      end

      state :attacking do            # NEW STATE: Attack animation
        animate :attack
        on :attack_finish, :idle     # Return to idle when attack completes
      end

      state :hurt do                 # NEW STATE: Damage reaction
        animate :hurt
        on :hurt_finish, :idle       # Recover to idle
      end

      state :dead do                 # NEW STATE: Death animation
        animate :death
        # No transitions (game over)
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
  @jump_sound.play
end

def do_attack
  # Can attack from idle, running, jumping, or falling states
  return if state_machine.state == :attacking  # Prevent spam - must finish current attack
  return if state_machine.state == :hurt       # Can't attack while taking damage
  return if state_machine.state == :dead       # Can't attack while dead

  state_machine.trigger(:attack)
  @attack_sound.play  # Play sword swing sound
  @camera.shake(strength: ATTACK_SHAKE_STRENGTH, duration: ATTACK_SHAKE_DURATION)
end

def configure_console
  # Console uses 360p baseline values - engine auto-scales to any resolution
  Console.enable(
    height: 150,
    font_size: 14,
    line_height: 18,
    padding: 8,
    font: @custom_font
  ).allow_ruby_eval
end

def toggle_resolution
  @retro_mode = !@retro_mode

  if @retro_mode
    # Retro mode: use fixed low virtual resolution
    @virtual_width = RETRO_WIDTH
    @virtual_height = RETRO_HEIGHT
    Window.set_virtual_resolution(@virtual_width, @virtual_height)
  else
    # Native mode: disable virtual resolution, use actual window size
    Window.clear_virtual_resolution
    @virtual_width = Window.width
    @virtual_height = Window.height
  end

  # Update camera viewport and offset
  @camera.viewport_size = Mathf::Vec2.new(@virtual_width, @virtual_height)
  @camera.offset = Mathf::Vec2.new(@virtual_width / 2.0, @virtual_height * CAMERA_OFFSET_Y_PERCENT)

  # Reconfigure console for new resolution
  configure_console
end

# Called by engine when window is resized
def on_resize(width, height)
  # Only update in native mode (retro mode has fixed virtual resolution)
  return if @retro_mode

  @virtual_width = width
  @virtual_height = height

  # Update camera viewport and offset to match new window size
  @camera.viewport_size = Mathf::Vec2.new(@virtual_width, @virtual_height)
  @camera.offset = Mathf::Vec2.new(@virtual_width / 2.0, @virtual_height * CAMERA_OFFSET_Y_PERCENT)
end

def update(dt)
  moving_left = Input.action_down?(:move_left)
  moving_right = Input.action_down?(:move_right)
  moving = moving_left || moving_right

  # Check for attack input - keyboard (Z/X) OR left mouse button
  if Input.action_pressed?(:attack) || Input.mouse_pressed?(:left)
    do_attack
  end

  if !@on_ground
    @velocity_y += GRAVITY * dt
    state_machine.trigger(:fall) if @velocity_y > 0 && state_machine.state != :falling
  end

  # Set horizontal velocity based on input
  @velocity_x = 0.0
  @velocity_x = -MOVE_SPEED if moving_left
  @velocity_x = MOVE_SPEED if moving_right
  @sprite.flip_x = true if moving_left
  @sprite.flip_x = false if moving_right

  # Apply velocities to position
  @sprite_transform.x += @velocity_x * dt
  @sprite_transform.y += @velocity_y * dt

  check_tilemap_collision(moving)

  if @on_ground
    if moving && state_machine.state != :running
      state_machine.trigger(:move)
    elsif !moving && state_machine.state != :idle
      state_machine.trigger(:stop)
    end
  end

  # Camera automatically follows @sprite_transform with smoothing and deadzone
end

def check_tilemap_collision(moving)
  # Get hitbox in tilemap local coordinates
  local_x = @sprite_transform.x + CHAR_HITBOX_OFFSET_X - MAP_OFFSET_X
  local_y = @sprite_transform.y + CHAR_HITBOX_OFFSET_Y - MAP_OFFSET_Y

  # Resolve horizontal collision first (pass 0 for vertical velocity)
  h_result = Collision.tilemap_resolve(
    @tilemap,
    local_x, local_y,
    CHAR_HITBOX_WIDTH, CHAR_HITBOX_HEIGHT,
    @velocity_x, 0.0
  )
  local_x = h_result.x
  @velocity_x = h_result.vx

  # Then resolve vertical collision (pass 0 for horizontal velocity)
  v_result = Collision.tilemap_resolve(
    @tilemap,
    local_x, local_y,
    CHAR_HITBOX_WIDTH, CHAR_HITBOX_HEIGHT,
    0.0, @velocity_y
  )

  # Apply resolved position (convert back to world coordinates)
  @sprite_transform.x = local_x + MAP_OFFSET_X - CHAR_HITBOX_OFFSET_X
  @sprite_transform.y = v_result.y + MAP_OFFSET_Y - CHAR_HITBOX_OFFSET_Y
  @velocity_y = v_result.vy

  # Update ground state
  if v_result.bottom?
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

  # Everything is now drawn inside the camera - parallax is handled automatically
  # by the Transform2D.parallax property
  @camera.use do
    # Draw parallax backgrounds (rendered with parallax factor applied)
    @bg1_sprites.each { |s| s.draw }
    @bg2_sprites.each { |s| s.draw }
    @bg3_sprites.each { |s| s.draw }

    # Draw world objects (tilemap and player have parallax=1.0 by default)
    @tilemap.draw(MAP_OFFSET_X, MAP_OFFSET_Y)
    @sprite.draw
  end

  # HUD text - uses 360p baseline values, engine auto-scales to any resolution
  mode_name = @retro_mode ? "Retro" : "Native"
  if @retro_mode
    Graphics.draw_text("#{GMR::Time.fps} FPS", 5, 5, 30, :cyan)
    Graphics.draw_text("#{@virtual_width}x#{@virtual_height} [R - Toggle]", 5, 30, 30, :cyan)
  else
    Graphics.draw_text("#{GMR::Time.fps} FPS", 5, 5, 18, :cyan, font: @custom_font)
    Graphics.draw_text("#{@virtual_width}x#{@virtual_height} [R - Toggle]", 5, 22, 18, :cyan, font: @custom_font)
  end
end

# Helper to create a parallax layer with multiple repeating panels
def create_parallax_layer(texture, parallax_factor, y_pos, start_x = 0.0, scale = 1.0)
  sprites = []
  # Create enough panels to cover the world width with some overlap
  # For parallax layers, we need more coverage since they scroll slower
  num_panels = 7

  num_panels.times do |i|
    x_pos = start_x + i * BG_LAYER_WIDTH * scale
    t = Transform2D.new(x: x_pos, y: y_pos, parallax: parallax_factor, scale_x: scale, scale_y: scale)
    s = Graphics::Sprite.new(texture, t)
    sprites << s
  end
  sprites
end
