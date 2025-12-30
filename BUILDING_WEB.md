# Building GMR for Web (Emscripten/WebAssembly)

This guide covers building GMR to run in web browsers using Emscripten.

## Overview

The web build compiles GMR to WebAssembly, allowing it to run in any modern browser. This requires:

1. Emscripten SDK (emsdk)
2. raylib compiled for Emscripten
3. mruby compiled for Emscripten

## Prerequisites

### 1. Install Emscripten SDK

```bash
# Clone emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest version
./emsdk install latest
./emsdk activate latest

# Set up environment (add to your shell profile for persistence)
source ./emsdk_env.sh
```

Verify installation:

```bash
emcc --version
# Should show something like: emcc 3.x.x
```

### 2. Build raylib for Emscripten

```bash
# Clone raylib
git clone https://github.com/raysan5/raylib.git
cd raylib

# Create build directory for web
mkdir build-web && cd build-web

# Configure with Emscripten
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=Web \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_INSTALL_PREFIX=../install-web

# Build and install
emmake make -j$(nproc)
emmake make install

# Note the install path for later
cd ../..
```

The raylib web library will be in `raylib/install-web/lib/libraylib.a`.

### 3. Build mruby for Emscripten

mruby requires a custom build configuration for Emscripten:

```bash
# Clone mruby
git clone https://github.com/mruby/mruby.git
cd mruby

# Create Emscripten build config
cat > build_config/emscripten.rb << 'EOF'
MRuby::CrossBuild.new('emscripten') do |conf|
  toolchain :clang
  
  conf.cc do |cc|
    cc.command = 'emcc'
    cc.flags = %w(-Os -DMRB_NO_STDIO)
  end
  
  conf.linker do |linker|
    linker.command = 'emcc'
  end
  
  conf.archiver do |archiver|
    archiver.command = 'emar'
  end
  
  # Core gems only for smaller size
  conf.gembox 'default'
  
  # Disable features not needed for web
  conf.disable_presym
end
EOF

# Build for Emscripten
MRUBY_CONFIG=build_config/emscripten.rb rake

# The library will be in build/emscripten/lib/libmruby.a
cd ..
```

## Building GMR for Web

### Option A: Using Environment Variables

Set paths to your compiled libraries:

```bash
export RAYLIB_WEB_PATH=/path/to/raylib/install-web
export MRUBY_WEB_PATH=/path/to/mruby/build/emscripten

# Make sure emsdk is activated
source /path/to/emsdk/emsdk_env.sh

# Configure with preset
cmake --preset web-release

# Build
cmake --build build-web
```

### Option B: Using libs Directory

Create a `libs` directory in your GMR project:

```bash
mkdir -p libs/raylib-web/lib libs/raylib-web/include
mkdir -p libs/mruby-web/lib libs/mruby-web/include

# Copy raylib
cp /path/to/raylib/install-web/lib/libraylib.a libs/raylib-web/lib/
cp /path/to/raylib/install-web/include/* libs/raylib-web/include/

# Copy mruby
cp /path/to/mruby/build/emscripten/lib/libmruby.a libs/mruby-web/lib/
cp -r /path/to/mruby/include/* libs/mruby-web/include/
```

Then build:

```bash
source /path/to/emsdk/emsdk_env.sh
cmake --preset web-release
cmake --build build-web
```

### Option C: Manual CMake Configuration

```bash
source /path/to/emsdk/emsdk_env.sh

emcmake cmake -B build-web \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=Web \
    -DCMAKE_CXX_FLAGS="-I/path/to/raylib/include -I/path/to/mruby/include" \
    -DCMAKE_EXE_LINKER_FLAGS="-L/path/to/raylib/lib -L/path/to/mruby/lib"

cmake --build build-web
```

## Output Files

After building, `build-web/` will contain:

- `gmr.html` - Main HTML file to open in browser
- `gmr.js` - JavaScript glue code
- `gmr.wasm` - WebAssembly binary
- `gmr.data` - Preloaded assets (scripts folder)

## Running Locally

Browsers require a web server due to CORS restrictions. You cannot simply open the HTML file directly.

### Using Python's built-in server:

```bash
cd build-web
python3 -m http.server 8080
```

Then open http://localhost:8080/gmr.html

### Using Node.js:

```bash
npx serve build-web
```

### Using PHP:

```bash
cd build-web
php -S localhost:8080
```

## Deployment

To deploy your game:

1. Copy these files to your web server:
   - `gmr.html`
   - `gmr.js`
   - `gmr.wasm`
   - `gmr.data`

2. Configure your server to serve `.wasm` files with the correct MIME type:
   ```
   application/wasm
   ```

3. For Apache, add to `.htaccess`:
   ```
   AddType application/wasm .wasm
   ```

4. For Nginx, add to config:
   ```nginx
   types {
       application/wasm wasm;
   }
   ```

## Customizing the HTML Shell

GMR includes two shell templates in the `web/` directory:

- `shell.html` - Full-featured with loading indicator, controls, and styling
- `minshell.html` - Minimal template for smaller file size

To use the minimal shell:

```bash
# Rename or copy
cp web/minshell.html web/shell.html
```

Or modify `CMakeLists.txt` to prefer a different shell.

## Build Size Optimization

### Smaller mruby Build

Edit `mruby/build_config/emscripten.rb` to include fewer gems:

```ruby
MRuby::CrossBuild.new('emscripten') do |conf|
  # ... (toolchain config)
  
  # Minimal gems
  conf.gem :core => 'mruby-compiler'
  conf.gem :core => 'mruby-error'
  conf.gem :core => 'mruby-numeric-ext'
  conf.gem :core => 'mruby-array-ext'
  conf.gem :core => 'mruby-hash-ext'
  conf.gem :core => 'mruby-string-ext'
  conf.gem :core => 'mruby-proc-ext'
  conf.gem :core => 'mruby-symbol-ext'
  conf.gem :core => 'mruby-kernel-ext'
  conf.gem :core => 'mruby-class-ext'
  conf.gem :core => 'mruby-math'
  conf.gem :core => 'mruby-time'
  conf.gem :core => 'mruby-print'
  
  conf.disable_presym
end
```

### Compiler Optimizations

For even smaller builds, add to CMakeLists.txt web section:

```cmake
# In the Emscripten linker flags
"-Oz"                           # Optimize for size
"-sEVAL_CTORS=1"               # Evaluate constructors at compile time
"--closure=1"                   # Advanced JS optimizations (requires Java)
```

Note: `--closure=1` requires Java to be installed.

## Limitations

The web build has some limitations compared to native builds:

1. **No file system write access** - Scripts are preloaded and read-only
2. **No hot reload** - Must rebuild to change scripts
3. **No console eval** - The developer console won't work for live code changes
4. **Audio may require user interaction** - Browsers block autoplay
5. **Performance** - Slightly slower than native, but still very good

## Troubleshooting

### "emcc: command not found"

Make sure you've sourced the emsdk environment:

```bash
source /path/to/emsdk/emsdk_env.sh
```

### "cannot find -lraylib" or "cannot find -lmruby"

Library paths are incorrect. Check:
- `RAYLIB_WEB_PATH` and `MRUBY_WEB_PATH` environment variables
- Or `libs/raylib-web/` and `libs/mruby-web/` directories

### "RuntimeError: memory access out of bounds"

Increase memory limits in CMakeLists.txt:

```cmake
"-sTOTAL_MEMORY=134217728"      # 128MB
"-sALLOW_MEMORY_GROWTH=1"
```

### Black screen / nothing renders

Check browser console (F12) for errors. Common issues:
- WebGL not supported
- CORS errors (need local server)
- JavaScript errors

### Audio doesn't play

Browsers require user interaction before playing audio. Click the canvas first, or add a "Start" button.

### Game runs too fast or too slow

The web build uses `requestAnimationFrame` which may have different timing. Make sure your game logic uses `dt` (delta time) for all movement.

## Quick Reference

```bash
# Full build from scratch
source ~/emsdk/emsdk_env.sh
export RAYLIB_WEB_PATH=~/raylib/install-web
export MRUBY_WEB_PATH=~/mruby/build/emscripten
cmake --preset web-release
cmake --build build-web

# Rebuild after script changes
cmake --build build-web

# Run locally
cd build-web && python3 -m http.server 8080
# Open http://localhost:8080/gmr.html
```

## Example Project Structure

```
gmr/
├── CMakeLists.txt
├── CMakePresets.json
├── include/
├── src/
├── scripts/
│   ├── main.rb
│   └── engine/
├── web/
│   ├── shell.html
│   └── minshell.html
├── libs/                    # Optional: local lib copies
│   ├── raylib-web/
│   │   ├── include/
│   │   └── lib/
│   └── mruby-web/
│       ├── include/
│       └── lib/
├── build/                   # Native build output
└── build-web/              # Web build output
    ├── gmr.html
    ├── gmr.js
    ├── gmr.wasm
    └── gmr.data
```