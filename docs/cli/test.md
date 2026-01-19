# gmrcli test

Run unit tests

## Usage

```bash
gmrcli test [options]
```

## Description

Build and run the GMR engine unit tests.

      Targets:
        all       - Run all tests (default)
        math      - Run math tests (Matrix2D, Vec2)
        node      - Run node hierarchy tests
        transform - Run transform manager tests
        resources - Run resource manager tests
        draw_queue - Run draw queue tests
        particle  - Run particle lifecycle tests
        input     - Run input context tests
        animation - Run animation/easing tests

      Examples:
        gmrcli test              # Run all tests
        gmrcli test math         # Run only math tests
        gmrcli test --rebuild    # Clean rebuild before running
        gmrcli test --filter=easing  # Run tests with [easing] tag

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|
| `--rebuild` | -r | boolean | `false` | Clean and rebuild before testing |
| `--filter` | -f | string | - | Run tests matching this tag |

## Stages

1. Validating Environment
2. Configuring Test Build
3. Compiling Tests
4. Running Tests

## Examples

```bash
# Basic usage
gmrcli test

# With rebuild flag
gmrcli test --rebuild
```

---

*See also: [CLI Reference](README.md)*
