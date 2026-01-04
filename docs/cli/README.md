# CLI Reference

GMR uses the `gmrcli` tool for all operations. The CLI is **machine-first** by default, outputting structured JSON for IDE and automation integration.

## Quick Reference

| Command | Description |
|---------|-------------|
| `gmrcli setup` | Install dependencies (raylib, mruby, emscripten) |
| `gmrcli build debug` | Build debug version (hot-reload enabled) |
| `gmrcli build release` | Build optimized release version |
| `gmrcli build web` | Build WebAssembly version |
| `gmrcli run` | Run the native build |
| `gmrcli run web` | Start local web server |
| `gmrcli new <name>` | Create a new game project |
| `gmrcli info` | Show environment info |
| `gmrcli help` | Show all commands |

## Output Modes

```bash
# JSON output (default) - for IDE integration
gmrcli build debug

# Text output - for humans
gmrcli build debug -o text

# Environment variable
GMRCLI_OUTPUT=text gmrcli build debug
```

## Common Workflows

### First Time Setup
```bash
./bootstrap.sh         # Install gmrcli
gmrcli setup           # Install all dependencies
gmrcli build -o text   # Build the engine
gmrcli run             # Run the game
```

### Daily Development
```bash
gmrcli run             # Run and edit scripts - they hot-reload!
```

### Release Build
```bash
gmrcli build release   # Optimized build with bytecode-compiled scripts
```

### Web Deployment
```bash
gmrcli build web       # Build WebAssembly version
gmrcli run web         # Test locally at http://localhost:8000
```

## Detailed Documentation

- **[setup](setup.md)** - Dependency installation
- **[build](build.md)** - Build targets and options
- **[run](run.md)** - Running your game
- **[project](project.md)** - Project management commands
- **[output-formats](output-formats.md)** - JSON format, exit codes
- **[cmake-presets](cmake-presets.md)** - Alternative CMake workflow
