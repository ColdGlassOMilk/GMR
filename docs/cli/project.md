# Project Commands

Commands for managing GMR projects.

## gmrcli new

Create a new game project.

```bash
gmrcli new <project-name>
```

### Example

```bash
gmrcli new my-awesome-game
cd my-awesome-game
gmrcli build -o text
gmrcli run
```

### What It Creates

```
my-awesome-game/
├── game/
│   ├── scripts/
│   │   └── main.rb      # Your game code
│   └── assets/          # Images, sounds, etc.
├── CMakeLists.txt
└── README.md
```

---

## gmrcli info

Display environment and configuration information.

```bash
gmrcli info
```

### Output Includes
- GMR version
- Platform information
- Installed dependencies
- Build configuration
- Project paths

---

## gmrcli version

Show version and protocol information.

```bash
gmrcli version
```

### Output
```json
{
  "gmrcli": "0.1.0",
  "protocol": "v1",
  "raylib": "5.6-dev",
  "mruby": "3.4.0"
}
```

---

## gmrcli help

Show all available commands and options.

```bash
gmrcli help
gmrcli help <command>   # Help for specific command
```

### Examples

```bash
gmrcli help           # All commands
gmrcli help build     # Build command help
gmrcli help setup     # Setup command help
```
