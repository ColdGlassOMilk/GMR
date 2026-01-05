# GameScene - Main gameplay scene
# Uses TX Tileset sprite sheets

class GameScene < GMR::Scene
  # =============================================================================
  # Constants
  # =============================================================================
  TILE_SIZE = 32
  MAP_WIDTH = 30
  MAP_HEIGHT = 20
  SCREEN_WIDTH = 960
  SCREEN_HEIGHT = 640

  # TX Tileset Grass.png is 256x256 (8x8 tiles of 32x32)
  # Row 0: Plain grass tiles
  # Rows 4-7: Stone path tiles embedded in grass
  GRASS_TILE = 0       # Top-left grass tile (normal speed 1.0x)
  GRASS_VAR1 = 1       # Grass variation
  GRASS_VAR2 = 2       # Grass with small details
  STONE_PATH = 32      # Stone path (row 4, col 0) - fast 1.5x

  # We'll use different textures for walls and water
  # For simplicity, we'll draw walls as colored rectangles or use wall tileset

  # =============================================================================
  # Scene Lifecycle
  # =============================================================================
  def init
    setup_window
    setup_input
    load_assets
    build_map
    create_player
    setup_camera
    setup_systems
  end

  def update(dt)
    return if console_open?

    update_player(dt)
    update_camera(dt)
    update_systems(dt)
    handle_input(dt)
  end

  def draw
    GMR::Graphics.clear([34, 51, 34])

    @camera.use do
      draw_world
      draw_player
      @particles.draw
    end

    draw_ui
  end

  def unload
    # Cleanup if needed
  end

  # =============================================================================
  # Setup Methods
  # =============================================================================

  def setup_window
    GMR::Window.set_title("GMR Demo")
    GMR::Window.set_size(SCREEN_WIDTH, SCREEN_HEIGHT)
    GMR::Window.set_virtual_resolution(SCREEN_WIDTH, SCREEN_HEIGHT)
    GMR::Window.set_filter_point
  end

  def setup_input
    GMR::Input.map(:move_left, [:left, :a])
    GMR::Input.map(:move_right, [:right, :d])
    GMR::Input.map(:move_up, [:up, :w])
    GMR::Input.map(:move_down, [:down, :s])
  end

  def load_assets
    @tex_grass = GMR::Graphics::Texture.load("assets/TX Tileset Grass.png")
    @tex_stone = GMR::Graphics::Texture.load("assets/TX Tileset Stone Ground.png")
    @tex_wall = GMR::Graphics::Texture.load("assets/TX Tileset Wall.png")
    @tex_plant = GMR::Graphics::Texture.load("assets/TX Plant.png")
    @tex_player = GMR::Graphics::Texture.load("assets/TX Player.png")
  end

  def build_map
    # Create map data structure (simple 2D array)
    # Tile types: :grass, :stone, :wall, :water, :sand
    @tiles = Array.new(MAP_HEIGHT) { Array.new(MAP_WIDTH, :grass) }
    @tile_variants = Array.new(MAP_HEIGHT) { Array.new(MAP_WIDTH, 0) }

    # Add grass variations
    (MAP_HEIGHT * MAP_WIDTH / 4).times do
      x = GMR::System.random_int(0, MAP_WIDTH - 1)
      y = GMR::System.random_int(0, MAP_HEIGHT - 1)
      @tile_variants[y][x] = GMR::System.random_int(1, 3)
    end

    # Build stone paths (center cross)
    build_stone_paths

    # Build walls (border only)
    build_walls

    # Build water ponds (2 bodies)
    build_water

    # Build sand pits (2 areas)
    build_sand
  end

  def build_stone_paths
    # Horizontal main path
    (5...25).each { |x| set_tile(x, 10, :stone) }

    # Vertical path
    (4...16).each { |y| set_tile(15, y, :stone) }

    # Small plaza around intersection
    (12...19).each do |x|
      (8...13).each do |y|
        set_tile(x, y, :stone)
      end
    end
  end

  def build_walls
    # Border walls only - clean perimeter
    MAP_WIDTH.times do |x|
      set_tile(x, 0, :wall)
      set_tile(x, MAP_HEIGHT - 1, :wall)
    end
    MAP_HEIGHT.times do |y|
      set_tile(0, y, :wall)
      set_tile(MAP_WIDTH - 1, y, :wall)
    end
  end

  def build_water
    # Pond in bottom-left corner
    (3...7).each do |x|
      (14...18).each do |y|
        set_tile(x, y, :water)
      end
    end

    # Pond in top-right corner
    (23...27).each do |x|
      (3...6).each do |y|
        set_tile(x, y, :water)
      end
    end
  end

  def build_sand
    # Sand pit in top-left
    (3...7).each do |x|
      (3...6).each do |y|
        set_tile(x, y, :sand)
      end
    end

    # Sand pit in bottom-right
    (23...27).each do |x|
      (14...17).each do |y|
        set_tile(x, y, :sand)
      end
    end
  end

  def set_tile(x, y, type)
    return if x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT
    @tiles[y][x] = type
  end

  def get_tile(x, y)
    return :wall if x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT
    @tiles[y][x]
  end

  def create_player
    start_x = 15 * TILE_SIZE + TILE_SIZE / 2
    start_y = 10 * TILE_SIZE + TILE_SIZE / 2
    @player = Player.new(@tex_player, start_x, start_y, TILE_SIZE, self)
  end

  def setup_camera
    @camera = Camera2D.new
    @camera.target = @player.position
    @camera.offset = Vec2.new(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2)
    @camera.zoom = 1.5
  end

  def setup_systems
    @particles = ParticleSystem.new
    @sand_dust_timer = 0
  end

  # =============================================================================
  # Tile Queries (for player collision/speed)
  # =============================================================================

  def solid?(tx, ty)
    get_tile(tx, ty) == :wall
  end

  def water?(tx, ty)
    get_tile(tx, ty) == :water
  end

  def sand?(tx, ty)
    get_tile(tx, ty) == :sand
  end

  # For minimap overlay
  def player_position
    @player.position
  end

  def camera_bounds
    half_w = SCREEN_WIDTH / 2 / @camera.zoom
    half_h = SCREEN_HEIGHT / 2 / @camera.zoom
    {
      left: @camera.target.x - half_w,
      top: @camera.target.y - half_h,
      right: @camera.target.x + half_w,
      bottom: @camera.target.y + half_h
    }
  end

  def get_speed_modifier(tx, ty)
    case get_tile(tx, ty)
    when :wall then 0
    when :water then 0.3
    when :sand then 0.7
    when :stone then 1.5
    else 1.0  # grass
    end
  end

  # =============================================================================
  # Update Methods
  # =============================================================================

  def update_player(dt)
    @player.update(dt)

    if @player.just_entered_water?
      @particles.spawn_water_splash(@player.x, @player.y)
      @camera.shake(strength: 3, duration: 0.15)
    end

    # Spawn dust continuously while walking on sand
    if @player.moving_on_sand?
      @sand_dust_timer -= dt
      if @sand_dust_timer <= 0
        @particles.spawn_sand_dust(@player.x, @player.y)
        @sand_dust_timer = 0.15  # Spawn every 150ms while moving
      end
    else
      @sand_dust_timer = 0
    end
  end

  def update_camera(dt)
    @camera.target = lerp_vec(@camera.target, @player.position, 0.1)

    # Clamp to map bounds
    half_w = SCREEN_WIDTH / 2 / @camera.zoom
    half_h = SCREEN_HEIGHT / 2 / @camera.zoom
    @camera.target = Vec2.new(
      clamp(@camera.target.x, half_w, MAP_WIDTH * TILE_SIZE - half_w),
      clamp(@camera.target.y, half_h, MAP_HEIGHT * TILE_SIZE - half_h)
    )
  end

  def update_systems(dt)
    @particles.update(dt)
  end

  def handle_input(dt)
    wheel = GMR::Input.mouse_wheel
    if wheel != 0
      @camera.zoom = clamp(@camera.zoom + wheel * 0.15, 0.75, 3.0)
    end

    if GMR::Input.key_pressed?(:r)
      @camera.zoom = 1.5
    end

    if GMR::Input.key_pressed?(:f11)
      GMR::Window.toggle_fullscreen
    end
  end

  # =============================================================================
  # Draw Methods
  # =============================================================================

  def draw_world
    # Calculate visible tile range
    cam_left = @camera.target.x - SCREEN_WIDTH / 2 / @camera.zoom
    cam_top = @camera.target.y - SCREEN_HEIGHT / 2 / @camera.zoom
    cam_right = @camera.target.x + SCREEN_WIDTH / 2 / @camera.zoom
    cam_bottom = @camera.target.y + SCREEN_HEIGHT / 2 / @camera.zoom

    start_x = [(cam_left / TILE_SIZE).to_i - 1, 0].max
    start_y = [(cam_top / TILE_SIZE).to_i - 1, 0].max
    end_x = [(cam_right / TILE_SIZE).to_i + 1, MAP_WIDTH - 1].min
    end_y = [(cam_bottom / TILE_SIZE).to_i + 1, MAP_HEIGHT - 1].min

    # Draw tiles
    (start_y..end_y).each do |ty|
      (start_x..end_x).each do |tx|
        draw_tile(tx, ty)
      end
    end
  end

  def draw_tile(tx, ty)
    px = tx * TILE_SIZE
    py = ty * TILE_SIZE
    tile_type = @tiles[ty][tx]

    case tile_type
    when :grass
      # The grass tileset is mostly solid green - just use top-left area
      # Add slight position variation for visual interest
      @tex_grass.draw_pro(0, 0, TILE_SIZE, TILE_SIZE, px + TILE_SIZE/2, py + TILE_SIZE/2, TILE_SIZE, TILE_SIZE, 0)

    when :stone
      # Draw from stone ground tileset (top-left 64x64 is solid stone, take 32x32 chunk)
      @tex_stone.draw_pro(0, 0, TILE_SIZE, TILE_SIZE, px + TILE_SIZE/2, py + TILE_SIZE/2, TILE_SIZE, TILE_SIZE, 0)

    when :wall
      # Use a solid center brick tile (row 1, col 1 area has full bricks)
      # The tileset has edges/corners around perimeter, solid tiles in center
      @tex_wall.draw_pro(64, 64, 64, 64, px + TILE_SIZE/2, py + TILE_SIZE/2, TILE_SIZE, TILE_SIZE, 0)

    when :water
      # Draw water as blue with wave effect
      wave = Math.sin(GMR::Time.elapsed * 3 + tx * 0.5 + ty * 0.3) * 0.1
      alpha = (200 + wave * 55).to_i
      GMR::Graphics.draw_rect(px, py, TILE_SIZE, TILE_SIZE, [64, 164, 223, alpha])
      # Add wave highlights
      hl_y = py + 8 + (Math.sin(GMR::Time.elapsed * 4 + tx) * 4).to_i
      GMR::Graphics.draw_rect(px + 6, hl_y, 10, 3, [100, 200, 255, 180])
      GMR::Graphics.draw_rect(px + 18, hl_y + 8, 8, 2, [120, 210, 255, 150])

    when :sand
      # Draw grass base first, then flower overlay
      @tex_grass.draw_pro(0, 0, TILE_SIZE, TILE_SIZE, px + TILE_SIZE/2, py + TILE_SIZE/2, TILE_SIZE, TILE_SIZE, 0)
      # Draw flower from plant tileset (varies by position for variety)
      flower_idx = (tx * 3 + ty * 7) % 4
      src_x = flower_idx * 32
      @tex_plant.draw_pro(src_x, 0, 32, 32, px + TILE_SIZE/2, py + TILE_SIZE/2, TILE_SIZE, TILE_SIZE, 0)
    end
  end

  def draw_player
    @player.draw
  end

  def draw_ui
    panel_w = 200
    panel_h = 90
    GMR::Graphics.draw_rect(8, 8, panel_w, panel_h, [0, 0, 0, 180])

    y = 14
    line = 18

    GMR::Graphics.draw_text("GMR Tile Demo", 14, y, 16, [255, 220, 100])
    y += line + 4

    fps_color = GMR::Time.fps >= 55 ? [100, 255, 100] : [255, 100, 100]
    GMR::Graphics.draw_text("FPS: #{GMR::Time.fps}", 14, y, 14, fps_color)
    y += line

    tile_x = (@player.x / TILE_SIZE).to_i
    tile_y = (@player.y / TILE_SIZE).to_i
    tile_type = get_tile(tile_x, tile_y)
    speed = get_speed_modifier(tile_x, tile_y)

    GMR::Graphics.draw_text("Tile: #{tile_type} (#{speed}x)", 14, y, 14, tile_color(tile_type))
    y += line

    GMR::Graphics.draw_text("WASD: Move | Scroll: Zoom", 14, y, 12, [140, 140, 140])
  end

  def tile_color(type)
    case type
    when :grass then [100, 200, 100]
    when :stone then [180, 180, 200]
    when :water then [100, 180, 255]
    when :wall then [255, 100, 100]
    when :sand then [220, 190, 120]
    else [200, 200, 200]
    end
  end

  def lerp_vec(a, b, t)
    Vec2.new(lerp(a.x, b.x, t), lerp(a.y, b.y, t))
  end

  # =============================================================================
  # Player Class
  # =============================================================================
  class Player
    attr_accessor :x, :y
    attr_reader :current_speed_mod

    def initialize(texture, x, y, tile_size, scene)
      @x = x.to_f
      @y = y.to_f
      @base_speed = 150
      @tile_size = tile_size
      @scene = scene
      @texture = texture
      @in_water = false
      @was_in_water = false
      @in_sand = false
      @was_in_sand = false
      @is_moving = false
      @current_speed_mod = 1.0

      # TX Player.png: 4 character sprites on a ~128x48 canvas
      # Each sprite is roughly 24x32, starting around x=8
      @sprite = Sprite.new(texture)
      @sprite.source_rect = Rect.new(8, 8, 24, 36)
      @sprite.origin = Vec2.new(12, 32)  # Bottom center
      @sprite.scale_x = 1.5
      @sprite.scale_y = 1.5
      update_sprite
    end

    def position
      Vec2.new(@x, @y)
    end

    def update(dt)
      @was_in_water = @in_water
      @was_in_sand = @in_sand

      dx, dy = 0, 0
      dx -= 1 if GMR::Input.action_down?(:move_left)
      dx += 1 if GMR::Input.action_down?(:move_right)
      dy -= 1 if GMR::Input.action_down?(:move_up)
      dy += 1 if GMR::Input.action_down?(:move_down)

      @is_moving = dx != 0 || dy != 0

      if dx != 0 && dy != 0
        dx *= 0.707
        dy *= 0.707
      end

      tx = (@x / @tile_size).to_i
      ty = (@y / @tile_size).to_i
      @current_speed_mod = @scene.get_speed_modifier(tx, ty)
      current_speed = @base_speed * @current_speed_mod

      # Move with collision
      if dx != 0
        new_x = @x + dx * current_speed * dt
        ntx = (new_x / @tile_size).to_i
        @x = new_x unless @scene.solid?(ntx, ty)
      end

      if dy != 0
        new_y = @y + dy * current_speed * dt
        nty = (new_y / @tile_size).to_i
        @y = new_y unless @scene.solid?(tx, nty)
      end

      # Clamp to map
      @x = clamp(@x, @tile_size, (GameScene::MAP_WIDTH - 1) * @tile_size)
      @y = clamp(@y, @tile_size, (GameScene::MAP_HEIGHT - 1) * @tile_size)

      # Update terrain state
      px = (@x / @tile_size).to_i
      py = (@y / @tile_size).to_i
      @in_water = @scene.water?(px, py)
      @in_sand = @scene.sand?(px, py)

      update_sprite
    end

    def draw
      @sprite.draw

      # Draw a colored indicator as fallback/debug
      size = 24
      color = @in_water ? [100, 180, 255, 200] : [255, 200, 100, 200]
      GMR::Graphics.draw_rect(@x.to_i - size/2, @y.to_i - size/2, size, size, color)
      GMR::Graphics.draw_rect_outline(@x.to_i - size/2, @y.to_i - size/2, size, size, [50, 50, 50])
    end

    def just_entered_water?
      @in_water && !@was_in_water
    end

    def just_entered_sand?
      @in_sand && !@was_in_sand
    end

    def moving_on_sand?
      @in_sand && @is_moving
    end

    private

    def update_sprite
      @sprite.x = @x
      @sprite.y = @y

      if @in_water
        bob = Math.sin(GMR::Time.elapsed * 6) * 2
        @sprite.y = @y + bob
        @sprite.color = [150, 200, 255, 255]
      else
        @sprite.color = [255, 255, 255, 255]
      end
    end
  end

  # =============================================================================
  # Particle System
  # =============================================================================
  class ParticleSystem
    def initialize
      @particles = []
    end

    def spawn_water_splash(x, y)
      10.times do
        angle = GMR::System.random_int(0, 360) * Math::PI / 180
        speed = GMR::System.random_int(40, 100)
        @particles << {
          x: x.to_f, y: y.to_f,
          vx: Math.cos(angle) * speed,
          vy: Math.sin(angle) * speed - 50,
          life: 0.5 + GMR::System.random_float * 0.3,
          max_life: 0.8,
          size: GMR::System.random_int(2, 5),
          gravity: 200,
          type: :water
        }
      end
    end

    def spawn_sand_dust(x, y)
      # Fewer particles, more subtle than water
      5.times do
        angle = GMR::System.random_int(0, 360) * Math::PI / 180
        speed = GMR::System.random_int(15, 35)  # Much slower than water
        @particles << {
          x: x.to_f + GMR::System.random_int(-8, 8),
          y: y.to_f + GMR::System.random_int(-4, 4),
          vx: Math.cos(angle) * speed,
          vy: Math.sin(angle) * speed - 15,  # Gentle upward drift
          life: 0.4 + GMR::System.random_float * 0.3,
          max_life: 0.7,
          size: GMR::System.random_int(2, 4),
          gravity: 30,  # Very light gravity, dust floats
          type: :sand
        }
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

        color = case p[:type]
                when :sand
                  # Sandy tan/brown color, more transparent
                  [180, 155, 100, (alpha * 0.6).to_i]
                else
                  # Water - blue
                  [100, 180, 255, alpha]
                end

        GMR::Graphics.draw_circle(p[:x].to_i, p[:y].to_i, p[:size], color)
      end
    end
  end
end
