# gmrcli run

Run your GMR game.

## Usage

```bash
gmrcli run [target] [options]
```

## Targets

| Target | Description |
|--------|-------------|
| (none) | Run native build (default) |
| `web` | Start local web server |

## Examples

```bash
# Run native build
gmrcli run

# Run web build locally
gmrcli run web

# With text output
gmrcli run -o text
```

## Native Mode

Runs the compiled executable from `release/gmr.exe` (Windows) or `release/gmr` (Linux/macOS).

### Hot Reload

In debug builds, editing any `.rb` file in `game/scripts/` will automatically reload:

1. Run your game with `gmrcli run`
2. Edit `game/scripts/main.rb` (or any script)
3. Save the file
4. Changes apply instantly!

### Developer Console

Press **`** (backtick) to open the in-game console:

```ruby
$player[:x] = 400      # Modify variables
puts Time.fps          # Print values
GMR::System.quit       # Exit game
```

## Web Mode

Starts a local HTTP server to test your WebAssembly build:

```bash
gmrcli run web
# Opens http://localhost:8000
```

### Requirements
- Must run `gmrcli build web` first
- Python 3 (for HTTP server)

### Web Server Details
- Serves from `release/web/`
- Default port: 8000
- Press Ctrl+C to stop

## Options

| Option | Description |
|--------|-------------|
| `-o text` | Human-readable output |

## Troubleshooting

**"No executable found"**
- Run `gmrcli build` first

**Hot reload not working?**
- Only works in debug builds
- Check for Ruby syntax errors in console
- Make sure file is saved

**Web build not loading?**
- Run `gmrcli build web` first
- Check browser console for errors
- Try clearing browser cache

See [Troubleshooting](../troubleshooting.md) for more solutions.
