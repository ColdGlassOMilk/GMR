include GMR

# === 3D DEPTH ILLUSION STRESS TEST ===
# Tests: Z-ordering, scaling, rotation, camera, mixed primitives, sprites, text
# Goal: Create pseudo-3D scene with objects at different depths bouncing around

# Virtual resolution
VIRTUAL_WIDTH = 960
VIRTUAL_HEIGHT = 540
WINDOW_WIDTH = 960
WINDOW_HEIGHT = 540

# World bounds (much larger than viewport for exploration)
WORLD_WIDTH = 3840  # 4x viewport width
WORLD_HEIGHT = 2160 # 4x viewport height

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

    # Calculate world space bounds (much larger than viewport)
    # Camera is centered at (0, 0), so bounds are ±half of world size
    half_width = WORLD_WIDTH / 2.0
    half_height = WORLD_HEIGHT / 2.0

    # Random position (within world area, with some margin)
    margin = 50.0
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

    # Create transform for ALL object types (not just sprites)
    s = scale
    # Sprites are scaled down (0.15x), primitives are scaled normally (0.3x = 2x larger)
    scale_multiplier = (@type == :sprite) ? 0.15 : 0.3
    @transform = Transform2D.new(
      x: @x,
      y: @y,
      rotation: @rotation,
      scale_x: s * scale_multiplier,
      scale_y: s * scale_multiplier
    )

    # Set appropriate origins for each type
    if @type == :sprite && texture
      @sprite = Sprite.new(texture, @transform)
      @sprite.center_origin
      @sprite.color = @color
    elsif @type == :rect
      # Set origin to center for proper rotation (base size 60x60)
      @transform.center_origin(60, 60)
    elsif @type == :circle
      # Circles already rotate around center at (0,0)
      @transform.origin_x = 0
      @transform.origin_y = 0
    elsif @type == :triangle
      # Triangle vertices are in local space, no origin needed
      @transform.origin_x = 0
      @transform.origin_y = 0
    elsif @type == :line
      # Line rotates around its center, no origin needed
      @transform.origin_x = 0
      @transform.origin_y = 0
    elsif @type == :text
      # Text draws from top-left by default
      @transform.origin_x = 0
      @transform.origin_y = 0
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
    # Calculate bounds from world size
    half_width = WORLD_WIDTH / 2.0
    half_height = WORLD_HEIGHT / 2.0

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

    # Update transform (used by ALL object types)
    s = scale
    scale_multiplier = (@type == :sprite) ? 0.15 : 0.3
    @transform.x = @x
    @transform.y = @y
    @transform.rotation = @rotation
    @transform.scale_x = s * scale_multiplier
    @transform.scale_y = s * scale_multiplier
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

# Helper function to spawn random objects
def spawn_objects(count, texture = nil)
  objects = []

  # Randomize types
  types = []

  # Add sprites if texture available
  if texture
    (count * 0.3).round.times { types << :sprite }
  end

  # Add primitives
  (count * 0.2).round.times { types << :rect }
  (count * 0.2).round.times { types << :circle }
  (count * 0.15).round.times { types << :triangle }
  (count * 0.1).round.times { types << :line }
  (count * 0.05).round.times { types << :text }

  # Fill remaining with random types
  while types.size < count
    types << [:rect, :circle, :triangle, :line, :text].sample
  end

  # Shuffle and create objects
  types.shuffle.each do |type|
    objects << DepthObject.new(type, texture)
  end

  objects
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
       .map(:reset, :r)
       .map(:add_objects, :equal)       # = key (Shift+= gives +)
       .map(:remove_objects, :minus)    # - key

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

  # === CREATE INITIAL DEPTH OBJECTS ===
  @objects = spawn_objects(50, @tex)
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
    @camera.target = Mathf::Vec2.new(0, 0)
  end

  # Object count controls
  if Input.action_pressed?(:add_objects)
    @objects += spawn_objects(50, @tex)
  end

  if Input.action_pressed?(:remove_objects)
    count = [@objects.size, 50].min
    @objects = @objects[0...-count] if count > 0
  end

  # Update all objects
  @objects.each { |obj| obj.update(dt) }
end

def draw
  Graphics.clear("#0a0a1e")

  @camera.use do
    # Draw world bounds
    world_half_w = WORLD_WIDTH / 2.0
    world_half_h = WORLD_HEIGHT / 2.0
    Graphics.draw_rect_outline(-world_half_w, -world_half_h, WORLD_WIDTH, WORLD_HEIGHT, [100, 100, 100, 255])

    # Draw viewport reference (smaller box showing initial viewport size)
    view_half_w = VIRTUAL_WIDTH / 2.0
    view_half_h = VIRTUAL_HEIGHT / 2.0
    Graphics.draw_line(-view_half_w, 0, view_half_w, 0, :green, 2)  # Horizontal axis
    Graphics.draw_line(0, -view_half_h, 0, view_half_h, :green, 2)  # Vertical axis

    # Test rect at origin
    Graphics.draw_rect(-10, -10, 20, 20, :white)

    # Sort objects by depth (z) to ensure correct draw order
    sorted = @objects.sort_by { |obj| obj.z }

    sorted.each do |obj|
      case obj.type
      when :sprite
        # Just draw the sprite - it's already set up with transform
        obj.sprite.draw if obj.sprite

      when :rect
        # NEW: Use transform instead of manual calculations (2x larger: 60x60)
        Graphics.draw_rect(obj.transform, 60, 60, obj.color)

      when :circle
        # NEW: Use transform - mix of filled and outline circles (2x larger: radius 30)
        if obj.z > 5.0
          Graphics.draw_circle(obj.transform, 30, obj.color)
        else
          Graphics.draw_circle_outline(obj.transform, 30, obj.color)
        end

      when :triangle
        # NEW: Vertices in local space (2x larger equilateral triangle)
        # For side length ~48: height = 41.6, offset from center = ±20.8 vertically, ±24 horizontally
        Graphics.draw_triangle(obj.transform, 0, -24, -20.8, 24, 20.8, 24, obj.color)

      when :line
        # NEW: Line in local space (2x longer: horizontal line from -45 to 45)
        Graphics.draw_line(obj.transform, -45, 0, 45, 0, obj.color, 2)

      when :text
        # NEW: Use transform with scaled font size
        s = obj.scale
        font_size = (20.0 * s).round.clamp(8, 60)
        Graphics.draw_text(obj.transform, obj.text, font_size, obj.color)
      end
    end

    # Simple test text at world origin (INSIDE camera block for world-space rendering)
    Graphics.draw_text("WORLD", 0, 20, 30, :red)
    Graphics.draw_text("CENTER", -30, -20, 20, :yellow)
  end

  # UI overlay (screen space - not affected by camera)
  fps = GMR::Time.fps

  # Test: Draw a bright rect to confirm primitives work in screen space
  Graphics.draw_rect(5, 5, 100, 80, [0, 0, 0, 180])  # Semi-transparent black background

  # Draw text over the background
  Graphics.draw_text("FPS: #{fps}", 10, 10, 20, :yellow)
  Graphics.draw_text("Objects: #{@objects.size}", 10, 35, 16, :cyan)
  Graphics.draw_text("Camera: (#{@camera_x.round}, #{@camera_y.round}) Zoom: #{@camera.zoom.round(2)}", 10, 55, 14, :white)
  Graphics.draw_text("Controls: WASD/Arrows=Pan | Wheel=Zoom | R=Reset | +/-=Add/Remove", 10, 75, 12, [200, 200, 200, 255])

  # Draw frame border
  Graphics.draw_rect_outline(1, 1, VIRTUAL_WIDTH - 2, VIRTUAL_HEIGHT - 2, :white)
end
