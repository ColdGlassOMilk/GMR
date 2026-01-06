[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Utilities](../utilities/README.md) > **Collision**

# GMR::Collision

Collision detection between shapes.

## Table of Contents

- [Functions](#functions)
  - [circle_overlap?](#circle_overlap)
  - [circle_rect_overlap?](#circle_rect_overlap)
  - [distance](#distance)
  - [distance_squared](#distance_squared)
  - [point_in_circle?](#point_in_circle)
  - [point_in_rect?](#point_in_rect)
  - [rect_contains?](#rect_contains)
  - [rect_overlap?](#rect_overlap)
  - [rect_tiles](#rect_tiles)
  - [tile_rect](#tile_rect)

## Functions

<a id="point_in_rect"></a>

### point_in_rect?

Check if a point is inside a rectangle.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `px` | `Float` | Point X coordinate |
| `py` | `Float` | Point Y coordinate |
| `rx` | `Float` | Rectangle X position (top-left) |
| `ry` | `Float` | Rectangle Y position (top-left) |
| `rw` | `Float` | Rectangle width |
| `rh` | `Float` | Rectangle height |

**Returns:** `Boolean` - true if the point is inside the rectangle

**Example:**

```ruby
if GMR::Collision.point_in_rect?(mouse_x, mouse_y, btn.x, btn.y, btn.w, btn.h)
  button_hovered = true
end
```

---

<a id="point_in_circle"></a>

### point_in_circle?

Check if a point is inside a circle.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `px` | `Float` | Point X coordinate |
| `py` | `Float` | Point Y coordinate |
| `cx` | `Float` | Circle center X coordinate |
| `cy` | `Float` | Circle center Y coordinate |
| `radius` | `Float` | Circle radius |

**Returns:** `Boolean` - true if the point is inside the circle

**Example:**

```ruby
if GMR::Collision.point_in_circle?(x, y, orb.x, orb.y, orb.radius)
  orb.collect
end
```

---

<a id="rect_overlap"></a>

### rect_overlap?

Check if two rectangles overlap (AABB collision).

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Float` | First rectangle X position |
| `y1` | `Float` | First rectangle Y position |
| `w1` | `Float` | First rectangle width |
| `h1` | `Float` | First rectangle height |
| `x2` | `Float` | Second rectangle X position |
| `y2` | `Float` | Second rectangle Y position |
| `w2` | `Float` | Second rectangle width |
| `h2` | `Float` | Second rectangle height |

**Returns:** `Boolean` - true if the rectangles overlap

**Example:**

```ruby
if GMR::Collision.rect_overlap?(player.x, player.y, 32, 48,
                                         platform.x, platform.y, 64, 16)
  player.on_ground = true
end
```

---

<a id="rect_contains"></a>

### rect_contains?

Check if the outer rectangle fully contains the inner rectangle.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `outer_x` | `Float` | Outer rectangle X position |
| `outer_y` | `Float` | Outer rectangle Y position |
| `outer_w` | `Float` | Outer rectangle width |
| `outer_h` | `Float` | Outer rectangle height |
| `inner_x` | `Float` | Inner rectangle X position |
| `inner_y` | `Float` | Inner rectangle Y position |
| `inner_w` | `Float` | Inner rectangle width |
| `inner_h` | `Float` | Inner rectangle height |

**Returns:** `Boolean` - true if the inner rectangle is fully inside the outer rectangle

**Example:**

```ruby
if GMR::Collision.rect_contains?(screen_x, screen_y, screen_w, screen_h,
                                          entity.x, entity.y, entity.w, entity.h)
  entity.draw  # Only draw if fully on screen
end
```

---

<a id="circle_overlap"></a>

### circle_overlap?

Check if two circles overlap.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Float` | First circle center X |
| `y1` | `Float` | First circle center Y |
| `r1` | `Float` | First circle radius |
| `x2` | `Float` | Second circle center X |
| `y2` | `Float` | Second circle center Y |
| `r2` | `Float` | Second circle radius |

**Returns:** `Boolean` - true if the circles overlap

**Example:**

```ruby
if GMR::Collision.circle_overlap?(ball1.x, ball1.y, ball1.r,
                                           ball2.x, ball2.y, ball2.r)
  bounce_balls(ball1, ball2)
end
```

---

<a id="circle_rect_overlap"></a>

### circle_rect_overlap?

Check if a circle overlaps with a rectangle.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `cx` | `Float` | Circle center X |
| `cy` | `Float` | Circle center Y |
| `cr` | `Float` | Circle radius |
| `rx` | `Float` | Rectangle X position |
| `ry` | `Float` | Rectangle Y position |
| `rw` | `Float` | Rectangle width |
| `rh` | `Float` | Rectangle height |

**Returns:** `Boolean` - true if the circle and rectangle overlap

**Example:**

```ruby
if GMR::Collision.circle_rect_overlap?(ball.x, ball.y, ball.r,
                                                 wall.x, wall.y, wall.w, wall.h)
  ball.bounce
end
```

---

<a id="rect_tiles"></a>

### rect_tiles

Get all tile coordinates that a rectangle overlaps. Useful for tile-based collision detection.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x` | `Float` | Rectangle X position |
| `y` | `Float` | Rectangle Y position |
| `w` | `Float` | Rectangle width |
| `h` | `Float` | Rectangle height |
| `tile_size` | `Integer` | Size of each tile in pixels |

**Returns:** `Array<Array<Integer>>` - Array of [tx, ty] tile coordinate pairs

**Example:**

```ruby
tiles = GMR::Collision.rect_tiles(player.x, player.y, 32, 48, 16)
  tiles.each do |tx, ty|
    if tilemap.solid?(tx, ty)
      # Handle collision with this tile
    end
  end
```

---

<a id="tile_rect"></a>

### tile_rect

Convert tile coordinates to a world-space rectangle.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `tx` | `Integer` | Tile X coordinate |
| `ty` | `Integer` | Tile Y coordinate |
| `tile_size` | `Integer` | Size of each tile in pixels |

**Returns:** `Array<Integer>` - Rectangle as [x, y, width, height]

**Example:**

```ruby
x, y, w, h = GMR::Collision.tile_rect(5, 3, 16)
  # Returns [80, 48, 16, 16]
```

---

<a id="distance"></a>

### distance

Calculate the Euclidean distance between two points.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Float` | First point X |
| `y1` | `Float` | First point Y |
| `x2` | `Float` | Second point X |
| `y2` | `Float` | Second point Y |

**Returns:** `Float` - Distance between the points

**Example:**

```ruby
dist = GMR::Collision.distance(player.x, player.y, enemy.x, enemy.y)
  if dist < attack_range
    attack_enemy(enemy)
  end
```

---

<a id="distance_squared"></a>

### distance_squared

Calculate the squared distance between two points. Faster than distance() since it avoids the square root. Use for comparisons.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `x1` | `Float` | First point X |
| `y1` | `Float` | First point Y |
| `x2` | `Float` | Second point X |
| `y2` | `Float` | Second point Y |

**Returns:** `Float` - Squared distance between the points

**Example:**

```ruby
# More efficient for distance comparisons
  dist_sq = GMR::Collision.distance_squared(a.x, a.y, b.x, b.y)
  if dist_sq < range * range
    in_range = true
  end
```

---

---

[Back to Utilities](README.md) | [Documentation Home](../../../README.md)
