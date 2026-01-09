# gmrcli dev

Build and run in one command

## Usage

```bash
gmrcli dev [options]
```

## Description

Streamlined development workflow that builds and runs.

      Targets:
        debug   - Build debug and run (default)
        release - Build release and run
        web     - Build web and start server

      Options:
        --clean     - Clean before building (for fresh builds)
        --no-run    - Build only, don't run afterward

      Examples:
        gmrcli dev              # Build debug, run
        gmrcli dev --clean      # Clean, build debug, run
        gmrcli dev release      # Build release, run
        gmrcli dev web          # Build web, start server
        gmrcli dev web --clean  # Clean, build web, start server

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|
| `--clean` |  | boolean | `false` | Clean before building (use --clean for fresh build) |
| `--run` |  | boolean | `true` | Run after building (use --no-run to skip) |
| `--port` |  | numeric | `8080` | Port for web server |
| `--topmost` |  | boolean | `false` | Keep game window on top of other windows |

## Examples

```bash
# Basic usage
gmrcli dev

# With clean flag
gmrcli dev --clean
```

---

*See also: [CLI Reference](README.md)*
