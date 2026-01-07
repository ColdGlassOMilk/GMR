# Contributing to GMR

Thank you for your interest in contributing to GMR! This document outlines our philosophy, design principles, and guidelines to help you make contributions that align with the project's goals.

## Quick Start

```bash
# Fork and clone
git clone https://github.com/YOUR_USERNAME/GMR
cd GMR

# Setup dependencies
./bootstrap.sh

# Run the engine
gmrcli dev

# Make your changes, then submit a PR
```

---

## Table of Contents

1. [GMR Engine Philosophy](#gmr-engine-philosophy)
2. [DSL Design Rules](#dsl-design-rules)
3. [Contributor Guardrails](#contributor-guardrails)
4. [Architectural Notes](#architectural-notes)
5. [Roadmap](#roadmap)

---

## GMR Engine Philosophy

### What GMR Is

GMR is a **Ruby-first game middleware** that prioritizes:

1. **Developer joy over feature count** - A beautiful API is more valuable than a complete one
2. **Iteration speed over runtime performance** - Hot reload in 50ms beats 0.1ms frame time optimization
3. **Clarity over cleverness** - Code that reads like English beats code that's "smart"
4. **Opinionated simplicity over flexible complexity** - One good way to do things, not ten mediocre ways

GMR is the "Rails of game engines" - conventions over configuration, batteries included, immediately productive.

### What GMR Is Not

- **Not a Unity/Godot competitor** - We don't need visual editors, node trees, or 100 inspector panels
- **Not an ECS engine** - Composition happens in Ruby, not component arrays
- **Not a 3D engine** - 2D is a feature, not a limitation
- **Not enterprise software** - No plugins, no DRM, no analytics, no storefronts
- **Not a framework for frameworks** - You build games with GMR, not engines on top of GMR

### Target Developer

**Primary**: Ruby enthusiasts who want to make games
- Comfortable with Ruby idioms (blocks, symbols, duck typing)
- Values expressiveness and readability
- Likely: indie developers, game jam participants, educators, hobbyists
- Experience level: Intermediate programmer, may be new to game dev

**Secondary**: Game developers tired of heavyweight engines
- Wants to prototype fast without fighting tools
- Knows game development but wants Ruby's ergonomics
- Values code-centric workflows over visual editors

### Core Values

1. **Immediate Feedback** - Save file, see change. No compile step, no restart.
2. **Beautiful Defaults** - The obvious code should work. Sprite loads, draws, animates.
3. **Progressive Disclosure** - Simple things are simple, complex things are possible.
4. **Zero Friction Setup** - One command installs dependencies. One command runs the game.
5. **Predictable Behavior** - Same code, same result. No hidden state surprises.

### Non-Goals (Explicitly Rejected)

| Feature | Why Rejected |
|---------|--------------|
| Visual scene editor | Code is the source of truth. Editors create sync problems. |
| Built-in networked multiplayer | Massive complexity. Defer until core is stable. |
| 3D rendering | Focus enables quality. 2D is our domain. |
| Plugin/addon marketplace | Encourages fragmentation. Core should be complete. |
| Multiple scripting languages | Ruby is the identity. No Lua, no C# bindings. |
| Entity-Component-System | Adds complexity without proportional benefit for 2D. |
| Mobile deployment | Platform constraints conflict with hot reload focus. |
| AI/pathfinding/physics built-in | Better as optional libraries, not core dependencies. |

---

## DSL Design Rules

### What Belongs in the DSL vs. Engine Internals

**IN THE DSL** (Ruby-facing API):
- Object creation (`Sprite.new`, `Tilemap.new`)
- Property access (`sprite.x`, `sprite.alpha`)
- Lifecycle hooks (`init`, `update`, `draw`)
- Declarative configuration (`state_machine do ... end`)
- Query methods (`Input.action_down?`, `collision.grounded?`)

**IN ENGINE INTERNALS** (Hidden from Ruby):
- Memory management (handles, managers, GC sync)
- Render pipeline (draw queue, z-sorting, camera transforms)
- File I/O (asset loading, hot reload detection)
- Platform abstraction (raylib calls, Emscripten specifics)
- mruby integration (arena management, safe calls)

**The Rule**: Ruby developers should never need to know about handles, managers, or C++ types.

### Lifecycle, Time, and State Expression

**Lifecycle** - Three functions, always:
```ruby
def init   # Once, on start or reload
def update(dt)  # Every frame, receives delta time
def draw   # Every frame, after update
```

No inheritance required. No base class. Just define the functions.

**Time** - Always explicit:
```ruby
def update(dt)
  @x += @speed * dt  # Delta time always passed, always used
end
```

Never assume frame rate. Never use `Time.delta` when `dt` is available.

**State** - Instance variables, managed by you:
```ruby
@velocity = 0.0      # You own it
@on_ground = false   # You update it
@sprite.x += dx      # You mutate it
```

No magic syncing. No automatic serialization. Explicit is better than implicit.

### When Explicit Code is Preferred Over "Magic"

**Prefer explicit when**:
- Order of operations matters (physics before collision)
- State transitions should be visible (explicit `trigger` calls)
- Errors should be debuggable (no silent fallbacks)
- Performance is relevant (no hidden allocations)

**Acceptable "magic"**:
- Automatic animator frame updates (opt-in via `Animator.new`)
- State machine callback invocation (declared explicitly, called automatically)
- Tween property animation (explicitly created, implicitly updated)

### When "Beautiful" Code Becomes Harmful

**Harmful Beauty Anti-Patterns**:

1. **DSL that hides critical logic**
   ```ruby
   # BAD: What does auto_collide do? When? To what?
   @player.auto_collide(@tilemap)

   # GOOD: Explicit, controllable
   result = Collision.resolve(@player.hitbox, @tilemap)
   @player.apply_collision(result)
   ```

2. **Implicit parent-child relationships**
   ```ruby
   # BAD: Sprite auto-positions relative to some implicit parent
   @sprite.attach_to(@player)
   @sprite.local_x = 10  # Where is this relative to?

   # GOOD: Explicit transform parenting
   @sprite.transform.parent = @player.transform
   @sprite.transform.local_position = Vec2.new(10, 0)
   ```

3. **Method chaining that obscures mutation** (but note: returning `self` for configuration is fine!)
   ```ruby
   # BAD: Does this return a new Tween or mutate? Unclear builder pattern
   Tween.to(@sprite, :x, 100).delay(0.5).ease(:bounce).loop(3)

   # GOOD: Clear that it's building a single tween
   tween = Tween.new(@sprite, :x, 100, duration: 1.0, delay: 0.5, ease: :bounce)
   tween.loop_count = 3

   # ALSO GOOD: Module configuration methods returning self for chaining
   # This is idiomatic Ruby and doesn't obscure mutation
   Window.set_size(1280, 720).set_virtual_resolution(320, 180).set_filter_point
   Input.map(:jump, :space).map(:move_left, :a).map(:move_right, :d)
   Console.enable(height: 150).allow_ruby_eval
   ```

4. **Over-abstracted error handling**
   ```ruby
   # BAD: Swallows errors, returns nil on failure
   texture = Texture.try_load("missing.png")

   # GOOD: Raises on failure, caller handles it
   texture = Texture.load("player.png")  # Raises LoadError if missing
   ```

### DSL Examples

**Good DSL Usage**:

```ruby
# Clear, explicit animation setup
@animator = Animation::Animator.new(@sprite,
  columns: 8,
  frame_width: 32,
  frame_height: 32)
@animator.add(:walk, frames: 0..7, fps: 12)
@animator.add(:jump, frames: 8..11, fps: 10, loop: false)
@animator.play(:idle)

# State machine with visible transitions
state_machine do
  state :idle do
    enter { @animator.play(:idle) }
    on :move, :walking
    on :jump, :jumping, if: -> { @on_ground }
  end

  state :walking do
    enter { @animator.play(:walk) }
    on :stop, :idle
  end
end

# Explicit collision handling
result = Collision.tilemap_resolve(@tilemap, x, y, w, h, vx, vy)
@sprite.x = result.x
@sprite.y = result.y
@on_ground = result.grounded?
```

**Bad DSL Usage** (anti-patterns to avoid):

```ruby
# BAD: Implicit behavior, unclear ownership
@player.enable_physics(@tilemap)  # What physics? When updated?
@player.enable_animation           # From what spritesheet?

# BAD: Magic string APIs
@player.set("health", 100)
@player.get("position")

# BAD: Over-engineered entity system
Entity.define(:player) do
  component :transform
  component :sprite, "player.png"
  component :physics, gravity: 800
  component :health, max: 100
end

# BAD: Hidden configuration
GMR.configure do |c|
  c.default_gravity = 800
  c.auto_collision = true
  c.animation_fps = 60
end
```

---

## Contributor Guardrails

### Forbidden Abstractions

**Never add**:
- Entity-Component-System or "component bags"
- Reflection/introspection systems
- Dependency injection containers
- Event bus / pub-sub systems (beyond current input callbacks)
- Object pooling "frameworks"
- Automatic serialization/deserialization
- Scene graph "nodes" with lifecycle management
- Plugin or addon architectures
- Configuration file parsers (YAML, TOML, etc.)

**Why**: These abstractions solve problems we don't have, add complexity we can't afford, and conflict with Ruby-first philosophy.

### Performance vs. Ergonomics Tradeoffs

**Default to ergonomics**. GMR is not for AAA games or 10,000-entity simulations.

**When performance matters**:
- Drawing hot path (`DrawQueue::flush`)
- Collision detection for tilemaps
- Animation system frame updates

**When ergonomics win**:
- API design (prefer clarity over micro-optimization)
- Binding layer (safety over raw speed)
- Error handling (fail loudly over fail fast)

**Decision Framework**:
1. Profile before optimizing
2. If <1% of frame time, optimize for readability
3. If >5% of frame time, consider C++ fast path
4. Never sacrifice Ruby API cleanliness for C++ performance

### Ruby Expressiveness in C++

When writing C++ bindings, think Ruby-first:

**Good Binding Design**:
```cpp
// Returns a proper Ruby object, not a hash
mrb_value mrb_collision_resolve(...) {
    auto result = /* ... */;
    mrb_value obj = mrb_obj_new(mrb, collision_result_class, 0, NULL);
    set_collision_data(obj, result);
    return obj;  // Ruby gets: result.x, result.grounded?
}
```

**Bad Binding Design**:
```cpp
// Leaks C++ implementation details
mrb_value mrb_collision_resolve(...) {
    auto result = /* ... */;
    mrb_value hash = mrb_hash_new(mrb);
    mrb_hash_set(mrb, hash, mrb_str_new_cstr(mrb, "x"), ...);  // String keys!
    return hash;  // Ruby gets: result[:x], result[:bottom]
}
```

### Contribution Checklist

Before submitting a PR:

- [ ] Does this add a new abstraction? If yes, is it absolutely necessary?
- [ ] Does the Ruby API read like English?
- [ ] Are there string-based APIs that should be symbol-based?
- [ ] Does failure raise an error or silently return nil?
- [ ] Will this require users to learn new concepts?
- [ ] Is there a simpler way to solve this problem?
- [ ] Have I added to DrawQueue, InputManager, or other central systems? If yes, is it justified?
- [ ] Do configuration/action methods return `self` for chaining? (See Method Chaining below)

### Code Style

**Ruby DSL APIs**:
- Use symbols for identifiers (`:idle`, `:running`)
- Use keyword arguments for optional parameters
- Use blocks for scoping (`camera.use { ... }`)
- Avoid returning `nil` on failure - raise or return default
- Return `self` from configuration/action methods to enable chaining (see below)

**Method Chaining (Fluent Interface)**:
Module configuration methods should return `self` to enable fluent chaining:
```cpp
// GOOD: Return self for configuration methods
static mrb_value mrb_console_enable(mrb_state* mrb, mrb_value self) {
    // ... implementation ...
    return self;  // Enables: Console.enable.allow_ruby_eval
}
```

When to return `self`:
- Configuration/setup methods (`enable`, `set_size`, `map`)
- Action methods that don't have a meaningful return value (`show`, `hide`, `clear`)
- Callback registration that doesn't need to return an ID

When NOT to return `self`:
- Query methods that return data (`width`, `height`, `enabled?`)
- Property setters (`x=`, `y=`) - these should return the assigned value per Ruby convention
- Methods that return a useful value (`on` returns callback ID for `off`)

**C++ Bindings**:
- Use typed handles, never expose pointers
- Validate arguments at binding entry
- Use `scripting::safe_call` for Ruby callbacks
- Document ownership in comments (who owns the handle lifecycle?)

**Naming**:
- Ruby: snake_case for methods, CamelCase for classes
- C++: snake_case for functions, CamelCase for classes
- Symbols: past tense for events (`:jumped`), present for states (`:jumping`)

---

## Architectural Notes

### What's Working Well

| System | Responsibility | Status |
|--------|----------------|--------|
| **DrawQueue** | Deferred z-sorted rendering | Solid. Hybrid z-ordering works well for 2D. |
| **Handle-based Resources** | Memory safety across Ruby/C++ boundary | Excellent. Ruby never sees raw pointers. |
| **Loader + Hot Reload** | Script loading, file watching | Working well. Clear separation of concerns. |
| **Animation System** | Tweens, sprite animations, animators | Well-architected. Unified manager with deferred completions. |
| **State Machine DSL** | Declarative state transitions | Elegant. Clean `instance_exec` block pattern. |
| **Input System** | Action mapping, callbacks, contexts | Solid. Symbol-based bindings feel Rubyish. |

### Known Issues to Address

1. **CollisionResult returns hash** - Should return proper Ruby object with methods
2. **Input symbol typos fail silently** - Should raise on unknown symbols
3. **InputManager couples to StateMachineManager** - Consider event queue pattern
4. **Global State singleton** - Makes testing harder; consider injection

### Key Files

| System | Primary Files |
|--------|---------------|
| Main Loop | `src/main.cpp` |
| Draw Queue | `include/gmr/draw_queue.hpp`, `src/draw_queue.cpp` |
| Input | `include/gmr/input/`, `src/bindings/input.cpp` |
| Animation | `include/gmr/animation/`, `src/animation/` |
| State Machine | `include/gmr/state_machine/`, `src/bindings/state_machine.cpp` |
| Collision | `include/gmr/bindings/collision.hpp`, `src/bindings/collision.cpp` |
| Scripting | `include/gmr/scripting/`, `src/scripting/` |

---

## Roadmap

### Priority 1: Strengthen Core

- **CollisionResult as proper object** - Replace hash with Ruby class
- **Input symbol validation** - Raise on unknown key/action symbols
- **Lightweight entity grouping** - Optional wrapper for sprite + animator + state machine

### Priority 2: Necessary Completeness

- Scene transitions (fade in/out)
- Custom font loading
- Audio improvements (crossfade, basic spatial)

### Priority 3: Consider Carefully

- Basic physics helpers (provide examples, not built-in)
- Particle system (defer until core stable)
- UI/Menu system (defer; provide primitives, let Ruby handle layout)

### Explicitly Delayed/Rejected

| Feature | Status | Rationale |
|---------|--------|-----------|
| Networked multiplayer | Delayed | Massive scope |
| Mobile export | Delayed | Platform constraints |
| 3D rendering | Rejected | Focus is strength |
| Built-in physics | Rejected | Complexity explosion |
| Visual editor | Rejected | Code is truth |
| ECS | Rejected | Over-engineering |

---

## The GMR Filter

Before every feature, PR, or design decision, ask:

1. **Does this make Ruby game code more beautiful?**
2. **Does this reduce friction for the common case?**
3. **Would a solo game jam developer need this?**
4. **Does this add complexity proportional to its value?**
5. **Is there a simpler way?**

If the answer to any of these is "no," reconsider.

GMR succeeds by being *less* than alternatives, not more. Every feature should earn its place by making games more joyful to create.

---

*This document should be referenced before every task, feature request, and PR review.*
