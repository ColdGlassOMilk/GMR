class Level
  attr_reader :tilemap

  def initialize
    setup_tilemap
    build_layout
    setup_parallax
  end

  def draw
    @bg1_sprites.each { |s| s.draw }
    @bg2_sprites.each { |s| s.draw }
    @bg3_sprites.each { |s| s.draw }
    @tilemap.draw(MAP_OFFSET_X, MAP_OFFSET_Y)
  end

  def spawn_point
    spawn_x = WALL_WIDTH + 5
    pos_x = MAP_OFFSET_X + spawn_x * TILE_SIZE
    pos_y = MAP_OFFSET_Y + 18 * TILE_SIZE - FRAME_HEIGHT
    Mathf::Vec2.new(pos_x, pos_y)
  end

  private

  def setup_tilemap
    @tileset_tex = Graphics::Texture.load("oak_woods/oak_woods_tileset.png")
    @tilemap = Graphics::Tilemap.new(@tileset_tex, TILE_SIZE_PX, TILE_SIZE_PX, MAP_WIDTH, MAP_HEIGHT)

    Tiles::SOLID_TILES.each do |tile_id|
      @tilemap.define_tile(tile_id, { solid: true })
    end
  end

  def build_layout
    playable_start = WALL_WIDTH
    playable_end = MAP_WIDTH - WALL_WIDTH - 1

    @tilemap.fill_rect(playable_start, 19, playable_end - playable_start + 1, 1, Tiles::GROUND_TOP_CENTER)
    @tilemap.fill_rect(playable_start, 20, playable_end - playable_start + 1, MAP_HEIGHT - 20, Tiles::GROUND_MID_CENTER)
    @tilemap.fill_rect(0, 0, WALL_WIDTH, MAP_HEIGHT, Tiles::GROUND_MID_CENTER)
    @tilemap.fill_rect(MAP_WIDTH - WALL_WIDTH, 0, WALL_WIDTH, MAP_HEIGHT, Tiles::GROUND_MID_CENTER)
  end

  def setup_parallax
    @bg1_tex = Graphics::Texture.load("oak_woods/background/background_layer_1.png")
    @bg2_tex = Graphics::Texture.load("oak_woods/background/background_layer_2.png")
    @bg3_tex = Graphics::Texture.load("oak_woods/background/background_layer_3.png")

    target_y = 16.0
    visual_y = 10.0
    bg_scale = 1.5

    bg1_y = visual_y - target_y * (1.0 - BG_PARALLAX_1)
    bg2_y = visual_y - target_y * (1.0 - BG_PARALLAX_2)
    bg3_y = visual_y - target_y * (1.0 - BG_PARALLAX_3)

    @bg1_sprites = create_parallax_layer(@bg1_tex, BG_PARALLAX_1, bg1_y, -30.0, bg_scale)
    @bg2_sprites = create_parallax_layer(@bg2_tex, BG_PARALLAX_2, bg2_y, -30.0, bg_scale)
    @bg3_sprites = create_parallax_layer(@bg3_tex, BG_PARALLAX_3, bg3_y, -30.0, bg_scale)
  end

  def create_parallax_layer(texture, parallax_factor, y_pos, start_x = 0.0, scale = 1.0)
    sprites = []
    num_panels = 7

    num_panels.times do |i|
      x_pos = start_x + i * BG_LAYER_WIDTH * scale
      t = Transform2D.new(x: x_pos, y: y_pos, parallax: parallax_factor, scale_x: scale, scale_y: scale)
      s = Graphics::Sprite.new(texture, t)
      sprites << s
    end
    sprites
  end
end
