# GMR Engine - Main Entry Point
# Delegates to SceneManager for gameplay

include GMR

# =============================================================================
# Global Entry Points
# =============================================================================

def init
  # Enable the built-in console with custom styling
  # The console is toggled with the backtick (`) key
  GMR::Console.enable({
    background: "#141820",
    foreground: "#ffffff",
    prompt_color: [100, 200, 255],
    result_color: [120, 255, 150],
    error_color: [255, 100, 100],
    height: 300,
    font_size: 14
  })

  # Register some demo commands
  GMR::Console.register_command("spawn", "Spawn an entity (demo)") do |args|
    type = args[0] || "enemy"
    "Spawning #{type} (demo command)"
  end

  GMR::Console.register_command("teleport", "Teleport to x,y (demo)") do |args|
    if args.length >= 2
      x, y = args[0].to_i, args[1].to_i
      "Teleported to #{x}, #{y} (demo command)"
    else
      "Usage: teleport <x> <y>"
    end
  end

  GMR::Console.register_command("fps", "Show current FPS") do
    "FPS: #{GMR::Time.fps}"
  end

  # Load the main game scene
  @game_scene = GameScene.new
  SceneManager.load(@game_scene)

  # Add minimap overlay (set_game_scene AFTER add_overlay since add_overlay calls init)
  @minimap = MinimapOverlay.new
  SceneManager.add_overlay(@minimap)
  @minimap.set_game_scene(@game_scene)
end

def update(dt)
  # Delegate to scene manager
  SceneManager.update(dt)
end

def draw
  # Delegate to scene manager
  SceneManager.draw
end

# =============================================================================
# Utility Functions (available globally)
# =============================================================================

def lerp(a, b, t)
  a + (b - a) * t
end

def clamp(val, min, max)
  val < min ? min : (val > max ? max : val)
end

def ease_in_out(t)
  t < 0.5 ? 2 * t * t : 1 - (-2 * t + 2) ** 2 / 2
end

def lerp_color(c1, c2, t)
  [
    lerp(c1[0], c2[0], t).to_i,
    lerp(c1[1], c2[1], t).to_i,
    lerp(c1[2], c2[2], t).to_i
  ]
end

def hue_to_rgb(h, s, v)
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
            else [c, 0, x]
            end

  [((r + m) * 255).to_i, ((g + m) * 255).to_i, ((b + m) * 255).to_i]
end
