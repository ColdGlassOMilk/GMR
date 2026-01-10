[GMR Docs](../../../README.md) > [Engine](../../engine/README.md) > [Audio](../audio/README.md) > **Sound**

# Sound

Loaded audio file for playback.

## Table of Contents

- [Class Methods](#class-methods)
  - [.load](#load)
- [Instance Methods](#instance-methods)
  - [#pan](#pan)
  - [#pan=](#pan)
  - [#pause](#pause)
  - [#pitch](#pitch)
  - [#pitch=](#pitch)
  - [#play](#play)
  - [#playing?](#playing)
  - [#resume](#resume)
  - [#stop](#stop)
  - [#volume](#volume)
  - [#volume=](#volume)

## Class Methods

<a id="load"></a>

### .load

Load a sound file from disk. Supports WAV, OGG, MP3, and other formats.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `path` | `String` | Path to the audio file |
| `options` | `Hash` | Optional configuration |

**Returns:** `Sound` - The loaded sound object

**Raises:**
- RuntimeError If the file cannot be loaded

**Example:**

```ruby
jump_sound = GMR::Audio::Sound.load("assets/sfx/jump.wav", volume: 0.7, pitch: 1.2)
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

<a id="pause"></a>

### #pause

Pause the sound if it's currently playing.

**Returns:** `nil`

**Example:**

```ruby
sound.pause
```

---

<a id="resume"></a>

### #resume

Resume the sound if it was paused.

**Returns:** `nil`

**Example:**

```ruby
sound.resume
```

---

<a id="playing"></a>

### #playing?

Check if the sound is currently playing.

**Returns:** `Boolean` - true if the sound is playing, false otherwise

**Example:**

```ruby
if sound.playing? then puts "Playing!" end
```

---

<a id="pitch"></a>

### #pitch=

Set the playback pitch for this sound.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | Pitch multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed) |

**Returns:** `Float` - The pitch that was set

**Example:**

```ruby
sound.pitch = 1.5  # Play 50% faster
```

---

<a id="pan"></a>

### #pan=

Set the stereo pan for this sound.

**Parameters:**

| Name | Type | Description |
|------|------|-------------|
| `value` | `Float` | Pan value from 0.0 (left) to 1.0 (right), 0.5 is center |

**Returns:** `Float` - The pan value that was set

**Example:**

```ruby
sound.pan = 0.0  # Full left
```

---

<a id="volume"></a>

### #volume

Get the current volume level for this sound.

**Returns:** `Float` - Current volume (0.0-1.0)

**Example:**

```ruby
vol = sound.volume
```

---

<a id="pitch"></a>

### #pitch

Get the current pitch for this sound.

**Returns:** `Float` - Current pitch multiplier

**Example:**

```ruby
p = sound.pitch
```

---

<a id="pan"></a>

### #pan

A streaming music player for longer audio tracks. Music is streamed from disk rather than loaded entirely into memory, making it suitable for background music and longer audio files.

**Returns:** `Float` - Current pan value (0.0-1.0)

**Example:**

```ruby
# Dynamic music transitions
  class GameMusic
    def initialize
```

---

---

[Back to Audio](README.md) | [Documentation Home](../../../README.md)
