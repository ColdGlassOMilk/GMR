# MinimapOverlay - Renders a small minimap in the corner
# Demonstrates the overlay system - this scene renders on top of GameScene

class MinimapOverlay < GMR::Scene
  MINIMAP_SCALE = 4       # Each tile = 4 pixels on minimap
  PADDING = 10            # Distance from screen edge
  BORDER_SIZE = 2         # Border thickness

  def init
    @game_scene = nil
    @visible = true
  end

  def set_game_scene(scene)
    @game_scene = scene
  end

  def update(dt)
    # Toggle minimap with M key
    if GMR::Input.key_pressed?(:m)
      @visible = !@visible
    end
  end

  def draw
    return unless @visible
    return unless @game_scene

    # Calculate minimap dimensions
    map_w = GameScene::MAP_WIDTH * MINIMAP_SCALE
    map_h = GameScene::MAP_HEIGHT * MINIMAP_SCALE

    # Position in top-right corner
    screen_w = GameScene::SCREEN_WIDTH
    x = screen_w - map_w - PADDING - BORDER_SIZE * 2
    y = PADDING

    # Draw border/background
    GMR::Graphics.draw_rect(
      x - BORDER_SIZE,
      y - BORDER_SIZE,
      map_w + BORDER_SIZE * 2,
      map_h + BORDER_SIZE * 2,
      [20, 20, 20, 220]
    )

    # Draw tiles
    GameScene::MAP_HEIGHT.times do |ty|
      GameScene::MAP_WIDTH.times do |tx|
        tile = @game_scene.get_tile(tx, ty)
        color = tile_to_color(tile)

        px = x + tx * MINIMAP_SCALE
        py = y + ty * MINIMAP_SCALE

        GMR::Graphics.draw_rect(px, py, MINIMAP_SCALE, MINIMAP_SCALE, color)
      end
    end

    # Draw player position
    if @game_scene.respond_to?(:player_position)
      pos = @game_scene.player_position
      player_tx = (pos.x / GameScene::TILE_SIZE).to_i
      player_ty = (pos.y / GameScene::TILE_SIZE).to_i

      px = x + player_tx * MINIMAP_SCALE
      py = y + player_ty * MINIMAP_SCALE

      # Player dot (slightly larger, with glow effect)
      GMR::Graphics.draw_rect(px - 1, py - 1, MINIMAP_SCALE + 2, MINIMAP_SCALE + 2, [255, 255, 100, 150])
      GMR::Graphics.draw_rect(px, py, MINIMAP_SCALE, MINIMAP_SCALE, [255, 255, 0, 255])
    end

    # Draw camera viewport rectangle
    if @game_scene.respond_to?(:camera_bounds)
      bounds = @game_scene.camera_bounds
      cx = x + (bounds[:left] / GameScene::TILE_SIZE * MINIMAP_SCALE).to_i
      cy = y + (bounds[:top] / GameScene::TILE_SIZE * MINIMAP_SCALE).to_i
      cw = ((bounds[:right] - bounds[:left]) / GameScene::TILE_SIZE * MINIMAP_SCALE).to_i
      ch = ((bounds[:bottom] - bounds[:top]) / GameScene::TILE_SIZE * MINIMAP_SCALE).to_i

      GMR::Graphics.draw_rect_outline(cx, cy, cw, ch, [255, 255, 255, 180])
    end

    # Label
    GMR::Graphics.draw_text("M: Toggle", x, y + map_h + 4, 10, [150, 150, 150])
  end

  def unload
    @game_scene = nil
  end

  private

  def tile_to_color(tile)
    case tile
    when :grass then [80, 160, 80, 255]
    when :stone then [160, 160, 180, 255]
    when :water then [64, 140, 200, 255]
    when :wall  then [100, 60, 60, 255]
    when :sand  then [200, 170, 100, 255]
    else [100, 100, 100, 255]
    end
  end
end
