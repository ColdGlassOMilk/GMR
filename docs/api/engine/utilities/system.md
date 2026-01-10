[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Utilities](../utilities/README.md) > **System**

# GMR::System

System utilities and debugging.

## Table of Contents

- [Functions](#functions)
  - [build_type](#build_type)
  - [compiled_scripts?](#compiled_scripts)
  - [gl_version](#gl_version)
  - [glsl_version](#glsl_version)
  - [gpu_renderer](#gpu_renderer)
  - [gpu_vendor](#gpu_vendor)
  - [in_error_state?](#in_error_state)
  - [last_error](#last_error)
  - [platform](#platform)
  - [quit](#quit)
  - [raylib_version](#raylib_version)

## Functions

<a id="quit"></a>

### quit

Exit the application cleanly. On native platforms, triggers proper shutdown through the main loop. On web, this is a no-op (user must close the browser tab).

**Returns:** `nil`

**Example:**

```ruby
GMR::System.quit  # Exit the game
```

---

<a id="platform"></a>

### platform

Get the current platform identifier.

**Returns:** `String` - Platform name: "windows", "macos", "linux", "web", or "unknown"

**Example:**

```ruby
if GMR::System.platform == "web"
  # Disable desktop-only features
end
```

---

<a id="build_type"></a>

### build_type

Get the build configuration type.

**Returns:** `String` - Build type: "debug", "release", or "unknown"

**Example:**

```ruby
if GMR::System.build_type == "debug"
  enable_debug_overlay
end
```

---

<a id="compiled_scripts"></a>

### compiled_scripts?

Check if scripts were precompiled into the binary.

**Returns:** `Boolean` - true if scripts are compiled in, false if loading from files

**Example:**

```ruby
puts "Scripts compiled: #{GMR::System.compiled_scripts?}"
```

---

<a id="raylib_version"></a>

### raylib_version

Get the version of the underlying Raylib graphics library.

**Returns:** `String` - Raylib version string (e.g., "5.0")

**Example:**

```ruby
puts "Raylib: #{GMR::System.raylib_version}"
```

---

<a id="gpu_vendor"></a>

### gpu_vendor

Get the GPU vendor name from OpenGL.

**Returns:** `String` - GPU vendor name (e.g., "NVIDIA Corporation") or "unknown"

**Example:**

```ruby
puts "GPU Vendor: #{GMR::System.gpu_vendor}"
```

---

<a id="gpu_renderer"></a>

### gpu_renderer

Get the GPU renderer name from OpenGL.

**Returns:** `String` - GPU renderer name (e.g., "GeForce RTX 3080") or "WebGL"

**Example:**

```ruby
puts "GPU: #{GMR::System.gpu_renderer}"
```

---

<a id="gl_version"></a>

### gl_version

Get the OpenGL version string.

**Returns:** `String` - OpenGL version (e.g., "4.6.0") or "WebGL 2.0"

**Example:**

```ruby
puts "OpenGL: #{GMR::System.gl_version}"
```

---

<a id="glsl_version"></a>

### glsl_version

Get the GLSL (shader language) version string.

**Returns:** `String` - GLSL version (e.g., "4.60") or "GLSL ES 3.00"

**Example:**

```ruby
puts "GLSL: #{GMR::System.glsl_version}"
```

---

<a id="last_error"></a>

### last_error

Get details about the last script error. Returns nil if no error occurred.

**Returns:** `Hash, nil` - Error hash with keys :class, :message, :file, :line, :backtrace, or nil

**Example:**

```ruby
error = GMR::System.last_error
  if error
    puts "#{error[:class]}: #{error[:message]}"
    puts "  at #{error[:file]}:#{error[:line]}"
    error[:backtrace].each { |line| puts "    #{line}" }
  end
```

---

<a id="in_error_state"></a>

### in_error_state?

Check if the scripting engine is currently in an error state.

**Returns:** `Boolean` - true if an unhandled error has occurred

**Example:**

```ruby
if GMR::System.in_error_state?
  show_error_screen
end
```

---

---

[Back to Utilities](README.md) | [Documentation Home](../../../README.md)
