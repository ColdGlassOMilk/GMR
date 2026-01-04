# gmrcli build

Build the GMR engine and your game.

## Usage

```bash
gmrcli build [target] [options]
```

## Targets

| Target | Description |
|--------|-------------|
| `debug` | Debug build with hot-reload (default) |
| `release` | Optimized release build |
| `web` | WebAssembly build for browsers |
| `clean` | Remove all build artifacts |

## Examples

```bash
# Debug build (default)
gmrcli build debug
gmrcli build           # Same as above

# Release build
gmrcli build release

# WebAssembly build
gmrcli build web

# Clean build artifacts
gmrcli build clean

# With text output
gmrcli build debug -o text
```

## Build Differences

### Debug Build
- Hot-reload enabled - scripts reload on save
- Scripts loaded from disk at runtime
- Debug symbols included
- Faster compile time
- Larger binary size
- Console window visible on Windows

### Release Build
- Scripts compiled to bytecode and embedded
- Optimized for performance
- Smaller binary size
- No hot-reload
- No console window on Windows

### Web Build
- Compiles to WebAssembly
- Scripts compiled to bytecode
- Assets embedded in `.data` file
- Outputs to `release/web/`
- Requires Emscripten (run `gmrcli setup`)

## Output Artifacts

### Debug/Release (Native)
```
release/
└── gmr.exe          # Windows
└── gmr              # Linux/macOS
```

### Web
```
release/web/
├── gmr.html         # Main HTML file
├── gmr.js           # JavaScript loader
├── gmr.wasm         # WebAssembly binary
└── gmr.data         # Embedded assets
```

## Build Process

1. **Configure** - CMake generates build files
2. **Compile** - C++ sources compiled
3. **Scripts** - Ruby scripts compiled to bytecode (release/web only)
4. **Link** - Final executable linked
5. **Copy** - Assets copied to output directory

## Options

| Option | Description |
|--------|-------------|
| `-o text` | Human-readable output |
| `--protocol-version v1` | Use specific protocol version |

## Environment Variables

| Variable | Description |
|----------|-------------|
| `GMRCLI_OUTPUT` | Output mode (`json` or `text`) |

## Troubleshooting

**"cmake is not recognized"**
- Run `gmrcli setup` first
- Add CMake to your PATH

**"cannot find -lmruby"**
- Run `gmrcli setup` to install dependencies

See [Troubleshooting](../troubleshooting.md) for more solutions.
