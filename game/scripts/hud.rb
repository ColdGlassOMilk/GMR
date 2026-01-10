class HUD
  def initialize(font)
    @font = font
  end

  def draw(fps, width, height, retro_mode)
    if retro_mode
      Graphics.draw_text("#{fps} FPS", 5, 5, 30, :cyan)
      Graphics.draw_text("#{width}x#{height} [R - Toggle]", 5, 30, 30, :cyan)
    else
      Graphics.draw_text("#{fps} FPS", 5, 5, 18, :cyan, font: @font)
      Graphics.draw_text("#{width}x#{height} [R - Toggle]", 5, 22, 18, :cyan, font: @font)
    end
  end
end
