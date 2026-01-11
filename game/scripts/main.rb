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
  setup_shaders

  @level = Level.new
  spawn = @level.spawn_point
  @player = Player.new(spawn.x, spawn.y)

  deadzone = Graphics::Rect.new(0, 0, CAMERA_DEADZONE_WIDTH, CAMERA_DEADZONE_HEIGHT)
  @camera.target = Mathf::Vec2.new(@player.x, @player.y)
  @camera.follow(@player, smoothing: CAMERA_SMOOTHING, deadzone: deadzone)

  @hud = HUD.new(@custom_font)
  @input_handler = InputHandler.new(@player, @camera)
  @input_handler.on_resolution_toggle { toggle_resolution }
  @input_handler.on_next_shader { next_shader }
  @input_handler.on_prev_shader { prev_shader }
  @input_handler.setup
end

def update(dt)
  @input_handler.update
  @player.update(dt, @level.tilemap)
end

def draw
  Graphics.clear([80, 120, 160])

  current_shader = @shaders[@shader_index]
  if current_shader
    # Set shader uniforms based on which shader is active
    set_shader_uniforms(current_shader, @shader_names[@shader_index])

    current_shader.use do
      @camera.use do
        @level.draw
        @player.draw
      end
    end
  else
    @camera.use do
      @level.draw
      @player.draw
    end
  end

  @hud.draw(GMR::Time.fps, @virtual_width, @virtual_height, @retro_mode, @shader_names[@shader_index])
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

def setup_shaders
  @shader_names = [
    "none", "grayscale", "sepia", "invert", "wave", "pixelate",
    "crt", "chromatic", "vignette", "blur", "posterize", "glitch", "bloom"
  ]
  @shaders = [
    nil,  # "none" - no shader
    Graphics::Shader.load(fragment: "shaders/grayscale.fs", surface_mode: true),
    Graphics::Shader.load(fragment: "shaders/sepia.fs", surface_mode: true),
    Graphics::Shader.load(fragment: "shaders/invert.fs", surface_mode: true),
    Graphics::Shader.load(fragment: "shaders/wave.fs", surface_mode: true),       # Spatial distortion needs unified UV
    Graphics::Shader.load(fragment: "shaders/pixelate.fs"),
    Graphics::Shader.load(fragment: "shaders/crt.fs", surface_mode: true),        # Curvature/scanlines need unified UV
    Graphics::Shader.load(fragment: "shaders/chromatic.fs"),  # Chromatic aberration needs unified UV
    Graphics::Shader.load(fragment: "shaders/vignette.fs", surface_mode: true),
    Graphics::Shader.load(fragment: "shaders/blur.fs", surface_mode: true),
    Graphics::Shader.load(fragment: "shaders/posterize.fs", surface_mode: true),
    Graphics::Shader.load(fragment: "shaders/glitch.fs", surface_mode: true),     # Glitch displacement needs unified UV
    Graphics::Shader.load(fragment: "shaders/bloom.fs", surface_mode: true)
  ]
  @shader_index = 0
end

def set_shader_uniforms(shader, name)
  res_x = @virtual_width.to_f
  res_y = @virtual_height.to_f
  time = GMR::Time.elapsed

  case name
  when "grayscale", "sepia", "invert"
    shader.set(:intensity, 1.0)
  when "wave"
    shader.set(:time, time)
    shader.set(:amplitude, 0.02)
    shader.set(:frequency, 15.0)
  when "pixelate"
    shader.set(:pixelSize, 6.0)
    shader.set(:resolution, res_x, res_y)
  when "crt"
    shader.set(:resolution, res_x, res_y)
    shader.set(:curvature, 6.0)
    shader.set(:scanlineIntensity, 0.3)
  when "chromatic"
    shader.set(:offset, 0.005)
  when "vignette"
    shader.set(:radius, 0.4)
    shader.set(:softness, 0.5)
  when "blur"
    shader.set(:resolution, res_x, res_y)
    shader.set(:radius, 2.0)
  when "posterize"
    shader.set(:levels, 6.0)
  when "glitch"
    shader.set(:time, time)
    shader.set(:intensity, 0.5)
  when "bloom"
    shader.set(:resolution, res_x, res_y)
    shader.set(:threshold, 0.6)
    shader.set(:intensity, 1.2)
    shader.set(:radius, 2.0)
  end
end

def next_shader
  @shader_index = (@shader_index + 1) % @shaders.length
  Console.puts("Shader: #{@shader_names[@shader_index]}")
end

def prev_shader
  @shader_index = (@shader_index - 1) % @shaders.length
  Console.puts("Shader: #{@shader_names[@shader_index]}")
end
