# Changelog

All notable changes to GMR will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
