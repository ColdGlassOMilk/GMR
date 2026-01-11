# Changelog

All notable changes to GMR will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

### Fixed
- **Window Resize Crash**: Fixed crash when resizing window with minimal scripts that don't define `on_resize`
  - Now checks if method exists with `mrb_respond_to` before calling
- **State Synchronization**: Window state now properly synchronized after `InitWindow()`
  - Ensures correct resize handling even when script doesn't call `Window.set_size()`

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
