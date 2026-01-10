# Changelog

All notable changes to GMR will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0] - Initial Release

### Added

#### Engine
- JSON serialization support
- World space coordinate transforms

#### Build System
- Semantic versioning infrastructure with `engine.json` as single source of truth
- Dependency version pinning for raylib, mruby, and emscripten
- CMake integration reading version from `engine.json`
- Automatic git tagging on version bump (via gmrcli bump <major|minor|patch>)
