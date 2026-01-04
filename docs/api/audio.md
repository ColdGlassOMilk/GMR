# Audio Module

Sound loading and playback.

```ruby
include GMR
```

## Loading Sounds

### Sound.load

Load a sound file.

```ruby
sound = Audio::Sound.load(path)
```

**Parameters:**
- `path` - Path to audio file (relative to game folder)

**Returns:** Sound object or `nil` on failure

**Supported formats:** WAV, OGG, MP3

**Example:**
```ruby
def init
  $jump_sound = Audio::Sound.load("assets/jump.wav")
  $music = Audio::Sound.load("assets/background.ogg")
  $explosion = Audio::Sound.load("assets/boom.wav")
end
```

## Playback

### play

Play a sound.

```ruby
sound.play
```

**Example:**
```ruby
def update(dt)
  if Input.action_pressed?(:jump)
    $jump_sound.play
    $player[:vy] = -JUMP_FORCE
  end
end
```

### stop

Stop a playing sound.

```ruby
sound.stop
```

**Example:**
```ruby
def pause_game
  $music.stop
  $paused = true
end
```

### playing?

Check if a sound is currently playing.

```ruby
sound.playing?
```

**Returns:** `true` if sound is playing

**Example:**
```ruby
def update(dt)
  # Start music if not playing
  unless $music.playing?
    $music.play
  end
end
```

## Volume

### set_volume

Set volume for a sound.

```ruby
sound.set_volume(volume)
```

**Parameters:**
- `volume` - Volume level from 0.0 (silent) to 1.0 (full)

**Example:**
```ruby
def init
  $music = Audio::Sound.load("assets/music.ogg")
  $music.set_volume(0.5)  # 50% volume

  $sfx_explosion = Audio::Sound.load("assets/explosion.wav")
  $sfx_explosion.set_volume(0.8)
end
```

## Complete Example

```ruby
include GMR

def init
  Window.set_size(800, 600)
  Window.set_title("Audio Demo")

  # Load sounds
  $music = Audio::Sound.load("assets/music.ogg")
  $jump = Audio::Sound.load("assets/jump.wav")
  $coin = Audio::Sound.load("assets/coin.wav")

  # Set volumes
  $music.set_volume(0.4)
  $jump.set_volume(0.7)
  $coin.set_volume(0.6)

  # Start background music
  $music.play

  $player = { x: 400, y: 500, vy: 0, on_ground: true }
  $score = 0
end

def update(dt)
  return if console_open?

  # Jump with sound
  if Input.key_pressed?(:space) && $player[:on_ground]
    $player[:vy] = -400
    $player[:on_ground] = false
    $jump.play
  end

  # Gravity
  $player[:vy] += 800 * dt
  $player[:y] += $player[:vy] * dt

  # Ground collision
  if $player[:y] >= 500
    $player[:y] = 500
    $player[:vy] = 0
    $player[:on_ground] = true
  end

  # Collect coin (simulated)
  if Input.key_pressed?(:c)
    $coin.play
    $score += 10
  end

  # Toggle music
  if Input.key_pressed?(:m)
    if $music.playing?
      $music.stop
    else
      $music.play
    end
  end
end

def draw
  Graphics.clear([30, 30, 50])

  # Player
  Graphics.draw_rect($player[:x] - 20, $player[:y] - 40, 40, 40, [100, 200, 255])

  # UI
  Graphics.draw_text("Score: #{$score}", 10, 10, 24, [255, 255, 255])
  Graphics.draw_text("SPACE: Jump | C: Collect coin | M: Toggle music", 10, 570, 18, [200, 200, 200])

  music_status = $music.playing? ? "Playing" : "Stopped"
  Graphics.draw_text("Music: #{music_status}", 10, 40, 20, [180, 180, 180])
end
```

## Tips

1. **Load in `init`** - Load all sounds once at startup
2. **Short sounds for SFX** - Use WAV for quick sound effects
3. **OGG for music** - Compressed format for longer audio
4. **Adjust volumes** - Balance SFX and music levels
5. **Check nil** - Handle missing audio files gracefully

```ruby
def init
  $sound = Audio::Sound.load("assets/sound.wav")
  unless $sound
    System.error("Failed to load sound!")
  end
end
```

## See Also

- [System](system.md) - Error handling
- [API Overview](README.md) - Resource loading conventions
