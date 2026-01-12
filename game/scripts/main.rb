include GMR

def init
  Window.set_size(WINDOW_WIDTH, WINDOW_HEIGHT)
        .set_filter_point

  # Start with the title scene
  SceneManager.load(TitleScene.new)
end

def update(dt)
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
