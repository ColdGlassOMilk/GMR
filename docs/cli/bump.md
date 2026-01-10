# gmrcli bump

Bump engine version (major|minor|patch)

## Usage

```bash
gmrcli bump [options]
```

## Description

Increment the engine version following semantic versioning:
        major         - Incompatible API changes (1.0.0 -> 2.0.0)
        minor         - New backwards-compatible features (1.0.0 -> 1.1.0)
        patch/revision - Backwards-compatible bug fixes (1.0.0 -> 1.0.1)

      This command will:
        1. Verify the working tree is clean
        2. Update engine.json with the new version
        3. Update engine/language/version.json
        4. Commit the changes
        5. Create an annotated git tag v<version>

      After bumping, push with: git push && git push --tags

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|

## Examples

```bash
# Basic usage
gmrcli bump
```

---

*See also: [CLI Reference](README.md)*
