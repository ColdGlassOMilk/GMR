# gmrcli setup

Install all dependencies required to build GMR.

## Usage

```bash
gmrcli setup [options]
```

## Options

| Option | Description |
|--------|-------------|
| `--native-only` | Skip web/Emscripten (faster setup) |
| `--web-only` | Only install web/Emscripten |
| `--clean` | Clean rebuild everything |

## Examples

```bash
# Full setup (native + web)
gmrcli setup

# Native development only (faster)
gmrcli setup --native-only

# Web development only
gmrcli setup --web-only

# Force clean rebuild
gmrcli setup --clean
```

## What Gets Installed

### Native Build
- **raylib** - Graphics, audio, input library
- **mruby** - Embedded Ruby interpreter

### Web Build (Emscripten)
- **emsdk** - Emscripten SDK
- **raylib (web)** - raylib compiled for WebAssembly
- **mruby (web)** - mruby compiled for WebAssembly

## Platform Notes

### Windows (MSYS2)
Run from **MSYS2 MinGW64** terminal, not Command Prompt or PowerShell.

### Linux
May require `sudo` for installing system packages:
```bash
sudo apt install build-essential cmake git
```

### macOS
Requires Xcode command line tools:
```bash
xcode-select --install
```

## Timing

| Setup Type | Approximate Time |
|------------|------------------|
| Native only | 2-5 minutes |
| Web only | 10-20 minutes |
| Full setup | 15-25 minutes |

Web setup downloads ~1GB and compiles raylib/mruby for WebAssembly.

## Troubleshooting

**"gmrcli: command not found"**
- Run `source ~/.bashrc` or open a new terminal
- Re-run `./bootstrap.sh`

**Setup taking too long?**
- Use `--native-only` to skip web setup
- Web setup can be done later

See [Troubleshooting](../troubleshooting.md) for more solutions.
