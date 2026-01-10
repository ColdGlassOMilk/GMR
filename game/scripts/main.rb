include GMR

def init
  @retro_mode = false

  Window.set_size(WINDOW_WIDTH, WINDOW_HEIGHT)
        .set_filter_point

  @virtual_width = Window.width
  @virtual_height = Window.height

  @custom_font = Graphics::Font.load("fonts/Ubuntu-Regular.ttf", size: 48)
  configure_console

  @music = Audio::Music.load("music/jungle.mp3", volume: MUSIC_VOLUME, loop: true)
  @music.play

  setup_camera

  @level = Level.new
  spawn = @level.spawn_point
  @player = Player.new(spawn.x, spawn.y)

  deadzone = Graphics::Rect.new(0, 0, CAMERA_DEADZONE_WIDTH, CAMERA_DEADZONE_HEIGHT)
  @camera.target = Mathf::Vec2.new(@player.x, @player.y)
  @camera.follow(@player, smoothing: CAMERA_SMOOTHING, deadzone: deadzone)

  @hud = HUD.new(@custom_font)
  @input_handler = InputHandler.new(@player, @camera)
  @input_handler.on_resolution_toggle { toggle_resolution }
  @input_handler.setup
end

def update(dt)
  @input_handler.update
  @player.update(dt, @level.tilemap)
end

def draw
  Graphics.clear([80, 120, 160])

  @camera.use do
    @level.draw
    @player.draw
  end

  @hud.draw(GMR::Time.fps, @virtual_width, @virtual_height, @retro_mode)
end

def on_resize(width, height)
  return if @retro_mode

  @virtual_width = width
  @virtual_height = height

  @camera.viewport_size = Mathf::Vec2.new(@virtual_width, @virtual_height)
  @camera.offset = Mathf::Vec2.new(@virtual_width / 2.0, @virtual_height * CAMERA_OFFSET_Y_PERCENT)
end

def toggle_resolution
  @retro_mode = !@retro_mode

  if @retro_mode
    @virtual_width = RETRO_WIDTH
    @virtual_height = RETRO_HEIGHT
    Window.set_virtual_resolution(@virtual_width, @virtual_height)
  else
    Window.clear_virtual_resolution
    @virtual_width = Window.width
    @virtual_height = Window.height
  end

  @camera.viewport_size = Mathf::Vec2.new(@virtual_width, @virtual_height)
  @camera.offset = Mathf::Vec2.new(@virtual_width / 2.0, @virtual_height * CAMERA_OFFSET_Y_PERCENT)

  configure_console
end

def setup_camera
  @camera = Graphics::Camera.new(
    viewport_size: Mathf::Vec2.new(@virtual_width, @virtual_height),
    view_height: VIEW_HEIGHT
  )
  @camera.offset = Mathf::Vec2.new(@virtual_width / 2.0, @virtual_height * CAMERA_OFFSET_Y_PERCENT)
  @camera.zoom = 1.0
end

def configure_console
  Console.enable(
    height: 150,
    font_size: 14,
    line_height: 18,
    padding: 8,
    font: @custom_font
  ).allow_ruby_eval
end
