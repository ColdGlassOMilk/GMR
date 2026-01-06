# gmrcli new

Create a new GMR game project

## Usage

```bash
gmrcli new [options]
```

## Description

Scaffolds a new GMR project with the following structure:
        NAME/
          game/
            scripts/
              main.rb       - Main game script
            assets/
              sprites/
              sounds/
              fonts/
              music/
          gmr.json        - Project configuration
          .gitignore

      Templates:
        basic  - Starter template with movement example (default)
        empty  - Minimal template with empty callbacks

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|
| `--template` | -t | string | `basic` | Template: basic, empty |

## Examples

```bash
# Basic usage
gmrcli new
```

---

*See also: [CLI Reference](README.md)*
