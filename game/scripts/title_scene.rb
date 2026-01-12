include GMR

class TitleScene < GMR::Scene
  def init
    @font = Graphics::Font.load("fonts/Ubuntu-Regular.ttf", size: 96)
    @timer = 0.0
    @hold_duration = 1.5  # Show title for 1.5 seconds before transitioning
    @transitioned = false
  end

  def update(dt)
    @timer += dt

    # After hold duration, transition to game scene
    if @timer >= @hold_duration && !@transitioned
      @transitioned = true
      SceneManager.load(GameScene.new, transition: :fade, duration: 1.0, easing: :out_cubic)
    end
  end

  def draw
    # Dark navy background
    Graphics.clear([26, 26, 46])

    # Get screen dimensions
    width = Window.width
    height = Window.height

    # Draw "GMR" centered (shifted up slightly for subtitle)
    title = "GMR"
    title_size = 96
    title_measured = Graphics.measure_text(title, title_size, font: @font)
    title_x = (width - title_measured.x) / 2.0
    title_y = (height - title_measured.y) / 2.0 - 15

    # Vibrant pink-red with fade-in
    title_alpha = [(@timer / 0.5) * 255, 255].min.to_i
    Graphics.draw_text(title, title_x, title_y, title_size, [233, 69, 96, title_alpha], font: @font)

    # Draw subtitle with delayed fade-in
    subtitle = "Build Games with Ruby!"
    subtitle_size = 24
    subtitle_measured = Graphics.measure_text(subtitle, subtitle_size, font: @font)
    subtitle_x = (width - subtitle_measured.x) / 2.0
    subtitle_y = title_y + title_measured.y + 8

    # Delayed fade-in (starts 0.3s after title)
    subtitle_timer = [@timer - 0.3, 0].max
    subtitle_alpha = [(subtitle_timer / 0.5) * 255, 255].min.to_i
    Graphics.draw_text(subtitle, subtitle_x, subtitle_y, subtitle_size, [180, 180, 200, subtitle_alpha], font: @font)
  end

  def unload
    # Cleanup if needed
  end
end
