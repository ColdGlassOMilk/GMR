include GMR

# === 3D DEPTH ILLUSION STRESS TEST ===
# Tests: Z-ordering, scaling, rotation, camera, mixed primitives, sprites, text
# Goal: Create pseudo-3D scene with objects at different depths bouncing around

# Virtual resolution
VIRTUAL_WIDTH = 960
VIRTUAL_HEIGHT = 540
WINDOW_WIDTH = 960
WINDOW_HEIGHT = 540

# Particle/object definition
class DepthObject
  attr_accessor :x, :y, :z, :vx, :vy, :vz, :rotation, :rotation_speed, :type, :color, :text, :transform, :sprite

  def initialize(type, texture = nil, color = nil)
    @type = type  # :sprite, :rect, :circle, :triangle, :line, :text

    # Generate random color with random alpha (transparency)
    if color.nil?
      base_color = [:red, :green, :blue, :yellow, :magenta, :cyan, :orange, :pink].sample
      # Convert symbol to RGB array and add random alpha (100-255 for visibility)
      @color = color_with_alpha(base_color, rand(100..255))
    else
      @color = color
    end

    # Calculate world space bounds from virtual resolution
    # Camera is centered at (0, 0), so bounds are ±half of resolution
    half_width = VIRTUAL_WIDTH / 2.0
    half_height = VIRTUAL_HEIGHT / 2.0

    # Random position (within visible area, with some margin)
    margin = 20.0
    @x = rand((-half_width + margin)..(half_width - margin))
    @y = rand((-half_height + margin)..(half_height - margin))
    @z = rand(2.0..8.0)  # Depth: 2.0 (far) to 8.0 (near) - stay visible

    # Random velocity (slower so you can see z-order changes)
    @vx = rand(-30.0..30.0)
    @vy = rand(-30.0..30.0)
    @vz = rand(-0.5..0.5)  # Slower depth velocity

    # Rotation
    @rotation = rand(0.0..360.0)
    @rotation_speed = rand(-180.0..180.0)

    # Text content (for :text type)
    if @type == :text
      words = ["WOW", "SUCH 3D", "GMR", "TEST", "ZOOM", "3D", "DEPTH"]
      @text = words.sample
    end

    # Create transform and sprite ONCE for sprite objects
    if @type == :sprite && texture
      s = scale
      @transform = Transform2D.new(
        x: @x,
        y: @y,
        rotation: @rotation,
        scale_x: s * 0.3,
        scale_y: s * 0.3
      )
      @sprite = Sprite.new(texture, @transform)
      @sprite.center_origin
      @sprite.color = @color
    end
  end

  def color_with_alpha(color_symbol, alpha)
    # Convert color symbol to [r, g, b, a] array
    case color_symbol
    when :red then [255, 0, 0, alpha]
    when :green then [0, 255, 0, alpha]
    when :blue then [0, 0, 255, alpha]
    when :yellow then [255, 255, 0, alpha]
    when :magenta then [255, 0, 255, alpha]
    when :cyan then [0, 255, 255, alpha]
    when :orange then [255, 165, 0, alpha]
    when :pink then [255, 192, 203, alpha]
    else [255, 255, 255, alpha]
    end
  end

  def update(dt)
    # Update position
    @x += @vx * dt
    @y += @vy * dt
    @z += @vz * dt

    # Bounce off walls (in world space)
    # Calculate bounds from virtual resolution
    half_width = VIRTUAL_WIDTH / 2.0
    half_height = VIRTUAL_HEIGHT / 2.0

    if @x < -half_width || @x > half_width
      @vx *= -1
      @x = @x.clamp(-half_width, half_width)
    end

    if @y < -half_height || @y > half_height
      @vy *= -1
      @y = @y.clamp(-half_height, half_height)
    end

    # Bounce depth (narrower range to keep objects visible)
    if @z < 2.0 || @z > 8.0
      @vz *= -1
      @z = @z.clamp(2.0, 8.0)
    end

    # Update rotation
    @rotation += @rotation_speed * dt
    @rotation %= 360

    # Update transform if this is a sprite
    if @transform
      s = scale
      @transform.x = @x
      @transform.y = @y
      @transform.rotation = @rotation
      @transform.scale_x = s * 0.3
      @transform.scale_y = s * 0.3
    end
  end

  def scale
    # Objects further away (low z) are smaller
    # Objects closer (high z) are larger
    # Scale range adjusted for z=2.0 to z=8.0
    # At z=2.0: 0.5, at z=8.0: 2.5
    0.5 + ((@z - 2.0) / 6.0) * 2.0
  end

  def size
    # Base size affected by scale (larger base for visibility)
    (30.0 * scale).round
  end

  def draw_order_z
    # Higher z = closer to camera = drawn later (on top)
    @z
  end
end

def init
  # === WINDOW SETUP ===
  Window.set_size(WINDOW_WIDTH, WINDOW_HEIGHT)
        .set_virtual_resolution(VIRTUAL_WIDTH, VIRTUAL_HEIGHT)
        .set_filter_point


  Console.enable(height: 150).allow_ruby_eval

  # === INPUT MAPPING ===
  Input.map(:move_left, [:left, :a])
       .map(:move_right, [:right, :d])
       .map(:move_up, [:up, :w])
       .map(:move_down, [:down, :s])
       .map(:reset, [:r])

  # === CAMERA ===
  @camera = Camera.new
  @camera.offset = Mathf::Vec2.new(VIRTUAL_WIDTH / 2.0, VIRTUAL_HEIGHT / 2.0)
  @camera.zoom = 1.0
  @camera.target = Mathf::Vec2.new(0, 0)
  @camera_x = 0.0
  @camera_y = 0.0

  @time = 0.0

  # === LOAD SPRITE TEXTURE ===
  @tex = Texture.load("assets/logo.png")

  # === CREATE DEPTH OBJECTS ===
  @objects = []

  # Add sprites if texture loaded
  if @tex
    15.times { @objects << DepthObject.new(:sprite, @tex) }
  end

  # Add primitives
  10.times { @objects << DepthObject.new(:rect) }
  10.times { @objects << DepthObject.new(:circle) }
  8.times { @objects << DepthObject.new(:triangle) }
  5.times { @objects << DepthObject.new(:line) }
  5.times { @objects << DepthObject.new(:text) }
end

def update(dt)
  @time += dt

  # Camera controls
  camera_speed = 100.0
  @camera_x -= camera_speed * dt if Input.action_down?(:move_left)
  @camera_x += camera_speed * dt if Input.action_down?(:move_right)
  @camera_y -= camera_speed * dt if Input.action_down?(:move_up)
  @camera_y += camera_speed * dt if Input.action_down?(:move_down)

  @camera.target = Mathf::Vec2.new(@camera_x, @camera_y)

  # Zoom controls
  wheel = Input.mouse_wheel
  @camera.zoom += wheel * 0.1
  @camera.zoom = [[@camera.zoom, 0.1].max, 5.0].min

  # Reset camera
  if Input.action_pressed?(:reset)
    @camera_x = 0.0
    @camera_y = 0.0
    @camera.zoom = 1.0
  end

  # Update all objects
  @objects.each { |obj| obj.update(dt) }
end

def draw
  Graphics.clear("#0a0a1e")

  begin
    @camera.use do
      # Draw a reference grid FIRST (before objects)
      half_width = VIRTUAL_WIDTH / 2.0
      half_height = VIRTUAL_HEIGHT / 2.0

      Graphics.draw_line(-half_width, 0, half_width, 0, :green, 2)  # Horizontal axis
      Graphics.draw_line(0, -half_height, 0, half_height, :green, 2)  # Vertical axis

      # Test rect at origin
      Graphics.draw_rect(-10, -10, 20, 20, :white)

      # Sort objects by depth (z) to ensure correct draw order
      sorted = @objects.sort_by { |obj| obj.z }

      sorted.each do |obj|
        s = obj.scale
        sz = obj.size

        case obj.type
        when :sprite
          # Just draw the sprite - it's already set up with transform
          obj.sprite.draw if obj.sprite

        when :rect
          # Filled rect
          Graphics.draw_rect(obj.x - sz/2, obj.y - sz/2, sz, sz, obj.color)

        when :circle
          # Mix of filled and outline circles
          if obj.z > 5.0
            Graphics.draw_circle(obj.x, obj.y, sz/2, obj.color)
          else
            Graphics.draw_circle_outline(obj.x, obj.y, sz/2, obj.color)
          end

        when :triangle
          # Rotating triangle
          rad = obj.rotation * Math::PI / 180.0
          offset = sz * 0.8
          x1 = obj.x + Math.cos(rad) * offset
          y1 = obj.y + Math.sin(rad) * offset
          x2 = obj.x + Math.cos(rad + 2.094) * offset  # +120 degrees
          y2 = obj.y + Math.sin(rad + 2.094) * offset
          x3 = obj.x + Math.cos(rad + 4.189) * offset  # +240 degrees
          y3 = obj.y + Math.sin(rad + 4.189) * offset

          Graphics.draw_triangle(x1, y1, x2, y2, x3, y3, obj.color)

        when :line
          # Rotating line
          rad = obj.rotation * Math::PI / 180.0
          length = sz * 1.5
          x1 = obj.x - Math.cos(rad) * length / 2
          y1 = obj.y - Math.sin(rad) * length / 2
          x2 = obj.x + Math.cos(rad) * length / 2
          y2 = obj.y + Math.sin(rad) * length / 2

          Graphics.draw_line(x1, y1, x2, y2, obj.color, 2)

        when :text
          # Scaled text based on depth
          # Font size scales with depth to create 3D effect
          # Note: Text rotation is not supported by raylib's DrawText
          font_size = (20.0 * s).round.clamp(8, 60)
          Graphics.draw_text(obj.text, obj.x, obj.y, font_size, obj.color)
        end
      end

      # Simple test text at world origin (INSIDE camera block for world-space rendering)
      Graphics.draw_text("WORLD", 0, 20, 30, :red)
      Graphics.draw_text("CENTER", -30, -20, 20, :yellow)
    end
  rescue => e
    # If there's an exception, show it in console
    Console.log("ERROR in camera block: #{e.message}")
    Console.log("Backtrace: #{e.backtrace.first(5).join("\n")}")
  end

  # UI overlay (screen space - not affected by camera)
  fps = GMR::Time.fps

  # Test: Draw a bright rect to confirm primitives work in screen space
  Graphics.draw_rect(5, 5, 100, 80, [0, 0, 0, 180])  # Semi-transparent black background

  # Draw text over the background
  Graphics.draw_text("FPS: #{fps}", 10, 10, 20, :yellow)
  Graphics.draw_text("TEST", 10, 35, 24, :red)
  Graphics.draw_text("Objects: #{@objects.size}", 10, 60, 14, :cyan)

  # Draw frame border
  Graphics.draw_rect_outline(1, 1, VIRTUAL_WIDTH - 2, VIRTUAL_HEIGHT - 2, :white)
end
