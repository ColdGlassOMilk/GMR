[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [State Machine](../state-machine/README.md) > **StateMachine**

# StateMachine

Finite state machine for gameplay logic.

## Table of Contents

- [Instance Methods](#instance-methods)
  - [#active?](#active)
  - [#animate](#animate)
  - [#attach](#attach)
  - [#count](#count)
  - [#enter](#enter)
  - [#exit](#exit)
  - [#on](#on)
  - [#on_input](#on_input)
  - [#state](#state)
  - [#state=](#state)
  - [#state_machine](#state_machine)
  - [#trigger](#trigger)

## Instance Methods

<a id="attach"></a>

### #attach

Attach a state machine to an object. Called internally by Object#state_machine.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `owner` | `Object` | The object to attach the state machine to |

**Returns:** `StateMachine` - The created state machine

---

<a id="count"></a>

### #count

Get the number of active state machines.

**Returns:** `Integer` - Number of active machines

---

<a id="trigger"></a>

### #trigger

Trigger an event, causing a transition if one is defined.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `event` | `Symbol` | The event to trigger |

**Returns:** `Boolean` - true if a transition occurred

**Example:**

```ruby
# Chain triggers from animation callbacks
```

---

<a id="state"></a>

### #state

Define a state with its behavior.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `name` | `Symbol` | The state name |

---

<a id="state"></a>

### #state=

Force a state change, bypassing transitions.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `new_state` | `Symbol` | The state to switch to |

**Returns:** `Symbol` - The new state

---

<a id="active"></a>

### #active?

Check if the state machine is active.

**Returns:** `Boolean` - true if active

---

<a id="animate"></a>

### #animate

Set the animation to play when entering this state.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `name` | `Symbol` | The animation name (looked up in @animations hash) |

---

<a id="on"></a>

### #on

Define a transition from this state.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `event` | `Symbol` | The event that triggers the transition |
| `target` | `Symbol` | The target state |

**Example:**

```ruby
# Multiple transitions from same state
  state :grounded do
    on :jump, :jumping
    on :crouch, :crouching
    on :damage, :hurt
    on :fall, :falling
  end
```

---

<a id="enter"></a>

### #enter

Set a callback to run when entering this state.

**Example:**

```ruby
# Initialize state-specific data on enter
  state :charging do
    enter do
```

---

<a id="exit"></a>

### #exit

Set a callback to run when exiting this state.

**Example:**

```ruby
# Cancel pending actions on state exit
  state :aiming do
    exit do
```

---

<a id="on_input"></a>

### #on_input

Define an input-driven transition from this state.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `action` | `Symbol` | The input action that triggers the transition |
| `target` | `Symbol` | The target state |

**Example:**

```ruby

  state :idle do
    on_input :jump, :air
    on_input :attack, :attack, when: :pressed, if: -> { @stamina > 0 }
  end
Builder#method_missing - Verb-style input transition
Usage: jump :air
       attack.hold :charge
       die! :dead
Builder#respond_to_missing? - For Ruby introspection
```

---

<a id="state_machine"></a>

### #state_machine

Define or get the state machine for this object.

**Returns:** `StateMachine` - The state machine

---

---

[Back to State Machine](README.md) | [Documentation Home](../../../README.md)
