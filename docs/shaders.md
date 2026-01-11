# Shaders

Shaders are GPU programs that apply visual effects to your game. GMR provides a simple Ruby DSL for loading GLSL shaders and applying them to any drawable content—sprites, tilemaps, primitives, or entire scenes.

## Core Concepts

| Concept | Description |
|---------|-------------|
| **Fragment Shader** | GPU program that determines each pixel's final color |
| **Uniform** | Variable you pass from Ruby to the shader |
| **Block-based Usage** | Apply shaders with `shader.use { }` blocks |

Most 2D effects only need a fragment shader. GMR handles the vertex shader automatically.

---

## Loading Shaders

### From Files

```ruby
# Fragment shader only (most common)
@grayscale = Graphics::Shader.load(fragment: "shaders/grayscale.fs")

# Both vertex and fragment
@custom = Graphics::Shader.load(
  vertex: "shaders/custom.vs",
  fragment: "shaders/custom.fs"
)
```

Paths are relative to your `game/assets/` directory.

### From Source Code

```ruby
glsl_code = <<~GLSL
  #version 100
  precision mediump float;
  varying vec2 fragTexCoord;
  varying vec4 fragColor;
  uniform sampler2D texture0;
  uniform vec4 colDiffuse;

  void main() {
    vec4 texel = texture2D(texture0, fragTexCoord);
    float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    gl_FragColor = vec4(vec3(gray), texel.a) * colDiffuse * fragColor;
  }
GLSL

@shader = Graphics::Shader.from_source(fragment: glsl_code)
```

---

## Setting Uniforms

Use `shader.set` to pass values to your shader. The type is inferred from the arguments:

| Arguments | GLSL Type | Example |
|-----------|-----------|---------|
| 1 float | `float` | `shader.set(:intensity, 0.5)` |
| 1 integer | `int` | `shader.set(:levels, 4)` |
| 2 floats | `vec2` | `shader.set(:resolution, 800.0, 600.0)` |
| 3 floats | `vec3` | `shader.set(:tint, 1.0, 0.5, 0.0)` |
| 4 floats | `vec4` | `shader.set(:color, 1.0, 0.5, 0.0, 1.0)` |
| 1 texture | `sampler2D` | `shader.set(:noise, @noise_texture)` |

```ruby
def draw
  @shader.set(:time, GMR::Time.elapsed)
  @shader.set(:resolution, Window.width.to_f, Window.height.to_f)
  @shader.set(:intensity, 0.8)

  @shader.use do
    @sprite.draw
  end
end
```

**Note:** Set uniforms *before* the `use` block, not inside it.

---

## Using Shaders

### Block Syntax (Recommended)

```ruby
def draw
  @camera.use do
    # Normal rendering (no shader)
    @background.draw

    # Apply shader to specific draws
    @crt_shader.use do
      @level.draw
      @player.draw
    end

    # Back to normal
    @foreground.draw
  end
end
```

Everything inside `shader.use { }` is rendered with the shader applied. Draws outside use the default shader.

### Nesting with Camera

Shaders compose naturally with camera blocks:

```ruby
@shader.use do
  @camera.use do
    @tilemap.draw
    @player.draw
  end
end
```

Or camera first, then shader:

```ruby
@camera.use do
  @shader.use do
    @level.draw
    @player.draw
  end
end
```

### Explicit Begin/End (Advanced)

For cases where block syntax isn't convenient:

```ruby
@shader.begin
@sprite1.draw
@sprite2.draw
@shader.end
```

**Warning:** Always pair `begin` with `end`. Prefer block syntax when possible.

---

## Writing GLSL Shaders

GMR supports two GLSL versions depending on the build target:

| Target | GLSL Version | Notes |
|--------|--------------|-------|
| **Native** (Windows/Linux/macOS) | GLSL 330 | OpenGL 3.3 core profile |
| **Web** (WebAssembly) | GLSL ES 100 | WebGL 1.0 compatible |

**Recommendation:** Write shaders in GLSL ES 100 for maximum compatibility across all platforms.

### GLSL ES 100 (Recommended - Works Everywhere)

```glsl
#version 100

precision mediump float;

// Inputs from vertex shader (provided by raylib)
varying vec2 fragTexCoord;   // Texture coordinate (0-1)
varying vec4 fragColor;      // Vertex color

// Raylib's default uniforms
uniform sampler2D texture0;   // Primary texture
uniform vec4 colDiffuse;      // Diffuse color multiplier

// Your custom uniforms
uniform float intensity;
uniform vec2 resolution;
uniform float time;

void main() {
    // Sample the texture
    vec4 texel = texture2D(texture0, fragTexCoord);

    // Apply your effect
    // ...

    // Output final color (multiply by colDiffuse and fragColor for correct blending)
    gl_FragColor = texel * colDiffuse * fragColor;
}
```

### GLSL 330 (Native Only)

```glsl
#version 330

// Inputs from vertex shader (provided by raylib)
in vec2 fragTexCoord;   // Texture coordinate (0-1)
in vec4 fragColor;      // Vertex color

// Raylib's default uniforms
uniform sampler2D texture0;   // Primary texture
uniform vec4 colDiffuse;      // Diffuse color multiplier

// Your custom uniforms
uniform float intensity;
uniform vec2 resolution;
uniform float time;

// Output (required)
out vec4 finalColor;

void main() {
    // Sample the texture
    vec4 texel = texture(texture0, fragTexCoord);

    // Apply your effect
    // ...

    // Output final color (multiply by colDiffuse and fragColor for correct blending)
    finalColor = texel * colDiffuse * fragColor;
}
```

### GLSL Version Differences

| Feature | GLSL ES 100 (Web) | GLSL 330 (Native) |
|---------|-------------------|-------------------|
| Version directive | `#version 100` | `#version 330` |
| Precision | `precision mediump float;` required | Not needed |
| Vertex inputs | `varying vec2` | `in vec2` |
| Fragment output | `gl_FragColor` | `out vec4 finalColor` |
| Texture sampling | `texture2D()` | `texture()` |
| For loops | Must use constant bounds | Variable bounds allowed |

### Built-in Inputs

| Name | Type | Description |
|------|------|-------------|
| `fragTexCoord` | `vec2` | UV coordinates (0-1 range) |
| `fragColor` | `vec4` | Vertex color from sprite/primitive |
| `texture0` | `sampler2D` | The texture being drawn |
| `colDiffuse` | `vec4` | Color multiplier (for tinting) |

**Important:** Always multiply your final color by `colDiffuse * fragColor` to preserve sprite tinting and alpha.

---

## Common Effects

All examples use GLSL ES 100 for cross-platform compatibility.

### Grayscale

Convert to black and white:

```glsl
#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float intensity;  // 0.0 = color, 1.0 = full grayscale

void main() {
    vec4 texel = texture2D(texture0, fragTexCoord);
    float gray = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
    vec3 result = mix(texel.rgb, vec3(gray), intensity);
    gl_FragColor = vec4(result, texel.a) * colDiffuse * fragColor;
}
```

```ruby
@grayscale = Graphics::Shader.load(fragment: "shaders/grayscale.fs")
@grayscale.set(:intensity, 1.0)
```

### Wave Distortion

Animated wavy effect:

```glsl
#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;
uniform float amplitude;  // 0.01 to 0.05
uniform float frequency;  // 5.0 to 20.0

void main() {
    vec2 uv = fragTexCoord;
    uv.x += sin(uv.y * frequency + time * 3.0) * amplitude;

    vec4 texel = texture2D(texture0, uv);
    gl_FragColor = texel * colDiffuse * fragColor;
}
```

```ruby
@wave = Graphics::Shader.load(fragment: "shaders/wave.fs")

def draw
  @wave.set(:time, GMR::Time.elapsed)
  @wave.set(:amplitude, 0.02)
  @wave.set(:frequency, 15.0)

  @wave.use do
    @sprite.draw
  end
end
```

### CRT Monitor

Retro scanlines and curvature:

```glsl
#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;
uniform float curvature;         // 4.0 to 10.0
uniform float scanlineIntensity; // 0.1 to 0.5

void main() {
    // Apply barrel distortion
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    vec2 offset = uv.yx / curvature;
    uv += uv * offset * offset;
    uv = uv * 0.5 + 0.5;

    // Clamp to texture bounds
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 texel = texture2D(texture0, uv);

    // Scanlines
    float scanline = sin(uv.y * resolution.y * 3.14159) * 0.5 + 0.5;
    texel.rgb *= 1.0 - scanlineIntensity * (1.0 - scanline);

    gl_FragColor = texel * colDiffuse * fragColor;
}
```

```ruby
@crt = Graphics::Shader.load(fragment: "shaders/crt.fs")

def draw
  @crt.set(:resolution, Window.width.to_f, Window.height.to_f)
  @crt.set(:curvature, 6.0)
  @crt.set(:scanlineIntensity, 0.3)

  @crt.use do
    @camera.use do
      @level.draw
      @player.draw
    end
  end
end
```

### Pixelate

Chunky retro pixels:

```glsl
#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float pixelSize;   // 2.0 to 16.0
uniform vec2 resolution;

void main() {
    vec2 pixelCount = resolution / pixelSize;
    vec2 uv = floor(fragTexCoord * pixelCount) / pixelCount;

    vec4 texel = texture2D(texture0, uv);
    gl_FragColor = texel * colDiffuse * fragColor;
}
```

```ruby
@pixelate = Graphics::Shader.load(fragment: "shaders/pixelate.fs")
@pixelate.set(:pixelSize, 4.0)
@pixelate.set(:resolution, Window.width.to_f, Window.height.to_f)
```

---

## Shader Cycling

Cycle through multiple shaders at runtime:

```ruby
def init
  @shaders = [
    nil,  # No shader
    Graphics::Shader.load(fragment: "shaders/grayscale.fs"),
    Graphics::Shader.load(fragment: "shaders/crt.fs"),
    Graphics::Shader.load(fragment: "shaders/wave.fs")
  ]
  @shader_names = ["none", "grayscale", "crt", "wave"]
  @shader_index = 0

  Input.map(:next_shader, [:e])
  Input.map(:prev_shader, [:q])
  Input.on(:next_shader) { @shader_index = (@shader_index + 1) % @shaders.length }
  Input.on(:prev_shader) { @shader_index = (@shader_index - 1) % @shaders.length }
end

def draw
  current = @shaders[@shader_index]

  if current
    set_shader_uniforms(current, @shader_names[@shader_index])
    current.use do
      draw_game
    end
  else
    draw_game
  end

  Graphics.draw_text("Shader: #{@shader_names[@shader_index]} [Q/E]", 5, 5, 16, :white)
end
```

---

## Resource Management

Shaders are reference-counted and cached by path:

```ruby
# Same shader file = same handle (cached)
shader1 = Graphics::Shader.load(fragment: "shaders/blur.fs")
shader2 = Graphics::Shader.load(fragment: "shaders/blur.fs")  # Returns same shader

# Check if shader is valid
if @shader.valid?
  @shader.use { @sprite.draw }
end

# Manually release (optional - GC handles this)
@shader.release
```

---

## Complete Example

```ruby
include GMR

VIEW_HEIGHT = 9

def init
  Window.set_size(960, 540)
  Window.set_title("Shader Demo")

  # Setup camera
  @camera = Graphics::Camera.new(
    viewport_size: Mathf::Vec2.new(Window.width, Window.height),
    view_height: VIEW_HEIGHT
  )
  @camera.offset = Mathf::Vec2.new(Window.width / 2.0, Window.height / 2.0)

  # Load player
  @texture = Texture.load("player.png")
  @transform = Transform2D.new(x: 5.0, y: 5.0)
  @sprite = Sprite.new(@texture, @transform)
  @sprite.center_origin

  # Load shaders
  @wave_shader = Graphics::Shader.load(fragment: "shaders/wave.fs")
  @use_shader = true

  Input.map(:toggle_shader, [:space])
  Input.on(:toggle_shader) { @use_shader = !@use_shader }
end

def update(dt)
  speed = 5.0 * dt
  @transform.x -= speed if Input.key_down?(:left)
  @transform.x += speed if Input.key_down?(:right)
  @transform.y -= speed if Input.key_down?(:up)
  @transform.y += speed if Input.key_down?(:down)
end

def draw
  Graphics.clear("#1a1a2e")

  if @use_shader
    @wave_shader.set(:time, GMR::Time.elapsed)
    @wave_shader.set(:amplitude, 0.015)
    @wave_shader.set(:frequency, 12.0)

    @wave_shader.use do
      @camera.use do
        @sprite.draw
      end
    end
  else
    @camera.use do
      @sprite.draw
    end
  end

  status = @use_shader ? "ON" : "OFF"
  Graphics.draw_text("Shader: #{status} [SPACE]", 10, 10, 16, :white)
  Graphics.draw_text("Arrow keys to move", 10, 30, 16, :gray)
end
```

---

## API Summary

### Loading

| Method | Description |
|--------|-------------|
| `Graphics::Shader.load(fragment:, vertex:)` | Load shader from files |
| `Graphics::Shader.from_source(fragment:, vertex:)` | Load shader from GLSL strings |

### Uniforms

| Method | Description |
|--------|-------------|
| `shader.set(name, value)` | Set float uniform |
| `shader.set(name, x, y)` | Set vec2 uniform |
| `shader.set(name, x, y, z)` | Set vec3 uniform |
| `shader.set(name, x, y, z, w)` | Set vec4 uniform |
| `shader.set(name, texture)` | Set sampler2D uniform |

### Usage

| Method | Description |
|--------|-------------|
| `shader.use { }` | Apply shader within block |
| `shader.begin` / `shader.end` | Manual shader control |

### Resource Management

| Method | Description |
|--------|-------------|
| `shader.valid?` | Check if shader is loaded |
| `shader.release` | Manually release shader |

---

## See Also

- [Graphics](graphics.md) - Drawing and rendering
- [Camera](camera.md) - World-space rendering
- [Animation](animation.md) - Sprite animation
