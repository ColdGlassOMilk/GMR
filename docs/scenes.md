# Scenes

For larger games with multiple screens, GMR provides Scene classes and a SceneManager. Scenes encapsulate game state and lifecycle, making it easier to manage title screens, gameplay, pause menus, and game over states.

## Scene Basics

Create a scene by subclassing `Scene`:

```ruby
include GMR

class GameScene < Scene
  def init
    # Called when this scene becomes active
    @player = Player.new(100, 100)
  end

  def update(dt)
    # Called every frame while this scene is on top
    @player.update(dt)
  end

  def draw
    # Called every frame for rendering
    Graphics.clear(:dark_gray)
    @player.draw
  end

  def unload
    # Called when this scene is removed from the stack
  end
end
```

Start your game with a scene:

```ruby
SceneManager.load(GameScene.new)
```

## Scene Lifecycle

| Method | When Called |
|--------|-------------|
| `init` | Scene becomes active (via `load` or `push`) |
| `update(dt)` | Every frame while scene is on top of stack |
| `draw` | Every frame while scene is on top of stack |
| `unload` | Scene is removed from the stack |

Only the **top scene** receives `update` and `draw` calls. Scenes below are paused but retain their state.

## SceneManager Operations

### `load(scene)`

Clears the entire stack and sets the given scene as the only active scene:

```ruby
SceneManager.load(TitleScene.new)
```

Use `load` for major transitions like starting a new game or returning to the main menu.

### `push(scene)`

Pushes a new scene onto the stack. The current scene is paused:

```ruby
SceneManager.push(PauseMenu.new)
```

Use `push` for pause menus, dialog boxes, or any screen that should return to the previous state.

### `pop`

Removes the top scene and resumes the one below:

```ruby
SceneManager.pop
```

The popped scene's `unload` is called. The scene below resumes receiving `update` and `draw`.

### `current`

Returns the currently active (top) scene:

```ruby
scene = SceneManager.current
```

## Scene Stack Example

A typical game flow:

```ruby
include GMR

class TitleScene < Scene
  def init
    @selected = 0
  end

  def update(dt)
    @selected -= 1 if Input.key_pressed?(:up)
    @selected += 1 if Input.key_pressed?(:down)
    @selected = @selected.clamp(0, 1)

    if Input.key_pressed?(:enter)
      if @selected == 0
        SceneManager.load(GameScene.new)  # Start game
      else
        System.quit  # Exit
      end
    end
  end

  def draw
    Graphics.clear("#1a1a2e")
    Graphics.draw_text("MY GAME", 400, 150, 48, :white)

    colors = [@selected == 0 ? :yellow : :gray, @selected == 1 ? :yellow : :gray]
    Graphics.draw_text("Start Game", 400, 300, 24, colors[0])
    Graphics.draw_text("Quit", 400, 340, 24, colors[1])
  end
end

class GameScene < Scene
  def init
    @player = Player.new(480, 270)
    @paused = false
  end

  def update(dt)
    if Input.key_pressed?(:escape)
      SceneManager.push(PauseScene.new)  # Pause without losing state
      return
    end

    @player.update(dt)
  end

  def draw
    Graphics.clear("#141428")
    @player.draw
  end
end

class PauseScene < Scene
  def update(dt)
    if Input.key_pressed?(:escape)
      SceneManager.pop  # Resume game
    end

    if Input.key_pressed?(:q)
      SceneManager.load(TitleScene.new)  # Return to title
    end
  end

  def draw
    # Draw semi-transparent overlay
    Graphics.draw_rect(0, 0, 960, 540, [0, 0, 0, 180])
    Graphics.draw_text("PAUSED", 400, 200, 48, :white)
    Graphics.draw_text("Press ESC to resume", 400, 280, 20, :gray)
    Graphics.draw_text("Press Q to quit to title", 400, 310, 20, :gray)
  end
end

# Start the game
SceneManager.load(TitleScene.new)
```

## Input Contexts with Scenes

Pair input contexts with scene transitions for clean input handling:

```ruby
# Define contexts
input_context :gameplay do |i|
  i.move_left [:a, :left]
  i.move_right [:d, :right]
  i.jump [:space, :w]
  i.pause :escape
end

input_context :menu do |i|
  i.confirm :enter
  i.cancel :escape
  i.nav_up :up
  i.nav_down :down
end

class GameScene < Scene
  def init
    Input.push_context(:gameplay)
    @player = Player.new(100, 100)
  end

  def update(dt)
    if Input.action_pressed?(:pause)
      SceneManager.push(PauseMenu.new)
    end

    @player.update(dt)
  end

  def unload
    Input.pop_context
  end
end

class PauseMenu < Scene
  def init
    Input.push_context(:menu)
    @selected = 0
  end

  def update(dt)
    @selected -= 1 if Input.action_pressed?(:nav_up)
    @selected += 1 if Input.action_pressed?(:nav_down)

    if Input.action_pressed?(:cancel)
      SceneManager.pop
    end
  end

  def unload
    Input.pop_context
  end
end
```

## Overlays

Overlays render on top of the current scene without pausing it. Use overlays for HUDs, minimaps, notifications, and debug info.

### Adding Overlays

```ruby
class HUDOverlay < Scene
  def init
    @score = 0
  end

  def draw
    Graphics.draw_text("Score: #{@score}", 10, 10, 20, :white)
    Graphics.draw_text("Health: #{@health}", 10, 35, 20, :red)
  end

  def update_score(score)
    @score = score
  end
end

# In your game init:
@hud = HUDOverlay.new
SceneManager.add_overlay(@hud)
```

### Overlay Behavior

- Overlays receive `draw` calls after the main scene
- The scene beneath continues receiving both `update` and `draw`
- Overlays do not receive `update` calls by default
- Multiple overlays render in the order they were added

### Managing Overlays

```ruby
SceneManager.add_overlay(overlay)      # Add overlay
SceneManager.remove_overlay(overlay)   # Remove overlay
SceneManager.has_overlay?(overlay)     # Check if overlay exists
```

### Overlay Example: Minimap

```ruby
class MinimapOverlay < Scene
  def initialize(player, level)
    @player = player
    @level = level
  end

  def draw
    # Draw minimap background
    Graphics.draw_rect(10, 10, 150, 100, [0, 0, 0, 150])

    # Draw player position on minimap
    map_x = 10 + (@player.x / @level.width * 150)
    map_y = 10 + (@player.y / @level.height * 100)
    Graphics.draw_circle(map_x, map_y, 3, :cyan)
  end
end
```

## Passing Data Between Scenes

### Constructor Arguments

Pass data through the scene constructor:

```ruby
class GameOverScene < Scene
  def initialize(final_score)
    @final_score = final_score
  end

  def draw
    Graphics.draw_text("Game Over", 400, 200, 48, :red)
    Graphics.draw_text("Final Score: #{@final_score}", 400, 280, 24, :white)
  end
end

# When player dies:
SceneManager.load(GameOverScene.new(@player.score))
```

### Shared State Objects

For complex state, pass shared objects:

```ruby
class GameState
  attr_accessor :player_data, :inventory, :progress
end

class GameScene < Scene
  def initialize(game_state)
    @state = game_state
  end
end

class InventoryScene < Scene
  def initialize(game_state)
    @state = game_state
  end
end

# Both scenes share the same state object
@game_state = GameState.new
SceneManager.load(GameScene.new(@game_state))
```

## When to Use Scenes vs Global Hooks

| Use Global Hooks When | Use Scenes When |
|-----------------------|-----------------|
| Building a prototype | Multiple distinct screens |
| Single-screen game | Pause menu that preserves game state |
| Learning GMR | Clean separation of concerns |
| Game jam with time pressure | Larger, maintainable codebase |

You can also mix approaches - use global hooks for your main game and push Scene-based pause menus.

## See Also

- [Game Loop](game-loop.md) - Global hooks pattern
- [Input](input.md) - Input contexts
- [Engine Model](engine-model.md) - Lifecycle guarantees
