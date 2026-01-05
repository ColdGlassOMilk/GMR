# GMR::SceneManager

API reference for GMR::SceneManager.

## SceneManager

TODO: Add description

### Instance Methods

### load

Clear the scene stack and load a new scene. Calls unload on all existing scenes (top to bottom), then init on the new scene.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scene | GMR::Scene | The scene to load |

**Example:**
```ruby
GMR::SceneManager.load(TitleScene.new)
```

---

### register

Alias for load. Clear the scene stack and load a new scene.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scene | GMR::Scene | The scene to load |

**Example:**
```ruby
GMR::SceneManager.register(TitleScene.new)
```

---

### push

Push a new scene onto the stack. The current scene is paused (no longer receives update/draw). Calls init on the new scene.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scene | GMR::Scene | The scene to push |

**Example:**
```ruby
GMR::SceneManager.push(PauseScene.new)
```

---

### pop

Remove the top scene from the stack. Calls unload on the removed scene. The previous scene resumes receiving update/draw calls.

**Example:**
```ruby
GMR::SceneManager.pop
```

---

### update

Call update on the top scene. Call this from your game's update function.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| dt | Float | Delta time in seconds |

**Example:**
```ruby
def update(dt)
  GMR::SceneManager.update(dt)
end
```

---

### draw

Call draw on the top scene. Call this from your game's draw function.

**Example:**
```ruby
def draw
  GMR::SceneManager.draw
end
```

---

### add_overlay

Add an overlay scene that renders on top of the main scene. Overlays receive update and draw calls. Multiple overlays can be active.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scene | GMR::Scene | The overlay scene to add |

**Example:**
```ruby
GMR::SceneManager.add_overlay(MinimapOverlay.new)
```

---

### remove_overlay

Remove an overlay scene.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scene | GMR::Scene | The overlay scene to remove |

**Example:**
```ruby
GMR::SceneManager.remove_overlay(@minimap)
```

---

### has_overlay?

Check if an overlay is currently active.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| scene | GMR::Scene | The overlay scene to check |

**Returns:** `Boolean` - true if the overlay is active

**Example:**
```ruby
if GMR::SceneManager.has_overlay?(@minimap)
```

---

