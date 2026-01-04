# Bytecode Compilation

How GMR compiles and loads Ruby scripts.

## Overview

GMR uses [mruby](https://mruby.org/) to run Ruby scripts. Scripts can be loaded in two ways:

1. **Source mode** (debug) - Scripts loaded directly as `.rb` files
2. **Bytecode mode** (release) - Scripts pre-compiled to `.mrb` bytecode

## Debug vs Release

### Debug Build

```bash
gmrcli build debug
gmrcli run
```

- Loads `.rb` source files directly
- Supports hot reload (edit and see changes instantly)
- Shows line numbers in error messages
- Slightly slower startup (compilation happens at runtime)

### Release Build

```bash
gmrcli build release
gmrcli run release
```

- Loads pre-compiled `.mrb` bytecode
- Faster startup (no runtime compilation)
- Smaller distribution size
- No hot reload
- Error messages may have less detail

## How Compilation Works

### Build Process

When you run `gmrcli build release`:

1. GMR finds all `.rb` files in `game/scripts/`
2. Each file is compiled to mruby bytecode (`.mrb`)
3. Bytecode is embedded in the executable (or placed alongside for web)

### Script Loading Order

Scripts are loaded in this order:

1. `util.rb` (if exists) - Utility functions
2. `main.rb` - Main game script
3. Additional scripts in alphabetical order

### require Not Supported

GMR does not support Ruby's `require` statement. All scripts in `game/scripts/` are loaded automatically.

```ruby
# DON'T DO THIS - won't work
require 'enemy'
require 'player'

# INSTEAD - just define in separate files
# enemy.rb and player.rb are loaded automatically
```

## Bytecode Details

### File Format

Compiled bytecode uses mruby's `.mrb` format:

- Binary format (not human-readable)
- Platform-independent
- Contains compiled instructions and constants

### Inspecting Bytecode

You can view bytecode disassembly using mruby tools:

```bash
# If you have mruby installed
mrbc --verbose game/scripts/main.rb
```

## Hot Reload (Debug Only)

In debug mode, GMR monitors script files for changes:

1. You edit `main.rb`
2. GMR detects the file change
3. Script is recompiled and reloaded
4. `init` is called again

### Hot Reload Tips

- Use global variables (`$player`, `$enemies`) for state
- `init` reinitializes everything - design for this
- Some changes may require manual restart

```ruby
def init
  # Check if already initialized (for hot reload)
  $player ||= { x: 100, y: 100 }

  # Or always reinitialize
  $player = { x: 100, y: 100 }
end
```

## Troubleshooting

### "Script error" on startup

- Check for syntax errors in your `.rb` files
- Run Ruby syntax check: `ruby -c game/scripts/main.rb`

### Changes not appearing (release mode)

- Release mode doesn't hot reload
- Rebuild with `gmrcli build release`

### Error line numbers wrong

- In release mode, line numbers may be approximate
- Use debug mode for development

### Script not loading

- Ensure script is in `game/scripts/`
- Check filename spelling (case-sensitive on Linux/macOS)
- Verify file has `.rb` extension

## Web Build Considerations

For web builds:

- Bytecode is always used (no source loading)
- Scripts are compiled during `gmrcli build web`
- Hot reload not available in browser

## See Also

- [CLI Build Command](../cli/build.md) - Build options
- [CLI Run Command](../cli/run.md) - Running debug vs release
- [Project Structure](project-structure.md) - Script organization
