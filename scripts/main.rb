# game.rb - Interactive particle physics playground

# Key constants (raylib key codes)
KEY_SPACE = 32
KEY_R = 82
KEY_G = 71
KEY_C = 67
KEY_F = 70
KEY_D = 68  # Toggle debug info
MOUSE_LEFT = 0
MOUSE_RIGHT = 1

def init
  # width = 1280
  # height = 720
  # set_virtual_resolution(width, height)
  # set_window_size(width, height)
  # toggle_fullscreen

  set_window_title "GMR Demo"
  set_window_size(960, 640)
  set_virtual_resolution(960, 640)

  set_filter_bilinear
  # set_filter_point

  $width = screen_width
  $height = screen_height
  $time = 0.0
  $gravity_enabled = false  # Start with gravity OFF
  $show_connections = true
  $show_debug_info = true   # Toggle debug info panel
  
  # Player controlled attractor/repulsor
  $attractor = {
    x: $width / 2.0,
    y: $height / 2.0,
    radius: 30,
    strength: 500,
    mode: :attract  # :attract or :repel
  }
  
  # Starfield background
  $stars = []
  300.times do
    $stars << {
      x: rand * $width,
      y: rand * $height,
      z: rand * 3 + 0.5,
      brightness: random_int(100, 255)
    }
  end
  
  # Physics particles - start with more!
  $particles = []
  spawn_particle_burst($width / 2, $height / 2, 40)
  
  # Orbital rings
  $rings = []
  5.times do |i|
    $rings << {
      radius: 80 + i * 40,
      rotation: rand * 360,
      speed: (i % 2 == 0 ? 1 : -1) * (30 + i * 10),
      segments: 8 + i * 4,
      hue: i * 60
    }
  end
  
  # Floating text particles
  $text_particles = []
  
  # Stats
  $spawn_count = 40
  
  puts "Init complete!"
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
      radius: random_int(4, 12),
      hue: random_int(1, 1),
      life: 1.0,
      decay: rand * 0.0003 + 0.0001,  # Slower decay - live longer
      # trail: [],
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

def update(dt)
  $width = screen_width
  $height = screen_height

  unless $stars && $particles && $rings && $text_particles && $attractor
    init
    return
  end

  # Clamp dt to prevent huge jumps when window loses focus
  dt = [dt, 0.1].min

  $time += dt

  # Only process game input when console is closed
  unless console_open?
    # Update attractor position (follows mouse)
    $attractor[:x] = mouse_x
    $attractor[:y] = mouse_y

    # Left click = attract, right click = repel
    if mouse_down?(MOUSE_LEFT)
      $attractor[:mode] = :attract
    elsif mouse_down?(MOUSE_RIGHT)
      $attractor[:mode] = :repel
    end

    # Spawn particles on middle click or space
    if key_pressed?(KEY_SPACE)
      spawn_particle_burst(mouse_x, mouse_y, 25)
      spawn_text_particle(mouse_x, mouse_y, "+25")
    end

    if key_pressed?(KEY_G)
      $gravity_enabled = !$gravity_enabled
      spawn_text_particle($width / 2, 50, $gravity_enabled ? "Gravity ON" : "Gravity OFF")
    end

    if key_pressed?(KEY_C)
      $show_connections = !$show_connections
    end

    if key_pressed?(KEY_R)
      $particles.clear
      spawn_particle_burst($width / 2, $height / 2, 40)
      spawn_text_particle($width / 2, $height / 2, "Reset!")
    end

    if key_pressed?(KEY_F)
      toggle_fullscreen
    end

    if key_pressed?(KEY_D)
      $show_debug_info = !$show_debug_info
    end
  end

  mouse_active = !console_open? && (mouse_down?(MOUSE_LEFT) || mouse_down?(MOUSE_RIGHT))
  
  # Update stars (parallax)
  $stars.each do |star|
    star[:x] -= star[:z] * dt * 40
    if star[:x] < 0
      star[:x] = $width
      star[:y] = rand * $height
    end
  end
  
  # Update rings
  $rings.each do |ring|
    ring[:rotation] += ring[:speed] * dt
  end
  
  # Update particles with physics
  $particles.each do |p|
    # p[:trail] ||= []
    # p[:trail] << { x: p[:x], y: p[:y] }
    # p[:trail].shift if p[:trail].length > 15
    
    # Gravity toward bottom (only if enabled)
    if $gravity_enabled
      p[:vy] += 150 * dt
    end
    
    # Attractor/repulsor force
    if mouse_active
      dx = $attractor[:x] - p[:x]
      dy = $attractor[:y] - p[:y]
      dist = Math.sqrt(dx * dx + dy * dy)
      if dist > 15 && dist < 400
        # Stronger force, and flip sign for repel mode
        strength = $attractor[:mode] == :attract ? 600 : -800
        force = strength / (dist * p[:mass])
        p[:vx] += (dx / dist) * force * dt
        p[:vy] += (dy / dist) * force * dt
      end
    end
    
    # Slight damping
    p[:vx] *= 0.998
    p[:vy] *= 0.998
    
    # Move
    p[:x] += p[:vx] * dt * 60
    p[:y] += p[:vy] * dt * 60
    
    # Bounce off walls with wrap option
    if p[:x] < p[:radius]
      p[:x] = p[:radius]
      p[:vx] = -p[:vx] * 0.9
      p[:hue] = (p[:hue] + 30) % 360
    elsif p[:x] > $width - p[:radius]
      p[:x] = $width - p[:radius]
      p[:vx] = -p[:vx] * 0.9
      p[:hue] = (p[:hue] + 30) % 360
    end
    
    if p[:y] < p[:radius]
      p[:y] = p[:radius]
      p[:vy] = -p[:vy] * 0.9
    elsif p[:y] > $height - p[:radius]
      p[:y] = $height - p[:radius]
      p[:vy] = -p[:vy] * 0.9
      p[:hue] = (p[:hue] + 15) % 360
    end
    
    # Decay
    p[:life] -= p[:decay]
    p[:hue] = (p[:hue] + dt * 20) % 360
  end
  
  # Remove dead particles
  $particles.reject! { |p| p[:life] <= 0 }
  
  # Auto-spawn if we're running low
  if $particles.length < 10
    spawn_particle_burst($width / 2, $height / 2, 15)
  end
  
  # Update text particles
  $text_particles.each do |tp|
    tp[:y] += tp[:vy]
    tp[:vy] -= dt * 2
    tp[:life] -= dt * 0.8
  end
  $text_particles.reject! { |tp| tp[:life] <= 0 }
end

def draw
  unless $stars && $particles && $rings && $text_particles && $attractor
    clear_screen([20, 20, 20])
    set_color([255, 100, 100])
    draw_text("Initializing...", 10, 10, 20)
    return
  end
  
  clear_screen([8, 8, 20])
  
  # Draw starfield
  $stars.each do |star|
    b = star[:brightness]
    set_color([b, b, (b * 0.9).to_i])
    size = star[:z] > 2 ? 2 : 1
    draw_rect(star[:x].to_i, star[:y].to_i, size, size)
  end

  # Draw orbital rings
  cx, cy = $width / 2, $height / 2
  $rings.each do |ring|
    rgb = hue_to_rgb(ring[:hue] + $time * 30, 0.7, 0.8)
    set_color(rgb)

    ring[:segments].times do |i|
      angle = (i.to_f / ring[:segments]) * Math::PI * 2 + ring[:rotation] * Math::PI / 180
      next_angle = ((i + 1).to_f / ring[:segments]) * Math::PI * 2 + ring[:rotation] * Math::PI / 180

      x1 = cx + Math.cos(angle) * ring[:radius]
      y1 = cy + Math.sin(angle) * ring[:radius]
      x2 = cx + Math.cos(next_angle) * ring[:radius]
      y2 = cy + Math.sin(next_angle) * ring[:radius]

      if i % 2 == 0
        draw_line_thick(x1, y1, x2, y2, 2)
      end

      draw_circle(x1.to_i, y1.to_i, 3)
    end
  end
  
  # Draw connections between nearby particles
  max_dist = 120

  if $show_connections && $particles.length > 1
    $particles.each_with_index do |p1, i|
      $particles.each_with_index do |p2, j|
        next if j <= i

        dx = p1[:x] - p2[:x]
        dy = p1[:y] - p2[:y]
        dist = Math.sqrt(dx * dx + dy * dy)

        if dist < max_dist
          alpha = [[((max_dist - dist) / max_dist.to_f * 255).to_i, 0].max, 255].min
          set_color([120, 200, 255, alpha])
          draw_line(p1[:x].to_i, p1[:y].to_i, p2[:x].to_i, p2[:y].to_i)
        end
      end
    end
  end

  
  # Draw particle trails and particles
  $particles.each do |p|
    rgb = hue_to_rgb(p[:hue], 0.8, 1.0)
    # trail = p[:trail] || []
    
    # trail.each_with_index do |pos, i|
    #   t = (i + 1).to_f / [trail.length, 1].max
    #   alpha = (t * p[:life] * 150).to_i
    #   inner = [rgb[0], rgb[1], rgb[2], alpha]
    #   outer = [rgb[0] / 3, rgb[1] / 3, rgb[2] / 3, 0]
      
    #   trail_radius = (p[:radius] * t * 0.6).to_i
    #   trail_radius = 2 if trail_radius < 2
    #   draw_circle_gradient(pos[:x].to_i, pos[:y].to_i, trail_radius, inner, outer)
    # end
    
    glow_alpha = (p[:life] * 100).to_i
    draw_circle_gradient(
      p[:x].to_i, p[:y].to_i, p[:radius] + 8,
      [rgb[0], rgb[1], rgb[2], glow_alpha],
      [rgb[0], rgb[1], rgb[2], 0]
    )
    
    set_color([rgb[0], rgb[1], rgb[2], (p[:life] * 255).to_i])
    draw_circle(p[:x].to_i, p[:y].to_i, p[:radius])
    
    set_color([255, 255, 255, (p[:life] * 200).to_i])
    draw_circle(p[:x].to_i, p[:y].to_i, (p[:radius] * 0.4).to_i)
  end
  
  # Draw attractor/repulsor cursor
  mouse_active = mouse_down?(MOUSE_LEFT) || mouse_down?(MOUSE_RIGHT)
  
  if mouse_active
    pulse = (Math.sin($time * 10) + 1) * 0.5
    
    if $attractor[:mode] == :attract
      # Blue for attract
      set_color([100, 200, 255, (100 + pulse * 100).to_i])
      draw_circle_gradient(
        $attractor[:x].to_i, $attractor[:y].to_i, 25,
        [150, 220, 255, 150],
        [100, 180, 255, 0]
      )
    else
      # Red/orange for repel
      set_color([255, 150, 50, (100 + pulse * 100).to_i])
      draw_circle_gradient(
        $attractor[:x].to_i, $attractor[:y].to_i, 30,
        [255, 100, 50, 150],
        [255, 50, 0, 0]
      )
    end
    
    draw_circle_lines($attractor[:x].to_i, $attractor[:y].to_i, ($attractor[:radius] + pulse * 10).to_i)
    draw_circle_lines($attractor[:x].to_i, $attractor[:y].to_i, ($attractor[:radius] * 0.5).to_i)
  else
    set_color([80, 120, 160, 80])
    draw_circle_lines($attractor[:x].to_i, $attractor[:y].to_i, 15)
  end
  
  # Draw floating text particles
  $text_particles.each do |tp|
    alpha = (tp[:life] * 255).to_i
    set_color([255, 255, 255, alpha])
    draw_text(tp[:text], tp[:x].to_i - 20, tp[:y].to_i, 22)
  end
  
  # UI
  set_color([255, 255, 255, 200])
  draw_text("FPS: #{get_fps}", 10, 10, 18)
  draw_text("Particles: #{$particles.length}", 10, 32, 18)

  # Debug info panel (top-right)
  if $show_debug_info
    panel_x = $width - 260
    panel_y = 10
    line_height = 20
    font_size = 16

    # Semi-transparent background
    set_color([0, 0, 0, 180])
    draw_rect(panel_x - 10, panel_y - 5, 260, 175)

    # Header
    set_color([100, 200, 255])
    draw_text("=== Build Info ===", panel_x, panel_y, font_size)

    # Build details
    set_color([200, 200, 200])
    draw_text("Platform: #{build_platform}", panel_x, panel_y + line_height, font_size)
    draw_text("Build: #{build_type}", panel_x, panel_y + line_height * 2, font_size)
    draw_text("Scripts: #{compiled_scripts? ? 'Compiled' : 'Interpreted'}", panel_x, panel_y + line_height * 3, font_size)
    draw_text("Raylib: #{raylib_version}", panel_x, panel_y + line_height * 4, font_size)

    # GPU Header
    set_color([100, 200, 255])
    draw_text("=== GPU Info ===", panel_x, panel_y + line_height * 5 + 5, font_size)

    # GPU details
    set_color([200, 200, 200])
    # Truncate long GPU strings
    renderer = gpu_renderer
    renderer = renderer[0...25] + "..." if renderer.length > 28
    draw_text("GPU: #{renderer}", panel_x, panel_y + line_height * 6 + 5, font_size)
    draw_text("GL: #{gl_version}", panel_x, panel_y + line_height * 7 + 5, font_size)
  end

  # Instructions (bottom-left) - always visible
  set_color([180, 180, 180, 220])
  draw_text("LMB: Attract | RMB: Repel | Space: Spawn", 10, $height - 52, 16)
  draw_text("G: Gravity (#{$gravity_enabled ? 'ON' : 'OFF'}) | C: Lines | R: Reset | D: Debug", 10, $height - 30, 16)
  
  # Title
  # title = "Particle Playground"
  # title_width = measure_text(title, 32)
  # glow_hue = ($time * 60) % 360
  # rgb = hue_to_rgb(glow_hue, 0.5, 1.0)
  # set_color([rgb[0], rgb[1], rgb[2], 100])
  # draw_text(title, ($width - title_width) / 2 + 2, 12, 32)
  # set_color([255, 255, 255])
  # draw_text(title, ($width - title_width) / 2, 10, 32)
end