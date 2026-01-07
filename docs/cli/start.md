# gmrcli start

Run the game

## Usage

```bash
gmrcli start [options]
```

## Description

Run GMR with the current project's scripts.

      By default, runs the native executable. Use 'web' to start a local web server.

      Targets:
        (default) - Run the native executable
        web       - Start a local web server for the web build

      The command looks for game/scripts/main.rb in the current directory.
      The GMR executable is found automatically from the project's release/ folder.

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|
| `--project` | -p | string | - | Path to project directory |
| `--port` |  | numeric | `8080` | Port for web server |
| `--topmost` |  | boolean | `false` | Keep game window on top of other windows |

## Examples

```bash
# Basic usage
gmrcli start
```

---

*See also: [CLI Reference](README.md)*
