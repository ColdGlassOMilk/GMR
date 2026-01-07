[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Audio](../audio/README.md) > **Sound**

# Sound

Loaded audio file for playback.

## Table of Contents

- [Class Methods](#class-methods)
  - [.load](#load)
- [Instance Methods](#instance-methods)
  - [#play](#play)
  - [#stop](#stop)
  - [#volume=](#volume)

## Class Methods

<a id="load"></a>

### .load

Load a sound file from disk. Supports WAV, OGG, MP3, and other formats.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `path` | `String` | Path to the audio file |

**Returns:** `Sound` - The loaded sound object

**Raises:**
- RuntimeError If the file cannot be loaded

**Example:**

```ruby
jump_sound = GMR::Audio::Sound.load("assets/sfx/jump.wav")
```

---

## Instance Methods

<a id="play"></a>

### #play

Play the sound. Can be called multiple times for overlapping playback.

**Returns:** `nil`

**Example:**

```ruby
sound.play
```

---

<a id="stop"></a>

### #stop

Stop the sound if it's currently playing.

**Returns:** `nil`

**Example:**

```ruby
sound.stop
```

---

<a id="volume"></a>

### #volume=

Set the playback volume for this sound.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | Volume level from 0.0 (silent) to 1.0 (full volume) |

**Returns:** `Float` - The volume that was set

**Example:**

```ruby
sound.volume = 0.5  # Half volume
```

---

---

[Back to Audio](README.md) | [Documentation Home](../../../README.md)
