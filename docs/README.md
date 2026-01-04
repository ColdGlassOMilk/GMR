# GMR Documentation

Welcome to the GMR documentation! GMR (Games Made with Ruby) is a modern, cross-platform game framework combining the elegance of Ruby with the performance of C++.

## Quick Navigation

- **[Getting Started](getting-started.md)** - Installation, first game, hot reload
- **[CLI Reference](cli/README.md)** - gmrcli commands and workflows
- **[API Reference](api/README.md)** - Complete Ruby API documentation
- **[Advanced Topics](advanced/README.md)** - Bytecode, project structure, IDE integration
- **[Troubleshooting](troubleshooting.md)** - Common issues and solutions

## Features

| Feature | Description |
|---------|-------------|
| Hot Reload | Edit Ruby scripts and see changes instantly |
| Live REPL | Built-in developer console for real-time code execution |
| Cross-Platform | Windows, Linux, macOS, and WebAssembly |
| Memory Safe | Handle-based resources - Ruby never touches raw pointers |
| Fast | mruby bytecode + raylib hardware-accelerated rendering |
| Simple API | Clean, Ruby-style API for rapid game development |

## Modules at a Glance

| Module | Purpose |
|--------|---------|
| [GMR::Graphics](api/graphics.md) | Drawing primitives, shapes, text |
| [GMR::Graphics::Texture](api/texture.md) | Image loading and sprite rendering |
| [GMR::Graphics::Tilemap](api/tilemap.md) | Tile-based map rendering |
| [GMR::Input](api/input.md) | Keyboard, mouse, action mapping |
| [GMR::Audio::Sound](api/audio.md) | Sound effects and music |
| [GMR::Window](api/window.md) | Window management, virtual resolution |
| [GMR::Time](api/time.md) | Frame timing, FPS |
| [GMR::System](api/system.md) | Platform info, random numbers, error handling |
| [GMR::Collision](api/collision.md) | Collision detection helpers |

## Quick Example

```ruby
include GMR

def init
  Window.set_title("My Game")
  $x, $y = 160, 120
end

def update(dt)
  $x += 100 * dt if Input.key_down?(:right)
  $x -= 100 * dt if Input.key_down?(:left)
end

def draw
  Graphics.clear([20, 20, 40])
  Graphics.draw_circle($x, $y, 8, [100, 200, 255])
end
```

## Getting Help

- [GitHub Issues](https://github.com/ColdGlassOMilk/GMR/issues) - Report bugs or request features
- [Troubleshooting Guide](troubleshooting.md) - Common problems and solutions
