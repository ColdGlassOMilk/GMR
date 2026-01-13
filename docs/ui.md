# UI

GMR provides a declarative UI system for building menus, HUDs, and interactive interfaces. The UI DSL uses a block-based syntax similar to the state machine, with automatic layout, scaling, and input handling.

## Basic Structure

```ruby
include GMR

def init
  @ui = Graphics.ui do
    panel layout: :vertical, padding: 16, anchor: :center, width: 300, height: 400 do
      label "My Game"
      button "Start" do
        on_click { start_game }
      end
    end
  end
end
```

The `Graphics.ui` block creates a UI tree that renders automatically on top of game content.

## Panels

Panels are containers that hold other UI elements. They support automatic layout.

### Basic Panel

```ruby
Graphics.ui do
  panel width: 200, height: 150, background_color: [40, 40, 60] do
    # Child elements here
  end
end
```

### Layout Modes

Panels can automatically arrange children:

```ruby
# Vertical stacking (top to bottom)
panel layout: :vertical, spacing: 8 do
  label "Option 1"
  label "Option 2"
  label "Option 3"
end

# Horizontal stacking (left to right)
panel layout: :horizontal, spacing: 12 do
  button "Yes"
  button "No"
end
```

| Layout | Behavior |
|--------|----------|
| `:none` (default) | Children positioned manually |
| `:vertical` | Stack children top to bottom |
| `:horizontal` | Stack children left to right |

### Panel Options

```ruby
panel(
  width: 200,                      # Width in 360p baseline units
  height: 150,                     # Height in 360p baseline units
  x: 10, y: 10,                    # Position offset
  padding: 16,                     # Padding on all sides
  spacing: 8,                      # Space between children (for layout modes)
  anchor: :center,                 # Positioning anchor
  background_color: [30, 30, 46],  # RGBA or RGB array
  visible: true                    # Show/hide panel
)
```

### Anchoring

Control where the panel is positioned relative to its parent:

```ruby
panel anchor: :center        # Center of parent
panel anchor: :top_left      # Top-left corner
panel anchor: :bottom_right  # Bottom-right corner
```

| Anchor | Position |
|--------|----------|
| `:top_left` | Top-left (default) |
| `:top_center` | Top, centered horizontally |
| `:top_right` | Top-right |
| `:center_left` | Left, centered vertically |
| `:center` | Center of parent |
| `:center_right` | Right, centered vertically |
| `:bottom_left` | Bottom-left |
| `:bottom_center` | Bottom, centered horizontally |
| `:bottom_right` | Bottom-right |

## Labels

Labels display static text.

```ruby
label "Hello, World!"

# With styling
label "Game Title", font_size: 32, text_color: [255, 200, 100]

# With custom font
@font = Graphics::Font.load("fonts/custom.ttf", size: 48)
label "Styled Text", font: @font, font_size: 24
```

### Label Options

```ruby
label "Text",
  font_size: 16,                   # Font size in 360p baseline units
  text_color: [255, 255, 255],     # RGB or RGBA array
  font: @custom_font               # Custom font object (optional)
```

## Buttons

Buttons are interactive elements that respond to hover and click events.

### Basic Button

```ruby
button "Click Me" do
  on_click { puts "Button clicked!" }
end
```

### Button Events

```ruby
button "Interactive" do
  on_click { perform_action }
  on_hover_enter { play_hover_sound }
  on_hover_exit { reset_cursor }
end
```

| Event | When it fires |
|-------|---------------|
| `on_click` | Button is clicked (mouse up while over button) |
| `on_hover_enter` | Mouse enters button area |
| `on_hover_exit` | Mouse leaves button area |

### Button Styling

Buttons support different colors for each state:

```ruby
button "Styled",
  width: 120,
  height: 40,
  background_color: [60, 60, 80],       # Normal state
  hover_background: [80, 80, 110],      # Mouse over
  pressed_background: [40, 40, 60],     # Mouse down
  text_color: [220, 220, 240],
  font_size: 14
```

## Styles

Define reusable style presets to avoid repetition.

### Registering Styles

```ruby
UI::Styles.register(:primary_button, {
  width: 140,
  height: 36,
  background_color: [60, 100, 60],
  hover_background: [80, 130, 80],
  pressed_background: [40, 70, 40],
  text_color: [220, 255, 220],
  font_size: 15
})

UI::Styles.register(:danger_button, {
  width: 140,
  height: 28,
  background_color: [100, 50, 50],
  hover_background: [130, 70, 70],
  pressed_background: [70, 30, 30],
  text_color: [255, 200, 200],
  font_size: 12
})
```

### Using Styles

```ruby
button "Start Game", style: :primary_button do
  on_click { start_game }
end

button "Quit", style: :danger_button do
  on_click { System.quit }
end
```

### Overriding Style Properties

Inline options override style defaults:

```ruby
button "Custom", style: :primary_button, width: 200 do
  on_click { ... }
end
```

## Resolution Independence

UI dimensions are specified in 360p baseline coordinates and automatically scale to any resolution. A button that's 100 units wide at 360p will scale proportionally at 720p, 1080p, or any other resolution.

```ruby
# These values work identically at all resolutions
panel width: 200, height: 150, padding: 16, spacing: 8 do
  label "Title", font_size: 24
  button "Action", width: 100, height: 30
end
```

The UI system uses the same scaling as `Graphics.draw_text`, ensuring consistent sizing across all UI elements.

## Custom Fonts

Load and use custom fonts in UI elements:

```ruby
def init
  @title_font = Graphics::Font.load("fonts/title.ttf", size: 48)
  @body_font = Graphics::Font.load("fonts/body.ttf", size: 32)

  Graphics.ui do
    panel layout: :vertical, padding: 20, anchor: :center do
      label "Game Title", font: @title_font, font_size: 32
      label "Press Start", font: @body_font, font_size: 16

      button "Play", font: @body_font do
        on_click { start_game }
      end
    end
  end
end
```

## Sound Effects

Add audio feedback to UI interactions:

```ruby
def init
  @hover_sfx = Audio::Sound.load("sfx/hover.mp3", volume: 0.5)
  @click_sfx = Audio::Sound.load("sfx/click.mp3", volume: 0.7)

  # Capture for use in block
  hover_sfx = @hover_sfx
  click_sfx = @click_sfx

  Graphics.ui do
    panel layout: :vertical, anchor: :center do
      button "Start" do
        on_hover_enter { hover_sfx.play }
        on_click do
          click_sfx.play
          start_game
        end
      end

      button "Quit" do
        on_hover_enter { hover_sfx.play }
        on_click do
          click_sfx.play
          System.quit
        end
      end
    end
  end
end
```

## Clearing UI

Remove the UI tree when leaving a scene or switching menus:

```ruby
def unload
  UI.clear
end

# Or when transitioning
button "Start Game" do
  on_click do
    UI.clear
    SceneManager.load(GameScene.new)
  end
end
```

## Complete Menu Example

```ruby
include GMR

class MenuScene < GMR::Scene
  def init
    @font = Graphics::Font.load("fonts/Ubuntu-Regular.ttf", size: 48)
    @hover_sfx = Audio::Sound.load("sfx/hover.mp3", volume: 1.0)
    @click_sfx = Audio::Sound.load("sfx/click.mp3", volume: 0.7)

    # Register styles
    UI::Styles.register(:menu_title, {
      font_size: 32,
      text_color: [233, 69, 96]
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

    # Capture for block scope
    font = @font
    hover_sfx = @hover_sfx
    click_sfx = @click_sfx

    @ui = Graphics.ui do
      panel layout: :vertical, padding: 20, spacing: 12, anchor: :center,
            width: 200, height: 250, background_color: [30, 30, 46, 240] do
        label "My Game", style: :menu_title
        label "A Ruby Adventure", font_size: 12, text_color: [150, 150, 170], font: font

        panel height: 12  # Spacer

        button "Start Game", style: :start_button, font: font do
          on_hover_enter { hover_sfx.play }
          on_click do
            click_sfx.play
            UI.clear
            SceneManager.load(GameScene.new, transition: :fade, duration: 0.5)
          end
        end

        button "Options", style: :menu_button, font: font do
          on_hover_enter { hover_sfx.play }
          on_click do
            click_sfx.play
            puts "Options not implemented"
          end
        end

        button "Quit", style: :menu_button, font: font do
          on_hover_enter { hover_sfx.play }
          on_click do
            click_sfx.play
            System.quit
          end
        end
      end
    end
  end

  def update(dt)
    # UI handles its own input
  end

  def draw
    Graphics.clear([26, 26, 46])
  end

  def unload
    UI.clear
  end
end
```

## Common Patterns

### Pause Menu Overlay

```ruby
def toggle_pause
  if @paused
    UI.clear
    @paused = false
  else
    create_pause_menu
    @paused = true
  end
end

def create_pause_menu
  Graphics.ui do
    # Semi-transparent overlay
    panel width: 0, height: 0, background_color: [0, 0, 0, 128] do
      panel layout: :vertical, padding: 20, spacing: 10, anchor: :center,
            width: 200, height: 200, background_color: [40, 40, 60] do
        label "Paused", font_size: 24

        button "Resume" do
          on_click { toggle_pause }
        end

        button "Quit to Menu" do
          on_click do
            UI.clear
            SceneManager.load(MenuScene.new)
          end
        end
      end
    end
  end
end
```

### HUD Elements

```ruby
def create_hud
  Graphics.ui do
    # Health bar in top-left
    panel x: 10, y: 10, width: 100, height: 20, anchor: :top_left,
          background_color: [40, 40, 40] do
      # Health fill would need dynamic updates
    end

    # Score in top-right
    panel x: -10, y: 10, anchor: :top_right do
      label "Score: 0", font_size: 16, text_color: [255, 255, 255]
    end
  end
end
```

### Nested Panels

```ruby
Graphics.ui do
  panel layout: :vertical, anchor: :center, width: 400, height: 300 do
    label "Settings", font_size: 24

    panel layout: :horizontal, spacing: 20 do
      panel layout: :vertical, width: 180 do
        label "Audio"
        button "Music: On"
        button "SFX: On"
      end

      panel layout: :vertical, width: 180 do
        label "Video"
        button "Fullscreen: Off"
        button "VSync: On"
      end
    end
  end
end
```

## Best Practices

1. **Use styles for consistency** - Define styles once, use everywhere
2. **Clear UI on scene transitions** - Call `UI.clear` in `unload` or before loading new scenes
3. **Capture variables for blocks** - Ruby blocks capture local variables, not instance variables
4. **Design for 360p baseline** - All dimensions scale automatically
5. **Add audio feedback** - Hover and click sounds improve feel
6. **Keep UI trees simple** - Deeply nested panels can be hard to maintain

## See Also

- [Graphics](graphics.md) - Drawing and text rendering
- [Audio](audio.md) - Sound effects for UI
- [Scenes](scenes.md) - Scene transitions with UI
- [Input](input.md) - Raw input alongside UI
