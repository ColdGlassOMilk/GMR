include GMR

# MVP Demo: Animation + State Machine + Input Binding
# Testing all three systems together with oak_woods character

FRAME_WIDTH = 56
FRAME_HEIGHT = 56
COLUMNS = 7

def init
  # Input mapping
  Input.map(:move_left, [:left, :a])
  Input.map(:move_right, [:right, :d])
  Input.map(:jump, :space)

  # Load character spritesheet
  @char_tex = Graphics::Texture.load("assets/oak_woods/character/char_blue.png")
  @sprite = Graphics::Sprite.new(@char_tex)
  @sprite.source_rect = Graphics::Rect.new(0, 0, FRAME_WIDTH, FRAME_HEIGHT)
  @sprite.x = 400.0
  @sprite.y = 300.0

  # Animator - manages spritesheet animations
  @animator = Animation::Animator.new(@sprite,
    columns: COLUMNS,
    frame_width: FRAME_WIDTH,
    frame_height: FRAME_HEIGHT)

  # Define animations (based on spritesheet layout)
  # Row 0: Idle (frames 0-6)
  # Row 2: Run (frames 14-20)
  @animator.add(:idle, frames: 0..5, fps: 8)
  @animator.add(:run, frames: 14..19, fps: 12)
  @animator.play(:idle)

  # State machine for character states
  animator = @animator

  state_machine do
    state :idle do
      enter { animator.play(:idle) }
      on :move, :running
    end

    state :running do
      enter { animator.play(:run) }
      on :stop, :idle
    end
  end
end

def update(dt)
  # Check for movement
  moving = Input.action_down?(:move_left) || Input.action_down?(:move_right)

  # Trigger state machine transitions
  if moving
    state_machine.trigger(:move)
  else
    state_machine.trigger(:stop)
  end

  # Apply movement
  speed = 200.0
  if Input.action_down?(:move_left)
    @sprite.x -= speed * dt
    @sprite.flip_x = true
  end
  if Input.action_down?(:move_right)
    @sprite.x += speed * dt
    @sprite.flip_x = false
  end
end

def draw
  Graphics.clear([50, 80, 50])
  @sprite.draw
end
