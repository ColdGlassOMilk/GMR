# Output Formats

GMR CLI supports two output modes for different use cases.

## JSON Mode (Default)

Machine-readable output for IDE integration and automation.

```bash
gmrcli build debug     # JSON output
```

### Success Envelope

```json
{
  "protocol": "v1",
  "status": "success",
  "command": "build",
  "result": {
    "target": "debug",
    "output_path": "/path/to/release/gmr.exe",
    "artifacts": [
      {"type": "executable", "path": "/path/to/release/gmr.exe"}
    ]
  },
  "metadata": {
    "timestamp": "2024-01-15T10:30:45.123Z",
    "duration_ms": 1234,
    "gmrcli_version": "0.1.0"
  }
}
```

### Error Envelope

```json
{
  "protocol": "v1",
  "status": "error",
  "command": "build",
  "error": {
    "code": "BUILD.CMAKE_FAILED",
    "message": "CMake configuration failed",
    "suggestions": ["Run 'gmrcli setup' if you haven't already"]
  },
  "metadata": {
    "timestamp": "2024-01-15T10:30:45.123Z",
    "gmrcli_version": "0.1.0"
  }
}
```

---

## Text Mode

Human-readable output with colors and progress indicators.

```bash
gmrcli build debug -o text
```

### Enabling Text Mode

```bash
# Flag
gmrcli build -o text

# Environment variable
GMRCLI_OUTPUT=text gmrcli build
```

---

## Protocol Versioning

For stable integrations, lock to a specific protocol version:

```bash
gmrcli build debug --protocol-version v1
```

This ensures consistent JSON structure even across CLI updates.

---

## Exit Codes

| Code | Meaning |
|------|---------|
| `0` | Success |
| `1` | Generic/internal error |
| `2` | Protocol error |
| `10-19` | Setup errors |
| `20-29` | Build errors |
| `30-39` | Run errors |
| `40-49` | Project errors |
| `50-59` | Platform errors |
| `130` | User interrupt (Ctrl+C) |

### Exit Code Categories

**10-19: Setup Errors**
- 10: Dependency installation failed
- 11: Network error during download
- 12: Disk space insufficient

**20-29: Build Errors**
- 20: CMake configuration failed
- 21: Compilation failed
- 22: Linking failed
- 23: Script compilation failed

**30-39: Run Errors**
- 30: Executable not found
- 31: Runtime error
- 32: Asset loading failed

**40-49: Project Errors**
- 40: Project creation failed
- 41: Invalid project name
- 42: Project already exists

---

## NDJSON Events

During long operations, the CLI emits newline-delimited JSON events:

```json
{"event": "progress", "stage": "compile", "percent": 45}
{"event": "progress", "stage": "compile", "percent": 78}
{"event": "progress", "stage": "link", "percent": 100}
```

Final result is always a complete JSON envelope.
