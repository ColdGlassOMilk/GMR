include GMR

class MenuScene < GMR::Scene
  def init
    @font = Graphics::Font.load("fonts/Ubuntu-Regular.ttf", size: 48)
    @hover_sfx = Audio::Sound.load("sfx/hover.mp3", volume: 1.0)
    @click_sfx = Audio::Sound.load("sfx/click.mp3", volume: 0.7)

    # Register menu styles (values in 360p baseline - auto-scaled to resolution)
    UI::Styles.register(:menu_title, {
      font_size: 32,
      text_color: [233, 69, 96]
    })

    UI::Styles.register(:menu_subtitle, {
      font_size: 12,
      text_color: [150, 150, 170]
    })

    UI::Styles.register(:menu_button, {
      width: 140,
      height: 32,
      background_color: [60, 60, 80],
      hover_background: [80, 80, 110],
      pressed_background: [40, 40, 60],
      text_color: [220, 220, 240],
      font_size: 14
    })

    UI::Styles.register(:start_button, {
      width: 140,
      height: 36,
      background_color: [60, 100, 60],
      hover_background: [80, 130, 80],
      pressed_background: [40, 70, 40],
      text_color: [220, 255, 220],
      font_size: 15
    })

    UI::Styles.register(:quit_button, {
      width: 140,
      height: 28,
      background_color: [100, 50, 50],
      hover_background: [130, 70, 70],
      pressed_background: [70, 30, 30],
      text_color: [255, 200, 200],
      font_size: 12
    })

    # Create UI (values in 360p baseline - auto-scaled to resolution)
    # Capture variables for use in block
    font = @font
    hover_sfx = @hover_sfx
    click_sfx = @click_sfx

    @ui = Graphics.ui do
      panel layout: :vertical, padding: 20, spacing: 12, anchor: :center, width: 200, height: 250, background_color: [30, 30, 46, 240] do
        label "GMR", style: :menu_title  # Title uses default font
        label "Engine Demo", style: :menu_subtitle, font: font

        # Spacer panel
        panel height: 4

        button "Start Demo", style: :start_button, font: font do
          on_hover_enter { hover_sfx.play }
          on_click do
            click_sfx.play
            UI.clear
            SceneManager.load(GameScene.new, transition: :fade, duration: 0.5, easing: :out_cubic)
          end
        end

        button "Options", style: :menu_button, font: font do
          on_hover_enter { hover_sfx.play }
          on_click do
            click_sfx.play
            puts "Options clicked - not implemented"
          end
        end

        button "Quit", style: :quit_button, font: font do
          on_hover_enter { hover_sfx.play }
          on_click do
            click_sfx.play
            GMR::System.quit
          end
        end
      end
    end
  end

  def update(dt)
    # UI handles its own input
  end

  def draw
    # Dark navy background matching title scene
    Graphics.clear([26, 26, 46])

    # Draw some subtle background decoration
    width = Window.width
    height = Window.height

    # Draw version info
    Graphics.draw_text("v#{GMR::VERSION}", 10, height - 25, 14, [80, 80, 100])
  end

  def unload
    UI.clear
  end
end
