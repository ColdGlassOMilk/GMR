# Changelog

All notable changes to GMR will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.3]

### Added

#### UI System
- **Declarative UI DSL**: Block-based syntax for building menus and interfaces
  - API: `Graphics.ui { }` - Create UI tree that renders on top of game content
  - Panels: `panel layout: :vertical, padding: 16, anchor: :center { }` - Container with auto-layout
  - Labels: `label "Text", font_size: 24, text_color: [255, 255, 255]` - Static text display
  - Buttons: `button "Click Me" { on_click { } }` - Interactive elements with callbacks
- **Automatic Layout**: Panels automatically arrange children
  - `:vertical` - Stack children top to bottom
  - `:horizontal` - Stack children left to right
  - `:none` - Manual positioning (default)
- **Anchoring System**: Position elements relative to parent
  - 9 anchor points: `:top_left`, `:top_center`, `:top_right`, `:center_left`, `:center`, `:center_right`, `:bottom_left`, `:bottom_center`, `:bottom_right`
- **Style Registration**: Define reusable style presets
  - API: `UI::Styles.register(:name, { width: 100, height: 40, ... })`
  - Usage: `button "Text", style: :name`
  - Inline options override style defaults
- **Button Events**: Interactive callbacks for user input
  - `on_click { }` - Fires when button is clicked
  - `on_hover_enter { }` - Fires when mouse enters button
  - `on_hover_exit { }` - Fires when mouse leaves button
- **Visual States**: Buttons support per-state styling
  - `background_color` - Normal state
  - `hover_background` - Mouse over
  - `pressed_background` - Mouse down
- **Resolution Independence**: All dimensions in 360p baseline coordinates
  - Automatically scales to any resolution
  - Same scaling system as `Graphics.draw_text`
- **Custom Font Support**: Use loaded fonts in UI elements
  - `label "Text", font: @custom_font`
  - `button "Click", font: @custom_font`
- **Cleanup**: `UI.clear` removes the UI tree (call in `unload` or before scene transitions)

#### Documentation
- `docs/ui.md` - Declarative UI system documentation

## [0.3.2]

### Added

#### Particle System
- **Data-Driven Particles**: JSON-configured visual effects with physics simulation
  - API: `ParticleEmitter.emit(path, position: vec2)` - Fire-and-forget effects
  - API: `ParticleEmitter.emit(path, transform: t) { on_complete }` - With callback
  - API: `ParticleEmitter.new(path)` - Create managed emitter
  - API: `ParticleEmitter.new(path, transform)` - Create attached emitter
  - API: `ParticleEmitter.new(hash)` - Create from Ruby Hash (declarative)
  - API: `ParticleEmitter.preload(path)` - Preload configs to avoid hitches
  - Control: `start`, `stop`, `pause`, `resume`, `reset`, `burst(count)`
  - Properties: `position=`, `transform=`, `emitting?`, `alive?`, `particle_count`
  - Draw control: `draw` - Control rendering order relative to other objects
  - Callback: `on_complete { }` - Called when one-shot effect finishes
- **Declarative Ruby Initialization**: Define effects directly in code without JSON files
  ```ruby
  ParticleEmitter.new({
    texture: "particles.jpg",
    columns: 6, rows: 6,
    spawn_rate: 10,
    lifetime: { min: 0.5, max: 1.0 },
    start_size: { min: 0.3, max: 0.5 },
    velocity_mode: "radial",
    speed: { min: 2.0, max: 4.0 }
  })
  ```
- **Textured Particles**: Support for spritesheet-based particle textures
  - Config: `texture`, `spritesheet_cols`, `spritesheet_rows`
  - Each particle randomly selects a frame from the spritesheet on spawn
  - Ideal for varied smoke puffs, dust clouds, debris
- **Effect Configuration**: JSON files define particle behavior
  - Emission: `spawn_rate`, `burst_count`, `burst_interval`, `max_particles`
  - Spawn shapes: point, circle, circle_edge, rectangle, rectangle_edge, line
  - Velocity modes: directional, radial, tangential, random
  - Physics: gravity, drag, acceleration
  - Interpolation: size/color easing with 30+ functions
  - Rendering: layer, z-order, world_space, time scaling
- **Memory Efficient**: Preallocated particle pools, zero per-frame allocations
- **Camera Integration**: Automatic world-to-screen coordinate conversion

#### Core Utilities
- **Timer System**: One-shot and repeating timers with automatic update
  - API: `Timer.after(delay) { callback }` - One-shot timer
  - API: `Timer.every(interval) { callback }` - Repeating timer
  - API: `Timer.cancel(:name)` - Cancel by name
  - Options: `name:` for named timers, `scaled:` to ignore `Time.scale`, `delay:` for initial delay
  - Instance methods: `pause`, `resume`, `cancel`, `active?`, `cancelled?`, `elapsed`, `remaining`

- **Random Number Generation**: Deterministic, seedable RNG with multiple streams
  - API: `Random.int(min, max)` - Integer in range (inclusive)
  - API: `Random.float(min, max)` - Float in range
  - API: `Random.bool` - Random boolean
  - API: `Random.chance(probability)` - True with given probability
  - API: `Random.choose(array)` - Random element from array
  - API: `Random.shuffle(array)` - Fisher-Yates shuffle in place
  - API: `Random.weighted(hash)` - Weighted random selection
  - API: `Random.seed(value)` - Seed for determinism
  - API: `Random.stream(:name)` - Named isolated RNG streams

- **Sequence System**: Multi-step behavior sequences for boss patterns, cutscenes
  - API: `Sequence.run { |s| s.call { }; s.wait(1.0); s.wait_until { } }` - Define sequence
  - API: `Sequence.run(:name) { }` - Named sequence for cancellation
  - API: `Sequence.cancel(:name)` - Cancel by name
  - Builder methods: `call`, `wait`, `wait_until`
  - Instance methods: `cancel`, `active?`, `completed?`, `then { }`

- **Signal/Observer System**: Decoupled object communication
  - Mixin: `include Signal` - Add signal capability to any class
  - API: `emit(:signal, *args)` - Emit a signal with arguments
  - API: `on(:signal) { |args| }` - Connect handler (returns connection ID)
  - API: `once(:signal) { }` - One-shot handler (auto-disconnects)
  - API: `off(:signal, id)` - Disconnect by ID
  - API: `off(:signal)` - Disconnect all handlers for signal
  - API: `has_signal?(:name)` - Check if handlers exist
  - API: `clear_signals` - Remove all handlers

- **Destroyable Pattern**: Standard object lifecycle with cleanup hooks
  - Mixin: `include Destroyable` - Add destroy capability
  - API: `destroy` - Mark for destruction (calls `on_destroy` hook)
  - API: `destroyed?` / `alive?` - Query state
  - `GameArray` class: Array subclass that auto-removes destroyed objects
    - `each_alive { }` - Iterate only alive objects
    - `compact_destroyed!` - Manual cleanup
    - `alive_count` - Count alive objects
    - `all_destroyed?` - Check if all destroyed

#### Spatial Queries
- **SpatialHash System**: Efficient entity lookup by position
  - API: `SpatialHash.add(entity, bounds: rect)` - Register entity
  - API: `SpatialHash.update(entity, bounds: rect)` - Update position
  - API: `SpatialHash.remove(entity)` - Remove entity
  - API: `SpatialHash.query_rect(x, y, w, h)` - Find entities in rectangle
  - API: `SpatialHash.query_circle(x, y, radius)` - Find entities in circle
  - API: `SpatialHash.query_point(x, y)` - Find entities at point
  - API: `SpatialHash.nearest(x, y, max_distance: n)` - Find nearest entity
  - API: `SpatialHash.cell_size=` / `SpatialHash.cell_size` - Configure grid size
  - API: `SpatialHash.clear` / `SpatialHash.count` - Management

#### Input
- **Gamepad Support**: First-class controller input with action binding
  - Action binding: `i.jump :space, gamepad: :a` - Bind gamepad buttons to actions
  - Multi-gamepad: `gamepad_index: 0` - Bind to specific gamepad
  - Raw API: `Gamepad.connected?(index)` / `Gamepad.count` / `Gamepad.name(index)`
  - Button state: `Gamepad.down?`, `pressed?`, `released?` - Per-gamepad or any
  - Analog sticks: `Gamepad.axis(index, :left_x)` - With dead zone applied
  - Raw axis: `Gamepad.axis_raw(index, :left_x)` - Without dead zone
  - Dead zones: `Gamepad.dead_zone=` / `Gamepad.outer_dead_zone=`
  - Vibration: `Gamepad.vibrate(index, left: 0.5, right: 0.3, duration: 0.2)`
  - Helpers: `Gamepad.any_pressed?(:button)` / `Gamepad.any_down?(:button)`

#### Debug Tools
- **Debug Draw Overlay**: Visualization utilities for development
  - API: `Debug.enabled?` / `Debug.enabled=` - Toggle debug drawing
  - API: `Debug.when_enabled { }` - Conditional debug block
  - API: `Debug.draw_rect`, `draw_rect_filled` - Rectangle outlines/fills
  - API: `Debug.draw_circle`, `draw_circle_filled` - Circle outlines/fills
  - API: `Debug.draw_line` - Line segments
  - API: `Debug.draw_arrow` - Directional arrows (for velocities)
  - API: `Debug.draw_point`, `draw_cross` - Position markers
  - API: `Debug.draw_text` - Text labels
  - All draws render on `DEBUG_OVERLAY` layer (z=250)

#### Scene Management
- **Scene Transitions**: Animated transitions between scenes with configurable effects
  - API: `SceneManager.load(scene, transition: :fade)` - Simple fade transition
  - API: `SceneManager.load(scene, transition: :fade, duration: 1.0, easing: :out_cubic)` - Full options
  - Supports `push` and `pop` operations with same transition options
  - Leverages existing SurfacePool for render target capture
  - Uses engine's 30+ easing functions for smooth animations
- **Scene `on_resize` Lifecycle Method**: Base `GMR::Scene` class now includes default `on_resize(width, height)` method
  - Scenes can override to handle window resize events
  - Eliminates need for `respond_to?` checks in main script

#### Documentation
- `docs/particles.md` - Particle system documentation with JSON config reference
- `docs/utilities.md` - Timer, Random, Sequence, Signal, Destroyable documentation
- `docs/spatial.md` - SpatialHash spatial query documentation
- `docs/debug.md` - Debug drawing overlay documentation
- Updated `docs/input.md` - Added gamepad section

### Changed

#### Code Organization
- **DrawQueue Split**: Split `draw_queue.cpp` (1,489 lines) into three focused files:
  - `draw_queue.cpp` - Command queuing, sorting, and flush logic
  - `draw_primitives.cpp` - Sprite, tilemap, and primitive rendering
  - `draw_surface_mode.cpp` - Surface mode and camera/shader application
- **DrawQueue Helper**: Extracted `create_command()` helper to centralize z-value calculation and draw_order management, removing ~100 lines of duplication
- **Global State Consolidation**: Moved `g_frame_delta` and `g_should_quit` globals into `State` singleton
  - Access via `State::instance().frame_delta` and `State::instance().should_quit`
  - Eliminates undocumented extern dependencies across bindings

#### CLI
- **Stageable Mixin**: Extracted shared `run_stage` pattern into `Gmrcli::Stageable` module
  - Reduces duplication between `build.rb` and `setup.rb`
  - Centralizes JSON event emission for stage tracking
- **Emscripten Helper**: Consolidated Emscripten environment configuration into `Gmrcli::Emscripten` module
  - Single implementation of `env()` method for web builds
  - Supports optional `include_lib_paths` parameter

## [0.3.1]

### Added

#### Rendering
- **Shader Surface Mode**: New `surface_mode` option for shaders that require unified UV space
  - Spatial/distortion shaders (wave, CRT, curvature) now operate on entire draw groups
  - Prevents per-tile and per-animation-frame UV discontinuities
  - API: `Shader.load(fragment: "wave.fs", surface_mode: true)`
  - API: `shader.surface_mode = true` / `shader.surface_mode?`
- **Surface Pool**: Render target pooling system for efficient surface mode rendering
  - Automatic reuse of render textures across frames
  - Power-of-two sizing for optimal GPU memory usage
  - Cleanup of unused surfaces after idle period

#### Documentation
- `docs/graphics.md` - Graphics system documentation

#### Web Platform
- **High-DPI Support**: Device pixel ratio detection and optional native-resolution rendering
  - API: `Window.use_native_dpr` - Enable crisp rendering on retina/high-DPI displays
  - API: `Window.use_css_dpr` - Render at CSS resolution (default, better performance)
  - API: `Window.device_pixel_ratio` - Query current device pixel ratio
  - API: `Window.native_dpr?` - Check if native DPR rendering is enabled

### Fixed
- **Window Resize Crash**: Fixed crash when resizing window with minimal scripts that don't define `on_resize`
  - Now checks if method exists with `mrb_respond_to` before calling
- **State Synchronization**: Window state now properly synchronized after `InitWindow()`
  - Ensures correct resize handling even when script doesn't call `Window.set_size()`
- **Web Canvas Resize Detection**: Centralized resize handling now properly detects all canvas size changes
  - Canvas resizes independent of window (e.g., opening dev tools in fullscreen) now detected via ResizeObserver
  - Browser zoom changes detected via matchMedia API
  - Unified handler eliminates race conditions between fragmented resize paths
  - Proper separation of CSS dimensions (for mouse input) from render dimensions (for WebGL)

### Changed
- Sample GLSL Shader files updated for GLSL ES compatibility

## [0.2.0] - Initial Release

### Added

#### Engine
- JSON serialization support
- World space coordinate transforms

#### Build System
- Semantic versioning infrastructure with `engine.json` as single source of truth
- Dependency version pinning for raylib, mruby, and emscripten
- Automatic git tagging on version bump (via gmrcli bump <major|minor|patch>)
