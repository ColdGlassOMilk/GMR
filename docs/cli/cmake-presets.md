# CMake Presets

For users who prefer direct CMake commands over `gmrcli`.

## Available Presets

GMR includes `CMakePresets.json` with pre-configured build settings.

| Preset | Description |
|--------|-------------|
| `windows-debug` | Windows debug build |
| `windows-release` | Windows release build |
| `linux-debug` | Linux debug build |
| `linux-release` | Linux release build |
| `macos-debug` | macOS debug build |
| `macos-release` | macOS release build |
| `web-release` | WebAssembly build |

## Usage

### Configure and Build

```bash
# Configure
cmake --preset windows-release

# Build
cmake --build build --config Release
```

### Web Build

```bash
# Configure (requires Emscripten)
cmake --preset web-release

# Build
cmake --build build-web
```

## Manual CMake

Without presets:

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build .
```

### Build Options

| Option | Description |
|--------|-------------|
| `-DCMAKE_BUILD_TYPE=Debug` | Debug build |
| `-DCMAKE_BUILD_TYPE=Release` | Release build |
| `-DPLATFORM=Web` | WebAssembly build |

## When to Use CMake Directly

- **Integration with other build systems**
- **Custom build configurations**
- **CI/CD pipelines** (though JSON output from gmrcli works well too)
- **IDE integration** (VSCode, CLion, etc.)

## IDE Integration

### VSCode

1. Install CMake Tools extension
2. Open folder in VSCode
3. Select preset from status bar
4. Press F7 to build

### CLion

1. Open CMakeLists.txt as project
2. Select preset from CMake profile dropdown
3. Build with Ctrl+F9

## Comparison

| Feature | gmrcli | CMake Direct |
|---------|--------|--------------|
| Ease of use | Simple commands | More verbose |
| JSON output | Built-in | Need wrapper |
| Hot reload | `gmrcli run` | Manual |
| Web server | `gmrcli run web` | Manual setup |
| Cross-platform | Unified commands | Platform-specific |

For most users, `gmrcli` is recommended. Use CMake directly when you need custom build configurations or IDE integration.
