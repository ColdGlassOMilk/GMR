# GMR Demo - scripts/main.rb

def init
  set_window_title("GMR Demo")
  
  $x = screen_width / 2.0
  $y = screen_height / 2.0
  $speed = 200
  $hue = 0
end

def update(dt)
  # Movement
  $x -= $speed * dt if key_down?(263)  # LEFT
  $x += $speed * dt if key_down?(262)  # RIGHT
  $y -= $speed * dt if key_down?(265)  # UP
  $y += $speed * dt if key_down?(264)  # DOWN
  
  # Fullscreen toggle
  toggle_fullscreen if key_pressed?(70)  # F key
  
  # Cycle color
  $hue = ($hue + 60 * dt) % 360
  
  # Keep in bounds
  $x = [[$x, 25].max, screen_width - 25].min
  $y = [[$y, 25].max, screen_height - 25].min
end

def draw
  clear_screen(20, 20, 30)
  
  # Draw player with cycling color
  r, g, b = hsv_to_rgb($hue, 1.0, 1.0)
  set_color(r, g, b)
  draw_circle($x.to_i, $y.to_i, 25)
  
  # Outline
  set_color(255, 255, 255)
  draw_circle_lines($x.to_i, $y.to_i, 25)
  
  # Info
  draw_text("GMR Demo", 10, 10, 20)
  draw_text("Arrow keys to move, F for fullscreen", 10, 35, 16)
  draw_text("FPS: #{get_fps}", 10, screen_height - 25, 16)
end

# Helper: HSV to RGB
def hsv_to_rgb(h, s, v)
  c = v * s
  x = c * (1 - ((h / 60.0) % 2 - 1).abs)
  m = v - c
  
  r, g, b = case (h / 60).to_i
    when 0 then [c, x, 0]
    when 1 then [x, c, 0]
    when 2 then [0, c, x]
    when 3 then [0, x, c]
    when 4 then [x, 0, c]
    else [c, 0, x]
  end
  
  [((r + m) * 255).to_i, ((g + m) * 255).to_i, ((b + m) * 255).to_i]
end
