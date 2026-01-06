include GMR

# MVP Platformer Demo: Animation + State Machine + Input + Parallax + Collision + Camera

FRAME_WIDTH = 56
FRAME_HEIGHT = 56
COLUMNS = 7

SCREEN_WIDTH = 800
SCREEN_HEIGHT = 600

GRAVITY = 800.0
MOVE_SPEED = 200.0
JUMP_FORCE = -400.0

GROUND_Y = 480.0
PLATFORM_HEIGHT = 48.0

def init
  Input.map(:move_left, [:left, :a])
  Input.map(:move_right, [:right, :d])
  Input.map(:jump, [:space, :up, :w])

  # === CAMERA ===
  @camera = Graphics::Camera2D.new
  @camera.offset = Mathf::Vec2.new(SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0)
  @camera.zoom = 1.0

  # === PARALLAX BACKGROUNDS ===
  @bg1_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_1.png")
  @bg2_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_2.png")
  @bg3_tex = Graphics::Texture.load("assets/oak_woods/background/background_layer_3.png")

  bg_scale = 3.5
  bg_width = 320.0 * bg_scale

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

  # === PLATFORM ===
  @tileset_tex = Graphics::Texture.load("assets/oak_woods/oak_woods_tileset.png")

  @platforms = []
  @platform_world_x = []
  tile_w = 48
  tile_h = 48
  num_tiles = 40

  num_tiles.times do |i|
    tile = Graphics::Sprite.new(@tileset_tex)
    tile.source_rect = Graphics::Rect.new(0, 0, tile_w, tile_h)
    tile.y = GROUND_Y
    @platforms << tile
    @platform_world_x << (i * tile_w - 400.0)
  end

  @platform_rect = {
    x: -400.0,
    y: GROUND_Y,
    w: num_tiles * tile_w,
    h: PLATFORM_HEIGHT
  }

  # === CHARACTER ===
  @char_tex = Graphics::Texture.load("assets/oak_woods/character/char_blue.png")
  @sprite = Graphics::Sprite.new(@char_tex)
  @sprite.source_rect = Graphics::Rect.new(0, 0, FRAME_WIDTH, FRAME_HEIGHT)
  @sprite.x = 400.0
  @sprite.y = GROUND_Y - FRAME_HEIGHT + 16

  @velocity_y = 0.0
  @on_ground = true

  @animator = Animation::Animator.new(@sprite,
    columns: COLUMNS,
    frame_width: FRAME_WIDTH,
    frame_height: FRAME_HEIGHT)

  @animator.add(:idle, frames: 0..5, fps: 8)
  @animator.add(:run, frames: 14..19, fps: 12)
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
      enter { animator.play(:idle) }
      on :land, :idle
      on :land_moving, :running
    end
  end
end

def update(dt)
  moving_left = Input.action_down?(:move_left)
  moving_right = Input.action_down?(:move_right)
  moving = moving_left || moving_right

  if Input.action_pressed?(:jump) && @on_ground
    @velocity_y = JUMP_FORCE
    @on_ground = false
    state_machine.trigger(:jump)
  end

  @velocity_y += GRAVITY * dt if !@on_ground

  @sprite.x -= MOVE_SPEED * dt if moving_left
  @sprite.x += MOVE_SPEED * dt if moving_right
  @sprite.flip_x = true if moving_left
  @sprite.flip_x = false if moving_right

  @sprite.y += @velocity_y * dt

  # Collision
  char_bottom = @sprite.y + FRAME_HEIGHT - 16
  char_left = @sprite.x + 15
  char_width = FRAME_WIDTH - 30

  if @velocity_y >= 0
    if Collision.rect_overlap?(
        char_left, char_bottom - 5, char_width, 10,
        @platform_rect[:x], @platform_rect[:y], @platform_rect[:w], @platform_rect[:h])

      @sprite.y = GROUND_Y - FRAME_HEIGHT + 16
      @velocity_y = 0.0

      if !@on_ground
        @on_ground = true
        state_machine.trigger(moving ? :land_moving : :land)
      end
    else
      @on_ground = false
    end
  end

  if @on_ground
    state_machine.trigger(moving ? :move : :stop)
  end

  # Update camera to follow player
  @camera.target = Mathf::Vec2.new(@sprite.x + FRAME_WIDTH / 2.0, @sprite.y + FRAME_HEIGHT / 2.0)
end

def draw
  Graphics.clear([80, 120, 160])

  # Camera X is player position (center of character)
  camera_x = @sprite.x + FRAME_WIDTH / 2.0

  # Draw parallax backgrounds in SCREEN SPACE (manual offset)
  draw_parallax_layer(@bg1_sprites, @bg1_width, @bg1_speed, 0, camera_x)
  draw_parallax_layer(@bg2_sprites, @bg2_width, @bg2_speed, 0, camera_x)
  draw_parallax_layer(@bg3_sprites, @bg3_width, @bg3_speed, 0, camera_x)

  # Draw world objects - manually offset by camera
  screen_center_x = SCREEN_WIDTH / 2.0
  offset_x = screen_center_x - camera_x

  # Draw platforms
  @platforms.each_with_index do |tile, i|
    tile.x = @platform_world_x[i] + offset_x
    tile.draw
  end

  # Draw character (centered on screen)
  world_x = @sprite.x
  @sprite.x = world_x + offset_x
  @sprite.draw
  @sprite.x = world_x
end

def draw_parallax_layer(sprites, width, speed, y_offset, camera_x)
  offset = -camera_x * speed
  sprites.each_with_index do |s, i|
    s.x = offset + (i - 1) * width
    s.y = y_offset
    s.draw
  end
end
