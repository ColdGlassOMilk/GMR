class Level
  attr_reader :tilemap

  # Decoration definitions: type => { texture:, frames:, fps:, width:, height: }
  DECORATIONS = {
    shop: {
      texture: "oak_woods/decorations/shop_anim.png",
      frames: 6,
      fps: 8,
      width: 118,
      height: 128
    }
  }

  def initialize
    @map_data = MapLoader.load("demo.txt")
    @decorations = []
    setup_tilemap
    build_layout
    setup_decorations
    setup_parallax
  end

  def update(dt)
    @decorations.each do |d|
      d[:frame_time] += dt
      if d[:frame_time] >= d[:frame_duration]
        d[:frame_time] -= d[:frame_duration]
        d[:frame] = (d[:frame] + 1) % d[:frame_count]
        d[:sprite].source_rect = Graphics::Rect.new(
          d[:frame] * d[:frame_width], 0,
          d[:frame_width], d[:frame_height]
        )
      end
    end
  end

  def draw
    @bg1_sprites.each { |s| s.draw }
    @bg2_sprites.each { |s| s.draw }
    @bg3_sprites.each { |s| s.draw }
    @tilemap.draw(MAP_OFFSET_X, MAP_OFFSET_Y)
    @decorations.each { |d| d[:sprite].draw }
  end

  def spawn_point
    spawn = @map_data[:spawn] || { x: WALL_WIDTH + 5, y: 18 }
    pos_x = MAP_OFFSET_X + spawn[:x] * TILE_SIZE
    pos_y = MAP_OFFSET_Y + spawn[:y] * TILE_SIZE - FRAME_HEIGHT
    Mathf::Vec2.new(pos_x, pos_y)
  end

  private

  def setup_tilemap
    @tileset_tex = Graphics::Texture.load("oak_woods/oak_woods_tileset.png")
    map_width = @map_data[:width] || MAP_WIDTH
    map_height = @map_data[:height] || MAP_HEIGHT
    @tilemap = Graphics::Tilemap.new(@tileset_tex, TILE_SIZE_PX, TILE_SIZE_PX, map_width, map_height)

    Tiles::SOLID_TILES.each do |tile_id|
      @tilemap.define_tile(tile_id, { solid: true })
    end

    # Platform tiles - can jump through from below, land on from above
    @tilemap.define_tile(Tiles::THIN_CENTER, { platform: true })
  end

  def build_layout
    tiles = @map_data[:tiles]
    tiles.each_with_index do |row, y|
      row.each_with_index do |tile_id, x|
        @tilemap.set(x, y, tile_id) if tile_id
      end
    end
  end

  def setup_decorations
    @map_data[:decorations].each do |dec|
      config = DECORATIONS[dec[:type]]
      next unless config

      tex = Graphics::Texture.load(config[:texture])

      # Convert tile coords to world coords (y + 1 to place on ground)
      world_x = MAP_OFFSET_X + dec[:x] * TILE_SIZE
      world_y = MAP_OFFSET_Y + (dec[:y] + 1) * TILE_SIZE

      # Anchor at bottom-center of decoration
      dec_width = config[:width] / ASSET_PPU
      dec_height = config[:height] / ASSET_PPU
      world_x -= dec_width / 2.0
      world_y -= dec_height

      t = Transform2D.new(x: world_x, y: world_y)
      sprite = Graphics::Sprite.new(tex, t)
      sprite.source_rect = Graphics::Rect.new(0, 0, config[:width], config[:height])

      # Manual animation state (no Animator)
      @decorations << {
        sprite: sprite,
        frame: 0,
        frame_time: 0.0,
        frame_count: config[:frames],
        frame_duration: 1.0 / config[:fps],
        frame_width: config[:width],
        frame_height: config[:height]
      }
    end
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
