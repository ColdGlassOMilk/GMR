# Getting Started

This guide walks through setting up GMR and running your first game.

## Prerequisites

### All Platforms

- Git
- A C++ compiler (see platform-specific requirements below)
- CMake 3.16 or later

### Windows

- Visual Studio 2019 or later with C++ workload, or
- MinGW-w64 with GCC 9+

### macOS

- Xcode Command Line Tools (`xcode-select --install`)

### Linux

- GCC 9+ or Clang 10+
- Development libraries: `libgl1-mesa-dev`, `libx11-dev`, `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`

On Ubuntu/Debian:
```bash
sudo apt install build-essential cmake libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

## Installation

### Clone the Repository

```bash
git clone https://github.com/ColdGlassOMilk/GMR
cd GMR
```

### Run Bootstrap

The bootstrap script installs dependencies and builds the CLI:

```bash
./bootstrap.sh
```

On Windows, use Git Bash or WSL, or run:
```cmd
bootstrap.bat
```

This will:
1. Clone and build mruby
2. Download raylib headers
3. Build the `gmrcli` tool
4. Set up the project structure

### Verify Installation

```bash
gmrcli version
```

You should see the GMR version number.

## Project Structure

After setup, your project looks like this:

```
GMR/
├── game/
│   ├── scripts/
│   │   └── main.rb        # Your game code
│   └── assets/
│       └── ...            # Images, sounds, etc.
├── src/                   # Engine C++ source (don't modify)
├── deps/                  # Dependencies (mruby, raylib)
├── docs/                  # Documentation
└── gmrcli                 # CLI tool
```

Your game code goes in `game/scripts/main.rb`. Assets go in `game/assets/`.

## Running Your First Game

### Build and Run

```bash
gmrcli dev
```

This builds the engine and runs your game. The default `main.rb` shows a simple example.

### Edit and Reload

With the game running, edit `game/scripts/main.rb` and save. Changes reload automatically without restarting.

### Try This

Replace `game/scripts/main.rb` with:

```ruby
include GMR

def init
  Window.set_title("My First Game")
  @x, @y = 400, 300
end

def update(dt)
  speed = 200 * dt
  @x -= speed if Input.key_down?(:left)
  @x += speed if Input.key_down?(:right)
  @y -= speed if Input.key_down?(:up)
  @y += speed if Input.key_down?(:down)
end

def draw
  Graphics.clear(:dark_gray)
  Graphics.draw_circle(@x, @y, 20, :cyan)
  Graphics.draw_text("Arrow keys to move", 10, 10, 20, :white)
end
```

Save and watch it reload.

## CLI Commands

### Development

```bash
gmrcli dev              # Build and run (most common)
gmrcli dev --clean      # Fresh build and run
```

### Building

```bash
gmrcli build debug      # Debug build (faster compile, more checks)
gmrcli build release    # Release build (optimized)
```

### Running

```bash
gmrcli run              # Run existing build
```

### Web Builds

```bash
gmrcli dev web          # Build for web and start local server
gmrcli build web        # Build for web only
gmrcli run web          # Start web server for existing build
```

Web builds require Emscripten. See [Web Build Setup](#web-build-setup) below.

## Adding Assets

Place images, sounds, and other assets in `game/assets/`:

```
game/assets/
├── player.png
├── tiles.png
├── sfx/
│   ├── jump.wav
│   └── coin.ogg
└── music/
    └── level1.ogg
```

Load them in your code:

```ruby
@texture = Texture.load("player.png")
@sound = Audio::Sound.load("sfx/jump.wav")
@music = Audio::Music.load("music/level1.ogg")
```

Paths are relative to `game/assets/`.

## Editor Setup

### Visual Studio Code

Install the Ruby extension for syntax highlighting. Create `.vscode/settings.json`:

```json
{
  "files.associations": {
    "*.rb": "ruby"
  }
}
```

### Other Editors

Any editor with Ruby syntax highlighting works. GMR uses standard Ruby syntax.

## Troubleshooting

### "gmrcli not found"

The CLI wasn't added to your PATH. Run it directly:
```bash
./gmrcli dev
```

Or add the GMR directory to your PATH.

### Build Errors

Try a clean build:
```bash
gmrcli dev --clean
```

### Missing Dependencies

Re-run bootstrap:
```bash
./bootstrap.sh
```

### Hot Reload Not Working

- Ensure you're editing `game/scripts/main.rb`
- Check for Ruby syntax errors in the console output
- Verify the file saved successfully

## Web Build Setup

Web builds compile your game to WebAssembly for browser play.

### Install Emscripten

Follow the [official guide](https://emscripten.org/docs/getting_started/downloads.html):

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Build for Web

```bash
gmrcli dev web
```

This builds and starts a local server. Open the displayed URL in your browser.

### Web Limitations

- No hot-reload (refresh page to see changes)
- Save data uses IndexedDB (persistent across sessions)
- Some file operations may be slower

## Next Steps

1. Read [Game Loop](game-loop.md) to understand the execution model
2. Try the [Platformer Tutorial](platformer-tutorial.md) for a complete example
3. Explore the [API Reference](api/README.md) for all available features

## Getting Help

- Check the [documentation](api/README.md)
- Review [example code](platformer-tutorial.md)
- Report issues on [GitHub](https://github.com/ColdGlassOMilk/GMR/issues)

## See Also

- [Game Loop](game-loop.md) - Understanding init/update/draw
- [Engine Model](engine-model.md) - How GMR works
- [CLI Reference](cli/README.md) - All CLI commands
