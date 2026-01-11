class InputHandler
  def initialize(player, camera)
    @player = player
    @camera = camera
    @on_resolution_toggle = nil
    @on_next_shader = nil
    @on_prev_shader = nil
  end

  def on_resolution_toggle(&block)
    @on_resolution_toggle = block
  end

  def on_next_shader(&block)
    @on_next_shader = block
  end

  def on_prev_shader(&block)
    @on_prev_shader = block
  end

  def setup
    Input.map(:move_left, [:left, :a])
         .map(:move_right, [:right, :d])
         .map(:jump, [:space, :up, :w])
         .map(:attack, [:z, :x])
        #  .map(:toggle_resolution, [:r])
         .map(:next_shader, [:e])
         .map(:prev_shader, [:q])

    Input.on(:jump) { @player.jump }
    Input.on(:attack) { handle_attack }
    # Input.on(:toggle_resolution) { @on_resolution_toggle.call if @on_resolution_toggle }
    Input.on(:next_shader) { @on_next_shader.call if @on_next_shader }
    Input.on(:prev_shader) { @on_prev_shader.call if @on_prev_shader }
  end

  def update
    if Input.action_down?(:move_left)
      @player.move_left
    elsif Input.action_down?(:move_right)
      @player.move_right
    else
      @player.stop
    end

    if Input.action_pressed?(:attack) || Input.mouse_pressed?(:left)
      handle_attack
    end
  end

  private

  def handle_attack
    if @player.attack
      @camera.shake(strength: ATTACK_SHAKE_STRENGTH, duration: ATTACK_SHAKE_DURATION)
    end
  end
end
