# GMR Demo
# Interactive particle physics playground

include GMR

# World dimensions (fixed coordinate system)
WORLD_WIDTH, WORLD_HEIGHT = 960, 640

def init
  Window.set_title("GMR Demo")
  Window.set_size(960, 640)

  update_camera

  $time = 0.0
  $gravity_enabled = false  # Start with gravity OFF
  $show_connections = true
  $show_debug_info = true   # Toggle debug info panel

  # Load logo texture (may fail on some platforms)
  begin
    $logo = Graphics::Texture.load("assets/logo.png")
  rescue
    $logo = nil
    puts "Warning: Could not load logo texture"
  end

  # Player controlled attractor/repulsor (world coordinates)
  $attractor = {
    x: WORLD_WIDTH / 2.0,
    y: WORLD_HEIGHT / 2.0,
    radius: 30,
    strength: 500,
    mode: :attract  # :attract or :repel
  }

  # Starfield background (world coordinates, but extended beyond edges)
  $stars = []
  400.times do
    $stars << {
      x: rand * WORLD_WIDTH * 1.5 - WORLD_WIDTH * 0.25,
      y: rand * WORLD_HEIGHT * 1.5 - WORLD_HEIGHT * 0.25,
      z: rand * 3 + 0.5,
      brightness: System.random_int(100, 255)
    }
  end

  # Physics particles (world coordinates)
  $particles = []
  spawn_particle_burst(WORLD_WIDTH / 2, WORLD_HEIGHT / 2, 40)

  # Orbital rings (world coordinates)
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
end

def update(dt)
  unless $stars && $particles && $rings && $text_particles && $attractor
    init
    return
  end

  # Update camera/scaling each frame
  update_camera

  # Clamp dt to prevent huge jumps when window loses focus
  dt = [dt, 0.1].min

  $time += dt

  # Convert mouse to world coordinates
  mouse_world_x = screen_to_world_x(Input.mouse_x)
  mouse_world_y = screen_to_world_y(Input.mouse_y)

  # Only process game input when console is closed
  unless console_open?
    # Update attractor position (in world coordinates)
    $attractor[:x] = mouse_world_x
    $attractor[:y] = mouse_world_y

    # Left click = attract, right click = repel
    if Input.mouse_down?(:left)
      $attractor[:mode] = :attract
    elsif Input.mouse_down?(:right)
      $attractor[:mode] = :repel
    end

    # Spawn particles on space
    if Input.key_pressed?(:space)
      spawn_particle_burst(mouse_world_x, mouse_world_y, 25)
      spawn_text_particle(mouse_world_x, mouse_world_y, "+25")
    end

    if Input.key_pressed?(:g)
      $gravity_enabled = !$gravity_enabled
      spawn_text_particle(WORLD_WIDTH / 2, 50, $gravity_enabled ? "Gravity ON" : "Gravity OFF")
    end

    if Input.key_pressed?(:c)
      $show_connections = !$show_connections
    end

    if Input.key_pressed?(:r)
      $particles.clear
      spawn_particle_burst(WORLD_WIDTH / 2, WORLD_HEIGHT / 2, 40)
      spawn_text_particle(WORLD_WIDTH / 2, WORLD_HEIGHT / 2, "Reset!")
    end

    # Fullscreen toggle - disabled on web (handled by shell.html button)
    if Input.key_pressed?(:f) && System.platform != "web"
      Window.toggle_fullscreen
    end

    if Input.key_pressed?(:d)
      $show_debug_info = !$show_debug_info
    end
  end

  mouse_active = !console_open? && (Input.mouse_down?(:left) || Input.mouse_down?(:right))

  # Update stars (parallax) - wrap in extended world space
  $stars.each do |star|
    star[:x] -= star[:z] * dt * 40
    if star[:x] < -WORLD_WIDTH * 0.25
      star[:x] = WORLD_WIDTH * 1.25
      star[:y] = rand * WORLD_HEIGHT * 1.5 - WORLD_HEIGHT * 0.25
    end
  end

  # Update rings
  $rings.each do |ring|
    ring[:rotation] += ring[:speed] * dt
  end

  # Update particles with physics (all in world coordinates)
  $particles.each do |p|
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

    # Bounce off world boundaries
    if p[:x] < p[:radius]
      p[:x] = p[:radius]
      p[:vx] = -p[:vx] * 0.9
      p[:hue] = (p[:hue] + 30) % 360
    elsif p[:x] > WORLD_WIDTH - p[:radius]
      p[:x] = WORLD_WIDTH - p[:radius]
      p[:vx] = -p[:vx] * 0.9
      p[:hue] = (p[:hue] + 30) % 360
    end

    if p[:y] < p[:radius]
      p[:y] = p[:radius]
      p[:vy] = -p[:vy] * 0.9
    elsif p[:y] > WORLD_HEIGHT - p[:radius]
      p[:y] = WORLD_HEIGHT - p[:radius]
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
    spawn_particle_burst(WORLD_WIDTH / 2, WORLD_HEIGHT / 2, 15)
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
    Graphics.clear([20, 20, 20])
    Graphics.draw_text("Initializing...", 10, 10, 20, [255, 100, 100])
    return
  end

  Graphics.clear([8, 8, 20])

  # Draw logo centered in world (behind everything)
  if $logo
    logo_scale = 0.5 * $scale
    logo_x = world_to_screen_x(WORLD_WIDTH / 2) - ($logo.width * logo_scale / 2)
    logo_y = world_to_screen_y(WORLD_HEIGHT / 2) - ($logo.height * logo_scale / 2)
    $logo.draw_ex(logo_x, logo_y, 0, logo_scale, [255, 255, 255, 40])
  end

  # Draw starfield (world coordinates)
  $stars.each do |star|
    b = star[:brightness]
    sx = world_to_screen_x(star[:x]).to_i
    sy = world_to_screen_y(star[:y]).to_i
    size = star[:z] > 2 ? 2 : 1
    Graphics.draw_rect(sx, sy, size, size, [b, b, (b * 0.9).to_i])
  end

  # Draw orbital rings (world coordinates)
  cx = world_to_screen_x(WORLD_WIDTH / 2)
  cy = world_to_screen_y(WORLD_HEIGHT / 2)

  $rings.each do |ring|
    rgb = hue_to_rgb(ring[:hue] + $time * 30, 0.7, 0.8)
    scaled_radius = ring[:radius] * $scale

    ring[:segments].times do |i|
      angle = (i.to_f / ring[:segments]) * Math::PI * 2 + ring[:rotation] * Math::PI / 180
      next_angle = ((i + 1).to_f / ring[:segments]) * Math::PI * 2 + ring[:rotation] * Math::PI / 180

      x1 = cx + Math.cos(angle) * scaled_radius
      y1 = cy + Math.sin(angle) * scaled_radius
      x2 = cx + Math.cos(next_angle) * scaled_radius
      y2 = cy + Math.sin(next_angle) * scaled_radius

      if i % 2 == 0
        Graphics.draw_line_thick(x1, y1, x2, y2, [2 * $scale, 1].max, rgb)
      end

      Graphics.draw_circle(x1.to_i, y1.to_i, [3 * $scale, 2].max.to_i, rgb)
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
          Graphics.draw_line(
            world_to_screen_x(p1[:x]).to_i, world_to_screen_y(p1[:y]).to_i,
            world_to_screen_x(p2[:x]).to_i, world_to_screen_y(p2[:y]).to_i,
            [120, 200, 255, alpha]
          )
        end
      end
    end
  end

  # Draw particles
  $particles.each do |p|
    rgb = hue_to_rgb(p[:hue], 0.8, 1.0)

    sx = world_to_screen_x(p[:x]).to_i
    sy = world_to_screen_y(p[:y]).to_i
    scaled_radius = (p[:radius] * $scale).to_i

    glow_alpha = (p[:life] * 100).to_i
    Graphics.draw_circle_gradient(
      sx, sy, scaled_radius + scaled(8),
      [rgb[0], rgb[1], rgb[2], glow_alpha],
      [rgb[0], rgb[1], rgb[2], 0]
    )

    Graphics.draw_circle(sx, sy, scaled_radius, [rgb[0], rgb[1], rgb[2], (p[:life] * 255).to_i])
    Graphics.draw_circle(sx, sy, (scaled_radius * 0.4).to_i, [255, 255, 255, (p[:life] * 200).to_i])
  end

  # Draw attractor/repulsor cursor
  mouse_active = Input.mouse_down?(:left) || Input.mouse_down?(:right)
  ax = world_to_screen_x($attractor[:x]).to_i
  ay = world_to_screen_y($attractor[:y]).to_i

  if mouse_active
    pulse = (Math.sin($time * 10) + 1) * 0.5

    if $attractor[:mode] == :attract
      Graphics.draw_circle_gradient(ax, ay, scaled(25), [150, 220, 255, 150], [100, 180, 255, 0])
      Graphics.draw_circle_outline(ax, ay, scaled($attractor[:radius] + pulse * 10), [100, 200, 255, (100 + pulse * 100).to_i])
      Graphics.draw_circle_outline(ax, ay, scaled($attractor[:radius] * 0.5), [100, 200, 255, (100 + pulse * 100).to_i])
    else
      Graphics.draw_circle_gradient(ax, ay, scaled(30), [255, 100, 50, 150], [255, 50, 0, 0])
      Graphics.draw_circle_outline(ax, ay, scaled($attractor[:radius] + pulse * 10), [255, 150, 50, (100 + pulse * 100).to_i])
      Graphics.draw_circle_outline(ax, ay, scaled($attractor[:radius] * 0.5), [255, 150, 50, (100 + pulse * 100).to_i])
    end
  else
    Graphics.draw_circle_outline(ax, ay, scaled(15), [80, 120, 160, 80])
  end

  # Draw floating text particles
  $text_particles.each do |tp|
    alpha = (tp[:life] * 255).to_i
    sx = world_to_screen_x(tp[:x]).to_i - scaled(30)
    sy = world_to_screen_y(tp[:y]).to_i
    Graphics.draw_text(tp[:text], sx, sy, scaled(32), [255, 255, 255, alpha])
  end

  # UI (screen coordinates, not world)
  font_size = 32
  Graphics.draw_text("FPS: #{GMR::Time.fps}", 10, 10, font_size, [255, 255, 255, 200])
  Graphics.draw_text("Particles: #{$particles.length}", 10, 46, font_size, [255, 255, 255, 200])

  # Debug info panel (top-right, screen coordinates)
  if $show_debug_info
    panel_x = $screen_width - 380
    panel_y = 10
    line_height = 36
    debug_font = 28

    # Semi-transparent background
    Graphics.draw_rect(panel_x - 10, panel_y - 5, 380, 230, [0, 0, 0, 180])

    # Header
    Graphics.draw_text("=== Build ===", panel_x, panel_y, debug_font, [100, 200, 255])

    # Build details
    Graphics.draw_text("Platform: #{System.platform}", panel_x, panel_y + line_height, debug_font, [200, 200, 200])
    Graphics.draw_text("Build: #{System.build_type}", panel_x, panel_y + line_height * 2, debug_font, [200, 200, 200])
    Graphics.draw_text("Scripts: #{System.compiled_scripts? ? 'Compiled' : 'Interpreted'}", panel_x, panel_y + line_height * 3, debug_font, [200, 200, 200])

    # GPU info
    Graphics.draw_text("=== GPU ===", panel_x, panel_y + line_height * 4 + 10, debug_font, [100, 200, 255])

    renderer = System.gpu_renderer
    renderer = renderer[0...22] + "..." if renderer.length > 25
    Graphics.draw_text(renderer, panel_x, panel_y + line_height * 5 + 10, debug_font, [200, 200, 200])
  end

  # Instructions (bottom-left, screen coordinates)
  instructions_font = 28
  Graphics.draw_text("LMB: Attract | RMB: Repel | Space: Spawn", 10, $screen_height - 70, instructions_font, [180, 180, 180, 220])
  Graphics.draw_text("G: Gravity (#{$gravity_enabled ? 'ON' : 'OFF'}) | C: Lines | R: Reset | D: Debug", 10, $screen_height - 38, instructions_font, [180, 180, 180, 220])
end
