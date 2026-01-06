# gmrcli build

Build the game

## Usage

```bash
gmrcli build [options]
```

## Description

Build GMR for the specified target:
        debug    - Native debug build (default)
        release  - Native release build (optimized, no console)
        web      - Web/Emscripten build
        clean    - Remove all build artifacts
        all      - Build all targets

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|
| `--rebuild` | -r | boolean | `false` | Clean before building |

## Stages

1. Validating Environment
2. Configuring Build
3. Compiling
4. Complete
5. Validating Environment
6. Configuring Build
7. Compiling (WASM)
8. Complete

## Examples

```bash
# Basic usage
gmrcli build

# With rebuild flag
gmrcli build --rebuild
```

---

*See also: [CLI Reference](README.md)*
