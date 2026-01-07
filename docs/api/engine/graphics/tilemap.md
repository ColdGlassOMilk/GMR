[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Graphics](../graphics/README.md) > **Tilemap**

# Tilemap

Tilemap rendering from Tiled JSON exports.

## Table of Contents

- [Class Methods](#class-methods)
  - [.new](#new)
- [Instance Methods](#instance-methods)
  - [#damage](#damage)
  - [#define_tile](#define_tile)
  - [#draw_region](#draw_region)
  - [#fill](#fill)
  - [#fill_rect](#fill_rect)
  - [#get](#get)
  - [#hazard?](#hazard)
  - [#ladder?](#ladder)
  - [#platform?](#platform)
  - [#set](#set)
  - [#slippery?](#slippery)
  - [#solid?](#solid)
  - [#tile_height](#tile_height)
  - [#tile_properties](#tile_properties)
  - [#tile_property](#tile_property)
  - [#tile_width](#tile_width)
  - [#wall?](#wall)
  - [#water?](#water)

## Class Methods

<a id="new"></a>

### new(tileset:, tile_width:, tile_height:, width:, height:)

Create a new tilemap with the specified dimensions. All tiles are initialized to -1 (empty/transparent).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `tileset` | `Texture` | The tileset texture containing all tile graphics |
| `tile_width` | `Integer` | Width of each tile in pixels |
| `tile_height` | `Integer` | Height of each tile in pixels |
| `width` | `Integer` | Map width in tiles |
| `height` | `Integer` | Map height in tiles |

**Returns:** `Tilemap` - The new tilemap object

**Raises:**
- ArgumentError if dimensions are not positive

**Example:**

```ruby
tileset = GMR::Graphics::Texture.load("assets/tiles.png")
map = GMR::Graphics::Tilemap.new(tileset, 16, 16, 100, 50)
```

---

## Instance Methods

<a id="tile_width"></a>

### tile_width

TODO: Add documentation

**Returns:** `unknown`

---

<a id="tile_height"></a>

### tile_height

TODO: Add documentation

**Returns:** `unknown`

---

<a id="get"></a>

### get(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="set"></a>

### set(arg1, arg2, arg3)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |
| `arg3` | `Integer` |  |

**Returns:** `unknown`

---

<a id="fill"></a>

### fill(arg1)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |

**Returns:** `unknown`

---

<a id="fill_rect"></a>

### fill_rect(arg1, arg2, arg3, arg4, arg5)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |
| `arg3` | `Integer` |  |
| `arg4` | `Integer` |  |
| `arg5` | `Integer` |  |

**Returns:** `unknown`

---

<a id="draw_region"></a>

### draw_region(arg1, arg2, arg3, arg4, arg5, arg6, [arg7])

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Float` |  |
| `arg2` | `Float` |  |
| `arg3` | `Integer` |  |
| `arg4` | `Integer` |  |
| `arg5` | `Integer` |  |
| `arg6` | `Integer` |  |
| `arg7` | `Array (optional)` |  |

**Returns:** `unknown`

---

<a id="define_tile"></a>

### define_tile(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Hash` |  |

**Returns:** `unknown`

---

<a id="tile_properties"></a>

### tile_properties(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="tile_property"></a>

### tile_property(arg1, arg2, arg3)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |
| `arg3` | `Symbol` |  |

**Returns:** `unknown`

---

<a id="solid"></a>

### solid?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="wall"></a>

### wall?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `any` |  |
| `arg2` | `any` |  |

**Returns:** `unknown`

---

<a id="hazard"></a>

### hazard?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="platform"></a>

### platform?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="ladder"></a>

### ladder?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="water"></a>

### water?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="slippery"></a>

### slippery?(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

<a id="damage"></a>

### damage(arg1, arg2)

TODO: Add documentation

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `arg1` | `Integer` |  |
| `arg2` | `Integer` |  |

**Returns:** `unknown`

---

---

[Back to Graphics](README.md) | [Documentation Home](../../../README.md)
