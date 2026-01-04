
# Transform from world to screen coordinates
def world_to_screen_x(wx)
  (wx - $camera_x) * $scale + $offset_x
end

def world_to_screen_y(wy)
  (wy - $camera_y) * $scale + $offset_y
end

# Transform from screen to world coordinates (for mouse input)
def screen_to_world_x(sx)
  (sx - $offset_x) / $scale + $camera_x
end

def screen_to_world_y(sy)
  (sy - $offset_y) / $scale + $camera_y
end

# Scaled size
def scaled(size)
  (size * $scale).to_i
end

def update_camera
  $screen_width = W.width
  $screen_height = W.height

  # Calculate scale to fit world in screen (fill, not letterbox)
  scale_x = $screen_width / WORLD_WIDTH
  scale_y = $screen_height / WORLD_HEIGHT
  $scale = [scale_x, scale_y].max  # Use max to fill screen (crop edges)

  # Calculate visible world area
  visible_width = $screen_width / $scale
  visible_height = $screen_height / $scale

  # Center camera on world center
  $camera_x = (WORLD_WIDTH - visible_width) / 2
  $camera_y = (WORLD_HEIGHT - visible_height) / 2

  # Offset to center the view
  $offset_x = 0
  $offset_y = 0
end

def spawn_particle_burst(cx, cy, count)
  $particles ||= []
  count.times do |i|
    angle = (i.to_f / count) * Math::PI * 2 + rand * 0.5
    speed = rand * 3 + 1.5
    $particles << {
      x: cx.to_f,
      y: cy.to_f,
      vx: Math.cos(angle) * speed,
      vy: Math.sin(angle) * speed,
      radius: System.random_int(4, 12),
      hue: System.random_int(1, 1),
      life: 1.0,
      decay: rand * 0.0003 + 0.0001,
      mass: rand * 2 + 1
    }
    $spawn_count = ($spawn_count || 0) + 1
  end
end

def spawn_text_particle(x, y, text)
  $text_particles ||= []
  $text_particles << {
    x: x.to_f,
    y: y.to_f,
    vy: -2,
    text: text,
    life: 1.0
  }
end

def hue_to_rgb(h, s = 1.0, v = 1.0)
  h = h % 360
  c = v * s
  x = c * (1 - ((h / 60.0) % 2 - 1).abs)
  m = v - c

  r, g, b = case (h / 60).to_i
    when 0 then [c, x, 0]
    when 1 then [x, c, 0]
    when 2 then [0, c, x]
    when 3 then [0, x, c]
    when 4 then [x, 0, c]
    else        [c, 0, x]
  end

  [((r + m) * 255).to_i, ((g + m) * 255).to_i, ((b + m) * 255).to_i]
end