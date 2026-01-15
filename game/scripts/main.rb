include GMR

def init
  Window.set_size(WINDOW_WIDTH, WINDOW_HEIGHT)
        .set_filter_point

  # Start with the title scene
  # Virtual resolution is enabled per-scene (GameScene enables it)
  SceneManager.load(TitleScene.new)
end

def update(dt)
  # Global keyboard shortcuts (non-web only)
  unless System.platform == "web"
    System.quit if Input.key_pressed?(:escape)
    Window.toggle_fullscreen if Input.key_down?(:left_control) && Input.key_pressed?(:f)
    SceneManager.load(TitleScene.new) if Input.key_down?(:left_control) && Input.key_pressed?(:r)
  end

  # Update the current scene
  SceneManager.update(dt)
end

def draw
  # Draw the current scene
  SceneManager.draw
end

def on_resize(width, height)
  # Forward resize events to current scene
  scene = SceneManager.current
  scene.on_resize(width, height)
end
