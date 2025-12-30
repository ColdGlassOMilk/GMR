# Building GMR on WSL (Windows Subsystem for Linux)

This guide covers building GMR on WSL2 with Ubuntu.

## Prerequisites

### 1. Install Build Tools

```bash
sudo apt update
sudo apt install build-essential git cmake
```

### 2. Install raylib

**Option A: From package manager (Ubuntu 22.04+)**

```bash
sudo apt install libraylib-dev
```

**Option B: Build from source (recommended for latest version)**

```bash
# Install dependencies
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

# Clone and build raylib
git clone https://github.com/raysan5/raylib.git
cd raylib
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
sudo make install
cd ../..
```

### 3. Install mruby

```bash
# Install Ruby (needed to build mruby)
sudo apt install ruby bison

# Clone and build mruby
git clone https://github.com/mruby/mruby.git
cd mruby
make -j$(nproc)

# Install to system
sudo cp build/host/lib/libmruby.a /usr/local/lib/
sudo cp -r include/* /usr/local/include/
cd ..
```

### 4. Verify Installations

```bash
# Check that libraries exist
ls /usr/local/lib/libmruby.a
ls /usr/local/include/mruby.h
pkg-config --libs raylib  # or check /usr/local/lib/libraylib.a
```

## Building GMR

### 1. Update the Makefile (if needed)

If you installed to `/usr/local`, you may need to update the Makefile:

```makefile
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lraylib -lmruby -lm -lpthread -ldl -lGL -lX11
```

Or create a `config.mk` file:

```bash
cat > config.mk << 'EOF'
CXXFLAGS += -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lraylib -lmruby -lm -lpthread -ldl -lGL -lX11 -lXrandr -lXi -lXcursor -lXinerama
EOF
```

Then modify the Makefile to include it:

```makefile
-include config.mk
```

### 2. Build

```bash
cd gmr
make clean
make
```

If you encounter linker errors, you may need additional X11 libraries:

```bash
sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

And update LDFLAGS:

```bash
make LDFLAGS="-L/usr/local/lib -lraylib -lmruby -lm -lpthread -ldl -lGL -lX11 -lXrandr -lXi -lXcursor -lXinerama"
```

## Running GMR on WSL

### Option 1: WSLg (Windows 11)

If you're on Windows 11, WSLg provides native GUI support:

```bash
./gmr
```

It should just work!

### Option 2: X Server (Windows 10)

For Windows 10, you need an X server:

1. **Install VcXsrv** (or Xming):
   - Download from: https://sourceforge.net/projects/vcxsrv/
   - Run XLaunch with these settings:
     - Multiple windows
     - Start no client
     - **Check "Disable access control"**
     - **Uncheck "Native opengl"**

2. **Set DISPLAY in WSL**:

```bash
# Add to ~/.bashrc
export DISPLAY=$(grep -m 1 nameserver /etc/resolv.conf | awk '{print $2}'):0
export LIBGL_ALWAYS_INDIRECT=0
```

3. **Reload and run**:

```bash
source ~/.bashrc
./gmr
```

### Option 3: WSL2 with GPU Acceleration

For better performance with WSL2:

```bash
# Install mesa drivers
sudo apt install mesa-utils

# Test OpenGL
glxinfo | grep "OpenGL version"

# Run GMR
./gmr
```

## Troubleshooting

### "cannot find -lraylib"

raylib not installed or not in library path:

```bash
# Check where it is
find /usr -name "libraylib*" 2>/dev/null

# Add to LDFLAGS if in non-standard location
make LDFLAGS="-L/path/to/raylib/lib -lraylib ..."
```

### "cannot find -lmruby"

mruby not installed:

```bash
# Check if it exists
find /usr -name "libmruby*" 2>/dev/null

# If you built it but didn't install:
make LDFLAGS="-L/path/to/mruby/build/host/lib -lmruby ..."
make CXXFLAGS="-I/path/to/mruby/include ..."
```

### "mruby.h: No such file or directory"

mruby headers not in include path:

```bash
# Find where headers are
find /usr -name "mruby.h" 2>/dev/null

# Add to CXXFLAGS
make CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -Iinclude -I/path/to/mruby/include"
```

### "error: 'optional' is not a member of 'std'"

Need C++17 support:

```bash
# Check g++ version (need 7+)
g++ --version

# If too old, install newer version
sudo apt install g++-11
make CXX=g++-11
```

### GLX/OpenGL errors when running

```bash
# Install mesa
sudo apt install mesa-utils libgl1-mesa-glx

# For X server issues
export LIBGL_ALWAYS_INDIRECT=1

# Or try software rendering
export LIBGL_ALWAYS_SOFTWARE=1
./gmr
```

### Window doesn't appear (WSL2)

1. Make sure X server is running (Windows 10)
2. Check DISPLAY variable: `echo $DISPLAY`
3. Test with simple X app: `sudo apt install x11-apps && xclock`

## Quick Start Script

Save this as `setup_wsl.sh`:

```bash
#!/bin/bash
set -e

echo "=== Installing dependencies ==="
sudo apt update
sudo apt install -y build-essential git cmake ruby bison \
    libasound2-dev libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

echo "=== Building raylib ==="
if [ ! -d "raylib" ]; then
    git clone https://github.com/raysan5/raylib.git
fi
cd raylib
mkdir -p build && cd build
cmake -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
sudo make install
cd ../..

echo "=== Building mruby ==="
if [ ! -d "mruby" ]; then
    git clone https://github.com/mruby/mruby.git
fi
cd mruby
make -j$(nproc)
sudo cp build/host/lib/libmruby.a /usr/local/lib/
sudo cp -r include/* /usr/local/include/
cd ..

echo "=== Building GMR ==="
cd gmr
make clean
make CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -Iinclude -I/usr/local/include" \
     LDFLAGS="-L/usr/local/lib -lraylib -lmruby -lm -lpthread -ldl -lGL -lX11 -lXrandr -lXi -lXcursor -lXinerama"

echo "=== Done! ==="
echo "Run with: ./gmr"
```

Run with:

```bash
chmod +x setup_wsl.sh
./setup_wsl.sh
```

## Performance Tips

1. **Use WSL2** - Much better than WSL1 for graphics
2. **Store project in Linux filesystem** - `/home/user/gmr` not `/mnt/c/...`
3. **Windows 11 + WSLg** - Best experience, native GPU acceleration
4. **Disable VSync if needed**: Add `SetTargetFPS(0)` or modify in Ruby

## Next Steps

Once built successfully:

```bash
# Run the demo
./gmr

# Edit scripts/main.rb and watch hot-reload in action!
```
