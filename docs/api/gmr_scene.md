# GMR::Scene

API reference for GMR::Scene.

## Scene

TODO: Add description

### Instance Methods

### init

Called once when the scene becomes active. Override to initialize scene state.

---

### update

Called every frame while this scene is on top of the stack. Override to update game logic.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| dt | Float | Delta time in seconds since last frame |

---

### draw

Called every frame while this scene is on top of the stack. Override to draw scene content.

---

### unload

Stack-based scene lifecycle manager. Use load to switch scenes (clears stack), push to add a scene on top, and pop to return to the previous scene.

---

