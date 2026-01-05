# Node

API reference for Node.

## Node

TODO: Add description

### Instance Methods

### initialize

Create a new Node with default transform.

**Returns:** `Node` - The new node

**Example:**
```ruby
node = Node.new
```

---

### add_child

Add a child node. Removes from previous parent if any. Cycle detection prevents a node from becoming its own ancestor.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| child | Node | The node to add as a child |

**Returns:** `Node` - self for chaining

**Example:**
```ruby
root.add_child(child)
```

---

### remove_child

Remove a child node. The child becomes a root node.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| child | Node | The node to remove |

**Returns:** `Node` - self for chaining

**Example:**
```ruby
root.remove_child(child)
```

---

### parent

Get the parent node, or nil if this is a root.

**Returns:** `Node, nil` - The parent node

**Example:**
```ruby
if node.parent then puts "has parent" end
```

---

### children

Get an array of child nodes.

**Returns:** `Array<Node>` - Array of children (empty if none)

**Example:**
```ruby
node.children.each { |c| puts c.local_position }
```

---

### child_count

Get the number of children.

**Returns:** `Integer` - Number of children

**Example:**
```ruby
puts node.child_count
```

---

### local_position

Get the local position relative to parent.

**Returns:** `Vec2` - The local position

**Example:**
```ruby
pos = node.local_position
```

---

### local_rotation

Get the local rotation in degrees.

**Returns:** `Float` - The local rotation

**Example:**
```ruby
angle = node.local_rotation
```

---

### local_scale

Get the local scale.

**Returns:** `Vec2` - The local scale

**Example:**
```ruby
scale = node.local_scale
```

---

### local_position=

Set the local position relative to parent.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Vec2 | The new local position |

**Returns:** `Vec2` - The value that was set

**Example:**
```ruby
node.local_position = Vec2.new(100, 50)
```

---

### local_rotation=

Set the local rotation in degrees.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Float | The new local rotation |

**Returns:** `Float` - The value that was set

**Example:**
```ruby
node.local_rotation = 45
```

---

### local_scale=

Set the local scale.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Vec2 | The new local scale |

**Returns:** `Vec2` - The value that was set

**Example:**
```ruby
node.local_scale = Vec2.new(2, 2)
```

---

### world_position

Get the world position after hierarchy composition. Call Node.update_world_transforms(root) first to ensure accuracy.

**Returns:** `Vec2` - The world position

**Example:**
```ruby
pos = child.world_position
```

---

### world_rotation

Get the world rotation in degrees after hierarchy composition.

**Returns:** `Float` - The world rotation

**Example:**
```ruby
angle = child.world_rotation
```

---

### world_scale

Get the world scale after hierarchy composition.

**Returns:** `Vec2` - The world scale

**Example:**
```ruby
scale = child.world_scale
```

---

### active

Get the active flag of this node (ignores parent state).

**Returns:** `Boolean` - true if this node is active

**Example:**
```ruby
if node.active then ... end
```

---

### active=

Set the active flag of this node.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| value | Boolean | true to activate, false to deactivate |

**Returns:** `Boolean` - The value that was set

**Example:**
```ruby
node.active = false
```

---

### active?

Check if this node is active in the full hierarchy. Returns false if this node or any ancestor is inactive.

**Returns:** `Boolean` - true if active in hierarchy

**Example:**
```ruby
if node.active? then update(node) end
```

---

### Node.update_world_transforms

Update world transforms for a node and all descendants. Call this on the root before reading world_position/rotation/scale.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| root | Node | The root node to start from |

**Returns:** `nil`

**Example:**
```ruby
Node.update_world_transforms(root)
```

---

