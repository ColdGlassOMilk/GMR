class Player
  attr_reader :transform, :sprite

  def initialize(x, y)
    @transform = Transform2D.new(x: x, y: y)
    @char_tex = Graphics::Texture.load("oak_woods/character/char_blue.png")
    @sprite = Graphics::Sprite.new(@char_tex, @transform)
    @sprite.source_rect = Graphics::Rect.new(0, 0, FRAME_WIDTH_PX, FRAME_HEIGHT_PX)

    @velocity_x = 0.0
    @velocity_y = 0.0
    @on_ground = false
    @moving = false

    @jump_sound = Audio::Sound.load("sfx/jump.mp3", volume: SFX_VOLUME - 0.2)
    @attack_sound = Audio::Sound.load("sfx/sword_swing.mp3", volume: SFX_VOLUME)

    setup_animator
    setup_state_machine
  end

  def x
    @transform.x + FRAME_WIDTH / 2.0
  end

  def y
    @transform.y + FRAME_HEIGHT / 2.0
  end

  def update(dt, tilemap)
    apply_gravity(dt) unless @on_ground
    apply_velocity(dt)
    resolve_collision(tilemap)
    update_state_machine
  end

  def move_left
    @velocity_x = -MOVE_SPEED
    @sprite.flip_x = true
    @moving = true
  end

  def move_right
    @velocity_x = MOVE_SPEED
    @sprite.flip_x = false
    @moving = true
  end

  def stop
    @velocity_x = 0.0
    @moving = false
  end

  def jump
    return false unless @on_ground
    @velocity_y = JUMP_FORCE
    @on_ground = false
    state_machine.trigger(:jump)
    @jump_sound.play
    true
  end

  def attack
    return false if state_machine.state == :attacking
    return false if state_machine.state == :hurt
    return false if state_machine.state == :dead
    state_machine.trigger(:attack)
    @attack_sound.play
    true
  end

  def draw
    @sprite.draw
  end

  private

  def setup_animator
    @animator = Animation::Animator.new(@sprite,
      columns: COLUMNS,
      frame_width: FRAME_WIDTH_PX,
      frame_height: FRAME_HEIGHT_PX)

    @animator.add(:idle, frames: 0..5, fps: 6, loop: true)
    @animator.add(:run, frames: 14..19, fps: 12, loop: true)
    @animator.add(:jump_up, frames: 22..23, fps: 15, loop: false)
    @animator.add(:fall, frames: 24..26, fps: 12, loop: false)
    @animator.add(:attack, frames: 8..12, fps: 18, loop: false)
    @animator.add(:hurt, frames: 43..47, fps: 15, loop: false)
    @animator.add(:death, frames: 50..57, fps: 10, loop: false)

    @animator.on_complete(:attack) do
      state_machine.trigger(:attack_finish) if state_machine.state == :attacking
    end

    @animator.on_complete(:hurt) do
      state_machine.trigger(:hurt_finish) if state_machine.state == :hurt
    end
  end

  def setup_state_machine
    state_machine do
      state :idle do
        animate :idle
        on :move, :running
        on :jump, :jumping
        on :attack, :attacking
      end

      state :running do
        animate :run
        on :stop, :idle
        on :jump, :jumping
        on :attack, :attacking
      end

      state :jumping do
        animate :jump_up
        on :fall, :falling
        on :land, :idle
        on :land_moving, :running
        on :attack, :attacking
      end

      state :falling do
        animate :fall
        on :land, :idle
        on :land_moving, :running
        on :attack, :attacking
      end

      state :attacking do
        animate :attack
        on :attack_finish, :idle
      end

      state :hurt do
        animate :hurt
        on :hurt_finish, :idle
      end

      state :dead do
        animate :death
      end
    end
  end

  def apply_gravity(dt)
    @velocity_y += GRAVITY * dt
    state_machine.trigger(:fall) if @velocity_y > 0 && state_machine.state != :falling
  end

  def apply_velocity(dt)
    @transform.x += @velocity_x * dt
    @transform.y += @velocity_y * dt
  end

  def resolve_collision(tilemap)
    local_x = @transform.x + CHAR_HITBOX_OFFSET_X - MAP_OFFSET_X
    local_y = @transform.y + CHAR_HITBOX_OFFSET_Y - MAP_OFFSET_Y

    h_result = Collision.tilemap_resolve(
      tilemap, local_x, local_y,
      CHAR_HITBOX_WIDTH, CHAR_HITBOX_HEIGHT,
      @velocity_x, 0.0
    )
    local_x = h_result.x
    @velocity_x = h_result.vx

    v_result = Collision.tilemap_resolve(
      tilemap, local_x, local_y,
      CHAR_HITBOX_WIDTH, CHAR_HITBOX_HEIGHT,
      0.0, @velocity_y
    )

    @transform.x = local_x + MAP_OFFSET_X - CHAR_HITBOX_OFFSET_X
    @transform.y = v_result.y + MAP_OFFSET_Y - CHAR_HITBOX_OFFSET_Y
    @velocity_y = v_result.vy

    if v_result.bottom?
      if !@on_ground
        @on_ground = true
        state_machine.trigger(@moving ? :land_moving : :land)
      end
    else
      @on_ground = false
    end
  end

  def update_state_machine
    return unless @on_ground
    if @moving && state_machine.state != :running
      state_machine.trigger(:move)
    elsif !@moving && state_machine.state != :idle
      state_machine.trigger(:stop)
    end
  end
end
