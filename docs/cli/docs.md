# gmrcli docs

Generate all documentation

## Usage

```bash
gmrcli docs [options]
```

## Description

Regenerates all API and CLI documentation:
        - JSON files (engine/language/*.json)
        - Markdown files (docs/api/*.md, docs/cli/*.md)
        - HTML files (docs/html/api/*.html, docs/html/cli/*.html)

      This is the same documentation generated during 'gmrcli setup',
      but can be run independently when you've made changes to:
        - C++ binding source files (src/bindings/*.cpp)
        - CLI command files (cli/lib/gmrcli/commands/*.rb)

## Options

| Option | Alias | Type | Default | Description |
|--------|-------|------|---------|-------------|

## Examples

```bash
# Basic usage
gmrcli docs
```

---

*See also: [CLI Reference](README.md)*
