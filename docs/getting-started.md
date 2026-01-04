# Getting Started with GMR

This guide will help you install GMR and create your first game.

## Installation

### Windows

1. **Install [MSYS2](https://www.msys2.org/)** (default path: `C:\msys64`)

2. **Open MSYS2 MinGW64** terminal and run:
   ```bash
   git clone https://github.com/ColdGlassOMilk/GMR
   cd GMR
   ./bootstrap.sh         # Install gmrcli & run setup
   gmrcli build -o text   # Build the engine
   gmrcli run             # Run the game!
   ```

3. **Edit `game/scripts/main.rb`** - changes reload automatically while running!

> **Note:** `gmrcli` outputs JSON by default for IDE/automation use. Add `-o text` for traditional colored text output.

### Linux (Ubuntu/Debian)

```bash
git clone https://github.com/ColdGlassOMilk/GMR
cd GMR
./bootstrap.sh             # Install gmrcli & run setup
gmrcli build -o text       # Build
gmrcli run                 # Run
```

### macOS

```bash
git clone https://github.com/ColdGlassOMilk/GMR
cd GMR
./bootstrap.sh             # Install gmrcli & run setup
gmrcli build -o text       # Build
gmrcli run                 # Run
```

## Your First Game

Create or edit `game/scripts/main.rb`:

```ruby
include GMR

def init
  Window.set_title("My First Game")
  Window.set_size(800, 600)

  $player = { x: 400.0, y: 300.0, speed: 200 }
end

def update(dt)
  # Movement
  $player[:x] -= $player[:speed] * dt if Input.key_down?(:left)
  $player[:x] += $player[:speed] * dt if Input.key_down?(:right)
  $player[:y] -= $player[:speed] * dt if Input.key_down?(:up)
  $player[:y] += $player[:speed] * dt if Input.key_down?(:down)

  # Keep player on screen
  $player[:x] = [[$player[:x], 0].max, 800].min
  $player[:y] = [[$player[:y], 0].max, 600].min
end

def draw
  Graphics.clear([20, 20, 40])

  # Draw player as a circle
  Graphics.draw_circle($player[:x], $player[:y], 16, [100, 200, 255])

  # Draw instructions
  Graphics.draw_text("Arrow keys to move!", 10, 10, 20, [255, 255, 255])
  Graphics.draw_text("FPS: #{Time.fps}", 10, 35, 16, [150, 150, 150])
end
```

Save the file and watch it reload instantly!

## Lifecycle Hooks

Every GMR game uses three main functions:

```ruby
def init
  # Called once at startup
  # Initialize variables, load assets, set up game state
end

def update(dt)
  # Called every frame
  # dt = time since last frame in seconds
  # Handle input, update positions, game logic
end

def draw
  # Called every frame after update
  # All rendering happens here
  # Always call Graphics.clear() first!
end
```

## Hot Reload

One of GMR's best features is hot reload. While your game is running:

1. Edit any `.rb` file in `game/scripts/`
2. Save the file
3. Changes apply instantly - no restart needed!

This works for:
- Function definitions
- Variable changes
- New code
- Bug fixes

**Note:** Hot reload only works in debug builds. Release builds compile scripts to bytecode.

## Developer Console

Press **`** (backtick/tilde) to open the developer console while your game runs.

You can execute Ruby code in real-time:

```ruby
$player[:x] = 400           # Teleport player
$player[:speed] = 500       # Speed boost!
puts Time.fps               # Print current FPS
puts GMR::System.platform   # Check platform
GMR::System.quit            # Exit the game
```

The console is perfect for:
- Debugging variables
- Testing changes without editing files
- Experimenting with API calls
- Tweaking game values

## Next Steps

- **[API Reference](api/README.md)** - Learn all the available functions
- **[CLI Reference](cli/README.md)** - Master the build tools
- **[Troubleshooting](troubleshooting.md)** - Fix common issues
