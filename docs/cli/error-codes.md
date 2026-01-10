# Error Codes

Machine-readable error codes returned by gmrcli.

## SETUP

| Code | Exit | Description |
|------|------|-------------|
| `SETUP.EMSCRIPTEN_FAILED` | 11 | Emscripten setup failed |
| `SETUP.MISSING_DEP` | 10 | Missing dependency |
| `SETUP.MISSING_DEP.CMAKE` | 10 | CMake not found |
| `SETUP.MISSING_DEP.GCC` | 10 | GCC not found |
| `SETUP.MISSING_DEP.GIT` | 10 | Git not found |
| `SETUP.MISSING_DEP.NINJA` | 10 | Ninja not found |
| `SETUP.MRUBY_BUILD_FAILED` | 12 | mruby build failed |
| `SETUP.PACKAGES_FAILED` | 14 | Package installation failed |
| `SETUP.RAYLIB_BUILD_FAILED` | 13 | raylib build failed |
| `SETUP.VERIFICATION_FAILED` | 15 | Setup verification failed |

## BUILD

| Code | Exit | Description |
|------|------|-------------|
| `BUILD.CMAKE_FAILED` | 20 | CMake configuration failed |
| `BUILD.COMPILE_FAILED` | 21 | Compilation failed |
| `BUILD.INVALID_TARGET` | 23 | Invalid build target |
| `BUILD.LINK_FAILED` | 24 | Linking failed |
| `BUILD.MISSING_DEP` | 22 | Missing build dependency |

## RUN

| Code | Exit | Description |
|------|------|-------------|
| `RUN.EXECUTABLE_NOT_FOUND` | 30 | Executable not found |
| `RUN.INVALID_TARGET` | 32 | Invalid run target |
| `RUN.LAUNCH_FAILED` | 33 | Failed to launch game |
| `RUN.PROJECT_NOT_FOUND` | 31 | Project not found |

## PROJECT

| Code | Exit | Description |
|------|------|-------------|
| `PROJECT.ALREADY_EXISTS` | 42 | Project already exists |
| `PROJECT.INVALID` | 41 | Invalid project structure |
| `PROJECT.MISSING_FILE` | 43 | Required file missing |
| `PROJECT.NOT_FOUND` | 40 | Not a GMR project |

## PLATFORM

| Code | Exit | Description |
|------|------|-------------|
| `PLATFORM.MISSING_TOOL` | 52 | Required tool missing |
| `PLATFORM.UNSUPPORTED` | 50 | Unsupported platform |
| `PLATFORM.WRONG_ENV` | 51 | Wrong environment |

## VERSION

| Code | Exit | Description |
|------|------|-------------|
| `VERSION.DIRTY_TREE` | 63 | Uncommitted changes |
| `VERSION.GIT_NOT_FOUND` | 62 | Git not available |
| `VERSION.INVALID_FORMAT` | 65 | Invalid version format |
| `VERSION.INVALID_PART` | 60 | Invalid version part |
| `VERSION.NOT_GIT_REPO` | 61 | Not a git repository |
| `VERSION.NO_ENGINE_JSON` | 64 | engine.json not found |
| `VERSION.TAG_EXISTS` | 66 | Git tag already exists |

## PROTOCOL

| Code | Exit | Description |
|------|------|-------------|
| `PROTOCOL.UNSUPPORTED` | 2 | Unsupported protocol version |

## INTERNAL

| Code | Exit | Description |
|------|------|-------------|
| `INTERNAL.COMMAND_FAILED` | 1 | Command execution failed |
| `INTERNAL.UNEXPECTED` | 1 | Unexpected internal error |

---

*See also: [CLI Reference](README.md)*
