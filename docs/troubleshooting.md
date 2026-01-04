# Troubleshooting

Common issues and solutions for GMR.

## Setup Issues

### "gmrcli not found"

**Symptom:** Command not recognized after installation.

**Solutions:**
1. Ensure GMR is in your PATH
2. Restart your terminal after installation
3. Use full path: `./gmrcli` or `.\gmrcli.exe`

### Setup fails to download dependencies

**Symptom:** `gmrcli setup` fails with network errors.

**Solutions:**
1. Check internet connection
2. Try again (temporary network issues)
3. Check firewall/proxy settings
4. Use `--native-only` to skip web dependencies

```bash
gmrcli setup --native-only
```

### Emscripten setup issues

**Symptom:** Web build dependencies fail to install.

**Solutions:**
1. Ensure Python 3 is installed
2. On Windows, run as Administrator
3. Check disk space (needs ~2GB)
4. Try manual Emscripten installation

## Build Issues

### "CMake not found"

**Symptom:** Build fails because CMake isn't installed.

**Solutions:**
1. Run `gmrcli setup` first
2. Install CMake manually from cmake.org
3. Add CMake to PATH

### Build fails with compiler errors

**Symptom:** C++ compilation errors during engine build.

**Solutions:**
1. Ensure you have a C++ compiler:
   - Windows: Visual Studio Build Tools
   - Linux: `sudo apt install build-essential`
   - macOS: `xcode-select --install`
2. Run `gmrcli setup --clean` to reset dependencies
3. Check for conflicting CMake cache (delete `build/` folder)

### "raylib not found"

**Symptom:** Build can't find raylib library.

**Solutions:**
1. Run `gmrcli setup` to install dependencies
2. Try `gmrcli setup --clean` to reinstall
3. Check `deps/` folder exists

## Script Errors

### "undefined method" at startup

**Symptom:** Game crashes with undefined method error.

**Causes:**
1. Typo in method name
2. Missing `include GMR`
3. Method called before definition

**Solutions:**
```ruby
# Make sure to include GMR
include GMR

# Check method names (case-sensitive)
Graphics.draw_rect(...)  # Correct
Graphics.DrawRect(...)   # Wrong
```

### "wrong number of arguments"

**Symptom:** Method called with incorrect parameters.

**Solution:** Check the [API Reference](api/README.md) for correct signatures.

```ruby
# Wrong: missing color parameter
Graphics.draw_rect(100, 100, 50, 50)

# Correct
Graphics.draw_rect(100, 100, 50, 50, [255, 0, 0])
```

### Scripts not loading

**Symptom:** Game starts but shows nothing or errors.

**Solutions:**
1. Ensure `game/scripts/main.rb` exists
2. Check file has `.rb` extension
3. Verify `init`, `update`, `draw` functions exist
4. Check for syntax errors: `ruby -c game/scripts/main.rb`

### Hot reload not working

**Symptom:** Changes to scripts don't appear.

**Solutions:**
1. Only works in debug mode (`gmrcli run`, not `gmrcli run release`)
2. Save the file (Ctrl+S)
3. Check terminal for reload errors
4. Restart game if state is corrupted

## Runtime Issues

### Low FPS / Performance

**Symptom:** Game runs slowly.

**Solutions:**
1. Use `draw_region` for large tilemaps (only draw visible tiles)
2. Don't load resources in `update` or `draw`
3. Reduce number of draw calls
4. Use release build for better performance

```ruby
# Bad: Loading every frame
def draw
  sprite = Graphics::Texture.load("player.png")  # DON'T DO THIS
  sprite.draw(100, 100)
end

# Good: Load once in init
def init
  $sprite = Graphics::Texture.load("player.png")
end

def draw
  $sprite.draw(100, 100)
end
```

### Black screen

**Symptom:** Window opens but nothing displays.

**Solutions:**
1. Check `draw` function exists and is called
2. Make sure you're drawing something visible:
```ruby
def draw
  Graphics.clear([255, 0, 0])  # Bright red to confirm draw works
end
```
3. Check for errors in terminal
4. Verify window size is set

### Window doesn't open

**Symptom:** Game crashes immediately or no window appears.

**Solutions:**
1. Check terminal for error messages
2. Verify graphics drivers are up to date
3. Try running from command line to see errors
4. Check `init` function for errors before window setup

### Input not responding

**Symptom:** Key presses have no effect.

**Solutions:**
1. Check `console_open?` - input is blocked when console is open
2. Verify key symbols are correct (`:space` not `:spacebar`)
3. Check `update` function is being called
4. Use `key_pressed?` for single actions, `key_down?` for continuous

```ruby
def update(dt)
  return if console_open?  # Skip game input when console open

  # Correct key symbol
  if Input.key_pressed?(:space)
    jump()
  end
end
```

## Audio Issues

### No sound

**Symptom:** Sounds don't play.

**Solutions:**
1. Check file path is correct
2. Verify file format (WAV, OGG, MP3)
3. Check volume isn't 0
4. Confirm sound loaded successfully:
```ruby
def init
  $sound = Audio::Sound.load("assets/sound.wav")
  if $sound.nil?
    puts "Failed to load sound!"
  end
end
```

### Sound distorted or stuttering

**Symptom:** Audio quality issues.

**Solutions:**
1. Use WAV for short sound effects
2. Use OGG for music (better compression)
3. Don't play many sounds simultaneously
4. Check audio file isn't corrupted

## Web Build Issues

### Web build fails

**Symptom:** `gmrcli build web` fails.

**Solutions:**
1. Run `gmrcli setup` first (installs Emscripten)
2. Ensure Emscripten is activated:
```bash
# The setup should handle this, but if manual:
source deps/emsdk/emsdk_env.sh  # Linux/macOS
deps\emsdk\emsdk_env.bat        # Windows
```
3. Check for Emscripten-incompatible code

### Web game doesn't start

**Symptom:** Browser shows loading but game never starts.

**Solutions:**
1. Open browser developer console (F12) for errors
2. Serve from local server (`gmrcli run web`), not file://
3. Check all assets are included in build
4. Verify WASM is supported (modern browser required)

### Assets not loading in web

**Symptom:** Textures/sounds missing in web build.

**Solutions:**
1. Use relative paths from game folder
2. Ensure assets are in `game/assets/`
3. Check case sensitivity (web is case-sensitive)

## Platform-Specific

### Windows: "VCRUNTIME140.dll not found"

**Solution:** Install Visual C++ Redistributable from Microsoft.

### Linux: "libGL.so not found"

**Solution:** Install OpenGL libraries:
```bash
sudo apt install libgl1-mesa-dev
```

### macOS: "App is damaged"

**Solution:** Allow app in Security preferences or run:
```bash
xattr -cr ./gmr
```

## Getting Help

If these solutions don't help:

1. Check error messages carefully
2. Try minimal reproduction (simplest code that shows the issue)
3. Report issues: https://github.com/anthropics/claude-code/issues

Include:
- Operating system and version
- GMR version (`gmrcli version`)
- Error message (full text)
- Minimal code to reproduce

## See Also

- [Getting Started](getting-started.md) - Setup walkthrough
- [CLI Reference](cli/README.md) - Command details
- [API Reference](api/README.md) - Correct API usage
