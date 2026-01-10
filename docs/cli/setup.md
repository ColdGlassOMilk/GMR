# gmrcli setup

Set up the GMR development environment

## Usage

```bash
gmrcli setup [options]
```

## Description

Installs all dependencies needed for GMR development:
      - System packages (gcc, cmake, ninja, mruby)
      - raylib (built from source)
      - Emscripten SDK (for web builds)
      - mruby for web (cross-compiled)

      Use --native-only for faster setup if you don't need web builds.

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|
| `--native_only` | -n | boolean | `false` | Skip web/Emscripten setup (faster) |
| `--skip_web` |  | boolean | `false` | Same as --native-only |
| `--web_only` | -w | boolean | `false` | Only setup web/Emscripten (skip native) |
| `--skip_native` |  | boolean | `false` | Same as --web-only |
| `--skip_pacman` |  | boolean | `false` | Skip pacman package installation |
| `--clean` |  | boolean | `false` | Clean everything and start fresh |
| `--fix_ssl` |  | boolean | `false` | Fix SSL certificate issues |

## Stages

1. Environment Detection
2. Directory Setup
3. Generating API Definitions
4. Installing Packages
5. Building mruby #{mruby_version_display} (native)
6. Building raylib #{raylib_version_display} (native)
7. Setting up Emscripten #{emscripten_version_display}
8. Building raylib #{raylib_version_display} (web)
9. Building mruby #{mruby_version_display} (web)
10. Verification
11. Complete

## Examples

```bash
# Basic usage
gmrcli setup

# With native_only flag
gmrcli setup --native_only
```

---

*See also: [CLI Reference](README.md)*
