# IDE Integration

Setting up development environments for GMR.

## VSCode (Recommended)

### Ruby Extension

Install the Ruby extension for syntax highlighting:

1. Open Extensions (Ctrl+Shift+X)
2. Search for "Ruby"
3. Install "Ruby" by Peng Lv or "Ruby LSP"

### CMake Tools (Optional)

For C++ engine development:

1. Install "CMake Tools" extension
2. Open folder containing CMakeLists.txt
3. Select preset from status bar
4. Press F7 to build

### Recommended Settings

Add to `.vscode/settings.json`:

```json
{
  "files.associations": {
    "*.rb": "ruby"
  },
  "editor.tabSize": 2,
  "editor.insertSpaces": true,
  "[ruby]": {
    "editor.tabSize": 2
  }
}
```

### Tasks Configuration

Create `.vscode/tasks.json` for quick build/run:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "GMR: Build Debug",
      "type": "shell",
      "command": "gmrcli build debug",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "GMR: Build Release",
      "type": "shell",
      "command": "gmrcli build release",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "GMR: Run",
      "type": "shell",
      "command": "gmrcli run",
      "group": "test",
      "problemMatcher": []
    },
    {
      "label": "GMR: Build & Run",
      "type": "shell",
      "command": "gmrcli build debug && gmrcli run",
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": []
    }
  ]
}
```

Access tasks with Ctrl+Shift+B (build) or Ctrl+Shift+P → "Tasks: Run Task".

### Launch Configuration

For debugging the C++ engine, create `.vscode/launch.json`:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug GMR",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/Debug/gmr.exe",
      "args": [],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}/game",
      "environment": [],
      "console": "integratedTerminal"
    }
  ]
}
```

## CLion

### Opening the Project

1. File → Open
2. Select `CMakeLists.txt`
3. Choose "Open as Project"

### Selecting Presets

1. Settings → Build, Execution, Deployment → CMake
2. Add profile using preset (e.g., `windows-debug`)

### Running

- Build: Ctrl+F9
- Run: Shift+F10

## Language Metadata

GMR includes metadata files for IDE integration:

### api.json

Located at `engine/language/api.json`, contains:

- All GMR modules and classes
- Method signatures and parameters
- Return types
- Documentation strings

IDEs can use this for:

- Autocompletion
- Type hints
- Inline documentation

### Example api.json Structure

```json
{
  "protocol_version": "1",
  "modules": {
    "GMR::Graphics": {
      "methods": {
        "clear": {
          "params": [
            { "name": "color", "type": "Array<Integer>", "description": "RGB or RGBA color" }
          ],
          "return_type": "nil",
          "description": "Clear the screen with a color"
        }
      }
    }
  }
}
```

### Using api.json

The metadata can power:

1. **Custom LSP** - Build a language server using the API data
2. **Snippets** - Generate VSCode snippets from method signatures
3. **Documentation** - Auto-generate docs (this documentation uses it)

## Terminal Integration

### Windows (PowerShell)

Add to your PowerShell profile (`$PROFILE`):

```powershell
function gmr { gmrcli $args }
function gmr-run { gmrcli build debug; gmrcli run }
function gmr-web { gmrcli build web; gmrcli run web }
```

### Linux/macOS (Bash/Zsh)

Add to `~/.bashrc` or `~/.zshrc`:

```bash
alias gmr='gmrcli'
alias gmr-run='gmrcli build debug && gmrcli run'
alias gmr-web='gmrcli build web && gmrcli run web'
```

## Hot Reload Workflow

Best development workflow with hot reload:

1. Run `gmrcli run` (starts game in debug mode)
2. Edit `.rb` files in your IDE
3. Save file
4. Game automatically reloads (no manual restart)

Keep terminal visible to see:
- Reload messages
- Script errors
- Console output

## Tips

1. **Two terminals** - One for `gmrcli run`, one for git/other commands
2. **Split editor** - Code on left, game window on right
3. **Console key** - Use backtick (`) in-game for debug console
4. **Error messages** - Check terminal for Ruby syntax errors

## See Also

- [Getting Started](../getting-started.md) - First project setup
- [CLI Reference](../cli/README.md) - All commands
- [Bytecode](bytecode.md) - Debug vs release builds
