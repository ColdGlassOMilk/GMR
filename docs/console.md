# Console

GMR includes a developer console for debugging and real-time game manipulation. Press **`** (backtick) to toggle it open.

## Enabling the Console

```ruby
include GMR

def init
  Console.enable
end
```

The console is disabled by default. Call `Console.enable` in your `init` to activate it.

### Styling Options

```ruby
Console.enable(
  height: 300,              # Console height in pixels
  background: "#1a1a2e",    # Background color
  font_size: 16             # Text size
)
```

## Basic Usage

When open, type Ruby code and press Enter to execute:

```ruby
@player.x = 400                    # Move player
@player.health = 100               # Reset health
Time.scale = 0.5                   # Slow motion
@camera.zoom = 3.0                 # Zoom in
System.quit                        # Exit game
```

The console has access to all your instance variables and game state.

## Console Control

### Programmatic Control

```ruby
Console.show            # Open console
Console.hide            # Close console
Console.toggle          # Toggle open/closed
Console.open?           # Check if open
```

### Pausing When Open

The console captures keyboard input. Pause your game logic when it's open:

```ruby
def update(dt)
  return if Console.open?

  # Game logic only runs when console is closed
  @player.update(dt)
  @enemies.each { |e| e.update(dt) }
end
```

## Custom Commands

Register commands for common debug operations:

```ruby
def init
  Console.enable

  Console.register_command("tp", "Teleport to x,y") do |args|
    if args.length >= 2
      @player.transform.x = args[0].to_f
      @player.transform.y = args[1].to_f
      "Teleported to #{args[0]}, #{args[1]}"
    else
      "Usage: tp <x> <y>"
    end
  end

  Console.register_command("god", "Toggle invincibility") do
    @player.god_mode = !@player.god_mode
    "God mode: #{@player.god_mode ? 'ON' : 'OFF'}"
  end

  Console.register_command("spawn", "Spawn enemy at cursor") do
    @enemies << Enemy.new(Input.mouse_x, Input.mouse_y)
    "Spawned enemy at #{Input.mouse_x}, #{Input.mouse_y}"
  end

  Console.register_command("level", "Load level by number") do |args|
    if args.length >= 1
      load_level(args[0].to_i)
      "Loaded level #{args[0]}"
    else
      "Usage: level <number>"
    end
  end

  Console.register_command("clear_saves", "Delete all save data") do
    (1..3).each { |i| SaveManager.delete_slot(i) }
    "Save data cleared"
  end
end
```

### Command Format

```ruby
Console.register_command(name, description) do |args|
  # args is an array of space-separated arguments
  # Return a string to display in the console
end
```

## Printing to Console

Output debug information:

```ruby
Console.println("Player position: #{@player.transform.x}, #{@player.transform.y}")
Console.println("FPS: #{Time.fps}")
Console.println("State: #{state_machine.state}")
```

## Debug Patterns

### Variable Inspection

```ruby
Console.register_command("inspect", "Show player state") do
  lines = [
    "Position: #{@player.transform.x}, #{@player.transform.y}",
    "Velocity: #{@player.vx}, #{@player.vy}",
    "Health: #{@player.health}",
    "State: #{@player.state_machine.state}",
    "On Ground: #{@player.on_ground?}"
  ]
  lines.join("\n")
end
```

### Quick Teleport Shortcuts

```ruby
Console.register_command("start", "Go to start position") do
  @player.transform.x = 100
  @player.transform.y = 300
  "Teleported to start"
end

Console.register_command("boss", "Go to boss room") do
  @player.transform.x = 5000
  @player.transform.y = 200
  "Teleported to boss"
end
```

### Game State Manipulation

```ruby
Console.register_command("heal", "Restore full health") do
  @player.health = @player.max_health
  "Health restored"
end

Console.register_command("kill", "Kill all enemies") do
  count = @enemies.length
  @enemies.clear
  "Killed #{count} enemies"
end

Console.register_command("give", "Give item") do |args|
  if args.length >= 1
    @player.inventory << args[0]
    "Added #{args[0]} to inventory"
  else
    "Usage: give <item_name>"
  end
end
```

### Time Control

```ruby
Console.register_command("slow", "Slow motion") do
  Time.scale = 0.25
  "Time scale: 0.25x"
end

Console.register_command("fast", "Speed up") do
  Time.scale = 2.0
  "Time scale: 2x"
end

Console.register_command("normal", "Normal speed") do
  Time.scale = 1.0
  "Time scale: 1x"
end

Console.register_command("pause", "Pause time") do
  Time.scale = 0
  "Time paused"
end
```

### Camera Debugging

```ruby
Console.register_command("zoom", "Set camera zoom") do |args|
  if args.length >= 1
    @camera.zoom = args[0].to_f
    "Zoom: #{@camera.zoom}"
  else
    "Current zoom: #{@camera.zoom}"
  end
end

Console.register_command("shake", "Test screen shake") do
  @camera.shake(strength: 10, duration: 0.5)
  "Shake triggered"
end
```

## Disabling in Production

For release builds, skip enabling the console:

```ruby
def init
  if System.build_type == :debug
    Console.enable
    setup_debug_commands
  end

  # Rest of initialization...
end
```

## Complete Example

```ruby
include GMR

def init
  Window.set_size(960, 540)
  Console.enable(height: 250)

  # Debug commands
  Console.register_command("tp", "Teleport player") do |args|
    if args.length >= 2
      @player.transform.x = args[0].to_f
      @player.transform.y = args[1].to_f
      "Teleported to #{args[0]}, #{args[1]}"
    else
      "Usage: tp <x> <y>"
    end
  end

  Console.register_command("god", "Toggle god mode") do
    @god_mode = !@god_mode
    "God mode: #{@god_mode ? 'ON' : 'OFF'}"
  end

  Console.register_command("fps", "Show FPS") do
    "FPS: #{Time.fps}"
  end

  Console.register_command("state", "Show player state") do
    "State: #{@player.state_machine.state}"
  end

  # Game setup
  setup_game
end

def update(dt)
  return if Console.open?

  @player.update(dt)
end

def draw
  Graphics.clear("#1e1e32")
  @player.draw

  # Show debug info when console is open
  if Console.open?
    Graphics.draw_text("Player: #{@player.transform.x.to_i}, #{@player.transform.y.to_i}", 10, 500, 14, :yellow)
  end
end
```

## See Also

- [Game Loop](game-loop.md) - Pausing when console is open
- [Engine Model](engine-model.md) - Build types
