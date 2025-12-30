# Building GMR on Windows with MSYS2/MinGW64

## Prerequisites

### 1. Install MSYS2

1. Download MSYS2 from https://www.msys2.org/
2. Run the installer (default path: `C:\msys64`)
3. Open "MSYS2 MINGW64" from the Start menu

### 2. Install Dependencies

In the MSYS2 MINGW64 terminal:

```bash
# Update package database
pacman -Syu

# Install build tools
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-cmake

# Install raylib
pacman -S mingw-w64-x86_64-raylib

# Install mruby (if available) or build from source
pacman -S mingw-w64-x86_64-mruby
```

### 3. If mruby is not available via pacman, build from source:

```bash
# Install Ruby (needed to build mruby)
pacman -S ruby bison

# Clone and build mruby
git clone https://github.com/mruby/mruby.git
cd mruby
./minirake

# Copy libraries and headers
cp build/host/lib/libmruby.a /mingw64/lib/
cp -r include/* /mingw64/include/
```

### 4. Add MinGW64 to Windows PATH

Add `C:\msys64\mingw64\bin` to your system PATH:

1. Open System Properties → Advanced → Environment Variables
2. Under "System variables", find `Path`
3. Click Edit → New
4. Add `C:\msys64\mingw64\bin`
5. Click OK

### 5. Verify Installation

Open a new Command Prompt or PowerShell:

```cmd
g++ --version
cmake --version
```

## Building with CMake

### Command Line

```bash
# Configure (Debug)
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run
./gmr.exe
```

Or use the presets:

```bash
# Configure with preset
cmake --preset windows-debug

# Build with preset
cmake --build --preset windows-debug

# Run
./gmr.exe
```

### VS Code

1. Install extensions:
   - C/C++ (Microsoft)
   - CMake Tools (Microsoft)

2. Open the `gmr` folder in VS Code

3. CMake Tools will auto-detect and configure

4. Use the status bar at the bottom to:
   - Select kit (MinGW)
   - Select build type (Debug/Release)
   - Click Build button

Or use keyboard shortcuts:
- `Ctrl+Shift+B` → Build
- `F5` → Debug
- `Ctrl+Shift+P` → "CMake: Configure"

### Using VS Code Tasks

Press `Ctrl+Shift+P` → "Tasks: Run Task":
- **CMake Configure (Debug)** - Configure for debug
- **CMake Configure (Release)** - Configure for release
- **Build GMR (Debug)** - Build debug (default: Ctrl+Shift+B)
- **Build GMR (Release)** - Build release
- **Run GMR** - Run the executable
- **Clean All** - Remove build directory

## Project Structure

```
gmr/
├── CMakeLists.txt          # Main CMake configuration
├── CMakePresets.json       # Build presets for different platforms
├── include/gmr/            # Headers
├── src/                    # Source files
├── scripts/                # Ruby game scripts
│   └── main.rb
├── build/                  # Build output (generated)
└── gmr.exe                 # Executable (generated)
```

## Troubleshooting

### "cmake is not recognized"

Make sure `C:\msys64\mingw64\bin` is in your PATH and restart your terminal.

### "The C compiler is not able to compile a simple test program"

Specify the compiler explicitly:

```bash
cmake -B build -G "MinGW Makefiles" ^
    -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe ^
    -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe
```

### "cannot find -lraylib"

```bash
# In MSYS2 MINGW64 terminal
pacman -S mingw-w64-x86_64-raylib
```

### "cannot find -lmruby"

Build mruby from source (see step 3 above).

### "mruby.h: No such file"

Make sure mruby headers are installed:

```bash
ls /mingw64/include/mruby.h
```

If not, copy them from your mruby build.

### Linker errors about undefined references

Try rebuilding from clean:

```bash
# Remove build directory
rmdir /S /Q build

# Reconfigure and build
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Runtime error: DLL not found

The CMakeLists.txt uses static linking, so DLLs shouldn't be needed. If you still get errors:

1. Make sure `C:\msys64\mingw64\bin` is in PATH
2. Or copy required DLLs next to gmr.exe

## Release Build

For distribution:

```bash
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The release build:
- Is optimized (-O2)
- Has no debug symbols
- Hides the console window (-mwindows)
- Is statically linked (no DLL dependencies)

## Distributing Your Game

1. Build with Release configuration
2. Copy these files together:
   - `gmr.exe`
   - `scripts/` folder
   - Any assets (images, sounds)

The static linking ensures no MinGW DLLs are needed.
