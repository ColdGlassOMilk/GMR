class HUD
  def initialize(font)
    @font = font
  end

  def draw(fps, width, height, retro_mode, shader_name = "none")
    if retro_mode
      Graphics.draw_text("#{fps} FPS", 5, 5, 30, :cyan)
      Graphics.draw_text("#{width}x#{height}", 5, 30, 30, :cyan)
      Graphics.draw_text("Shader: #{shader_name} [Q/E]", 5, 55, 30, :yellow)
    else
      Graphics.draw_text("#{fps} FPS", 5, 5, 18, :cyan, font: @font)
      Graphics.draw_text("#{width}x#{height}", 5, 22, 18, :cyan, font: @font)
      Graphics.draw_text("Shader: #{shader_name} [Q/E]", 5, 39, 18, :yellow, font: @font)
    end
  end
end
